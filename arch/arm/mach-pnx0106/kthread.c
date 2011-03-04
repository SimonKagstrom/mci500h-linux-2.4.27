
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/smp_lock.h>
#include <linux/unistd.h>

#include <asm/uaccess.h>
#include <asm/string.h>
#include <asm/io.h>

int trace_level = 9;

// Cheap hacks to emulate the Linux 2.6 kthread calls

static spinlock_t kthread_lock = SPIN_LOCK_UNLOCKED;
static struct task_struct *kthread_to_stop = NULL;
static DECLARE_COMPLETION(kthread_stop_complete);

struct kthread_create_info
{
    /* Information passed to kthread() from kthread_create. */
    int (*threadfn)(void *data);
    void *data;
    struct completion started;
};

int
kthread_should_stop(void)
{
    return kthread_to_stop == current;
}

static int
kthread(void *_create)
{
    struct kthread_create_info *create = _create;
    int (*threadfn)(void *data);
    void *data;

//    printk("kthread: create %p\n", create);

    threadfn = create->threadfn;
    data = create->data;
//    printk("kthread: threadfn %p data %p\n", threadfn, data);

    lock_kernel();
    daemonize();
    reparent_to_init();
    spin_lock_irq(&current->sigmask_lock);
    sigfillset(&current->blocked);
    recalc_sigpending(current);
    spin_unlock_irq(&current->sigmask_lock);
    unlock_kernel();

    set_current_state(TASK_INTERRUPTIBLE);
    complete(&create->started);

//    printk("kthread: about to call thread function\n");

    if (!kthread_should_stop())
	threadfn(data);

//    printk("kthread: thread function done\n");

    if (kthread_should_stop())
	complete(&kthread_stop_complete);

    return 0;
}

struct task_struct *
kthread_create(int (*fn)(void*), void *data, char *fmt, ...)
{
    struct kthread_create_info create;
    struct task_struct *task;
    int pid;

//    printk("kthread_create: start fn %p data %p\n", fn, data);

    create.threadfn = fn;
    create.data = data;
    init_completion(&create.started);

//    printk("kthread_create: creating thread\n");
    pid = kernel_thread(kthread, &create, CLONE_FS | CLONE_FILES | CLONE_VM );
//    printk("kthread_create: pid %d\n", pid);

    if (pid < 0)
	return ERR_PTR(pid);

    wait_for_completion(&create.started);
//    printk("kthread_create: thread created\n");
    task = find_task_by_pid(pid);
//    printk("kthread_create: thread task %p\n", task);

    if (!IS_ERR(task))
    {
	va_list args;
	va_start(args, fmt);
	vsnprintf(task->comm, sizeof task->comm, fmt, args);
	va_end(args);
//	printk("kthread_create: thread name %s\n", task->comm);
    }

    return task;
}

void
kthread_stop(struct task_struct *thread)
{
//    printk("kthread_stop: thread %p\n", thread);
    spin_lock(&kthread_lock);
    kthread_to_stop = thread;
//    printk("kthread_stop: thread %p set\n", thread);
    wait_for_completion(&kthread_stop_complete);
    kthread_to_stop = NULL;
//    printk("kthread_stop: thread %p stopped\n", thread);
    spin_unlock(&kthread_lock);
//    printk("kthread_stop: done %p\n", thread);
}

static void
kthread_init(void)
{
}

void *
dma_alloc_coherent(void *p, int size, dma_addr_t *phys, int flags)
{
    void *ret;

//    printk("dma_alloc_coherent: p %p, size %d, phys %p, flags 0x%x\n", p, size, phys, flags);
    ret = consistent_alloc(GFP_DMA | GFP_KERNEL | flags, size, phys);
//    printk("dma_alloc_coherent: ret %p\n", ret);
    return ret;
}

void
dma_free_coherent(void *p, int size, void *ptr, dma_addr_t phys)
{
    consistent_free(ptr, size, phys);
}
