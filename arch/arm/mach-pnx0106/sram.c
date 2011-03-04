

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>

#include <linux/list.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>

#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <asm/arch/sram.h>

#define PNX0106_SRAM_MINOR		231
#define PNX0106_SRAM_DEVNAME		"pnx0106_sram"

#define PNX0106_SRAM_MAX_SIZE		INTERNAL_SRAM_SIZE
#define PNX0106_SRAM_MAX_PAGES		(PNX0106_SRAM_MAX_SIZE/PAGE_SIZE)

//#define PRINTK		printk
#define PRINTK(fmt...)

struct pnx0106_sram_state
{
    int count;
    wait_queue_head_t wq;
    spinlock_t lock;
    volatile unsigned int pending;
    struct list_head free_list;
};

struct pnx0106_sram_state pnx0106_sram_state;

#define DEVICE_NAME     	PNX0106_SRAM_DEVNAME

struct pnx0106_sram_priv
{
    struct pnx0106_sram_state *state;
    int numpages;
    char *pages[PNX0106_SRAM_MAX_PAGES];
};

static void
pnx0106_sram_show_list(struct pnx0106_sram_state *state)
{
    struct list_head *page =  NULL;
    int i = 0;

    PRINTK(DEVICE_NAME ": show_list: state %p next %p\n", state, state->free_list.next);

    list_for_each(page, &state->free_list)
    {
	PRINTK(DEVICE_NAME ": show_list: #%d page %p\n", i, page);
	i++;
    }
}

static void *
__pnx0106_sram_alloc_page(struct pnx0106_sram_state *state)
{
    struct list_head *page =  NULL;
    PRINTK(DEVICE_NAME ": alloc_page: state %p next %p\n", state, state->free_list.next);
//    pnx0106_sram_show_list(state);

    if (!list_empty(&state->free_list))
    {
	page = state->free_list.next;
	list_del(page);
	memset(page, 0, PAGE_SIZE);
    }

    return page;
}

void *
pnx0106_sram_alloc_page(void)
{
    struct pnx0106_sram_state *state = &pnx0106_sram_state;
    void *page;

    spin_lock(&state->lock);
    page = __pnx0106_sram_alloc_page(state);
    spin_unlock(&state->lock);
    return page;
}

static void
__pnx0106_sram_free_page(struct pnx0106_sram_state *state, void *ptr)
{
    struct list_head *node = (struct list_head *)ptr;
    PRINTK(DEVICE_NAME ": free_page: state %p ptr %p\n", state, ptr);
    memset(ptr, 0, PAGE_SIZE);
    list_add(node, &state->free_list);
//    pnx0106_sram_show_list(state);
}

void
pnx0106_sram_free_page(void *ptr)
{
    struct pnx0106_sram_state *state = &pnx0106_sram_state;

    spin_lock(&state->lock);
    __pnx0106_sram_free_page(state, ptr);
    spin_unlock(&state->lock);
}

static loff_t
pnx0106_sram_llseek(struct file *file, loff_t offset, int origin)
{
    printk(DEVICE_NAME ": llseek???\n");
    return (loff_t)0;
}

static ssize_t
pnx0106_sram_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
    struct pnx0106_sram_priv *priv = file->private_data;
    loff_t pos = *ppos;
    int page;
    int off;
    int num = 0;

    PRINTK(DEVICE_NAME ": read, file %p, buf %p, count %d, ppos %p, *ppos %d\n", file, buf, count, ppos, (int)pos);

    if (pos > priv->numpages * PAGE_SIZE)
	return -EIO;

    page = pos / PAGE_SIZE;
    off = pos % PAGE_SIZE;

    while (num < count && page < priv->numpages)
    {
	int sz = count - num;

	if (sz > PAGE_SIZE - off)
	    sz = PAGE_SIZE - off;

	copy_to_user(&buf[num], &priv->pages[page][off], sz);
	num += sz;
	page++;
	off = 0;
    }

    *ppos += num;
    PRINTK(DEVICE_NAME ": read done!\n");
    return num;
}

