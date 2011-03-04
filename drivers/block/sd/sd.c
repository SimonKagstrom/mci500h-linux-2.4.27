#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/spinlock.h>
#include <linux/blkdev.h>
#include <linux/timer.h>
#include <linux/pci.h>
#include <linux/smp_lock.h>

#include <asm/arch/sdma.h>
#include <asm/arch/gpio.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/syscreg.h>
#include <asm/arch/event_router.h>
#include <asm/arch/tsc.h>

#include "sd.h"
#include "mci_reg.h"
#include "SD_protocol.h"

#define SD_RW_TIMEOUT 0xfff
#define MAX_REPEAT_COUNT 10
#define SD_DETECT_INT_MASK 0x80000

#ifdef SD_NO_SDMA_IRQ
#define SDMA_IRQ_STAT		(IO_ADDRESS_SDMA_BASE + 0x404)
#endif

#define SD_CARD_NOT_INSERTED		1
#define SD_CARD_INSERTED		0

static U32 SdmaChOut,SdmaChIn;/*LTPE Joe Channel numbers of SDMA engine*/
static void* SdmaChOut_Buf;
static void* SdmaChIn_Buf;
static int currentSDCardStatus = SD_CARD_NOT_INSERTED;

U32 SD_busy=0;

spinlock_t SD_dma_lock=SPIN_LOCK_UNLOCKED;
spinlock_t SD_reg_lock=SPIN_LOCK_UNLOCKED;
spinlock_t SD_rw_lock=SPIN_LOCK_UNLOCKED;
spinlock_t SD_request_lock=SPIN_LOCK_UNLOCKED;
spinlock_t SD_polling_lock=SPIN_LOCK_UNLOCKED;

struct SD_card* CurrentSDCard=NULL;
struct timer_list SD_timer;

static void SD_polling_timer (void);

extern void SD_issue_request(void);
extern char hotplug_path [];

static pid_t poll_thread = 0;
static s16  sd_hotplug_count = -1;

static U32 SD_insert_status (void) {
    U8 RetVal;
    
    //gpio_set_as_function(GPIO_IEEE1394_D0);
    gpio_set_as_input(GPIO_IEEE1394_D0);
    RetVal=gpio_get_value(GPIO_IEEE1394_D0);
    //printk(KERN_INFO "Card detect status=%d\n",RetVal);
    return (RetVal);
}
    
static void SD_remove(void) {
	
    kfree(CurrentSDCard);
    CurrentSDCard=NULL;
    printk(KERN_INFO "Card have been removed.\n");
}    	

static void SD_setting_timer (void) {
    
    init_timer(&SD_timer);
    SD_timer.function=SD_polling_timer;
    SD_timer.expires= jiffies +1*HZ;/*LTPE Joe Polling inserted status every one second*/
    add_timer(&SD_timer);
}  

static int poll_status(void *ptr)
{
	lock_kernel();
	/*
	 * This thread doesn't need any user-level access,
	 * so get rid of all our resources
	 */

	daemonize();
	reparent_to_init();

	/* Setup a nice name */
	strcpy(current->comm, "sd_status_poll");

	/* Send me a signal to get me die (for debugging) */
	do {
		SD_polling_timer();
		schedule_timeout(HZ *3 );
	} while (!signal_pending(current));

	printk(KERN_DEBUG "sd status thread exiting");
	unlock_kernel();
}

static void SD_polling_timer (void) {
    U32 RetVal,flags;
	char *argv [3], **envp;
	char scratch[25];
    
	/* only one standardized param to hotplug command: type */
	argv [0] = hotplug_path;
	argv [1] = "sd";
	argv [2] = 0;

	if (!(envp = (char **) kmalloc (20 * sizeof (char *), GFP_KERNEL))) {
		return;
	}
	/* minimal command environment */
	envp [0] = "HOME=/";
	envp [1] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	envp[4] = 0;
    spin_lock_irqsave(&SD_polling_lock, flags);  
    //printk(KERN_INFO "Polling timer\n");
    //DumpMciReg();
    //DumpSdmaReg();
    RetVal=SD_insert_status();
    if(currentSDCardStatus != RetVal) { 
	sd_hotplug_count++;
	sprintf(scratch, "COUNT=%04X", sd_hotplug_count);
	envp[3] = scratch;
    	if (RetVal==0 ) {/*LTPE Joe there is card in slot*/
		currentSDCardStatus = SD_CARD_INSERTED;
        	if (CurrentSDCard==NULL) {/*LTPE Joe this card doesn't be init, Init it now !!*/
            		CurrentSDCard=SD_card_discovery();  
            		if (CurrentSDCard==NULL) 
                		printk(KERN_INFO " Card Init failed !!\n");
            		else {
				envp[2] = "ACTION=add";
				call_usermodehelper (argv[0], argv, envp);
                		printk(KERN_INFO " Card Init success !!\n");      
	    		}
        	} 
	}
        //else
         //   printk(KERN_INFO "Card have been already initilized.\n");  
    	//}
    	else {
    		if (CurrentSDCard)  /*LTPE Joe card have been removed*/  {
    	    		SD_remove();                 
			envp[2] = "ACTION=remove";
			call_usermodehelper (argv[0], argv, envp);
	    		currentSDCardStatus = SD_CARD_NOT_INSERTED;
		}
    	}
    }
    //SD_setting_timer();/*LTPE Joe call polling timer again*/
    spin_unlock_irqrestore(&SD_polling_lock, flags); 
    kfree(envp);
}    	  	    	 

inline U32 SD_RW_reg(U32 Reg,U32 arg,U8 RW) {
    U32 TmpVal=0;
    U32 flags;
    
    //printk(KERN_INFO "SD_RW_reg:RegAddr=%x,Arg=%x\n",Reg,arg);
    spin_lock_irqsave(&SD_reg_lock, flags);    
    if (RW==SD_REG_WR) 
        writel(arg,Reg);
    else
        TmpVal=readl(Reg);
    //printk(KERN_INFO "TmpVal=%x\n",TmpVal);
    spin_unlock_irqrestore(&SD_reg_lock, flags);    
    return ( TmpVal);
}             	

void DumpCsd (struct SD_card* pCard) {
#ifdef SD_INIT_DBG    
    printk(KERN_INFO "\n");
    printk(KERN_INFO "DUMP CSD-----------------\n");
    printk(KERN_INFO "BlkSize=%x\n",(pCard->csd).BlkSize);
    printk(KERN_INFO "BlkSizeFactor=%x\n",(pCard->csd).BlkSizeFactor);
    printk(KERN_INFO "BlkNums=%x\n",(pCard->csd).BlkNums);
    printk(KERN_INFO "EraseBlkSize=%x\n",(pCard->csd).EraseBlkSize);
    printk(KERN_INFO "DevSize=%x\n",(pCard->csd).DevSize);
    printk(KERN_INFO "DataTransSpeed=%x\n",(pCard->csd).DataTransSpeed);
    printk(KERN_INFO "c_size=%x\n",(pCard->csd).c_size);
    printk(KERN_INFO "c_size_mult=%x\n",(pCard->csd).c_size_mult);
    printk(KERN_INFO "\n");
#endif    
}

void DumpSysReg(void) {
    U32 RegVal;
    
    RegVal=SD_RW_reg(VA_SYSCREG_SSA1_MCI_PL180_PRESERVED,0,SD_REG_RD);
    printk(KERN_INFO "VA_SYSCREG_SSA1_MCI_PL180_PRESERVED=%x\n", RegVal);
    RegVal=SD_RW_reg(VA_SYSCREG_MUX_DAI_MCI,0,SD_REG_RD);
    printk(KERN_INFO "VA_SYSCREG_MUX_DAI_MCI=%x\n", RegVal);
    RegVal=SD_RW_reg(VA_SYSCREG_MUX_PLCD_MCI ,0,SD_REG_RD);
    printk(KERN_INFO "VA_SYSCREG_MUX_PLCD_MCI =%x\n", RegVal);

} 

