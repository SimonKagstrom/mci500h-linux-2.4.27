/*
   w6_uart1.c - tino uart1 driver

   Copyright (C) 2005-2006 PDCC, Philips CE.
   Revision History :
   ----------------
   Version               DATE                           NAME     NOTES
   Initial version        2006-02-23 10:05AM       wxl         new ceated
                           2006-06-09 10:48AM       wxl

   This source code is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <asm/mman.h>
#include <linux/fs.h>
#include <linux/errno.h>    /* for -EBUSY */
#include <linux/ioport.h>   /* for verify_area */
#include <linux/init.h>     /* for module_init */
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/wrapper.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <asm/arch/hardware.h>
#include <asm/arch/gpio.h>
#include <asm/mach/irq.h>

//definitions for fields of uart1 registers
typedef union
{
     struct
         {
           unsigned int rxchar:8;
           unsigned int reserved:24;
         }__attribute__ ((packed)) RegField;
    unsigned int regval_32bit;
}Uart1RBRUnion32bitReg;

typedef union
{
     struct
         {
           unsigned int txchar:8;
           unsigned int reserved:24;
         }__attribute__ ((packed)) RegField;
    unsigned int regval_32bit;
}Uart1THRUnion32bitReg;

typedef union
{
     struct
         {
            unsigned int DR:1;
            unsigned int OE:1;
            unsigned int PE:1;
            unsigned int FE:1;
            unsigned int BI:1;
            unsigned int THRE:1;
            unsigned int TEMT:1;
            unsigned int RxErr:1;

            unsigned int reserved:24;
         }__attribute__ ((packed)) RegField;
    unsigned int regval_32bit;
}Uart1LSRUnion32bitReg;

typedef union
{
     struct
         {
            unsigned int WdLenSel:2;
            unsigned int StopBitNum:1;
            unsigned int ParEn:1;
            unsigned int ParEven:1;
            unsigned int ParStick:1;
            unsigned int BrkCtrl:1;
            unsigned int DLAB:1;

            unsigned int reserved:24;
         }__attribute__ ((packed)) RegField;
    unsigned int regval_32bit;
}Uart1LCRUnion32bitReg;

typedef union
{
     struct
         {
           unsigned int RDAIntEn:1;
           unsigned int THREIntEn:1;
           unsigned int RLSIntEn:1;
           unsigned int MSIntEn:1;
           unsigned int reserved1:3;
           unsigned int CTSIntEn:1;
           unsigned int ABEOIntEn:1;
           unsigned int ABTOIntEn:1;

           unsigned int reserved:22;
         }__attribute__ ((packed)) RegField;
    unsigned int regval_32bit;
}Uart1IERUnion32bitReg;

/* The major device number */
#define MAJOR_NUM 150
#define DEVICE_NAME        "uart1dev"

#define  UART1_TX_BUF_LEN   256
#define  UART1_RX_BUF_LEN   256

#define  IOCTRL_SET_PID_SIGNAL            0x100
#define  IOCTRL_GET_RXBUF_RECEIV_LEN   0x101
#define  IOCTRL_GET_TXBUF_FREE_LEN      0x102
#define  IOCTRL_CLEAR_RXBUF                 0x103
#define  IOCTRL_ENABLE_RECEIVE             0x104
#define  IOCTRL_DISABLE_RECEIVE            0x105
#define  IOCTRL_SET_SPEED_19200            0x106
#define  IOCTRL_SET_SPEED_7000             0x107
#define  IOCTRL_SET_RX_SWITCHPIN_LOW      0x108
#define  IOCTRL_SET_RX_SWITCHPIN_HIGH     0x109
#define  IOCTRL_SET_TX_FREQ                     0x10A

#define  UART1_IRQ                      14
#define  UART1_SPEED_19200                  0
#define  UART1_DIV_FRAC_19200             17   // this value was copy from Andre's code
#define  UART1_PRESCALE_VALUE_19200    ((10 << 4) | (13 << 0))  // this value was copy from Andre's code

