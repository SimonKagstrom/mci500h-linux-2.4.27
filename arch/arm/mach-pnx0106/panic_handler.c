/*
 *  linux/arch/arm/mach-pnx0106/panic_handler.c
 *
 *  Copyright (C) 2003-2006 Andre McCurdy, Philips Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/version.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/hardware.h>
#include <asm/arch/watchdog.h>


#if defined (CONFIG_DEBUG_PANIC_POSTMORTEM)

extern void show_module_symbols (void);
extern int uart_dppm_rx_line (char *pline);

static int handle_with_debugger = 0;

#define IS_SPACE(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r')

static inline char *skip_space (char *p)
{
	while (*p) {
		if (IS_SPACE(*p))
			p++;
		else
			break;
	}

	return p;
}

static int show_process_info (void)
{
	struct task_struct *p;

	printk ("task list:pid\t\tname\n");
	for_each_task (p) {
		printk ("\t%d\t\t%s\t\t0x%x\n", p->pid, p->comm, (unsigned int) p);
	}
	return 0;
}

static int show_process_stack_all (void)
{
	struct task_struct *p;
	unsigned int cur_sp;

	for_each_task(p) {
		cur_sp = ((unsigned int *)p)[792/4];			/* why 792:please reference the disassemble code of __switch_to */
		printk ("pid:%d(%s) sp=0x%08x\n", p->pid, p->comm, cur_sp);
		dump_mem ("Stack: ", cur_sp, 8192 + (unsigned long) p); /* current sp upwards to origin of stack */
		dump_mem ("Below stack: ", (unsigned long) p, cur_sp);	/* base of tsk upwards to current sp */
	}
	return 0;
}

static int show_process_stack (int pid)
{
	struct task_struct *p;
	unsigned int cur_sp;

	if ((pid > 0) && (p = find_task_by_pid (pid))) {
		cur_sp = ((unsigned int *)p)[792/4];			/* why 792:please reference the disassemble code of __switch_to */
		printk("pid:%d, sp=0x%08x\n", pid, cur_sp);
		dump_mem ("Stack: ", cur_sp, 8192 + (unsigned long) p); /* current sp upwards to origin of stack */
		dump_mem ("Below stack: ", (unsigned long) p, cur_sp);	/* base of tsk upwards to current sp */
	}
	else {
		printk ("\nwrong pid\n");
	}

	return 0;
}

static void show_panic_debugger_banner (void)
{
	printk ("Usage:\n");
	printk ("    d mem len          : dump memory\n");
	printk ("    s                  : dump current stack\n");
	printk ("    m                  : show module name and load address\n");
	printk ("    p                  : show current threads\n");
	printk ("    t pid              : show process's stack info\n");
	printk ("    r                  : reboot\n");
	printk ("    ?                  : help\n");
}

static void run_postmortem_debugger (void)
{
	static int calls = 0;
	static char buf[1024];
	int len;

	if (calls++ == 0) {
		printk ("====Welcome to kernel panic debugger====\n");
		show_panic_debugger_banner();
	}

	printk ("\npanic dbg=>");
	len = uart_dppm_rx_line (buf);

	switch (buf[0])
	{
		case 'd':	//dump memory
		{
			static unsigned int mem;
			static unsigned int mem_len;
			char *p=buf;

			p++;
			p=skip_space(p);
			if(*p==0) break;
			mem = simple_strtoul(p, &p, 0);
			p=skip_space(p);
			if(*p==0) break;
			mem_len=simple_strtoul(p, 0, 0);
			mem_len=(mem_len+3)&(~3);	//make mem_len four bytes aligned!
			dump_mem("\nmemory dump",mem,mem+mem_len);
			break;
		}
		case 's':		//current process's stack, you can derive the backtrace from the stack
		{
			unsigned int cur_sp;

			__asm__ __volatile__("mov %0,%%sp":"=r"(cur_sp));
			printk ("pid=%d\n", current->pid);
			dump_mem ("Stack: ", cur_sp, 8192 + (unsigned long) current);	/* current sp upwards to origin of stack */
			dump_mem ("Below stack: ", (unsigned long) current, cur_sp);	/* base of tsk upwards to current sp */
			break;
		}
		case 'm':		//show module info, the start address of the module symbol, mainly for backtrace
			show_module_symbols();
			break;
		case 'p':		//show process information
			show_process_info();
			break;
		case 't':		//show process's stack
		{
			static  int pid;
			char *p=buf;

			p++;
			p=skip_space(p);
			if(*p==0) break;
			pid=simple_strtoul(p,0,0);
			show_process_stack(pid);
			break;
		}

		case 'r':
			watchdog_force_reset (1);	/* fixme... this is PNX0106 specific (maybe should call generic code instead) */
			break;

		case 'a':
			show_process_stack_all();
			break;
		case '?':	//show usage
			show_panic_debugger_banner();
			break;

		default:
			break;
	}
}

#ifdef CONFIG_PROC_FS

static int proc_panic_dbg_read (char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;
	char *p = page;

	p += sprintf (p, "%u\n", handle_with_debugger);
	if ((len = (p - page) - off) < 0)
		len = 0;
	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}

static int proc_panic_dbg_write (struct file *file, const char *buffer, unsigned long count, void *data)
{
	char sbuf[16 + 1];
	int value;
	int len;

	if (count > 0) {
		len = (count > (sizeof(sbuf) - 1)) ? (sizeof(sbuf) - 1) : count;
		memset (sbuf, 0, sizeof(sbuf));
		if (copy_from_user (sbuf, buffer, len))
			return -EFAULT;
		if (sscanf (sbuf, "%u", &value) == 1) {
			if (value == 1234)
				panic ("%s: Forcing panic...", __FUNCTION__);
			handle_with_debugger = value;
		}
	}
	return count;
}

static int __init proc_init (void)
{
	struct proc_dir_entry *res;

	if ((res = create_proc_entry ("panic_dbg", (S_IWUSR | S_IRUGO), NULL)) == NULL)
		return -ENOMEM;
	res->read_proc = proc_panic_dbg_read;
	res->write_proc = proc_panic_dbg_write;
	return 0;
}

#else

#define proc_init()	do { } while (0)

#endif


/*******************************************************************************
*******************************************************************************/

#else

#define handle_with_debugger		(0)

#define run_postmortem_debugger()	do { } while (0)
#define proc_init()			do { } while (0)

#endif


/*******************************************************************************
*******************************************************************************/

static int pnx0106_panic_handler (struct notifier_block *this, unsigned long event, void *ptr)
{
	if (handle_with_debugger)
		while (1)
			run_postmortem_debugger();

	printk ("pnx0106_panic_handler: rebooting...");
//	mdelay (1000);
	watchdog_force_reset (1);	/* fixme... this is PNX0106 specific (should maybe call generic code instead ?) */
	return NOTIFY_DONE;
}

static struct notifier_block pnx0106_panic_block =
{
	notifier_call: pnx0106_panic_handler,
	priority: INT_MAX,
};


/*******************************************************************************
*******************************************************************************/

static int __init panic_handler_init (void)
{
	notifier_chain_register (&panic_notifier_list, &pnx0106_panic_block);
	proc_init();
	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)

__initcall (panic_handler_init);		/* 2.4 */

#else

device_initcall (panic_handler_init);		/* 2.6 */

#endif


/*******************************************************************************
*******************************************************************************/