void DumpMciReg(void) {
    U32 RegVal;
#ifdef MCI_DUMP    
    /*RegVal=SD_RW_reg(MCICLOCK,0,SD_REG_RD);
    printk(KERN_INFO "MCICLOCK=%x\n", RegVal);
    RegVal=SD_RW_reg(MCISTATUS,0,SD_REG_RD);
    printk(KERN_INFO "MCISTATUS=%x\n", RegVal);*/
    RegVal=SD_RW_reg(MCICOMMAND,0,SD_REG_RD);
    printk(KERN_INFO "MCICOMMAND=%x\n", RegVal);
    RegVal=SD_RW_reg(MCIDATACTRL,0,SD_REG_RD);
    printk(KERN_INFO "MCIDATACTRL=%x\n", RegVal);
    /*RegVal=SD_RW_reg(MCIPCELLID2,0,SD_REG_RD);
    printk(KERN_INFO "MCIPCELLID2=%x\n", RegVal);
    RegVal=SD_RW_reg(MCIPCELLID3,0,SD_REG_RD);
    printk(KERN_INFO "MCIPCELLID3=%x\n", RegVal);
    RegVal=SD_RW_reg(MCIPOWER,0,SD_REG_RD);
    printk(KERN_INFO "MCIPOWER=%x\n",RegVal);*/
    RegVal=SD_RW_reg(MCIDATALENGTH,0,SD_REG_RD);
    printk(KERN_INFO "MCIDATALENGTH=%x\n",RegVal);
    RegVal=SD_RW_reg(MCIDATACNT,0,SD_REG_RD);
    printk(KERN_INFO "MCIDATACNT=%x\n",RegVal);
    RegVal=SD_RW_reg(MCISTATUS,0,SD_REG_RD);
    printk(KERN_INFO "MCISTATUS=%x\n",RegVal);
    RegVal=SD_RW_reg(MCIFIFO,0,SD_REG_RD);
    printk(KERN_INFO "MCIFIFO=%x\n",RegVal);
    RegVal=SD_RW_reg(MCIFIFOCNT,0,SD_REG_RD);
    printk(KERN_INFO "MCIFIFOCNT=%x\n",RegVal);
    RegVal=SD_RW_reg(MCIMASK0,0,SD_REG_RD);
    printk(KERN_INFO "MCIMASK0=%x\n",RegVal);
    RegVal=SD_RW_reg(MCIMASK1,0,SD_REG_RD);
    printk(KERN_INFO "MCIMASK1=%x\n",RegVal);
#endif    
}  

void DumpSdmaReg(void) {
    U32 RegVal;
#ifdef SDMA_DUMP

#ifdef SDMA_DUMP_OUT    
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChOut)),0,SD_REG_RD);
    printk(KERN_INFO "SdmaSrc[%d]=%x\n",SdmaChOut,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChOut)+0x4),0,SD_REG_RD);
    printk(KERN_INFO "SdmaDst[%d]=%x\n",SdmaChOut,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChOut)+0x10),0,SD_REG_RD);
    printk(KERN_INFO "SdmaEna[%d]=%x\n",SdmaChOut,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChOut)+0xc),0,SD_REG_RD);
    printk(KERN_INFO "SdmaConfig[%d]=%x\n",SdmaChOut,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChOut)+0x8),0,SD_REG_RD);
    printk(KERN_INFO "SdmaTransLength[%d]=%x\n",SdmaChOut,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChOut)+0x1c),0,SD_REG_RD);
    printk(KERN_INFO "SdmaTransCounter[%d]=%x\n",SdmaChOut,RegVal);
#endif
#ifdef SDMA_DUMP_IN    
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChIn)),0,SD_REG_RD);
    printk(KERN_INFO "SdmaSrc[%d]=%x\n",SdmaChIn,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChIn)+0x4),0,SD_REG_RD);
    printk(KERN_INFO "SdmaDst[%d]=%x\n",SdmaChIn,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChIn)+0x10),0,SD_REG_RD);
    printk(KERN_INFO "SdmaEna[%d]=%x\n",SdmaChIn,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChIn)+0xc),0,SD_REG_RD);
    printk(KERN_INFO "SdmaConfig[%d]=%x\n",SdmaChIn,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChIn)+0x8),0,SD_REG_RD);
    printk(KERN_INFO "SdmaTransLength[%d]=%x\n",SdmaChIn,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * SdmaChIn)+0x1c),0,SD_REG_RD);
    printk(KERN_INFO "SdmaTransCounter[%d]=%x\n",SdmaChIn,RegVal);
#endif 
#ifdef SDMA_DUMP_AUDIO   
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * 2)),0,SD_REG_RD);
    printk(KERN_INFO "SdmaSrc[%d]=%x\n",2,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * 2)+0x4),0,SD_REG_RD);
    printk(KERN_INFO "SdmaDst[%d]=%x\n",2,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * 2)+0x10),0,SD_REG_RD);
    printk(KERN_INFO "SdmaEna[%d]=%x\n",2,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * 2)+0xc),0,SD_REG_RD);
    printk(KERN_INFO "SdmaConfig[%d]=%x\n",2,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * 2)+0x8),0,SD_REG_RD);
    printk(KERN_INFO "SdmaTransLength[%d]=%x\n",2,RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + (0x20 * 2)+0x1c),0,SD_REG_RD);
    printk(KERN_INFO "SdmaTransCounter[%d]=%x\n",2,RegVal);
#endif    
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + 0x404),0,SD_REG_RD);
    printk(KERN_INFO "IntStatus=%x\n",RegVal);
    RegVal=SD_RW_reg((IO_ADDRESS_SDMA_BASE + 0x408),0,SD_REG_RD);
    printk(KERN_INFO "IntMask=%x\n",RegVal);
    