#define  UART1_SPEED_7000                  1
#define  UART1_DIV_FRAC_7000             42   // this value was got by debugging and measuring
#define  UART1_PRESCALE_VALUE_7000    ((9 << 4) | ( 0xe << 0))  // this value was got by debugging and measuring

#define  CLK1_FREQ_455K                     0
#define  M_455K                                 208
#define  N_455K                                 8

#define  CLK1_FREQ_56K                       1
#define  M_56K                                   214
#define  N_56K                                   1
#define UART1_VIRT_BASE         IO_ADDRESS_UART1_BASE   //the virtual address mapped to uart1
                                                                //physical address,where uart1_virt_base is a global
                                                                //void pointer defined in uart1_drv.c
#define UART1_RBR                   (*((int *)(UART1_VIRT_BASE+0x00)))
#define UART1_THR                   (*((int *)(UART1_VIRT_BASE+0x00)))
#define UART1_DLL                    (*((int *)(UART1_VIRT_BASE+0x00)))
#define UART1_DLM                   (*((int *)(UART1_VIRT_BASE+0x04)))
#define UART1_IER                    (*((int *)(UART1_VIRT_BASE+0x04)))
#define UART1_LCR                    (*((int *)(UART1_VIRT_BASE+0x0C)))
#define UART1_LSR                    (*((int *)(UART1_VIRT_BASE+0x14)))
#define UART1_FDR                    (*((int *)(UART1_VIRT_BASE+0x28)))

#define CGUSWITCHBOX_VIRT_BASE    cguswitchbox_virt_base
#define CGUSWITCHBOX_REGION_LEN  0xC00
//for clk1,transmit clk
#define SCR_CLK1_BASE                  (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x048)))
#define FS1_CLK1_BASE                  (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x094)))
#define FS2_CLK1_BASE                  (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x0E0)))
#define SSR_CLK1_BASE                  (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x012C)))
#define PCR_CLK1                          (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x2DC)))
#define PSR_CLK1                          (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x48C)))
#define ESR_CLK1                          (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x62C)))
#define FDC_CLK1                          (*((unsigned int *)(CGUSWITCHBOX_VIRT_BASE+0x6AC)))

/*global varibles declarations*/
struct {int pid;int signal;} uart1signal; //for callback function implementation
int uart1_signal = 0;
struct task_struct *uart1_task = 0;
void   *cguswitchbox_virt_base=NULL;//the virtual address mapped to the physical address of CGU switchbox registers region
//Uart1 transmit buf and r/w pointer
unsigned char uart1_Tx_buf[UART1_TX_BUF_LEN];
int   uart1_Txbuf_write_pt=0;
int   uart1_Txbuf_read_pt=0;
//Uart1 receive buf and r/w pointer
unsigned char uart1_Rx_buf[UART1_RX_BUF_LEN];
int   uart1_Rxbuf_write_pt=0;
int   uart1_Rxbuf_read_pt=0;
//enable or disable uart1 receive flag
char uart1_receive_flag=1; // 1 means enable receive,0 means not receive
char uart1_send_flag=0;    //1 means a sending in progress,0 menas not
static char uart1_init_state=0;

/* function declarations*/
static void Uart1_Int_handler(int irq, void *dev_id, struct pt_regs *regs);
static int uart1dev_ioctl(struct inode *,struct file *,unsigned int ,unsigned long );
static ssize_t uart1dev_read(struct file *file, char *buffer,size_t length,loff_t *offset);
static ssize_t uart1dev_write(struct file *file, const char *buffer, size_t length, loff_t *offset);
static int __init uart1_module_init( void );
static void __exit uart1_module_cleanup(void);

struct file_operations uart1_Fops = {
    .read       = uart1dev_read,
    .write      = uart1dev_write,
    .ioctl      = uart1dev_ioctl
    };