static ssize_t
pnx0106_sram_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
    struct pnx0106_sram_priv *priv = file->private_data;
    loff_t pos = *ppos;
    int page;
    int off;
    int num = 0;

    PRINTK(DEVICE_NAME ": write, file %p, buf %p, count %d, ppos %p, *ppos %d\n", file, buf, count, ppos, (int)pos);

    if (pos >= priv->numpages * PAGE_SIZE)
	return -EIO;

    page = pos / PAGE_SIZE;
    off = pos % PAGE_SIZE;

    while (num < count && page < priv->numpages)
    {
	int sz = count - num;

	if (sz > PAGE_SIZE - off)
	    sz = PAGE_SIZE - off;

	copy_from_user(&priv->pages[page][off], &buf[num], sz);
	num += sz;
	page++;
	off = 0;
    }

    *ppos += num;
    PRINTK(DEVICE_NAME ": write done!\n");
    return num;
}

static int
pnx0106_sram_ioctl_resize(struct pnx0106_sram_priv *priv, unsigned long arg)
{
    struct pnx0106_sram_state *state = priv->state;
    int numbytes = (int)arg;
    int numpages = (numbytes + PAGE_SIZE - 1) / PAGE_SIZE;
    int ret = 0;
    int i;

    PRINTK(DEVICE_NAME ": ioctl_resize: size %d priv %p state %p\n", numbytes, priv, state);
    PRINTK(DEVICE_NAME ": ioctl_resize: priv->numpages %d numpages %d\n", priv->numpages, numpages);

    if (numpages > priv->numpages)
    {
	spin_lock(&state->lock);

	for (i = priv->numpages; i < numpages; i++)
	{
	    priv->pages[i] = __pnx0106_sram_alloc_page(state);
	    PRINTK(DEVICE_NAME ": ioctl_resize: page[%d] = %p\n", i, priv->pages[i]);

	    if (!priv->pages[i])
		break;

#if 0
	    ((unsigned int *)priv->pages[i])[0] = 0xDEADBEEF;
	    ((unsigned int *)priv->pages[i])[1] = (unsigned int)priv->pages[i];
	    ((unsigned int *)priv->pages[i])[2] = (unsigned int)priv->pages[i] - IO_ADDRESS_ISRAM_BASE;
	    ((unsigned int *)priv->pages[i])[3] = i;
#endif
	}

	if (i < numpages)
	{
	    PRINTK(DEVICE_NAME ": ioctl_resize: out of memory\n");

	    for (i--; i >= priv->numpages; i--)
		__pnx0106_sram_free_page(state, priv->pages[i]);

	    ret = -ENOMEM;
	}
	else
	    priv->numpages = numpages;

	spin_unlock(&state->lock);
    }
    else
    {
	PRINTK(DEVICE_NAME ": ioctl_resize: releasing unneeded memory\n");
	spin_lock(&state->lock);

	for (i = priv->numpages - 1; i >= numpages; i--)
	    __pnx0106_sram_free_page(state, priv->pages[i]);

	priv->numpages = numpages;
	spin_unlock(&state->lock);
    }

    PRINTK(DEVICE_NAME ": ioctl_resize: new numpages %d\n", priv->numpages);
    return ret;
}

static int
pnx0106_sram_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    struct pnx0106_sram_priv *priv = file->private_data;
    PRINTK(DEVICE_NAME ": ioctl: cmd 0x%08x priv %p\n", cmd, priv);

    switch (cmd)
    {
    case PNX0106_SRAM_RESIZE:
	return pnx0106_sram_ioctl_resize(priv, arg);
    }

    printk(DEVICE_NAME ": Mystery IOCTL called (0x%08x)\n", cmd);
    return -EINVAL;
}

static int
pnx0106_sram_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct pnx0106_sram_priv *priv = file->private_data;
    int i;
    int size = 0;
    int vmsize = vma->vm_end - vma->vm_start;

    PRINTK(DEVICE_NAME ": mmap\n");

    if (vma->vm_pgoff > (~0UL >> PAGE_SHIFT))
	return -EINVAL;

    /* This is an IO map */
    vma->vm_flags |= VM_IO;

    // remap each page
    for (i = vma->vm_pgoff; i < priv->numpages; i++)
    {
	unsigned int bus_addr;

    	if (size >= vmsize)
	    break;

	bus_addr = INTERNAL_SRAM_BASE + (priv->pages[i] - IO_ADDRESS_ISRAM_BASE);

	if (remap_page_range(vma->vm_start + size, bus_addr, PAGE_SIZE,
		vma->vm_page_prot))
	    return -EAGAIN;

	size += PAGE_SIZE;
    }

    return 0;
}

