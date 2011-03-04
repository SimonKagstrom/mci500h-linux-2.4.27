// Philips keypad driver for Tahoe board

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>

#define KEYPAD_DEBUG 			0
#if KEYPAD_DEBUG
	#define DPK(format, args...) do { printk(KERN_ERR "melody_keypad: " format, ## args); } while (0)
#else
	#define DPK(format, args...) do {} while (0)
#endif



#define KEYPAD_INPUTS			3
#define KEYPAD_OUTPUTS			3 
#define KEYPAD_INPUTS_BASE		4 
#define KEYPAD_OUTPUTS_BASE		7 
#define KEYPAD_INTRUPT_BASE		11 
#define KEYPAD_DEVICE_MAJOR 		120
#define KEYPAD_BUFFER_LENGTH 		16		// must be power of 2
#define KEYPAD_OUTPUTS_MASK 		(0x00000380)

#define KEYPAD_EVENT_IN_0	116
#define KEYPAD_EVENT_IN_1	117
#define KEYPAD_EVENT_IN_2	118


const char KEYPAD_DEVICE_NAME[] = "keypad";
static volatile int current_active_input  = 0;
static volatile unsigned long keypad_timeout = 0;
static volatile int keypad_timer_started = 0;
//volatile struct timer_list keypad_timer;
struct timer_list keypad_timer;
volatile static long  time_between_keypresses = HZ;  // 1 sec
volatile static long  last_keyin_time;
volatile static long  last_keyin_val;

static int keypad_device = 0;
static wait_queue_head_t keypad_wait_queue;


// read and write index to the buffer of keys
unsigned idx_read = 0, idx_write = 0;
static unsigned char keypad_buffer[KEYPAD_BUFFER_LENGTH];

#define KEYPAD_SCAN_TIME	((unsigned long)jiffies + HZ +10)   /* 10 ms */
#define KEYPAD_KEY_DELAY	((KEYPAD_SCAN_TIME) / 100 )    /* 100 micro seconds */		

static int keypad_open(struct inode *inode, struct file *file)
{
	DPK("keypad_open; pos=%llu\n", file->f_pos);
	return 0;
}
                  
static int keypad_release(struct inode *inode, struct file *filp)
{
	DPK("keypad_release\n");
	return 0;
}

static ssize_t keypad_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
	int i;

	/* If buffer is empty and NONBLOCK return immediately else wait */
	if( idx_write == idx_read ) {
		if (file->f_flags & O_NONBLOCK) {
			DPK("keypad_read; O_NONBLOCK enabled\n");
			return 0;
		} else {
			DPK("keypad_read; sleeping\n");
			interruptible_sleep_on(&keypad_wait_queue); 
		}
	}

	for( i = 0; (i < length) && (idx_read != idx_write); i++)
		buffer[i] = keypad_buffer[idx_read++ & (KEYPAD_BUFFER_LENGTH-1)];
	return i;
}

static int keypad_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	DPK("keypad_ioctl; cmd=0x%x, arg=0x%lx\n", cmd, arg);

	switch( cmd)
	{
	case 1:
		time_between_keypresses = (long)arg; 
		DPK("keypad_ioctl; setting time_between_keypresses = %ld\n", time_between_keypresses);
		break;
	}
	return 0;
}

static int keypad_read_proc(char *buf, char **start, off_t offset,
		       int count, int *eof, void *data)
{
	int len = 0;

	len += sprintf(buf + len, "Melody 3x3 keypad matrix driver (c) 2007\n\n");
	len += sprintf(buf + len, "major=%u\nminor=%u\n", KEYPAD_DEVICE_MAJOR, 0);
	len += sprintf(buf + len, "irqs used %d\n", IRQ_KEYPAD);

	*eof = 1;

	return len;
}

static struct file_operations keypad_fops=
{
	open: keypad_open,
	release: keypad_release,
	read: keypad_read,
	ioctl: keypad_ioctl
};