#endif/*SDMA_DUMP*/    
}    
void DumpCguReg(void) {
    U32 RegVal;
    
    RegVal=SD_RW_reg(PCR_MCI_PCLK,0,SD_REG_RD);
    printk(KERN_INFO "PCR_MCI_PCLK=%x\n",RegVal);
    RegVal=SD_RW_reg(PCR_MCI_MCLK,0,SD_REG_RD);
    printk(KERN_INFO "PCR_MCI_MCLK=%x\n",RegVal);
    RegVal=SD_RW_reg(PSR_MCI_PCLK,0,SD_REG_RD);
    printk(KERN_INFO "PSR_MCI_PCLK=%x\n",RegVal);
    RegVal=SD_RW_reg(PSR_MCI_MCLK,0,SD_REG_RD);
    printk(KERN_INFO "PSR_MCI_MCLK=%x\n",RegVal);
    RegVal=SD_RW_reg(SCR_CLK_MCI,0,SD_REG_RD);
    printk(KERN_INFO "SCR_CLK_MCI=%x\n",RegVal);
    RegVal=SD_RW_reg(FS1_CLK_MCI,0,SD_REG_RD);
    printk(KERN_INFO "FS1_CLK_MCI=%x\n",RegVal);
    RegVal=SD_RW_reg(FS2_CLK_MCI,0,SD_REG_RD);
    printk(KERN_INFO "FS2_CLK_MCI=%x\n",RegVal);
    RegVal=SD_RW_reg(SSR_CLK_MCI,0,SD_REG_RD);
    printk(KERN_INFO "SSR_CLK_MCI=%x\n",RegVal);
    RegVal=SD_RW_reg(ESR_MCI_PCLK,0,SD_REG_RD);
    printk(KERN_INFO "ESR_MCI_PCLK=%x\n",RegVal);
    RegVal=SD_RW_reg(ESR_MCI_MCLK,0,SD_REG_RD);
    printk(KERN_INFO "ESR_MCI_MCLK=%x\n",RegVal);

} 
void MCI_Set_CGU(void) {
      
    SD_RW_reg(SCR_CLK_MCI,0x3,SD_REG_WR);
    SD_RW_reg(PCR_MCI_PCLK,0x7,SD_REG_WR);
    SD_RW_reg(PCR_MCI_MCLK,0x7,SD_REG_WR);
    SD_RW_reg(PSR_MCI_PCLK,0x3,SD_REG_WR);
    SD_RW_reg(PSR_MCI_MCLK,0x2,SD_REG_WR);
    
  //  SD_RW_reg(PCR_IOCONF_PCLK,0x1,SD_REG_WR);/*LTPE Joe setting for IOCONF*/
  //  SD_RW_reg(PSR_IOCONF_PCLK,0x1,SD_REG_WR);
    
}		
void SD_set_dma (void* MemAddr,U32 length,U8 Out) {
        
    U32 BusAddr;
    U32 flags;
    u32 len;
    
    spin_lock_irqsave(&SD_dma_lock, flags);
    if (Out == DMA_DATA_OUT) {
    	//BusAddr=pci_map_single(NULL,MemAddr,length,PCI_DMA_BIDIRECTIONAL);
    	//printk(KERN_INFO "Physical address of SdmaChOut=%x\n",BusAddr);
    	//sdma_set_src_addr(SdmaChOut,BusAddr);
    	memcpy (SdmaChOut_Buf,MemAddr,length);
    	//sdma_set_len(SdmaChOut,(length / sizeof(U32))-1/*+7*/ );/*LTPE Joe for TEST*/
    	len = (length / sizeof(U32))- 7;/*Srini TEST*/
	printk("Setting out len : %d\n", len);
    	sdma_set_len(len - 1 );/*Srini TEST*/
    	sdma_start_channel(SdmaChOut);
    }
    else {
    	//BusAddr=pci_map_single(NULL,MemAddr,length,PCI_DMA_BIDIRECTIONAL);
    	//printk(KERN_INFO "Physical address of SdmaChIn=%x\n",BusAddr);
    	//sdma_set_dst_addr(SdmaChIn,BusAddr);
    	memset(SdmaChIn_Buf,0,4096);
    	//sdma_set_len(SdmaChIn,(length / sizeof(U32))-1 );/*LTPE Joe the unit of the transfer length in the SDMA engine is times ,not bytes --> Actual transfer times == Setting transfer times +1*/
	if( length < 512) {
    		sdma_set_len((length / sizeof(u32)) - 1 );/*Srini TEST*/
	}else {
    		len = (length / sizeof(U32))- 7;/*Srini TEST*/
		printk("Setting in len : %d\n", len);
	
    		sdma_set_len(len - 1 );/*Srini TEST*/
	}
    	//sdma_set_len(SdmaChIn,((length / sizeof(U32))-1 -7) );/*Srini Test*/
     	sdma_start_channel(SdmaChIn);
    }
    spin_unlock_irqrestore(&SD_dma_lock, flags);
   	
}  

U32 SD_send_cmd( struct sd_command* cmd) { 
    
    U32 CmdRegVal,RetVal,i,RegVal; 
    
    SD_Flow_Dbg("SD_send_cmd:cmd=%d,arg=%x\n",cmd->opcode,cmd->arg);
    if (cmd->data) {
    	return(-1);
    	/*LTPE Joe the data need to be sent in this cmd*/
    }
    else {/*LTPE Joe the data don't need to be sent in this cmd*/	 	
        
        SD_RW_reg(MCIARGUMENT,cmd->arg,SD_REG_WR);
       
        switch(cmd->flags) {
            
            case SD_RSP_NONE:            	
                CmdRegVal=cmd->opcode|CPSM_ENABLE;
            break;
            
            case SD_RSP_R1:
            case SD_RSP_R1B:
            case SD_RSP_R3:
            case SD_RSP_R6:	
                CmdRegVal=cmd->opcode|CPSM_ENABLE|CPSM_WAIT_RSP;
            break; 
            
            case SD_RSP_R2:
                CmdRegVal=cmd->opcode|CPSM_ENABLE|CPSM_WAIT_RSP|CPSM_LONG_RSP; 
            break;
            
            default:
                printk(KERN_INFO "There isn't valid response type :%d\n",cmd->flags);    
        }
              
        SD_RW_reg(MCICOMMAND,CmdRegVal,SD_REG_WR); /*LTPE Joe send the command now*/ 
        
        do {                	
            RetVal= SD_RW_reg(MCISTATUS,0,SD_REG_RD);
        }while((RetVal & MCI_STATUS_CmdActive_Mask) > 0);  /*LTPE Joe wait for cmd completed*/ 
                     
          
        if ( ((RetVal & ( MCI_STATUS_CmdSent_Mask | MCI_STATUS_CmdRespEnd_Mask ))==0) && (cmd->flags != SD_RSP_R3) ){/*LTPE Joe there are some error happened,and there is no CRC in R3*/   
           
           if ( (RetVal & MCI_STATUS_CmdCrcFail_Mask)>0)
               printk(KERN_INFO "MCI_STATUS_CmdCrcFail_Mask\n");
           if ( (RetVal & MCI_STATUS_CmdTimeOut_Mask)>0)
               printk(KERN_INFO "MCI_STATUS_CmdTimeOut_Mask\n");
            
           SD_RW_reg(MCICLEAR,0x3ff,SD_REG_WR); /*LTPE Joe clear all status */
           SD_RW_reg(MCICOMMAND,0x0,SD_REG_WR); /*LTPE Joe clear the command*/             
           return (-1);  
             
        }
        else {
        	     
             if ( cmd->flags==SD_RSP_NONE) {
                 SD_RW_reg(MCICLEAR,0x3ff,SD_REG_WR); /*LTPE Joe clear all status */
                 SD_RW_reg(MCICOMMAND,0x0,SD_REG_WR); /*LTPE Joe clear the command*/
                 
                 return 0;
             }
             else {
             	if ( cmd->flags==SD_RSP_R2){/*LTPE Joe long response*/
             	    for (i=0;i<4;i++) {	
             	        RetVal= SD_RW_reg((MCIRESPONSE0+4*i),0,SD_REG_RD);
             	        cmd->resp[i]= RetVal; 
             	    }
             	}    
             	else {/*LTPE Joe short response*/
             	    RetVal= SD_RW_reg(MCIRESPONSE0,0,SD_REG_RD);
             	    cmd->resp[0]= RetVal;
             	    //SD_Flow_Dbg(KERN_INFO "The return short SD response =%x\n",RetVal);
             	    SD_Flow_Dbg("The return short SD response =%x\n",RetVal);
             	}
              }
              RegVal=SD_RW_reg(MCISTATUS,0,SD_REG_RD);
              SD_Flow_Dbg(KERN_INFO "MCISTATUS=%x\n", RegVal);
              SD_RW_reg(MCICLEAR,0x3ff,SD_REG_WR); /*LTPE Joe clear all status */
              SD_RW_reg(MCICOMMAND,0x0,SD_REG_WR); /*LTPE Joe clear the command*/
              
              return 0;	              
        }
    }    
}                  	          

void MCI_SetDataOut(U32 length) {
     U32 TmpVal=0;
#ifdef SD_DMA_MODE     
     TmpVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataOut|MCI_StreamTransfer|MCI_DmaEnable|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
     //TmpVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataOut|MCI_BlkTransfer|MCI_DmaEnable|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
     writel(0xffff,MCIDATATIMER);
     //writel(length+28,MCIDATALENGTH); /*LTPE Joe for TEST*/      
     writel(length,MCIDATALENGTH); /*Srini TEST*/      
     writel(TmpVal,MCIDATACTRL);
#else
     TmpVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataOut|MCI_BlkTransfer|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
     writel(0xffff,MCIDATATIMER);
     writel(length,MCIDATALENGTH);     
     writel(TmpVal,MCIDATACTRL); 
#endif         
}