static int
pnx0106_sram_open(struct inode *inode, struct file *file)
{
    struct pnx0106_sram_state *state = &pnx0106_sram_state;
    struct pnx0106_sram_priv *priv = kmalloc(sizeof *priv, GFP_KERNEL);

    PRINTK("pnx0106_sram_open: start\n");

    if (!priv)
	return -ENOMEM;

    spin_lock(&state->lock);

    // allocate initial page
    priv->state = state;
    priv->numpages = 1;
    priv->pages[0] = __pnx0106_sram_alloc_page(state);

#if 0
    ((unsigned int *)priv->pages[0])[0] = 0xDEADBEEF;
    ((unsigned int *)priv->pages[0])[1] = (unsigned int)priv->pages[0];
    ((unsigned int *)priv->pages[0])[2] = (unsigned int)priv->pages[0] - IO_ADDRESS_ISRAM_BASE;
    ((unsigned int *)priv->pages[0])[3] = 0;
#endif

    if (!priv->pages[0])
    {
	spin_unlock (&state->lock);
	return -ENOMEM;
    }

    state->count++;
    spin_unlock (&state->lock);
    MOD_INC_USE_COUNT;
    file->private_data = priv;
    return 0;
}

static int
pnx0106_sram_release(struct inode *inode, struct file *file)
{
    struct pnx0106_sram_priv *priv = file->private_data;
    struct pnx0106_sram_state *state = priv->state;
    int i;

    PRINTK("pnx0106_sram_release: goodbye !!\n");

    spin_lock (&state->lock);

    for (i = 0; i < priv->numpages; i++)
    {
	PRINTK("pnx0106_sram_release: %d: state %p priv %p, page %p\n", i, state, priv, priv->pages[i]);
	__pnx0106_sram_free_page(state, priv->pages[i]);
    }

    state->count--;
    spin_unlock (&state->lock);
    MOD_DEC_USE_COUNT;
    kfree(priv);
    return 0;
}

static struct file_operations pnx0106_sram_fops =
{
   .llseek            = pnx0106_sram_llseek,
   .read              = pnx0106_sram_read,
   .write             = pnx0106_sram_write,
//   .readdir           = pnx0106_sram_readdir,
//   .poll              = pnx0106_sram_poll,
   .ioctl             = pnx0106_sram_ioctl,
   .mmap              = pnx0106_sram_mmap,
   .open              = pnx0106_sram_open,
//   .flush             = pnx0106_sram_flush,
   .release           = pnx0106_sram_release,
//   .fsync             = pnx0106_sram_fsync,
//   .fasync            = pnx0106_sram_fasync,
//   .lock              = pnx0106_sram_lock,
//   .readv             = pnx0106_sram_readv,
//   .writev            = pnx0106_sram_writev,
//   .sendpage          = pnx0106_sram_sendpage,
//   .get_unmapped_area = pnx0106_sram_get_unmapped_area,
   .owner             = THIS_MODULE,
};

static struct miscdevice pnx0106_sram_misc_device =
{
   PNX0106_SRAM_MINOR, PNX0106_SRAM_DEVNAME, &pnx0106_sram_fops
};

static void
pnx0106_sram_free_list_init(struct pnx0106_sram_state *state)
{
    int i = 0;
    char *sram = (char *)IO_ADDRESS_ISRAM_BASE;

    INIT_LIST_HEAD(&state->free_list);

    for (i = 0; i < PNX0106_SRAM_MAX_SIZE; i += PAGE_SIZE)
	__pnx0106_sram_free_page(state, &sram[i]);
}

static int __init pnx0106_sram_init(void)
{
    struct pnx0106_sram_state *state = &pnx0106_sram_state;
    int result;

    printk("pnx0106_sram_init\n");

    init_waitqueue_head(&state->wq);
    state->lock = SPIN_LOCK_UNLOCKED,

    pnx0106_sram_free_list_init(state);

    // register minor numbers
    result = misc_register (&pnx0106_sram_misc_device);

    if (result < 0)
    {
	printk(DEVICE_NAME ": Can't register misc minor no %d\n", PNX0106_SRAM_MINOR);
	return result;
    }

    PRINTK("pnx0106_sram_init: device registered\n");

    return 0;
}

static void __exit pnx0106_sram_exit(void)
{
    printk("pnx0106_sram_exit\n");
    misc_deregister(&pnx0106_sram_misc_device);
}

module_init(pnx0106_sram_init);
module_exit(pnx0106_sram_exit);