static void keypad_irq(int irq, void *dev_id, struct pt_regs * regs)
{
	int ind = 0;	
 	disable_irq(IRQ_KEYPAD); 	

	/* process only if previouslly pressed key is complete */
	if(!keypad_timeout) {
		DPK("keypad_irq; irq = %d\n", irq);
		for( ind = 0; ind < 3; ind++) {
			if(gpio_get_value(GPIO_KEYPAD_IN_0 + ind)) {
				current_active_input  = ind;
				DPK("keypad_irq; current active input line: %d\n", current_active_input);
				break;
			}
		}

		keypad_timeout = jiffies + 9; // 50 ms sec 
		if(!keypad_timer_started) {
			keypad_timer.expires = keypad_timeout;
			add_timer(&keypad_timer);
			keypad_timer_started = 1;
		} else {
			mod_timer(&keypad_timer, keypad_timeout);
		}
	}
			
}
static void keypad_scan(unsigned long timeoutval ) 
{
	int i = 0;
	unsigned int val = 0;
	//printk("\n");
	//ssa_dump_gpio_regs();

	//gpio_set_low(GPIO_KEYPAD_OUT_0); 
	//gpio_set_low(GPIO_KEYPAD_OUT_1); 
	//gpio_set_low(GPIO_KEYPAD_OUT_2); 

	for(i = 0; i < KEYPAD_OUTPUTS; i++) {
		if(i == 0) {
			gpio_set_high(GPIO_KEYPAD_OUT_0);
			gpio_set_low(GPIO_KEYPAD_OUT_1); 
			gpio_set_low(GPIO_KEYPAD_OUT_2); 
		} else if( i == 1 ) {
			gpio_set_low(GPIO_KEYPAD_OUT_0); 
			gpio_set_high(GPIO_KEYPAD_OUT_1);
			gpio_set_low(GPIO_KEYPAD_OUT_2); 
		} else {
			gpio_set_low(GPIO_KEYPAD_OUT_0); 
			gpio_set_low(GPIO_KEYPAD_OUT_1); 
			gpio_set_high(GPIO_KEYPAD_OUT_2);
		}
		udelay(200);

		val = 0;
		val = gpio_get_value(GPIO_KEYPAD_IN_0 + current_active_input);

	//	printk("val = %x\n", val);
	//	ssa_dump_gpio_regs();

		if(val) {
			val = (current_active_input) * KEYPAD_OUTPUTS + i;  
			if((last_keyin_val != val) || ((jiffies -last_keyin_time) > time_between_keypresses) )  {
				keypad_buffer[idx_write++ & (KEYPAD_BUFFER_LENGTH-1)] = val;
				last_keyin_time = jiffies;
				last_keyin_val = val;
				DPK("keypad_scan; line : %d out : %d val : %d\n", 
					current_active_input, i, val);  
				if(waitqueue_active(&keypad_wait_queue)) {
					DPK("keypad_scan; wakeup sleeper\n");
					wake_up_interruptible(&keypad_wait_queue);
				}
				break;
			}
		}
	}

	/* enable all the keypad GPIO outputs */
	gpio_set_high(GPIO_KEYPAD_OUT_0); 
	gpio_set_high(GPIO_KEYPAD_OUT_1); 
	gpio_set_high(GPIO_KEYPAD_OUT_2); 
	udelay(200);

	keypad_timeout = 0;
	//printk("\n");
	//ssa_dump_gpio_regs();
	//printk("\n");
	enable_irq(IRQ_KEYPAD);
	
}

int __init keypad_init(void)
{
	printk("Melody 3x3 keypad matrix driver (c) 2005\n");

	idx_read = idx_write = 0;
		
 
	keypad_device = devfs_register_chrdev( KEYPAD_DEVICE_MAJOR, KEYPAD_DEVICE_NAME, &keypad_fops);

	create_proc_read_entry( KEYPAD_DEVICE_NAME, 0, NULL, keypad_read_proc, NULL);

	//set irq type 
	gpio_set_as_input(GPIO_KEYPAD_IN_0); 
	gpio_set_as_input(GPIO_KEYPAD_IN_1); 	
	gpio_set_as_input(GPIO_KEYPAD_IN_2); 
	
	if( request_irq( IRQ_KEYPAD, keypad_irq, 0, KEYPAD_DEVICE_NAME, (void *)keypad_device))
	{
		printk("melody_keypad: request_irq(%d) failed\n",  IRQ_KEYPAD);
		return -EBUSY;
	}
	

	/* enable all the keypad GPIO outputs */
	gpio_set_as_output(GPIO_KEYPAD_OUT_0, 1);
	gpio_set_as_output(GPIO_KEYPAD_OUT_1, 1);
	gpio_set_as_output(GPIO_KEYPAD_OUT_2, 1);
	udelay(200);

	/* initialize and start timer for periodic scaning of keypad */
	init_timer(&keypad_timer);
	keypad_timer.function = keypad_scan;
	keypad_timer.data = 0;
	//tahoe_keypad_timer.expires = jiffies + HZ; /* delay for 1 sec */
	
	/* initialize wait queue for process reads to wait */
	init_waitqueue_head(&keypad_wait_queue);
	//ssa_dump_gpio_regs();
	return 0;
}
 
void keypad_cleanup(void)
{
  DPK("keypad_cleanup\n");

  devfs_unregister_chrdev( KEYPAD_DEVICE_MAJOR, KEYPAD_DEVICE_NAME);
  remove_proc_entry( KEYPAD_DEVICE_NAME, NULL);
 
}

__initcall(keypad_init);