void MCI_SetDataIn(U32 length) {
     U32 TmpVal;
#ifdef SD_DMA_MODE       
     TmpVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataIn|MCI_StreamTransfer|MCI_DmaEnable|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
     //TmpVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataIn|MCI_BlkTransfer|MCI_DmaEnable|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
     writel(0xffff,MCIDATATIMER); 
     //writel(length+28,MCIDATALENGTH); /*LTPE Joe for TEST*/    
     writel(length,MCIDATALENGTH); /*Srini TEST*/    
     writel(TmpVal,MCIDATACTRL);
#else
     TmpVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataIn|MCI_BlkTransfer|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
     writel(0xffff,MCIDATATIMER); 
     writel(length,MCIDATALENGTH); 
     writel(TmpVal,MCIDATACTRL); 
#endif        
}
	
struct sd_command* SD_issue_cmd (U8 command,U32 arg,void* buf) {
    
    struct sd_command* cmd;
    struct sd_command App_cmd;
    U32 result;
   
    cmd=kmalloc(sizeof(struct sd_command),GFP_KERNEL);
    if ( command > 56)
        cmd->opcode = command-56;/*LTPE Joe the application command*/
    else
        cmd->opcode = command;/*LTPE Joe the normal command*/  
          
    cmd->arg = arg;
    cmd->data= buf;
   
    switch (command) {
    	
    	case SD_GO_IDLE_STATE:
    	case SD_SET_DSR:
    	case SD_GO_INACTIVE_STATE:
    	    cmd->flags=SD_RSP_NONE;
    	break;
    	
    	case SD_SELECT_CARD :
    	case SD_SEND_STATUS :
    	case SD_SET_BLOCKLEN :
    	case SD_READ_SINGLE_BLOCK :
    	case SD_READ_MULTIPLE_BLOCK :
    	case SD_WRITE_DAT_UNTIL_STOP :
    	case SD_WRITE_BLOCK :
    	case SD_WRITE_MULTIPLE_BLOCK :
    	case SD_PROGRAM_CSD :
    	case SD_SEND_WRITE_PROT :
    	case SD_ERASE_WR_BLK_START :
    	case SD_ERASE_WR_BLK_END :
    	case SD_LOCK_UNLOCK :
    	case SD_APP_CMD :
    	case SD_GEN_CMD :
    	case SD_APP_SET_BUS_WIDTH :
    	case SD_SD_STATUS:
    	case SD_SEND_NUM_WR_BLOCKS:
    	case SD_SETWR_BLK_ERASE_COUNT:
    	case SD_APP_SEND_SCR:
    	    cmd->flags=SD_RSP_R1;
    	break;
    	
    	case SD_STOP_TRANSMISSION:
    	case SD_SET_WRITE_PROT:
    	case SD_CLR_WRITE_PROT:
    	case SD_ERASE:
    	    cmd->flags=SD_RSP_R1B;
    	break;
    	    
    	case SD_ALL_SEND_CID:
    	case SD_SEND_CSD:
    	case SD_SEND_CID:
    	    cmd->flags=SD_RSP_R2;
    	break;
    	
    	case SD_SEND_OP_COND:
    	case SD_APP_OP_COND: 
    	    cmd->flags=SD_RSP_R3;
    	break;
    	
    	case SD_SEND_RELATIVE_ADDR:
    	    cmd->flags=SD_RSP_R6;
    	break;
        default:
            printk(KERN_INFO "Wrong SD Command !!\n");
            kfree(cmd);
            return (NULL);   
    }	  
    
    if (command >56) {
    	App_cmd.opcode = SD_APP_CMD ;/*LTPE Joe send CMD 55 first*/            
        
        if (CurrentSDCard != NULL)
            App_cmd.arg = CurrentSDCard->rca << 16;/*With RCA=0x0 is described in spec chapter 4.2.3*/
        else    
            App_cmd.arg = 0x0;/*With RCA=0x0 is described in spec chapter 4.2.3*/
        
        App_cmd.data= NULL;	
	App_cmd.flags=SD_RSP_R1;
	
	result=SD_send_cmd(&App_cmd);

	if (result != 0) {
	    printk(KERN_INFO "Can't send application cmd :%d\n",App_cmd.opcode);
            kfree(cmd);
	    return (NULL);
	}    	
	
    }
     
    result=SD_send_cmd(cmd);

    if (result !=0) { 
        printk(KERN_INFO "Can't send cmd :%d\n",cmd->opcode);
        kfree(cmd);
	return (NULL);
    }
    
    return (cmd);
  	
} 

void SD_Pio_RW_handle (U32 flag) {     
     struct sd_command* pCmd;
     U32 flags,i,RetVal,RegVal;
     
     spin_lock_irqsave(&SD_request_lock, flags);  
     
     if (CurrentSDCard) {/*LTPE Joe send STOP command to stop the transfer*/                 
         DumpMciReg();                                                                      	                                      
         if (flag == SD_Request_OK)
             end_request(1);
         else
             end_request(0);                             	    
     }    
     else {
     	 printk(KERN_INFO "No Card Exist !!\n");
     	 end_request(0);
     }
          
     spin_unlock_irqrestore(&SD_request_lock, flags);
}    	

void SD_dma_isr (unsigned int irqStat) {     
     struct sd_command* pCmd;
     U32 flags,i,RetVal,RegVal;
     u32 *buff = NULL;
     
	//printk("SDMA ISR\n ");
     spin_lock_irqsave(&SD_request_lock, flags);      

		 
     if (CurrentSDCard) {/*LTPE Joe send STOP command to stop the transfer*/
        
         DumpMciReg();
         DumpSdmaReg();
         RegVal=SD_RW_reg(MCISTATUS,0,SD_REG_RD);
         printk(KERN_INFO "MCISTATUS=%x\n",RegVal);
         
         /*pCmd=SD_issue_cmd (SD_STOP_TRANSMISSION,0,NULL);
         if (pCmd ==NULL) {
             printk(KERN_INFO "Cmd issue failed :cmd=%d\n",SD_STOP_TRANSMISSION);
             return ;
         }
         kfree(pCmd);*/
     if ((CURRENT->cmd)==READ) { 
	buff = (u32 *)((char *)SdmaChIn_Buf +  (((CurrentSDCard->csd).BlkSize)*(CURRENT->current_nr_sectors) - 28));
	//buff = (u32 *)(SdmaChIn_Buf +  (512 - 28));
	i=7;
	//printk("Reading extra 7 words from %x", buff);
     	while(i) {
		*buff = readl(MCIFIFO);
		buff++;
		i--;
	}
     }else {
	buff = (u32 *)((char *)SdmaChOut_Buf +  (((CurrentSDCard->csd).BlkSize)*(CURRENT->current_nr_sectors) - 28));
	//buff = (u32 *)(SdmaChOut_Buf +  (512 - 28));
	i=7;
	//printk("Writing extra 7 words ");
     	while(i) {
		writel(*buff, MCIFIFO);
		buff++;
		i--;
	}

     }
         
         SD_RW_reg(MCIDATACTRL,0,SD_REG_WR);
         SD_RW_reg(MCIDATACTRL,1,SD_REG_WR);
         SD_RW_reg(MCIDATALENGTH,0,SD_REG_WR);/*LTPE Joe reset the data controller*/
         
         sdma_reset_count(SdmaChOut);
         sdma_reset_count(SdmaChIn);  

	 /* transfer rest of the 7 words */

        
         if ( /*(RegVal & MCI_STATUS_DataEnd_Mask) > 0*/1 ) {                                                          
                     
             if ((CURRENT->cmd)==READ) { 
	         memcpy((void*)(CURRENT->buffer),SdmaChIn_Buf,((CurrentSDCard->csd).BlkSize)*(CURRENT->current_nr_sectors));	         
#ifdef SD_DUMP_IN_DATA	         
	         for (i=0;i<(((CurrentSDCard->csd).BlkSize)*(CURRENT->current_nr_sectors));i++ )
                     printk(KERN_INFO "BLK Data[%d]=%x\n",i,*((U8*)SdmaChIn_Buf+i));
#endif             
             }        	             
    	                                      
             end_request(1);
             SD_busy=0;
             SD_RW_reg(MCICLEAR,0x3ff,SD_REG_WR); /*LTPE Joe clear all status */ 
             spin_unlock_irqrestore(&SD_request_lock, flags);
                      
             SD_issue_request();
         }
         else {
             printk(KERN_INFO "Data sending fail !!\n");                               
             end_request(0);
             SD_busy=0; 
             spin_unlock_irqrestore(&SD_request_lock, flags);
                       
             SD_issue_request();
         }         	    
     }    
     else {
     	 printk(KERN_INFO "No Card Exist !!\n");
         end_request(0);
         SD_busy=0; 
     }   
     spin_unlock_irqrestore(&SD_request_lock, flags);
}    	