void SendSinalToTask(void)
{
        if (uart1_signal)
             {
                  siginfo_t si;
                  si.si_signo = uart1_signal;
                  si.si_errno = 0;
                  si.si_code = SI_KERNEL;
                  send_sig_info(si.si_signo, &si, uart1_task);
             }
}

void SetUart1BaudRate(unsigned int divfractor,unsigned int prescale)
{
   Uart1LCRUnion32bitReg  tUart1LCRValue;
   unsigned int dll_val,dlm_val;
   unsigned long flags;

   local_irq_save (flags);
   dll_val=(divfractor & 0xff);
   dlm_val=((divfractor & 0xff00) >> 8);
   /* set DLAB */
   tUart1LCRValue.regval_32bit=UART1_LCR;
   tUart1LCRValue.RegField.DLAB=1;
   UART1_LCR=tUart1LCRValue.regval_32bit;
   //check DLAB value
   tUart1LCRValue.regval_32bit=UART1_LCR;
   if(tUart1LCRValue.RegField.DLAB != 1)
     {
         printk("\n DLAB value set to 1 failed"); //give some tips
     }

   /* Set baud rate */
   UART1_DLL= dll_val;
   UART1_DLM=dlm_val;

   /* clear DLAB */
   tUart1LCRValue.regval_32bit=UART1_LCR;
   tUart1LCRValue.RegField.DLAB=0;
   UART1_LCR=tUart1LCRValue.regval_32bit;
   //check DLAB value
   tUart1LCRValue.regval_32bit=UART1_LCR;
   if(tUart1LCRValue.RegField.DLAB != 0)
     {
         printk("\n DLAB value set to 0 failed"); //give some tips
     }

   UART1_FDR=prescale;//pre-scale value
   local_irq_restore (flags);
}

void SetUart1Speed(unsigned char speed)
{
   if(UART1_SPEED_19200==speed)
      {
        SetUart1BaudRate(UART1_DIV_FRAC_19200,UART1_PRESCALE_VALUE_19200);
        //printk("\n set UART1_SPEED_19200");
      }
   else if(UART1_SPEED_7000==speed)
     {
        SetUart1BaudRate(UART1_DIV_FRAC_7000,UART1_PRESCALE_VALUE_7000);
        //printk("\n set UART1_SPEED_7000");
     }
   else
    {
         printk("\n speed not supported yet.");
    }
}

void EnableUart1THREInt(void)
{
   Uart1IERUnion32bitReg  tUart1IERValue;

   uart1_send_flag=1;
   tUart1IERValue.regval_32bit = UART1_IER;
   tUart1IERValue.RegField.THREIntEn=1;
   UART1_IER = tUart1IERValue.regval_32bit;
}

void DisableUart1THREInt(void)
{
   Uart1IERUnion32bitReg  tUart1IERValue;

   tUart1IERValue.regval_32bit = UART1_IER;
   tUart1IERValue.RegField.THREIntEn=0;
   UART1_IER = tUart1IERValue.regval_32bit;
}

int InstallUART1IrqHandler(void)
{
    int retVal;

    retVal=request_irq(UART1_IRQ, Uart1_Int_handler,0, "uart1", NULL);
    if (retVal)
      {
         printk("\n FATAL error ,can't install irq handler for PNX0106 UART1,retVal=%d ",retVal);
         return retVal;
      }
    printk("\n UART1 int handler has been installed.  ");
    return 0;
}

