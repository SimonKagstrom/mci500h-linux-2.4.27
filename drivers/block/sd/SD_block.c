/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>

#include <linux/devfs_fs_kernel.h>

#include <asm/system.h>
#include <asm/uaccess.h>




#include "sd.h"

extern U32 SD_write(struct request *pReq );
extern U32 SD_read(struct request *pReq );
extern struct SD_card* SD_card_discovery (void);
extern U32 MCI_init(void);

extern struct SD_card* CurrentSDCard;
extern spinlock_t SD_rw_lock;
extern SD_busy;


/*
 * max 8 partitions per card
 */
void SD_RW_Test (void) {
    struct request rq;
    U8 data[2048];
    U32 i;
    
    rq.current_nr_sectors=4;
    rq.sector=0x20abf;
    rq.buffer=data;
    for (i=0;i<2048;i++)
        data[i]=0xa5; 
    //SD_write(&rq);
    SD_read(&rq);
    SD_busy=1;
    //rq.sector++;
}    
    
#ifdef SD_DMA_MODE    	
void SD_request (request_queue_t *q) {
       int status;
       unsigned int flags;
       
       SD_Flow_Dbg("SD_request:SD_busy=%x\n",SD_busy);
       
       //SD_RW_Test();
       
       if (SD_busy)
           return;
           	
       while(1) {
           
           INIT_REQUEST;
	    
	   if (CurrentSDCard == NULL) {
	       printk(KERN_INFO "there is no CARD !!\n");
	       end_request(0);
	       continue;
	   }
	  
	   spin_lock_irqsave(&SD_rw_lock, flags); 
	   if ((CURRENT->cmd)==READ) 
	       status=SD_read(CURRENT);
	   else
	       status=SD_write(CURRENT);	 	    	
	   	   
	   SD_busy=1; 
	   spin_unlock_irqrestore(&SD_rw_lock, flags);
	  
	   return;	
	   //end_request(status+1);/*LTPE Joe Return value: 0-->success,-1-->error*/
        } 
}           	    
#else/*LTPE Joe PIO MODE*/
void SD_request (request_queue_t *q) {
       int status;
       unsigned int flags;
       
       SD_Flow_Dbg("SD_request:SD_busy=%x\n",SD_busy);
       
       //SD_RW_Test(); 
       if (SD_busy)
           return;                 	
       while(1) {
           
           INIT_REQUEST;
	    
	   if (CurrentSDCard == NULL) {
	       printk(KERN_INFO "there is no CARD !!\n");
	       end_request(0);
	       continue;
	   }
	  
	   spin_lock_irqsave(&SD_rw_lock, flags); 
	   if ((CURRENT->cmd)==READ) 
	       status=SD_read(CURRENT);
	   else
	       status=SD_write(CURRENT);	 	    	
	   	   
	   spin_unlock_irqrestore(&SD_rw_lock, flags);	  

        } 
}
#endif
void SD_issue_request(void) {
	
	SD_request(NULL);
}
	        
static int SD_blk_open(struct inode *inode, struct file *filp)
{
        
        SD_Flow_Dbg("SD_blk_open:CurrentSDCard= %x\n",CurrentSDCard);
        
        if ( CurrentSDCard == NULL ) {		
             
             CurrentSDCard=SD_card_discovery(); /*LTPE Joe if there is no card exist,detect it*/
             
             if ( CurrentSDCard == NULL ) {
                 printk(KERN_INFO "there is no card !!\n");
                 return (-1);
             }    	
        }
          
	return 0;
}

static int SD_blk_release(struct inode *inode, struct file *filp)
{
        kfree(CurrentSDCard);
        CurrentSDCard=NULL;
	return 0;
}

static int
SD_blk_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	/*struct block_device *bdev = inode->i_bdev;

	if (cmd == HDIO_GETGEO) {
		struct hd_geometry geo;

		memset(&geo, 0, sizeof(struct hd_geometry));

		geo.cylinders	= get_capacity(bdev->bd_disk) / (4 * 16);
		geo.heads	= 4;
		geo.sectors	= 16;
		geo.start	= get_start_sect(bdev);

		return copy_to_user((void __user *)arg, &geo, sizeof(geo))
			? -EFAULT : 0;
	}

	return -ENOTTY;*/
}

static struct block_device_operations SD_bdops = {
	.open			= SD_blk_open,
	.release		= SD_blk_release,
	.ioctl			= SD_blk_ioctl,
	.owner			= THIS_MODULE,
};

void Register_SD_Disk(void) {    
    int* TmpPtr;
    
    read_ahead[SD_MAJOR]=8;
    
    TmpPtr=kmalloc(SD_MINOR*sizeof(int),GFP_KERNEL);
    if (!TmpPtr)
        printk(KERN_INFO "Can't allocate memory for blk size\n");
    TmpPtr[SD_MINOR]=((CurrentSDCard->csd).DevSize)>>10;/*LTPE Joe KBs unit*/     
    blk_size[SD_MAJOR]=TmpPtr;
    
    TmpPtr=kmalloc(SD_MINOR*sizeof(int),GFP_KERNEL);
    if (!TmpPtr)
        printk(KERN_INFO "Can't allocate memory for blksize_size\n");
    TmpPtr[SD_MINOR]=(CurrentSDCard->csd).BlkSize;  
    blksize_size[SD_MAJOR]=TmpPtr;
    
    TmpPtr=kmalloc(SD_MINOR*sizeof(int),GFP_KERNEL);
    if (!TmpPtr)
        printk(KERN_INFO "Can't allocate memory for hardsect_size\n");
    TmpPtr[SD_MINOR]=(CurrentSDCard->csd).BlkSize;     
    hardsect_size[SD_MAJOR]=TmpPtr;
    
    register_disk(NULL,MKDEV(SD_MAJOR,SD_MINOR),1,&SD_bdops,(CurrentSDCard->csd).BlkNums/*LTPE Joe the number of sector*/);
    
}
    
void Unregister_SD_Disk(void) {
    
    
    read_ahead[SD_MAJOR]=0;
    kfree(blk_size[SD_MAJOR]);
    blk_size[SD_MAJOR]=NULL;
    kfree(blksize_size[SD_MAJOR]);
    blksize_size[SD_MAJOR]=NULL;
    kfree(hardsect_size[SD_MAJOR]);
    hardsect_size[SD_MAJOR]=NULL;
    
}  
static int __init SD_blk_init(void)
{
	int res = -ENOMEM;
        
        SD_Flow_Dbg("SD_blk_init\n");
	res = register_blkdev(SD_MAJOR, "SD",&SD_bdops);/*LTPE Joe register block driver in linux 2.4*/
	if (res < 0) {
		printk(KERN_INFO "Unable to get major %d for SD media: %d\n",
		       SD_MAJOR, res);
		goto out;
	}

        blk_init_queue(BLK_DEFAULT_QUEUE(SD_MAJOR),SD_request);/*LTPE Joe to register the request function*/        
        MCI_init();/*LTPE Joe init the SD driver*/
        if (CurrentSDCard)
            Register_SD_Disk();
out:  
	return res;
}

static void __exit SD_blk_exit(void)
{
        fsync_dev(MKDEV(SD_MAJOR,SD_MINOR));
	unregister_blkdev(SD_MAJOR, "SD");
	blk_cleanup_queue(BLK_DEFAULT_QUEUE(SD_MAJOR));
	if (CurrentSDCard)
            kfree(CurrentSDCard);
}

module_init(SD_blk_init);
module_exit(SD_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SD Card (SD) block device driver");