void SD_data_send (void* buffer,U32 length) {

#ifdef SD_DMA_MODE    
    SD_set_dma(	buffer,length,DMA_DATA_OUT);
    MCI_SetDataOut(length);
#ifdef SD_NO_SDMA_IRQ
    while(!(readl(SDMA_IRQ_STAT) &  (1 << SdmaChOut)));
    printk("Got out ISR\n");
    SD_dma_isr(readl(SDMA_IRQ_STAT));
#endif
#else
    MCI_SetDataOut(length);
#endif     
       
}

void SD_data_get (void* buffer,U32 length) {
#ifdef SD_DMA_MODE      
    SD_Flow_Dbg("SD_data_get:length=%x\n",length);
    SD_set_dma(buffer,length,DMA_DATA_IN);
    MCI_SetDataIn(length);
#ifdef SD_NO_SDMA_IRQ
    while(!(readl(SDMA_IRQ_STAT) &  (1 << SdmaChIn)));
    printk("Got out ISR\n");
    SD_dma_isr(readl(SDMA_IRQ_STAT));
#endif
#else
    MCI_SetDataIn(length);
#endif        
} 

static inline void SD_Reset_REG (void) {
    writel(0,MCIDATACTRL);/*LTPE Joe reset the data controller*/
    writel(0,MCIDATATIMER);
    writel(0,MCICOMMAND); /*LTPE Joe send the command now*/     
    writel(0x7ff,MCICLEAR); /*LTPE Joe clear all status */
}     

#ifdef SD_DMA_MODE

U32 SD_write(struct request *rq) {
    U32 length;
    struct sd_command* pCmd;
    
    if (CurrentSDCard) {
    	pCmd=SD_issue_cmd (SD_WRITE_MULTIPLE_BLOCK,( (rq->sector) * ((CurrentSDCard->csd).BlkSize) ),NULL);
        if (pCmd ==NULL) {
             printk(KERN_INFO "Cmd issue failed :cmd=%d\n",SD_WRITE_MULTIPLE_BLOCK);
             return (-1);
        }
    	kfree(pCmd);
        
        length=((CurrentSDCard->csd).BlkSize)*(rq->current_nr_sectors);
        SD_data_send((void*)(rq->buffer),length);
        
    }
    else {
    	printk(KERN_INFO "No Card Exist !!\n");
    	return (-1);
    }	
} 

U32 SD_read(struct request *rq) {
    U32 length;
    struct sd_command* pCmd;
    
    SD_Flow_Dbg("SD_read:StartAddr=%x,BlkNR=%x\n",rq->sector,rq->current_nr_sectors);
    if (CurrentSDCard) {
    	
    	length=((CurrentSDCard->csd).BlkSize)*(rq->current_nr_sectors);
        SD_data_get((void*)(rq->buffer),length);
    	
    	//pCmd=SD_issue_cmd (SD_READ_MULTIPLE_BLOCK,( (rq->sector) * ((CurrentSDCard->csd).BlkSize) ),NULL);
    	pCmd=SD_issue_cmd (SD_READ_SINGLE_BLOCK,( (rq->sector) * ((CurrentSDCard->csd).BlkSize) ),NULL);
        if (pCmd ==NULL) {
             printk(KERN_INFO "Cmd issue failed :cmd=%d\n",SD_READ_MULTIPLE_BLOCK);
             return (-1);
        }
        kfree(pCmd);
                       
    }
    else {
    	printk(KERN_INFO "No Card Exist !!\n");
    	return (-1);
   } 	
} 

#else/*LTPE Joe PIO MODE*/

U32 SD_write(struct request *rq) {
    U32 length,i,DataCnt,TotalLength,RegVal;
    U32 StartAddr,CmdRegVal,DataRegVal,TimeOutCount,PreStarAddr,RepeatCount=0;
    unsigned long flags;
    unsigned long tsc;
    
    SD_Flow_Dbg("SD_write(PIO MODE):StartAddr=%x,BlkNR=%x\n",rq->sector,rq->current_nr_sectors);
    PreStarAddr=(rq->sector) * ((CurrentSDCard->csd).BlkSize);
SDRepeatWR:
    i=0; /*LTPE Joe for retransfer condition*/    
    
    if (CurrentSDCard) {
    	length=((CurrentSDCard->csd).BlkSize)*(rq->current_nr_sectors); 
    	StartAddr=(rq->sector) * ((CurrentSDCard->csd).BlkSize);   	    	  	
    	if (PreStarAddr==StartAddr) {
    	    RepeatCount++;
    	    if (RepeatCount > MAX_REPEAT_COUNT) {
    	    	printk(KERN_INFO "SD_write Fail:StartAddr=%x,BlkNR=%x\n",rq->sector,rq->current_nr_sectors);
    	    	SD_Reset_REG();
    	        SD_Pio_RW_handle(SD_Request_Fail);/*LTPE Joe it is deadlock now*/
    	    }    
    	} 
    	
    	TotalLength=length;
    	

    	do {
    	    if (TotalLength > ((CurrentSDCard->csd).BlkSize))
    	        length=(CurrentSDCard->csd).BlkSize;
    	    else
    	        length=	TotalLength;
    	    SD_Flow_Dbg("SD_write:Addr=%x,Total Length=%x,i=%x,length=%x\n",(StartAddr+i*4),TotalLength,i,length);       
    	    CmdRegVal= SD_WRITE_BLOCK|CPSM_ENABLE|CPSM_WAIT_RSP;/*LTPE Joe the responce of read is R1 type*/    	
    	    DataRegVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataOut|MCI_BlkTransfer|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
            DataCnt=length / sizeof(U32);    
        
            writel((StartAddr+i*4),MCIARGUMENT);  
            writel(0xffff,MCIDATATIMER); 
            writel(length,MCIDATALENGTH);
            writel(DataRegVal,MCIDATACTRL); 

            writel(CmdRegVal,MCICOMMAND); /*LTPE Joe send the command now*/  
            TimeOutCount=SD_RW_TIMEOUT;	    
            do {/*LTPE Joe to wait responce receiving complete*/
                RegVal=readl(MCISTATUS);
                //printk(KERN_INFO "MCISTATUS in CMD=%x\n",RegVal);
                if ((RegVal & MCI_STATUS_CmdTimeOut_Mask) > 0) {/*LTPE Joe the card is busy*/
                    SD_Flow_Dbg(KERN_INFO "Resent Command\n");
                    SD_Reset_REG();
               
                    writel((StartAddr+i*4),MCIARGUMENT);  
                    writel(0xffff,MCIDATATIMER); 
                    writel(length,MCIDATALENGTH);
                    writel(DataRegVal,MCIDATACTRL); 

                    writel(CmdRegVal,MCICOMMAND); /*LTPE Joe send the command now*/      
                }
            	
            }while((RegVal & MCI_STATUS_CmdRespEnd_Mask) == 0); 
    	    
	    //tsc=ReadTSC();
    	    local_irq_save(flags);           
            while((DataCnt >0) && (TimeOutCount > 0)) {
                RegVal=readl(MCIFIFOCNT);
                
                if (DataCnt == RegVal) {	
                    writel(*((U32*)(rq->buffer)+i),MCIFIFO );
                    DataCnt--;
                    i++;
                }  
                else {
                    TimeOutCount--;
                }        	  
            }       	
    	    local_irq_restore(flags);           
	    //printk("wtime: %u\n",ReadTSC()-tsc); 
            //printk(KERN_INFO "TimeOutCount=%x\n",TimeOutCount);
            //DumpMciReg();
            if (TimeOutCount == 0) {
            	//printk(KERN_INFO "Time Out : Data sending\n");
            	SD_Reset_REG();
            	goto SDRepeatWR;
            }
            
            TimeOutCount=SD_RW_TIMEOUT;
            do {/*LTPE Joe to wait data receiving complete*/
                RegVal=readl(MCISTATUS);
                TimeOutCount--;
                //printk(KERN_INFO "MCISTATUS in DATA=%x\n",RegVal);
            }while((RegVal & MCI_STATUS_DataEnd_Mask) == 0);
            //printk(KERN_INFO "TimeOutCount1=%x\n",TimeOutCount);
            if (TimeOutCount == 0) {
            	printk(KERN_INFO "Time Out : MCISTATUS=%x\n",RegVal);
            	SD_Reset_REG();
            	goto SDRepeatWR;/*LTPE Joe do R/W operation again*/
            }
            
            TotalLength-=length;             
            SD_Reset_REG();
            //DumpMciReg();	      
	    schedule();
        }while(TotalLength != 0);
        
        SD_Pio_RW_handle(SD_Request_OK);  
                       
    }
    else {
    	printk(KERN_INFO "No Card Exist !!\n");
    	return (-1);
   } 
}