void ConfigUart1ChannelPara(unsigned char speed)
{
     Uart1LCRUnion32bitReg  tUart1LCRValue;
     Uart1IERUnion32bitReg  tUart1IERValue;
     unsigned int temp;

     //set baud rate
     SetUart1Speed(speed);
     //set line control
     tUart1LCRValue.regval_32bit=0;
     tUart1LCRValue.RegField.WdLenSel=3;
     tUart1LCRValue.RegField.StopBitNum=0;
     tUart1LCRValue.RegField.ParEn=0;
     tUart1LCRValue.RegField.ParEven=0;
     tUart1LCRValue.RegField.ParStick=0;
     tUart1LCRValue.RegField.BrkCtrl=0;
     tUart1LCRValue.RegField.DLAB=0;
     UART1_LCR=tUart1LCRValue.regval_32bit;

     UART1_IER=0;
     temp=UART1_RBR;//read out current pending rx data if any
     InstallUART1IrqHandler();//install uart1 int irq handler
     //enable rx data and tx holding register empty interrupt
     tUart1IERValue.regval_32bit=0;
     tUart1IERValue.RegField.RDAIntEn=1;
     tUart1IERValue.RegField.THREIntEn=1;
     UART1_IER=tUart1IERValue.regval_32bit;
     //because the triggle data will be looped back,and we do not want to receive it,
     //this flag will automatically enabled after tx data finished
     uart1_receive_flag=0;
     UART1_THR=0;//send a byte to trigger THE interupt
}

int InitUart1Hardware(void)
{
     gpio_set_as_output (GPIO_IEEE1394_D7,0);//set G17 to low
     ConfigUart1ChannelPara(UART1_SPEED_19200);
     return 0;
}

/************* uart1 Rx buf read/write functions **************************/
int WriteUart1RxDataByte(unsigned char byData) //used by data received interrupt route
{
     int temp_pt;

     //test if the Rx buf is full
     temp_pt=uart1_Rxbuf_write_pt+1;
     if(UART1_RX_BUF_LEN==temp_pt)
        {
        temp_pt=0;
        }

     if(temp_pt==uart1_Rxbuf_read_pt)
       {
        //printk("\n warning!,uart1 Rx buf full !");
        return 0; //0 byte saved
       }

     uart1_Rx_buf[uart1_Rxbuf_write_pt]=byData;//save the received data
     uart1_Rxbuf_write_pt=temp_pt;//move the write pointer to the next write position
     return 1; //1 byte saved
}

//note pbuf is a buffer from user space
int ReadUart1RxDataBuf(unsigned char *pbuf,int len) //used by file read function
{
     int count;
     int readlen;

     readlen=0;
     for(count=0;count<len;count++)
        {
            //test if the Rx buf is empty
            if(uart1_Rxbuf_write_pt==uart1_Rxbuf_read_pt)
              {
              //printk("\n warning!,uart1 Rx buf empty !");
              break;
              }

           //read the data to pbuf
           copy_to_user((char *)(pbuf+count),(char *)(&uart1_Rx_buf[uart1_Rxbuf_read_pt]),1);
           readlen++;

           //move read pointer to next read position
           uart1_Rxbuf_read_pt++;
           if(UART1_RX_BUF_LEN==uart1_Rxbuf_read_pt)
             {
            uart1_Rxbuf_read_pt=0;
             }

        } // end of for(count=0;count<len;count++)

     return readlen;
}

unsigned int GetUart1RxBufReceivedLen(void)
{
   unsigned int length;

   if(uart1_Rxbuf_write_pt >=uart1_Rxbuf_read_pt )
     {
         length=uart1_Rxbuf_write_pt - uart1_Rxbuf_read_pt;
     }
    else
     {
         length=UART1_RX_BUF_LEN - uart1_Rxbuf_read_pt + uart1_Rxbuf_write_pt;
     }

   return length;
}

void ClearUart1RxBuf(void)
{
   uart1_Rxbuf_write_pt=0;
   uart1_Rxbuf_read_pt=0;
}