U32 SD_read(struct request *rq) {
    U32 length,i=0,j=0,DataCnt,TotalLength,RegVal;
    U32 StartAddr,CmdRegVal,DataRegVal,TimeOutCount,PreStarAddr,RepeatCount=0;
    unsigned long flags;
    unsigned long tsc;
    
    PreStarAddr=(rq->sector) * ((CurrentSDCard->csd).BlkSize);
SDRepeatRD:

    i=0;/*LTPE Joe for retransfer condition*/    
    //SD_Flow_Dbg("SD_read(PIO MODE):StartAddr=%x,BlkNR=%x\n",rq->sector,rq->current_nr_sectors);
    if (CurrentSDCard) {
    	length=((CurrentSDCard->csd).BlkSize)*(rq->current_nr_sectors); 
    	StartAddr=(rq->sector) * ((CurrentSDCard->csd).BlkSize);     	
    	if (PreStarAddr==StartAddr) {
    	    RepeatCount++;
    	    if (RepeatCount > MAX_REPEAT_COUNT) {
    	    	printk(KERN_INFO "SD_read Fail:StartAddr=%x,BlkNR=%x\n",rq->sector,rq->current_nr_sectors);
    	    	SD_Reset_REG();
    	        SD_Pio_RW_handle(SD_Request_Fail);/*LTPE Joe it is deadlock now*/
    	    }    
    	}            	
    	
    	TotalLength=length;
    	
    	do {
    	    if (TotalLength > ((CurrentSDCard->csd).BlkSize))
    	        length=(CurrentSDCard->csd).BlkSize;
    	    else
    	        length=	TotalLength;   	    	   	
    	    //SD_Flow_Dbg("SD_read:Addr=%x,Total Length=%x,i=%x,length=%x\n",(StartAddr+i*4),TotalLength,i,length);
    	    CmdRegVal= SD_READ_SINGLE_BLOCK|CPSM_ENABLE|CPSM_WAIT_RSP;/*LTPE Joe the responce of read is R1 type*/    	
    	    DataRegVal=(((CurrentSDCard->csd).BlkSizeFactor)<< 0x4)|MCI_DataIn|MCI_BlkTransfer|MCI_DataEnable;/*LTPE Joe can improvment performance here*/ 
            DataCnt=length / sizeof(U32);    
            
            writel((StartAddr+i*4),MCIARGUMENT);
            writel(0xffff,MCIDATATIMER); 
            writel(length,MCIDATALENGTH);
            writel(DataRegVal,MCIDATACTRL); 
            writel(CmdRegVal,MCICOMMAND); /*LTPE Joe send the command now*/  
            
            TimeOutCount=SD_RW_TIMEOUT;
	    //tsc = ReadTSC(); 
	    local_irq_save(flags);
            while((DataCnt >0) && (TimeOutCount > 0)) {
                RegVal=readl(MCIFIFOCNT);
                if ( DataCnt > RegVal) {
                    while(DataCnt > RegVal) {
                        *((U32*)(rq->buffer)+i)=readl(MCIFIFO);
                        DataCnt--;
                        i++;
                    }
                }
                else {    
                    TimeOutCount--;
                }    
             } 
	    local_irq_restore(flags);
	    //printk("rtime: %u\n",ReadTSC()-tsc); 

     	    //printk(KERN_INFO "TimeOutCount=%x\n",TimeOutCount);
     	    if (TimeOutCount == 0) {
            	//printk(KERN_INFO "Time Out : Data receiving\n");
            	SD_Reset_REG();
            	goto SDRepeatRD;
            }

            TimeOutCount=SD_RW_TIMEOUT;
#ifdef SD_DUMP_IN_DATA
            for (j=0;j<((CurrentSDCard->csd).BlkSize);j++ )
                 printk(KERN_INFO "BLK Data[%d]=%x\n",j,*((U8*)(rq->buffer)+(i*4 - ((CurrentSDCard->csd).BlkSize))+j));
#endif  
            do {/*LTPE Joe to wait data receiving complete*/
                RegVal=readl(MCISTATUS);
                TimeOutCount--;
            }while(((RegVal & MCI_STATUS_DataEnd_Mask) == 0 )&& (TimeOutCount > 0));    	      
            //printk(KERN_INFO "TimeOutCount2=%x\n",TimeOutCount);
            if (TimeOutCount == 0) {
            	printk(KERN_INFO "Time Out : MCISTATUS=%x\n",RegVal);
            	SD_Reset_REG();
            	goto SDRepeatRD;/*LTPE Joe do R/W operation again*/
            }
            
            TotalLength-=length;            
            SD_Reset_REG();
                	      
	    schedule_timeout(1);
        }while(TotalLength != 0);
        
        SD_Pio_RW_handle(SD_Request_OK);  
                       
    }
    else {
    	printk(KERN_INFO "No Card Exist !!\n");
    	return (-1);
   } 	
} 
 
#endif

void SD_Handle_CSD(struct SD_card* pCard) {
    
    (pCard->csd).c_size=(( (pCard->raw_csd[1])& 0x3ff ) << 2) | (( (pCard->raw_csd[2]) >> 30 ) );
    (pCard->csd).c_size_mult= ((pCard->raw_csd[2]) >> 15) & 0x7;
    (pCard->csd).BlkNums= ((pCard->csd).c_size + 1) * (1 << ((pCard->csd).c_size_mult + 2) );
    (pCard->csd).BlkSizeFactor=(((pCard->raw_csd[1])>>16) & 0xf);/*LTPE Joe READ_BL_LEN in csd[83:80] */   
    (pCard->csd).BlkSize=(1<<((pCard->csd).BlkSizeFactor));/*LTPE Joe the value=2^(READ_BL_LEN) */       
    (pCard->csd).DevSize=((pCard->csd).BlkSize)*((pCard->csd).BlkNums);	
    (pCard->csd).EraseBlkSize=((pCard->raw_csd[2]) >> 7) & 0x3f;
    (pCard->csd).DataTransSpeed=(pCard->raw_csd[0])& 0xff;

     DumpCsd(pCard);
}     
  		     	
struct SD_card* SD_card_discovery (void) {    
    struct sd_command* pCmd;
    struct SD_card* pCard;
    U32 i,result;
    