/************* uart1 Tx buf read/write functions **************************/
int WriteUart1TxDataBuf(unsigned char *pbuf,int len) //used by file write function
{
     int temp_pt;
     int count;
     int writelen;

     writelen=0;
     for(count=0;count<len;count++)
       {
           //test if the Tx buf is full
           temp_pt=uart1_Txbuf_write_pt+1;
           if(UART1_TX_BUF_LEN==temp_pt)
              {
             temp_pt=0;
              }

           if(temp_pt==uart1_Txbuf_read_pt)
              {
             printk("\n warning!,uart1 Tx buf full !");
             break;
              }

           //save the transmit data
           copy_from_user((char *)&uart1_Tx_buf[uart1_Txbuf_write_pt],(char *)(pbuf+count),1);
           writelen++;

           uart1_Txbuf_write_pt=temp_pt;//move the write pointer to the next write position
     }

     return writelen;
}

int ReadUart1TxDataByte(unsigned char *pbyData) //used by transmit holding register empty interrupt route
{
     //test if the Tx buf is empty
     if(uart1_Txbuf_write_pt==uart1_Txbuf_read_pt)
        {
             //printk("\n warning!,uart1 Tx buf empty !");
         return 0;//no data to send
        }

     //read the data to pbyData
     *(unsigned char *)pbyData = uart1_Tx_buf[uart1_Txbuf_read_pt];

     //move read pointer to next read position
     uart1_Txbuf_read_pt++;
     if(UART1_TX_BUF_LEN==uart1_Txbuf_read_pt)
         {
             uart1_Txbuf_read_pt=0;
         }

     return 1;//one data byte fetched
}

int GetUart1TxBufFreeLen(void)
{
   unsigned int length;

   if(uart1_Txbuf_write_pt>=uart1_Txbuf_read_pt)
   {
       length=UART1_TX_BUF_LEN -uart1_Txbuf_write_pt + uart1_Txbuf_read_pt -1;
   }
   else
   {
       length= uart1_Txbuf_read_pt - uart1_Txbuf_write_pt -1;
   }

   return length;
}

unsigned int gcd (unsigned int u, unsigned int v)
{
    unsigned int k = 0;

    if (u == 0)
        return v;
    if (v == 0)
        return u;

    while (((u | v) & 0x01) == 0) {
        u >>= 1;
        v >>= 1;
        k++;
    }

    do {
             if ((u & 0x01) == 0) u = u >> 1;
        else if ((v & 0x01) == 0) v = v >> 1;
        else if (u >= v)          u = (u - v) >> 1;
        else                      v = (v - u) >> 1;
    }
    while (u > 0);

    return (v << k);
}

static void cgu_fracdiv_set_ratio (unsigned int in, unsigned int out)
{
    int madd, msub;
    unsigned int temp, config;
    unsigned int fdwidth, fdwidth_mask;

    temp = gcd (in, out);                                   /* reduce... */
    in /= temp;
    out /= temp;

    madd = (in - out);
    msub = out;                                             /* defer making msub negative until after expansion */

    fdwidth = 8;    /* every fracdiv apart from one has 8bit config settings... */
    fdwidth_mask = (1 << fdwidth) - 1;

    while (((madd | msub) & ~(fdwidth_mask >> 1)) == 0) {   /* expand to fit available width */
        madd <<= 1;                                         /* not strictly necessary but apparently good for power consumption... */
        msub <<= 1;
    }

    config = (1<<0) | ((((0 - msub) & fdwidth_mask) << (3 + fdwidth)) | ((madd & fdwidth_mask) << 3));

    if (out <= (in / 2))                                    /* Stretch does a good job of evening out the duty cycle... */
        config |= (1<<2);                              /* but it can only be used if input:output ratio is >= 2:1 !!! */

    FDC_CLK1 = (1<<1);
    FDC_CLK1 = config;                           /* enable fracdiv with new settings */
}

void SetFreqForCLK1(unsigned int m,unsigned int n)
{
    unsigned int temp;

    //disable clk1
    PCR_CLK1=0x6;
    //select reference
    temp =SCR_CLK1_BASE;
    temp &= ~(1<<2);
    FS1_CLK1_BASE=1;
    FS2_CLK1_BASE=1;
    SCR_CLK1_BASE=((temp ^ 0x03) & ~(1<<3));
    mdelay(10);
    //set frac div
    cgu_fracdiv_set_ratio(m,n);
    //esr setting
    ESR_CLK1=(0 << 1) | (1<<0);
    //enable clk
    PCR_CLK1=0x7;
}

static void Uart1_Int_handler(int irq, void *dev_id, struct pt_regs *regs)
{
       Uart1RBRUnion32bitReg    tUart1RBRValue;
       Uart1THRUnion32bitReg    tUart1THRValue;
       Uart1LSRUnion32bitReg    tUart1LSRValue;
       unsigned char byTemp;
       int ret;

       tUart1LSRValue.regval_32bit=UART1_LSR;
       if(tUart1LSRValue.RegField.DR)//check whether data has been received
         {
             tUart1RBRValue.regval_32bit=UART1_RBR;
             byTemp=tUart1RBRValue.RegField.rxchar;
             if(uart1_receive_flag)
               {
                   ret=WriteUart1RxDataByte(byTemp);//write the received data to uart1 Rx buffer
               }
             //printk("\n data Rx =0x%02x",byTemp);
         }

       if( tUart1LSRValue.RegField.THRE)//check whether data can be sent now
         {
             ret=ReadUart1TxDataByte(&byTemp); //fetch data from Tx buf
             if(ret)
               {
                  uart1_receive_flag=0; //if has data to send,will not receive any data
                  tUart1THRValue.RegField.reserved=0;
                  tUart1THRValue.RegField.txchar=byTemp;
                  UART1_THR=tUart1THRValue.regval_32bit;//send the data
                  //printk("\n data Tx =0x%02x",byTemp);
               }
              else //notify finish sending
               {
                    //printk("\n enable receive for no data to tx");
                    uart1_receive_flag=1; //if has no data to send,then data can be received
                    DisableUart1THREInt();
                    if(uart1_send_flag)
                      {
                        uart1_send_flag=0;
                        //printk("\n notify finish sending.");
                        SendSinalToTask();
                      }
               }
         }
}

static ssize_t uart1dev_read(
    struct file *file,
    char *buffer,       /* The buffer to fill with the data */
    size_t length,      /* The length of the buffer */
    loff_t *offset)     /* offset to the file */
{
    int readlen;

    readlen=ReadUart1RxDataBuf(buffer,length);
    return readlen;
}

static ssize_t uart1dev_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
   int writelen;

   writelen=WriteUart1TxDataBuf((unsigned char *)buffer,length);
   EnableUart1THREInt();
   return writelen;
};

static int uart1dev_ioctl(
    struct inode *inode,
    struct file *file,
    unsigned int ioctl_num,/* The number of the ioctl */
    unsigned long ioctl_param) /* The parameter to it */
{
     int parabuf[10];
     unsigned int m,n;

     switch (ioctl_num)
       {
         case IOCTRL_SET_PID_SIGNAL :
               printk("\nuart1dev_ioctl set signal and pid\n");
               copy_from_user(&uart1signal, (void*)ioctl_param, sizeof(uart1signal));
               printk("\n uart1_signal=%d",uart1signal.signal);
               printk("\n lcd_task_pid=%d",uart1signal.pid);

               uart1_signal = uart1signal.signal;
               if (uart1_signal)
                  uart1_task = find_task_by_pid(uart1signal.pid);
             break;
         case IOCTRL_GET_RXBUF_RECEIV_LEN:
               parabuf[0]=GetUart1RxBufReceivedLen();
               copy_to_user((char *)ioctl_param,(char *)parabuf,4);
               break;
         case IOCTRL_GET_TXBUF_FREE_LEN:
               parabuf[0]=GetUart1TxBufFreeLen();
               copy_to_user((char *)ioctl_param,(char *)parabuf,4);
               break;
         case IOCTRL_CLEAR_RXBUF:
               ClearUart1RxBuf();
               break;
         case IOCTRL_ENABLE_RECEIVE:
               uart1_receive_flag=1;
               break;
         case IOCTRL_DISABLE_RECEIVE:
               uart1_receive_flag=0;
               break;
         case IOCTRL_SET_SPEED_19200:
               SetUart1Speed(UART1_SPEED_19200);
               break;
         case IOCTRL_SET_SPEED_7000:
               SetUart1Speed(UART1_SPEED_7000);
               break;
         case IOCTRL_SET_RX_SWITCHPIN_LOW:
               gpio_set_as_output (GPIO_UART0_NRTS, 0);
               break;
         case IOCTRL_SET_RX_SWITCHPIN_HIGH:
               gpio_set_as_output (GPIO_UART0_NRTS, 1);
               break;
         case IOCTRL_SET_TX_FREQ:
               copy_from_user((char* )parabuf, (void*)ioctl_param,2*4);
               m=parabuf[0];
               n=parabuf[1];
               SetFreqForCLK1(m,n);
               break;
         default:
             break;
         }
   return 0;
};