    pCard=kmalloc(sizeof( struct SD_card),GFP_KERNEL);
    
    if (!pCard) {
        printk(KERN_INFO "Can't allocate memory for Card !!\n");
        return (NULL);
    }
    
    MCI_ChgClk(0x1f);/*LTPE Joe the MCICLK must be less than 400KHz in identification mode  */    
    
    pCmd=SD_issue_cmd (SD_GO_IDLE_STATE,0,NULL);
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_GO_IDLE_STATE);
        goto failed;
    } 
    kfree(pCmd);
    
    do {
    	//printk(KERN_INFO "OCR_RESULT=%x\n",result);
        pCmd=SD_issue_cmd (SD_APP_OP_COND,0xff8000,NULL);
        if (pCmd ==NULL) {
            printk(KERN_INFO "Init failed :cmd=%d\n",SD_APP_OP_COND);
            goto failed;
        } 
        
        result= (pCmd->resp[0]) & SD_OCR_BUST_MASK; 
        kfree(pCmd);
    }while( result==0 );
    
    pCmd=SD_issue_cmd (SD_ALL_SEND_CID,0,NULL);
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_ALL_SEND_CID);
        goto failed;
    }    
    for (i=0;i<4;i++)  
        pCard->raw_cid[i]=pCmd->resp[i];         
    kfree(pCmd);
    
    SD_Init_Dbg("Product name=%x\n", pCard->raw_cid[1]);
    
    pCmd=SD_issue_cmd (SD_SEND_RELATIVE_ADDR,0x0,NULL);
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_SEND_RELATIVE_ADDR);
        goto failed;
    }
    
    pCard->rca=(pCmd->resp[0])>>16;    
    kfree(pCmd);
           
    pCmd=SD_issue_cmd (SD_SEND_CSD,(pCard->rca<<16),NULL);
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_SEND_CSD);
        goto failed;
    }    
    for (i=0;i<4;i++) {  
        pCard->raw_csd[i]=pCmd->resp[i];
        SD_Init_Dbg("pCard->raw_csd[%d]=%x\n",i,pCard->raw_csd[i]);
    }        
    kfree(pCmd);
    SD_Handle_CSD(pCard);
#ifdef nothing    
    pCmd=SD_issue_cmd (SD_SET_DSR,0x808,NULL);/*LTPE Joe send CMD4 to adjust the performanec of card*/
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_SELECT_CARD);
        goto failed;
    }   
    kfree(pCmd);
#endif    
    MCI_Clock_Bypass(MCI_Bypass_ON);/*LTPE Joe the MCICLK can be 10 MHZ in transfer mode  */  
    //MCI_ChgClk(0x2); 
    pCmd=SD_issue_cmd (SD_SELECT_CARD,(pCard->rca<<16),NULL);/*LTPE Joe send CMD7 to let card enter transfer state*/
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_SELECT_CARD);
        goto failed;
    }   
    kfree(pCmd);
    
    pCmd=SD_issue_cmd (SD_SEND_STATUS,(pCard->rca<<16),NULL);/*LTPE Joe send CMD7 to let card enter transfer state*/
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_SELECT_CARD);
        goto failed;
    }   
    kfree(pCmd);
           
    pCmd=SD_issue_cmd (SD_SEND_STATUS,(pCard->rca<<16),NULL);/*LTPE Joe send CMD7 to let card enter transfer state*/
    if (pCmd ==NULL) {
        printk(KERN_INFO "Init failed :cmd=%d\n",SD_SELECT_CARD);
        goto failed;
    }   
    kfree(pCmd);
    
    //MCI_ChgBusWidth(MCI_BusWudth_4bits);
    return (pCard);

failed:
    kfree(pCard);
    return (NULL); 
        
}    
  	
void MCI_ChgBusWidth (U8 BusWidth) {
    U32 RetVal=0;
    
    RetVal=SD_RW_reg(MCICLOCK,0,SD_REG_RD);
    if (BusWidth==MCI_BusWudth_4bits)
        RetVal|=MCI_BusWudth_Mask;
    else
        RetVal&=(0xfff-MCI_BusWudth_Mask);
        
    SD_RW_reg(MCICLOCK,RetVal,SD_REG_WR);  
}    

void SD_Set_WideBus(void) {
    struct sd_command* pCmd;
     
    pCmd=SD_issue_cmd (SD_APP_SET_BUS_WIDTH,BUS_WIDTH_4_bits,NULL);/*LTPE Joe send ACMD6 to set bus width*/
    if (pCmd ==NULL) {
        printk(KERN_INFO "Can't change mode to 4 bits mode\n");
    }  

    if (pCmd) { 
        MCI_ChgBusWidth(MCI_BusWudth_4bits);
        printk(KERN_INFO "Changing to 4 bits mode\n");
        kfree(pCmd);
    }
}
    	          
void MCI_ChgClk (U32 clock) {
    U32 RetVal=0;
      
    RetVal=SD_RW_reg(MCICLOCK,0,SD_REG_RD);

    RetVal&=(0xfff-MCI_Clock_Mask);
    RetVal|= clock;    
    
    SD_RW_reg(MCICLOCK,RetVal,SD_REG_WR);  
}

void MCI_EntPwrSav (U8 PwrSavEn) {
    U32 RetVal=0;
    
    RetVal=SD_RW_reg(MCICLOCK,0,SD_REG_RD);
    if (PwrSavEn==MCI_PwrSave_ON)
        RetVal|=MCI_PwrSave_Mask;
    else
        RetVal&=(0xfff-MCI_PwrSave_Mask);
        
    SD_RW_reg(MCICLOCK,RetVal,SD_REG_WR);  
}	

void MCI_Enable (U8 MciEnable) {
    U32 RetVal=0;
    
    RetVal=SD_RW_reg(MCICLOCK,0,SD_REG_RD);
    if (MciEnable==MCI_Enable_ON)
        RetVal|=MCI_Enable_Mask;
    else
        RetVal&=(0xfff-MCI_Enable_Mask);
        
    SD_RW_reg(MCICLOCK,RetVal,SD_REG_WR);  
} 	

void MCI_Clock_Bypass (U8 MciBypass) {
    U32 RetVal=0;
    
    RetVal=SD_RW_reg(MCICLOCK,0,SD_REG_RD);
    if (MciBypass==MCI_Bypass_ON)
        RetVal|=MCI_Bypass_Mask;
    else
        RetVal&=(0xfff-MCI_Bypass_Mask);
        
    SD_RW_reg(MCICLOCK,RetVal,SD_REG_WR);  
}

void MCI_PowerOn (U8 MciPwrOn) {
    U32 RetVal=0;
    
    switch (MciPwrOn) {
    	case MCI_PwrOn:
    	    RetVal=MCI_PwrOn;
    	break;
    	    
    	case MCI_PwrUp:
    	    RetVal=MCI_PwrOn;
    	break;
    	
    	case MCI_PwrOff:
    	    RetVal=MCI_PwrOn;
    	break;
    	
    	default:
    }
    	
        
    SD_RW_reg(MCIPOWER,RetVal,SD_REG_WR);  
}

void MCI_Set_GPIO (void) {
    
    gpio_set_as_function(GPIO_LCD_DB0);/*LTPE Joe SD CMD pin*/
    gpio_set_as_function(GPIO_DAI_WS);/*LTPE Joe SD Data0 pin*/
    gpio_set_as_function(GPIO_DAI_DATA);/*LTPE Joe SD Data1 pin*/
    gpio_set_as_function(GPIO_LCD_RW_WR);/*LTPE Joe SD Data3 pin*/
    gpio_set_as_function(GPIO_LCD_E_RD);/*LTPE Joe SD Data2 pin*/
    gpio_set_as_function(GPIO_DAI_BCK);/*LTPE Jor SD CLK pin*/
   
    gpio_get_config(GPIO_LCD_DB0);
    gpio_get_config(GPIO_LCD_E_RD);
    gpio_get_config(GPIO_DAI_BCK );
}
    