/* Initialize the module - Register the character device */
static int __init uart1_module_init( void )
{
    int ret_val;

    if( uart1_init_state == 0 )
    {
        uart1_init_state = 2;
        printk("\n begin init uart1 ...");
        if ( !request_mem_region( CGU_REGS_BASE, CGUSWITCHBOX_REGION_LEN, "PNX0106 CGU control unit registers") )
        {
            printk("\nFATAL EERROR !  failed to request the memory region for CGU control unit registers ! \n" );
            return 1;
        }
        cguswitchbox_virt_base = (void *)ioremap_nocache( CGU_REGS_BASE,CGUSWITCHBOX_REGION_LEN );
        printk("\n Request memory region success.");

        ret_val=InitUart1Hardware();
        if(ret_val)
        {
            printk("\nFATAL EERROR ! failed init hardware for uart1 \n" );
            return 2;
        }
        printk("\n Init Uart1 hardware done.");

        /* Register the character device (atleast try) */
        ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &uart1_Fops);
        if (ret_val < 0)
        {
            printk ("\n Sorry, registering the character device %s failed with %d\n",DEVICE_NAME,ret_val);
            return ret_val;
        }
        printk ("\n %s Registeration is a success, The major device number is %d.\n",DEVICE_NAME,MAJOR_NUM);
        printk ("\n If you want to talk to the device driver,a uart1 device file is needed.");
        printk ("\n  if /dev/uart1dev file not exist,u should create it by the following command");
        printk ("\n  mknod /dev/%s c %d 0\n", DEVICE_NAME,MAJOR_NUM);
        printk("\ndone.\n");

        /* version tips */
        printk("\n ****************************************************");
        printk("\n      current uart1Rcfast version: V1.4 (2007-04-26) ");
        printk("\n ****************************************************\n");
        uart1_init_state = 1;
    }
    return 0;
}

static void __exit uart1_module_cleanup(void)
{
    int ret_val;

    printk("\n Remove uart1 module...");
    uart1_init_state = 0;
    ret_val = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);    /* Unregister the device */
    if (ret_val < 0)
      printk("Error in uart1_module_cleanup: %d\n", ret_val);

   printk("\n free uart1 irq...");
   free_irq(UART1_IRQ, NULL);
   iounmap(cguswitchbox_virt_base);//ummap the CGU switchbox registers region mapping
   release_mem_region(CGU_REGS_BASE, CGUSWITCHBOX_REGION_LEN);//release the CGU switchbox registers region
   printk("\n done.\n");
   return;
}

module_init(uart1_module_init);
module_exit(uart1_module_cleanup);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wxl");
MODULE_DESCRIPTION("uart1 for rcfast char device driver in PNX0106");