void MCI_Set_SysReg (void) {
 
    //SD_RW_reg(VA_SYSCREG_SSA1_MCI_PL180_PRESERVED,0xf,SD_REG_WR); 
    SD_RW_reg(VA_SYSCREG_MUX_DAI_MCI,1,SD_REG_WR); 
    SD_RW_reg(VA_SYSCREG_MUX_PLCD_MCI,1,SD_REG_WR); 
} 

/****************************************************************************/
/* dma_alloc_coherent / dma_free_coherent ported from 2.5                  */
/****************************************************************************/

static void *dma_alloc_coherent( size_t size,dma_addr_t *dma_handle, int gfp)
{
    void *ret;
    /* ignore region specifiers */
    gfp &= ~(__GFP_DMA | __GFP_HIGHMEM);
    gfp |= GFP_DMA;
    ret = (void *)__get_free_pages(gfp, get_order(size));
    if (ret != NULL)
    {
        memset(ret, 0, size);
        *dma_handle = virt_to_phys(ret);
    }
    return ret;
}    	

void SD_Detsct_Isr(int irq, void *dev_id, struct pt_regs * regs)
{   
    U32 flags,RetVal;
     
    RetVal=er_read_int_status(GPIO_SD_DETECT);
    if ( (RetVal & SD_DETECT_INT_MASK) ==0 )
        return;/*LTPE Joe not my int*/
    
    spin_lock_irqsave(&SD_polling_lock, flags);  
    //printk(KERN_INFO "SD_Detsct_Isr\n");
  
    er_clr_gpio_int(GPIO_SD_DETECT);
    RetVal=SD_insert_status();
    
    if (RetVal==0) {/*LTPE Joe there is card in slot*/
        //er_add_gpio_input_event (IRQ_SD_DETECT, GPIO_SD_DETECT,EVENT_TYPE_RISING_EDGE_TRIGGERED);
        if (CurrentSDCard==NULL) {/*LTPE Joe this card doesn't be init, Init it now !!*/
            CurrentSDCard=SD_card_discovery();  
            if (CurrentSDCard==NULL)
                printk(KERN_INFO " Card Init failed !!\n");
            else
                printk(KERN_INFO " Card Init success !!\n");      
        } 
        else {
           // printk(KERN_INFO "Card have been already initilized.\n");
        }
              
    }
    else {
    	//er_add_gpio_input_event (IRQ_SD_DETECT,GPIO_SD_DETECT, EVENT_TYPE_FALLING_EDGE_TRIGGERED);  
    	if (CurrentSDCard) { /*LTPE Joe card have been removed*/
    	    SD_remove();    
    	}
    	               
    }

    spin_unlock_irqrestore(&SD_polling_lock, flags); 

}

U32 MCI_init( void ) {
    
    struct sdma_config dma_out,dma_in;
    U32 SdmaChOut_Bus_Addr,SdmaChIn_Bus_Addr,result;
    pid_t pid;
    
    printk(KERN_INFO "MCI_init\n");
   

    //MCI_Set_GPIO();
    //MCI_Set_CGU();
    //MCI_Set_SysReg();
    MCI_PowerOn(MCI_PwrUp);
    MCI_ChgBusWidth(MCI_BusWidth_1bit);
    //MCI_EntPwrSav(MCI_PwrSave_ON);
    MCI_Enable(MCI_Enable_ON);
    MCI_ChgClk(0x1f);/*LTPE Joe the MCICLK must be less than 400KHz in identification mode  */  
      
    SD_RW_reg(MCISELECT,0x1,SD_REG_WR); /*LTPE Joe set the address  now*/ 
    
    /*LTPE Joe setup the DMA channel*/
    SdmaChOut=sdma_allocate_channel();
    printk("SDMA Channel OUT allocated = %d\n",SdmaChOut);
    
    if ( SdmaChOut == -1) {
    	printk(KERN_INFO "Can't allocate SDMA OUT Channel !!\n");
    	return (-1);
    }
    
    SdmaChIn=sdma_allocate_channel();
    printk("SDMA Channel IN allocated = %d\n",SdmaChIn);
    
    if ( SdmaChIn == -1) {
    	printk(KERN_INFO "Can't allocate SDMA IN Channel !!\n");
    	return (-1);
    }
    
    SdmaChIn_Buf = dma_alloc_coherent(4096, &(SdmaChIn_Bus_Addr), GFP_KERNEL|GFP_DMA);
    if ( SdmaChIn_Buf == NULL) {
    	printk(KERN_INFO "Can't allocate IN DMA buffer !!\n");
    	return (-1);
    }
    
    SdmaChOut_Buf = dma_alloc_coherent(4096, &(SdmaChOut_Bus_Addr), GFP_KERNEL|GFP_DMA);
    if ( SdmaChOut_Buf == NULL) {
    	printk(KERN_INFO "Can't allocate OUT DMA buffer !!\n");
    	return (-1);
    }
    
    dma_out.src_addr = SdmaChOut_Bus_Addr;/* LTPE Joe memory buffer address*/ 
    dma_out.dst_addr = (MCI_REGS_BASE+0x80);/*LTPE Joe the physical address of MCI FIFO register*/ 
    dma_out.len      = 0;
    dma_out.size     = TRANFER_SIZE_32;/*LTPE Joe improvement performance here*/
    dma_out.src_name = DEVICE_MEMORY;
    dma_out.dst_name = DEVICE_MCI_BREQ;/*LTPE Joe improvement performance here*/
    dma_out.endian   = ENDIAN_NORMAL;
    dma_out.circ_buf = NO_CIRC_BUF;
    sdma_setup_channel(SdmaChOut,&dma_out,SD_dma_isr);
    
    dma_in.src_addr = (MCI_REGS_BASE+0x80);/*LTPE Joe the physical address of MCI FIFO register*/ ;
    dma_in.dst_addr = SdmaChIn_Bus_Addr; 
    dma_in.len      = 0;
    dma_in.size     = TRANFER_SIZE_32;/*LTPE Joe improvement performance here*/
    dma_in.src_name = DEVICE_MCI_BREQ;
    dma_in.dst_name = DEVICE_MEMORY;
    dma_in.endian   = ENDIAN_NORMAL;
    dma_in.circ_buf = NO_CIRC_BUF;
    sdma_setup_channel(SdmaChIn,&dma_in,SD_dma_isr);
#ifndef SD_NO_SDMA_IRQ    
    sdma_config_irq(SdmaChIn,IRQ_FINISHED );
    sdma_config_irq(SdmaChOut,IRQ_FINISHED );
#endif
    
    CurrentSDCard=SD_card_discovery();

    //if (CurrentSDCard==NULL) {
    //   printk(KERN_INFO "No Card Exist !!\n");
    //   er_add_gpio_input_event (IRQ_SD_DETECT, GPIO_SD_DETECT, EVENT_TYPE_FALLING_EDGE_TRIGGERED);
    //}
    //else {
    // 	er_add_gpio_input_event (IRQ_SD_DETECT, GPIO_SD_DETECT, EVENT_TYPE_RISING_EDGE_TRIGGERED);
    //}	    
    
    //result=request_irq(IRQ_SD_DETECT,SD_Detsct_Isr,SA_SHIRQ,"SD_Detect",0x77);
    //if (result) {  	
    //    printk(KERN_INFO "Can't install the ISR for SD card detection:Result=%x\n",result);
    //}
    
    SD_Set_WideBus();    
    //DumpMciReg();/*LTPE Joe for test*/
    //SD_setting_timer();/*LTPE Joe init polling timer*/    
    pid = kernel_thread(poll_status, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGHAND);
    if( pid >= 0) {
    	printk(KERN_CRIT "SD status check thread %d started..\n", pid);
	poll_thread = pid;
    }

    //DumpSdmaReg();
    DumpSysReg();
    DumpCguReg();
    return 0;
}	                                                          

