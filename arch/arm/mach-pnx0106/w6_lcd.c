/*
   w6_lcd.c - LCD device driver implementation code,for Philips melody
   Copyright (C) 2005-2006 PDCC, Philips CE.
   Revision History :
   ----------------
   Version               DATE                           NAME     NOTES
   2006-04-27 11:21AM       wxl          integrate all lcd drv in one
   2006-05-16 4:32PM         wxl          both lcd if and gpio are used in
                                                  WACS7K lcd controller,lcd if control
                                                  clk,data pin,gpio control cs,cd pin
  2006-06-17 6:10PM          wxl          remove lcd buf finish sending sigal notify
                                                   (because not used )
  2006-06-21 9:39AM          wxl         additional treat for lcd in pptr of wacs7k
                                                   (different in contrast and start com)
  2006-06-28 5:10PM           wxl         display a pic after init
  2006-07-10 3:08PM           wxl         resend lcd cfg data and hw reset lcd
                                                  periodically
  2006-07-12 2:34PM           wxl        ref to Andre McCurdy'code,perform a dummy write operation
                                                 while resetting lcd controller and replace using gpio to control
                                                 c/d pin with only polling lcd fifo empty status while transfer
                                                 type changed.
  2006-07-13 2:17PM           wxl        use sdma to transfer lcd buf data to lcd controller
  2006-08-16 10:03AM         wxl        modify default contrast accord to product sample lcd in wac7k
  2007-01-09 10:56AM         wxl        debugging color lcd driver in wac7k5
  2007-05-18 2:00PM	        roy.xu      debugging color lcd (HX8346A)driver in wac7k5
  2007-07-18 9:00AM          keith.l     switch to 32bit SMDA mode for data transmission (7k5 only).
   2007-07-26 9:30AM		roy.xu 		remove the flash the lcd sdram every one seconds
  2007-10-08 4:35PM		roy.xu     remove the hx8346a display on on init and ioctrl,hx8346a display off on ioctrl
  2007-11-14 11:00AM		roy.xu     remove the hx8346a LCD driver init functon, inorder to resolve LCD flash,because in bootload
  								LCD have been initiated; fix the power on register by function set_lcd_display_setting
  2007-12-19 12:40AM         Keith.L    HX8347 support.
  2008-01-04 10:47AM         Keith.L    hide the red philips logo during startup (by disabling and then reenabling backlight)
  2008-02-13 10:00AM         Keith.L    New Driver Interface for quering LCD controller type (8346/8347)
  2008-02-14 10:00AM         Keith.L    8346/46 LCD controller detection fixed. (8346/8347)
  2008-03-05 02:00PM         Keith.L    8346/46 LCD controller detection algorithm changed
  								
   This source code is free software ; you can redistribute it and/or modify it under the terms of the
    GNU General Public License version 2 as published by the Free Software Foundation.
 */

#define LCD_DRV_VER_INFO "v2.91, HX8346/8347 Mixed Mode Driver, 2008-03-05 02:00PM"

#ifdef LCDDRV_ALL_IN_ONE_LOCAL_DEBUG
  #include "w6_lcd.h"
#else
   #include <asm/arch/w6_lcd.h>
#endif

#define FIFO_CONTENTS_INST          (0)
#define FIFO_CONTENTS_DATA          (1)
#define FIFO_CONTENTS_UNKNOWN    (2)

//paras of lcd drv
static unsigned int   previous_fifo_contents=FIFO_CONTENTS_UNKNOWN;
static unsigned int   lcd_hw_just_init_flag=0;//used to enable display after hw init and a valid frame was transferred
static unsigned int   pptr_board_flag=0; //to compatible with the lcd in wacs7k in ptr board,1 means pptr board
static unsigned char curr_lcd_type =0xFF;
static unsigned char curr_tft_type=0xFF; //0x46=HX8346, 0x47=HX8347 --keith
static unsigned int   LCD_WIDTH;     //in pixels ,width of lcd pannel
static unsigned int   LCD_HEIGHT;       //in pixels
static unsigned int   LCD_BPP;        //bit per pixels
static unsigned int   LCD_PAGESIZE;
static unsigned int   LCD_PAGES;
static unsigned int   LCD_RAMBUF_LEN;
static unsigned char MAX_LCD_CONTRAST; //max LCD contrst
static unsigned char MIN_LCD_CONTRAST; //min LCD contrst
static unsigned char curr_lcd_contrast; //this value should be in range of MIN_LCD_CONTRAST to MAX_LCD_CONTRAST
static unsigned char lcd_disp_invert_flg;  //lcd display inverse flag
//lcd buf related var
static DECLARE_WAIT_QUEUE_HEAD(lcd_sdma_tx_wq);
static unsigned int        lcdbuf_sdma_int_count=0;
static int                  sdma_channel_for_lcdbuf=-1;  //channel number for lcd buf transfer
static TSdmaChanelSetupList   lcd_sdma_table;
static volatile unsigned char    *pLcdSdmaTxBufWas7k=NULL;//for was7k sdma transfer
static struct semaphore AccessLcdController_sem; // semaphore for access lcd controller
static struct semaphore LcdRamBufTrans_sem; // semaphore for Lcd ram buf to transfer
static unsigned char      *pLcdVideoBuf=NULL;//this buf contain the off-screen data
static unsigned int        lcd_buf_send_finish_flag=1;
static kthread_t          tLCDModuleThread[NTHREADS]; //lcd module thread record
//lcd configuration data resend related var
struct timer_list          timer_resend_lcd_cfg; //resend lcd configuration data timer
static unsigned char      need_resend_lcd_cfg=0;
static unsigned int        resend_lcd_cfg_count=0;

/* function declarations*/
static int lcddev_ioctl(struct inode *,struct file *,unsigned int ,unsigned long );
static ssize_t lcddev_read(struct file *file, char *buffer,size_t length,loff_t *offset);
static ssize_t lcddev_write(struct file *file, const char *buffer, size_t length, loff_t *offset);
static int lcddev_mmap(struct file * file, struct vm_area_struct * vma);
static void stop_kthread(kthread_t *kthread);
static void start_kthread(void (*func)(kthread_t *), kthread_t *kthread);
static void Fill_LCDRamBufArray_With_Byte(unsigned char byData);
static int  AllocateMemForLcdModule(void);
static void LCDBufTransThread(kthread_t *kthread);
static void write_batch_data_to_lcd_ctrl(const TBatchDataToLcdCtrl atLcdCtrlInitData[]);
static int  HardwareInitForLCD(void);
static void InitLCDInterfaceOfMelody(void);
static void InitLCDInterfaceOfMelody_Parallel(void);
static void InitLCDController(void);
static unsigned char  get_curr_lcd_type(void);
static void  init_lcddrv_para(void);
static void  Additional_Treat_For_LcdCtrl(void);
static void  show_lcd_type_ver_info(void);
static inline void Com_Out(unsigned char instByte);
static inline void Data_Out(unsigned char dataByte);

#if 0
static unsigned char read_hx8312a_reg(unsigned char addr);
static void write_hx8312a_reg(unsigned char addr, unsigned char cmd);
#endif

static unsigned char read_hx8346a_reg(unsigned char addr);
static void write_hx8346a_reg(unsigned char addr, unsigned char cmd);

static void  call_lcd_buf_transfer(void);
static void  display_startup_logo(void);
static void  set_lcd_display_on(void);
static void  set_lcd_display_off(void);
static void  lock_lcdctrl_access(void);
static void  release_lcdctrl_access(void);
static void init_lcd_cfg_resend_timer(void);
static void lcd_cfg_resend_timer_func(unsigned long unused);
static void del_resend_lcd_cfg_timer(void);
static void init_lcd_buf_sdma_tx_table(void);
static void LcdSdmaChannelIntHandler(int para);
static void prepare_for_lcdbuf_sdma(void);
static void set_lcdctrl_start_page_addr(unsigned char page_addr);
static void hx8346a_power_on(void);
static void hx8346a_power_off(void);
static void  set_lcd_display_setting(void);
/********************* Module Declarations **********************/
static struct file_operations lcd_fops =
      {
	.read 		= lcddev_read,
	.write 		= lcddev_write,
	.ioctl		= lcddev_ioctl,
	.mmap		=lcddev_mmap
       };

/********************* (1) parameter for different lcd controller *********************************/
static const TLcdCtrlPara tLcdCtrl_Para_UC1601= //used in WACS3k/5k
{
      .lcd_width=128,
      .lcd_height=64,
      .lcd_bpp=1,
      .lcd_pagesize=8,
      .lcd_max_contrast=0xFF,
      .lcd_min_contrast=0x00,
      .lcd_default_contrast=108,
      .lcd_disp_invert_flg=1,
};

static const TLcdCtrlPara tLcdCtrl_Para_S1D15E06= //used in WAC7K
{
      .lcd_width=160,
      .lcd_height=128,
      .lcd_bpp=1,
      .lcd_pagesize=8,
      .lcd_max_contrast=0x7F,
      .lcd_min_contrast=0x00,
      .lcd_default_contrast=0x60,
      .lcd_disp_invert_flg=1,
};

static const TLcdCtrlPara tLcdCtrl_Para_S6B0741= //used in WAS7K
{
      .lcd_width=128,
      .lcd_height=96,
      .lcd_bpp=1,
      .lcd_pagesize=8,
      .lcd_max_contrast=0x3F,
      .lcd_min_contrast=0x00,
      .lcd_default_contrast=0x20,
      .lcd_disp_invert_flg=1,
};

static const TLcdCtrlPara tLcdCtrl_Para_HX8312A= //used in WAC7K5
{
      .lcd_width=320,   // config as width is 320
      .lcd_height=240,  // config as width is 240
      .lcd_bpp=24,      // 18 out of 24 is actually used
      .lcd_pagesize=8,
      .lcd_max_contrast=0x1f,
      .lcd_min_contrast=0x00,
      .lcd_default_contrast=0x0f,
      .lcd_disp_invert_flg=1,
};

static const TLcdCtrlPara tLcdCtrl_Para_HX8346A= //used in WAC7K5
{
      .lcd_width=320,   // config as width is 320
      .lcd_height=240,  // config as width is 240
      .lcd_bpp=24,      // 18 out of 24 is actually used
      .lcd_pagesize=8,
      .lcd_max_contrast=0x16,
      .lcd_min_contrast=0x00,
      .lcd_default_contrast=0x10,
      .lcd_disp_invert_flg=1,
};

/********************** (2) init data serials of different lcd controller ****************************/
static const TBatchDataToLcdCtrl atLcdCtrl_InitData_UC1601[]= //used in WACS3k/5k
{
     {0xA1,0},  // set fram rate to 95
     {0xEB,0},  // set lcd bias ratio
     {0x89,0},  // RAM address control
     {0xC2,0},  // LCD mapping control
     {0xA7,0},  //set display inverse mode,this value should be modify if .lcd_disp_invert_flg changed
     {0,0xA5} //the last entry
};

static const TBatchDataToLcdCtrl atLcdCtrl_InitData_S1D15E06[]= //used in WAC7K
{
     {0xbf,0}, // Off Mode select
     {0xa7,0}, // Normal 0xa6 / Reversed Display 0xa7,this value should be modify if .lcd_disp_invert_flg changed
     {0xa4,0}, // a4: normal display / a5: All on
     {0xc5,0}, // Common Output scan direction 0xc4: 0->131 0xc5:131->0
     {0x84,0}, // scanning in Col
     {0xa0,0}, // Col scan direction
     {0x66,0}, // Binary display format
     {0x01,1}, // 00:4Gray 01: binary
     {0x39,0},  // grey scale pattern set
     {0x35,1}, // ...
     {0x00,0}, // Set temperature range
     {0x6d,0}, //duty set
     {0x20,1}, // 1/128 duty set
     {0x00,1}, // start block is from com 0
     {0xab,0},//built-in osc in
     {0xa8,0},//power save off
     {0x2b,0}, // LC Drive Voltage select
     {0x03,1}, // 00:lowest, 10:middle , 11: highest
     {0x25,0}, //Power control
     {0x1f,1}, //...
     {0x5f,0}, // Frame Frequency Adjust
     {0x03,1},// Range is from 0-15
     {0xe4,0}, // N-Line inversion on/off 0:e4
     {0x36,0}, // Scheme Adjust
     {0x11,1}, // Range is from 0-31
     {0x10,0}, //area scroll set
     {0x00,1}, //mode
     {0x8a,0}, // set Display start Line
     {0x06,1},//...
     {0x00,1},//...

     {0,0xA5} //the last entry
};

static const TBatchDataToLcdCtrl atLcdCtrl_InitData_S6B0741[]= //used in WAS7K
{
       {0x48,0}, // duty
       {0x60,0}, // ...
       {0x93,0}, //FRC and PWM mode
       {0xAB,0}, // SET START_OSC
       {0x88,0}, // SET GRAY_SET1_1
       {0x00,0}, // SET GRAY_SET1_2
       {0x89,0}, // SET GRAY_SET2_1
       {0x00,0}, // SET GRAY_SET2_2
       {0x8a,0}, // SET GRAY_SET3_1
       {0x66,0},
       {0x8b,0}, // SET GRAY_SET4_1
       {0x66,0},
       {0x8c,0}, // SET GRAY_SET5_1
       {0xaa,0},
       {0x8d,0}, // SET GRAY_SET6_1
       {0xaa,0},
       {0x8e,0}, // SET GRAY_SET7_1
       {0xff,0}, // SET GRAY_SET7_2
       {0x8f,0}, // SET GRAY_SET8_1
       {0xff,0}, // SET GRAY_SET8_2
       {0x66,0}, // SET DC_DC
       {0x27,0}, // SET RESISTOR_RADIO
       {0x55,0}, // SET BAIS
       {0x2f,0}, // SET POWER CONTROL
       {0xa0,0}, //ADC
       {0xc8,0}, // SET SHL
       {0x44,0}, // SET START_COM1
       {0x10,0}, // SET START_COM2
       {0x40,0}, // SET START_LINE1
       {0x00,0}, // SET START_LINE2
       {0xb0,0}, // SET PAGE ADDRESS
       {0x10,0}, // SET COL ADDRESS H
       {0x00,0}, // SET COL ADDRESS L
       {0xa7,0}, // SET DISPLAY reverse MODE
       {0xa2,0}, // SET ICON_FEG
       {0xE1,0}, // SET RELEASE POWER SAVE MODE
       {0xE4,0}, // SET N_LINE INVERSION

       {0,0xA5} //the last entry
};

static const TBatchDataToLcdCtrl atLcdCtrl_InitData_HX8312A[]= //used in WAC7K5
{
       //{0x01,0},{0x90,0}, // ADX set to 1 for current outlook
       //{0x00,0},{0x40,0}, // off in init stage
       //{0x03,0},{0x01,0},
       //{10,2},
       //{0x03,0},{0x00,0},
       {0x01,0},{0x90,0}, // ADX set to 1 for current outlook
       {0x05,0},{0x04,0},  // Y address increment ,then X addr increment
       {0x2b,0},{0x04,0},
       {0x59,0},{0x01,0},
       {0x60,0},{0x22,0},
       {0x59,0},{0x00,0},
       {0x28,0},{0x18,0},
       {0x1a,0},{0x05,0},
       {0x25,0},{0x05,0},
       {0x19,0},{0x00,0},
       {0x1c,0},{0x73,0},
       {0x24,0},{0x74,0},
       {0x1e,0},{0x01,0},
       {0x18,0},{0xc1,0},
       {10,2},
       {0x18,0},{0xe1,0},
       {0x18,0},{0xf1,0},
       {60,2},
       {0x18,0},{0xf5,0},
       {60,2},
       {0x1b,0},{0x09,0},
       {10,2},
       {0x1f,0},{0x11,0},
       {0x1e,0},{0x81,0},
       {10,2},
       {0x9d,0},{0x00,0},
       {0xc0,0},{0x00,0},
       {0xc1,0},{0x01,0},
       {0x0e,0},{0x00,0},
       {0x0f,0},{0x00,0},
       {0x10,0},{0x00,0},
       {0x11,0},{0x00,0},
       {0x12,0},{0x00,0},
       {0x13,0},{0x00,0},
       {0x14,0},{0x00,0},
       {0x15,0},{0x00,0},
       {0x16,0},{0x00,0},
       {0x17,0},{0x00,0},
       {0x34,0},{0x01,0},
       {0x35,0},{0x00,0},
       {0x4b,0},{0x00,0},
       {0x4c,0},{0x00,0},
       {0x4e,0},{0x00,0},
       {0x4f,0},{0x00,0},
       {0x50,0},{0x00,0},
       {0x3c,0},{0x00,0},
       {0x3d,0},{0x00,0},
       {0x3e,0},{0x01,0},
       {0x3f,0},{0x3f,0},
       {0x40,0},{0x02,0},
       {0x41,0},{0x02,0},
       {0x42,0},{0x00,0},
       {0x43,0},{0x00,0},
       {0x44,0},{0x00,0},
       {0x45,0},{0x00,0},
       {0x46,0},{0xef,0},
       {0x47,0},{0x00,0},
       {0x48,0},{0x00,0},
       {0x49,0},{0x01,0},
       {0x4a,0},{0x3f,0},
       {0x1d,0},{0x00,0},
       {0x86,0},{0x00,0},
       {0x87,0},{0x30,0},
       {0x88,0},{0x02,0},
       {0x89,0},{0x05,0},
       {0x8d,0},{0x01,0},
       {0x8b,0},{0x30,0},
       {0x33,0},{0x01,0},
       {0x37,0},{0x01,0},
       {0x76,0},{0x00,0},
       {0x8f,0},{0x10,0},
       {0x90,0},{0x67,0},
       {0x91,0},{0x07,0},
       {0x92,0},{0x56,0},
       {0x93,0},{0x07,0},
       {0x94,0},{0x01,0},
       {0x95,0},{0x76,0},
       {0x96,0},{0x65,0},
       {0x97,0},{0x00,0},
       {0x98,0},{0x00,0},
       {0x99,0},{0x00,0},
       {0x9a,0},{0x00,0},
       {0x3b,0},{0x01,0},
       {40,2},
       //{0x00,0},{0x40,0}, // off in init stage

       {0,0xA5} //the last entry
};


static const TBatchDataToLcdCtrl atLcdCtrl_InitData_hx8346A[]= //used in WAC7K5
{
	{0x46,0},{0x22,1},
	{0x47,0},{0x22,1},
	{0x48,0},{0x20,1},
	{0x49,0},{0x35,1},
	{0x4a,0},{0x00,1},
	{0x4b,0},{0x77,1},
	{0x4c,0},{0x24,1},
	{0x4d,0},{0x75,1},
	{0x4e,0},{0x12,1},
	{0x4f,0},{0x28,1},
	{0x50,0},{0x24,1},
	{0x51,0},{0x44,1},

/**********************************240x320 window setting*******************************/
	{0x02,0},{0x00,1}, 	// Column address start2
	{0x03,0},{0x00,1},	// Column address start1
	{0x04,0},{0x01,1}, 	// Column address end2 0x00
	{0x05,0},{0x3f,1}, 	// Column address end1 0xef

	{0x06,0},{0x00,1},	// Row address start2
	{0x07,0},{0x00,1}, 	// Row address start1
	{0x08,0},{0x00,1}, 	// Row address end2 0x01
	{0x09,0},{0xef,1},	// Row address end1 0x3f   //320*240;

/**************************************Display Setting*************************************/
	{0x01,0},{0x06,1},	// IDMON=0, INVON=1, NORON=1, PTLON=0

//	{0x16,0},{0xa8,1},	// for 8347a: MY=1, MX=0, MV=1, ML=0, BGR=1
	{0x16,0},{0xf8,1},	// for 8346a: MY=1, MX=1, MV=1, ML=1, BGR=1

	{0x23,0},{0x95,1},	// N_DC=1001 0101
	{0x24,0},{0x95,1},	// P_DC=1001 0101
	{0x25,0},{0xff,1},	 // I_DC=1111 1111

	{0x27,0},{0x02,1},	// N_BP=0000 0010
	{0x28,0},{0x02,1},	// N_FP=0000 0010
	{0x29,0},{0x02,1},	// P_BP=0000 0010
	{0x2a,0},{0x02,1},	// P_FP=0000 0010
	{0x2c,0},{0x02,1},	// I_BP=0000 0010
	{0x2d,0},{0x02,1},	// I_FP=0000 0010

	{0x3a,0},{0x01,1},	// N_RTN=0000, N_NW=001
	{0x3b,0},{0x01,1}, 	// P_RTN=0000, P_NW=001
	{0x3c,0},{0xf0,1},	// I_RTN=1111, I_NW=000
	{0x3d,0},{0x00,1},	// DIV=00

       {20,2},				//delay 2

/*************************************Power Supply Setting***********************************/


	{0x19,0},{0x41,1},	// OSCADJ=010000, OSD_EN=1;41
	{10,2},				//delay 10

/********************for the setting before power supply startup************/

	{0x20,0},{0x40,1},	// BT=0100;40
       {10,2},				//delay 2

	{0x1d,0},{0x47,1},	// VC2=100, VC1=100
	{0x1e,0},{0x00,1},	// VC3=000
	{0x1f,0},{0x03,1},	// VRH=0110

	{0x44,0},{0x20,1},	// VCM=101 1010,VCOMH=VREG1*0.845.20
	{0x45,0},{0x0c,1},	// VDV=1 0001,VCOM=1.08*VREG1£¬0e
       {10,2},				//delay 1

/********************for power supply setting********************************/

	{0x1c,0},{0x04,1}, 	// AP=100
       {20,2},				//delay 1

	{0x1b,0},{0x18,1},	// GASENB=0, PON=1, DK=1, XDK=0, DDVDH_TRI=0, STB=0
       {40,2},				//delay 4

	{0x1b,0},{0x10,1},	// GASENB=0, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
       {40,2},				//delay 4

	{0x43,0},{0x80,1}, 	// VCOMG=1
       {100,2},				//delay 1

/****** Display ON Setting******/
	{0x30,0},{0x08,1},
       {40,2},				//delay 4

	{0x26,0},{0x04,1},
       {40,2},				//delay 4
	{0x26,0},{0x24,1},
	{0x26,0},{0x2c,1},
       {40,2},				//delay 4

	{0x26,0},{0x3c,1},

       {0,0xA5} //the last entry


};

static const TBatchDataToLcdCtrl atLcdCtrl_InitData_hx8347A[]= //for WACS7k5 New model, Dec 17, 2007, by Keith
{
	{0x46,0},{0x95,1},
	{0x47,0},{0x51,1},
	{0x48,0},{0x00,1},
	{0x49,0},{0x36,1},
	{0x4a,0},{0x11,1},
	{0x4b,0},{0x66,1},
	{0x4c,0},{0x14,1},
	{0x4d,0},{0x77,1},
	{0x4e,0},{0x13,1},
	{0x4f,0},{0x4c,1},
	{0x50,0},{0x46,1},
	{0x51,0},{0x46,1},
	

/**********************************240x320 window setting*******************************/
	{0x02,0},{0x00,1}, 	// Column address start2
	{0x03,0},{0x00,1},	// Column address start1
	{0x04,0},{0x01,1}, 	// Column address end2 0x00
	{0x05,0},{0x3f,1}, 	// Column address end1 0xef

	{0x06,0},{0x00,1},	// Row address start2
	{0x07,0},{0x00,1}, 	// Row address start1
	{0x08,0},{0x00,1}, 	// Row address end2 0x01
	{0x09,0},{0xef,1},	// Row address end1 0x3f   //320*240;
	
/**************************************Display Setting*************************************/
	{0x01,0},{0x06,1},	// IDMON=0, INVON=1, NORON=1, PTLON=0

	{0x16,0},{0xa8,1},	// for 8347a: MY=1, MX=0, MV=1, ML=0, BGR=1
//	{0x16,0},{0xf8,1},	// for 8346a: MY=1, MX=1, MV=1, ML=1, BGR=1

	{0x23,0},{0x95,1},	// N_DC=1001 0101
	{0x24,0},{0x95,1},	// P_DC=1001 0101
	{0x25,0},{0xff,1},	 // I_DC=1111 1111

/*
	{0x27,0},{0x06,1},	 //
	{0x28,0},{0x06,1},	// N_BP=0000 0010
	{0x29,0},{0x06,1},	// N_FP=0000 0010
	{0x2a,0},{0x06,1},	// P_BP=0000 0010
	{0x2c,0},{0x06,1},	// I_BP=0000 0010
	{0x2d,0},{0x06,1},	// I_FP=0000 0010
*/
	{0x27,0},{0x02,1},	 //copied from hx8346a
	{0x28,0},{0x02,1},	// N_BP=0000 0010
	{0x29,0},{0x02,1},	// N_FP=0000 0010
	{0x2a,0},{0x02,1},	// P_BP=0000 0010
	{0x2c,0},{0x02,1},	// I_BP=0000 0010
	{0x2d,0},{0x02,1},	// I_FP=0000 0010

	{0x3a,0},{0x01,1},	// N_RTN=0000, N_NW=001
	{0x3b,0},{0x01,1}, 	// P_RTN=0000, P_NW=001
	{0x3c,0},{0xf0,1},	// I_RTN=1111, I_NW=000
	{0x3d,0},{0x00,1},	// DIV=00

       {50,2},				//delay 50ms

	{0x35,0},{0x38,1},	// internal used
	{0x36,0},{0x78,1}, 	// internal used
	{0x3e,0},{0x38,1},	// cycle control 5
	{0x40,0},{0x0f,1},	// cycle control 6
	{0x41,0},{0xf0,1},	// display control 3

       {50,2},				//delay 50ms


/*************************************Power Supply Setting***********************************/

	{0x19,0},{0x49,1},	// OSCADJ=010000, OSD_EN=1;41
	{0x93,0},{0x0c,1},	// OSC control 3
	{50,2},				//delay 50

/********************for the setting before power supply startup************/

	{0x20,0},{0x40,1},	// BT=0100;40
       {10,2},				//delay 2

	{0x1d,0},{0x07,1},	// 
	{0x1e,0},{0x00,1},	// VC3=000
	{0x1f,0},{0x04,1},	// VRH=0110

	{0x44,0},{0x4d,1},	// 
	{0x45,0},{0x11,1},	// VDV=1 0001,VCOM=1.08*VREG1£¬0e
       {50,2},				//delay 50

/********************for power supply setting********************************/

	{0x1c,0},{0x04,1}, 	// AP=100
       {50,2},				//delay 1

	{0x1b,0},{0x18,1},	// GASENB=0, PON=1, DK=1, XDK=0, DDVDH_TRI=0, STB=0
       {50,2},				//delay 4

	{0x1b,0},{0x10,1},	// GASENB=0, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
       {50,2},				//delay 4

	{0x43,0},{0x80,1}, 	// VCOMG=1

/****** Display ON Setting******/

	{0x90,0},{0x7f,1},     //internal again
	{0x26,0},{0x04,1},
       {50,2},				//delay 4
	{0x26,0},{0x24,1},
	{0x26,0},{0x2c,1},
       {50,2},				//delay 4

	{0x26,0},{0x3c,1},

/****** Internal registers Setting******/
	{0x57,0},{0x02,1},
	{0x95,0},{0x31,1},
	{0x57,0},{0x00,1},

       {0,0xA5} //the last entry


};

#define RESET_REG_ADD (0x01)

#define DISPLAY_ON_OFF_REG_ADD	(0x01)

#define DISPLAY_ON					(0x6)
#define DISPLAY_OFF					(0x2)

#define VCOM_REG_ADD				(0x45)  // the contrast for color lcd

/******************************* implementation of functions *********************************/
static unsigned char get_curr_lcd_type (void)//return 0xFF means can not identify the board type
{
    unsigned int w_ad_value;
    unsigned char lcd_type;
    char *product_name;

    printk("\n **** read current board type *******\n");
    adcdrv_set_mode(1);             //may be initialized twice, anyway should be ok
    w_ad_value = adc_read (3);
    printk("\n w_ad_value=%d",w_ad_value);

#if defined (CONFIG_PNX0106_W6)
    /* gen-1.5 server and client platform code goes here */
    if (((w_ad_value >=   0) && (w_ad_value <=  62)) ||         /*   0..  62: WAC-5000, 12 MHz xtal, 32 MByte SDRAM */
        ((w_ad_value >= 669) && (w_ad_value <= 740)))           /* 669.. 740: WAC-5000, 10 MHz xtal, 32 MByte SDRAM */
    {
        product_name = "wac5000";
        lcd_type = UC1601_LCD;
    }
    else if( ((w_ad_value >= 584) && (w_ad_value <= 645)) ||       /* 584.. 645: WAC-7000, 12 MHz xtal, 32 MByte SDRAM */
           ((w_ad_value >= 206) && (w_ad_value <= 235)))          /* wac7005*/
    {
        product_name = "wac7000";
        lcd_type = S1D15E06_LCD;
    }
    else if (((w_ad_value >= 389) && (w_ad_value <= 430)) ||    /* 389.. 430: WAS-7000, 12 MHz xtal, 32 MByte SDRAM */
             ((w_ad_value >= 823) && (w_ad_value <= 854)) ||      /* 823.. 854: WAS-7000, 12 MHz xtal, 16 MByte SDRAM */
             ((w_ad_value >= 788) && (w_ad_value <= 819)))          /* was7005*/
    {
        product_name = "was7000";
        lcd_type = S6B0741_LCD;
    }
    else if (((w_ad_value >= 973) && (w_ad_value <= 1024)) ||   /* 973..1024: WAS-5000, 12 MHz xtal, 32 MByte SDRAM */
             ((w_ad_value >= 169) && (w_ad_value <=  201)) ||   /* 169.. 201: WAS-5000, 12 MHz xtal, 16 MByte SDRAM */
             ((w_ad_value >= 303) && (w_ad_value <=  335)))     /* 303.. 335: WAS-5000, 10 MHz xtal, 32 MByte SDRAM */
    {
        product_name = "was5000";
        lcd_type = UC1601_LCD;
    }
    else
    {
        product_name = "UNKNOWN";
        lcd_type = 0xFF;
    }
#elif defined(CONFIG_PNX0106_HASLI7) || defined (CONFIG_PNX0106_MCIH)
    /* gen-2.0 server platforms (ie wac3k5 and wac7k5) code goes here */
    if ( (w_ad_value >= 0) && (w_ad_value <= 62) )     /*   0..  62: WAC-5000, 12 MHz xtal, 32 MByte SDRAM */
    {
        product_name = "wac3k5";
        lcd_type = S6B0741_LCD;
    }
    else if( ((w_ad_value >= 584) && (w_ad_value <= 645)) ||       /* 584.. 645: WAC-7000, 12 MHz xtal, 32 MByte SDRAM */
	   ((w_ad_value >= 101) && (w_ad_value <= 123)) || //WAC7500, HX8347
	   ((w_ad_value >= 296) && (w_ad_value <= 342)) || //MCi500, HX8347
	   ((w_ad_value >= 681) && (w_ad_value <= 727)) || //MCi500, HX8347
           ((w_ad_value >= 973) && (w_ad_value <= 1024)) )          /* wac7005*/
    {
        product_name = "wac7k5";
        lcd_type = HX8346A_LCD;
        curr_tft_type=0x46;
	if (((w_ad_value >= 101) && (w_ad_value <= 123)) || //WAC7500, HX8347
	   ((w_ad_value >= 296) && (w_ad_value <= 342)) || //MCi500, HX8347
	   ((w_ad_value >= 681) && (w_ad_value <= 727))) //MCi500, HX8347
	curr_tft_type=0x47; //keith
	
    }
    else
    {
        product_name = "UNKNOWN";
        lcd_type = 0xFF;
    }
#elif defined (CONFIG_PNX0106_HACLI7)
    /* gen-2.0 client platforms (ie was7k5) code goes here */
    product_name = "was7k5";
    lcd_type = HX8346A_LCD;
    curr_tft_type=0x46;
    if (((w_ad_value >= 901) && (w_ad_value <= 923))) //WAS7500, HX8347
    curr_tft_type=0x47; //keith
#elif defined(CONFIG_PNX0106_MCI)
    product_name = "mci300";
    lcd_type = S6B0741_LCD;
#else
#error "unknown platform"
#endif

    printk ("\n product=%s\n", product_name);
    return lcd_type;
}

static void  show_lcd_type_ver_info()
{
    printk("\n*******************************************************************");
    switch(curr_lcd_type) // type of lcd controller
       {
          case     UC1601_LCD:// used in WACS3k/5k
                    printk("\n type of lcd controller: UC1601,used in WACS3k/5k");
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    printk("\n type of lcd controller: S1D15E06_LCD,used in WAC7K");
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    printk("\n type of lcd controller: S6B0741,used in WAS7K");
                    break;
          case     HX8346A_LCD: // used in WAC7K5
                    printk("\n type of lcd controller: HX8346A/8347A,used in WAC7K5");
                    break;
          default:
                    printk("\n unknown board type!");
                    break;
       }
    printk("\n VERSION:  %s",LCD_DRV_VER_INFO);//version tips
    printk("\n*******************************************************************\n");
}

static void fill_lcd_ctrl_para(TLcdCtrlPara tLcdCtrlPara)
{
      LCD_WIDTH=tLcdCtrlPara.lcd_width;
      LCD_HEIGHT=tLcdCtrlPara.lcd_height;
      LCD_BPP=tLcdCtrlPara.lcd_bpp;
      LCD_PAGESIZE=tLcdCtrlPara.lcd_pagesize;
      MAX_LCD_CONTRAST=tLcdCtrlPara.lcd_max_contrast;
      MIN_LCD_CONTRAST=tLcdCtrlPara.lcd_min_contrast;
      curr_lcd_contrast=tLcdCtrlPara.lcd_default_contrast;
      lcd_disp_invert_flg=tLcdCtrlPara.lcd_disp_invert_flg;
}

static void SetLcdDisplayContrast(unsigned char contrast)
{
      if(contrast > MAX_LCD_CONTRAST) //check the range of para
         {
        	printk("\n the contrast value to set exceed max range of lcd contrast!");
        	return;
         }
       switch(curr_lcd_type)
          {
             case  UC1601_LCD:// used in WACS3k/5k
                    Com_Out(0x81);
                    Com_Out(contrast);
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    Com_Out(0x81);
                    Data_Out(contrast);
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    Com_Out(0x81);
                    Com_Out(contrast);
                    break;
          case     HX8346A_LCD: // used in WAC7K5
                    // the contrast for color lcd seems not needed.
                    write_hx8346a_reg(VCOM_REG_ADD, (MAX_LCD_CONTRAST-contrast)&0x1f);
					printk("HX8346A_LCD:set contast = %d\n",contrast);
                    break;
          default:
                    printk("\n current board type not known!");
                    return;
      }

    curr_lcd_contrast=contrast;//save current contrast
}

static void IncLcdContrast(void)
{
    if(curr_lcd_contrast<MAX_LCD_CONTRAST)
       SetLcdDisplayContrast(curr_lcd_contrast+1);
}

static void DecLcdContrast(void)
{
    if(curr_lcd_contrast>0)
       SetLcdDisplayContrast(curr_lcd_contrast-1);
}

static void init_lcddrv_para(void) //shoud do after current lcd type got
{
    switch(curr_lcd_type)
      {
          case     UC1601_LCD:// used in WACS3k/5k
                    fill_lcd_ctrl_para(tLcdCtrl_Para_UC1601);
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    fill_lcd_ctrl_para(tLcdCtrl_Para_S1D15E06);
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    fill_lcd_ctrl_para(tLcdCtrl_Para_S6B0741);
                    break;
          case     HX8346A_LCD:
                    fill_lcd_ctrl_para(tLcdCtrl_Para_HX8346A);
                    break;
          default:
                    printk("\n current board type not known!");
                    return;
      }

    LCD_PAGES = (LCD_HEIGHT/LCD_PAGESIZE);
    LCD_RAMBUF_LEN = (LCD_PAGES*LCD_WIDTH*LCD_BPP);

}

static void write_batch_data_to_lcd_ctrl(const TBatchDataToLcdCtrl atLcdCtrlInitData[])
{
    int index;

    index=0;
    while( atLcdCtrlInitData[index].byType != LASTENTRY_TYPE )
      {
      	switch(atLcdCtrlInitData[index].byType)
      	   {
      	       case  CMD_TYPE:
      	              Com_Out(atLcdCtrlInitData[index].byVal);
      	              break;
      	       case  DATA_TYPE:
      	              Data_Out(atLcdCtrlInitData[index].byVal);
      	              break;
      	       case  DELAY_TYPE:
      	              mdelay(atLcdCtrlInitData[index].byVal);
      	              break;
      	       default:
      	              break;
      	   } // end of switch(...
      	index++;
      } // end of while(...
}

static void lcdif_poll_for_fifo_space (unsigned int space_required)
{
    unsigned int space_available;
    LcdStatusUnion32bitReg tLcdStatusValue;

    if (curr_lcd_type==HX8346A_LCD)
    {
        // try not polling for the color lcd in WAC7K5
        //return;
    }

    while (1)
      {
         tLcdStatusValue.regval_32bit=lcd_hw_if_read(LCD_HW_IF_STATUS);
         space_available = (LCD_FIFO_LEN - tLcdStatusValue.RegField.lcd_counter);
         if (space_available >= space_required)
            break;
      }
}

static inline void Com_Out(unsigned char instByte)
{
    lcdif_poll_for_fifo_space ((previous_fifo_contents == FIFO_CONTENTS_INST) ? 1 : LCD_FIFO_LEN);
    SEND_INSTBYTE_TO_LCDCTRL(instByte);
    previous_fifo_contents = FIFO_CONTENTS_INST;
}
static inline void Data_Out(unsigned char dataByte)
{
    lcdif_poll_for_fifo_space ((previous_fifo_contents == FIFO_CONTENTS_DATA) ? 1 : LCD_FIFO_LEN);
    SEND_DATABYTE_TO_LCDCTRL(dataByte);
    previous_fifo_contents = FIFO_CONTENTS_DATA;
}


#if 0
static unsigned char read_hx8312a_reg(unsigned char addr)
{
    LcdDataByteUnion32bitReg tLcdDataByteUnion32bitReg;
    LcdStatusUnion32bitReg    tLcdStatusValue;
    unsigned int check_count;

    //gpio_set_low(LCD_CD);
    Com_Out(addr);
    lcd_hw_if_write(LCD_HW_IF_READ_CMD,0);
    check_count = 0;
    while (1)
    {
        tLcdStatusValue.regval_32bit=lcd_hw_if_read(LCD_HW_IF_STATUS);
        if (tLcdStatusValue.RegField.lcd_interface_busy == 0)
            break;

        check_count++;
        if (check_count > 1000) // about serval us
        {
            printk("\n wait for read response of lcd ctr for a long time,some thing wrong ?");
            break;
        }
    }
    tLcdDataByteUnion32bitReg.regval_32bit = lcd_hw_if_read(LCD_HW_IF_INST_BYTE);

    //udelay(1);
    //gpio_set_high(LCD_CD);

    return tLcdDataByteUnion32bitReg.RegField.data_byte;
}

static void write_hx8312a_reg(unsigned char addr, unsigned char cmd)
{
    //gpio_set_low(LCD_CD);
    //udelay(1);

    Com_Out(addr);
    Com_Out(cmd);
    //udelay(1);

    //gpio_set_high(LCD_CD);
}
#endif

static unsigned char read_hx8346a_reg(unsigned char addr)
{
    LcdDataByteUnion32bitReg tLcdDataByteUnion32bitReg;
    LcdStatusUnion32bitReg    tLcdStatusValue;
    unsigned int check_count;
//    gpio_set_low(LCD_CD);
    Com_Out(addr);
    lcd_hw_if_write(LCD_HW_IF_READ_CMD,1); //initial a read on INST_BYTE (RS=0) or DATA_BYTE (RS=1)

    check_count = 0;
    while (1)
    {
        tLcdStatusValue.regval_32bit=lcd_hw_if_read(LCD_HW_IF_STATUS);
        if (tLcdStatusValue.RegField.lcd_interface_busy == 0)
            break;

        check_count++;
        if (check_count > 1000) // about serval us
        {
            printk("\n wait for read response of lcd ctr for a long time,some thing wrong ?");
            break;
        }
    }
//	tLcdDataByteUnion32bitReg.regval_32bit=0;
//    tLcdDataByteUnion32bitReg.regval_32bit = lcd_hw_if_read(LCD_HW_IF_INST_BYTE);
    tLcdDataByteUnion32bitReg.RegField.data_byte = lcd_hw_if_read(LCD_HW_IF_INST_BYTE);

//    udelay(1);
//    gpio_set_high(LCD_CD);

//    printk("\n read reg=%x\n",tLcdDataByteUnion32bitReg.RegField.data_byte);

    return tLcdDataByteUnion32bitReg.RegField.data_byte;
}

static void write_hx8346a_reg(unsigned char addr, unsigned char cmd)
{
    Com_Out(addr);
    Data_Out(cmd);
}

static void call_lcd_buf_transfer(void)
{
	lcd_buf_send_finish_flag=0;
	up(&LcdRamBufTrans_sem);
}

static int AllocateMemForLcdModule(void)
{
    int i, orders, pages;

    // LCD video buf
    orders = get_order(LCD_RAMBUF_LEN);
    pLcdVideoBuf=(unsigned char *)__get_dma_pages(GFP_KERNEL, orders);
    if(!pLcdVideoBuf)
    {
        printk("\n FATAL error,can not malloc memory for lcd video buf.\n");
        return 1;
    }

    pages = 1 << orders;
    printk("\n allocated pages for lcd video buf =%d", pages);
    for (i=0; i<pages; i++)
    {
        //reserve the page of lcd video buf to make it remappable
//        printk("\n map reserve page #%d, virt=0x%x", i, (int)(pLcdVideoBuf + i * PAGE_SIZE));
        mem_map_reserve( virt_to_page(pLcdVideoBuf + i * PAGE_SIZE) );
    }

    //for was7k sdma transfer buf
    if (curr_lcd_type==S6B0741_LCD)
       {
       	// in this case , kmalloc is sufficient for allocating needed buf
           pLcdSdmaTxBufWas7k = (unsigned char *)kmalloc(PAGE_ALIGN(LCD_RAMBUF_LEN*2),GFP_KERNEL);
           if( !pLcdSdmaTxBufWas7k)
            {
                printk("\n FATAL error,can not malloc memory for was7k sdma buf.\n");
                return 2;
            }
       }
    return 0;
}

static void Additional_Treat_For_LcdCtrl(void) //process for some special case of lcd controller
{
   switch(curr_lcd_type)
      {
          case     UC1601_LCD:// used in WACS3k/5k
                    //no special treat needed present yet
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    if(pptr_board_flag)
                        SetLcdDisplayContrast(0x3f);//lcd display contrast
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    //no special treat needed now
                    break;
          case     HX8346A_LCD: // used in WAC7K5
                    //no special treat needed now
                    break;
          default:
                    printk("\n current board type not known!");
                    break;
      }
}

static void display_startup_logo(void)
{
   unsigned char *pStartupLogoBuf;
   unsigned int   length_of_logo_data_array;
   int i;

   switch(curr_lcd_type)
      {
          case     UC1601_LCD:// used in WACS3k/5k
                    pStartupLogoBuf=(unsigned char *)startup_logo_data_128_64;
                    length_of_logo_data_array=(tLcdCtrl_Para_UC1601.lcd_width * tLcdCtrl_Para_UC1601.lcd_height)/8;
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    pStartupLogoBuf=(unsigned char *)startup_logo_data_160_128;
                    length_of_logo_data_array=(tLcdCtrl_Para_S1D15E06.lcd_width * tLcdCtrl_Para_S1D15E06.lcd_height)/8;
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    pStartupLogoBuf=(unsigned char *)startup_logo_data_128_96;
                    length_of_logo_data_array=(tLcdCtrl_Para_S6B0741.lcd_width * tLcdCtrl_Para_S6B0741.lcd_height ) /8;
                    break;
          case     HX8346A_LCD:
                    pStartupLogoBuf=(unsigned char *)startup_logo_data_320_240;
                    length_of_logo_data_array=(tLcdCtrl_Para_HX8346A.lcd_width * \
                                                    tLcdCtrl_Para_HX8346A.lcd_height * \
                                                    tLcdCtrl_Para_HX8346A.lcd_bpp) /8;
                    break;
          default:
                    printk("\n current board type not known! no startup logo data filled");
                    return;
      }

	if(LCD_RAMBUF_LEN>length_of_logo_data_array)
	{
		printk("\n LCD_RAMBUF_LEN should not bigger than length_of_logo_data_array,pls check !");
		return;
	}
	for(i=0;i<LCD_RAMBUF_LEN;i++)
	{
		*(unsigned char *)(pLcdVideoBuf+i) = (*(unsigned char *)(pStartupLogoBuf+i)) ^ 0xFF;
	}
	if ( curr_lcd_type == HX8346A_LCD) //i want it displayed immediately --keith
	{
		lock_lcdctrl_access();
	        prepare_for_lcdbuf_sdma();
	        sdma_setup_channel (sdma_channel_for_lcdbuf,&lcd_sdma_table.config_list[0],(void *)LcdSdmaChannelIntHandler);
		Com_Out( 0x22 );
        	sdma_config_irq(sdma_channel_for_lcdbuf,1);//enable channel finished irq
        	sdma_start_channel(sdma_channel_for_lcdbuf);//start sdma transfer
        	//set cs high after a frame transmitted
        	lcdif_poll_for_fifo_space(LCD_FIFO_LEN);
        	release_lcdctrl_access();
		mdelay(300);

         	gpio_set_as_output (GPIO_LCD_BACKLIGHT, 0);		/* wac3k5: high == on, wacs7k5: high == off */

        }
        else
        {
        	call_lcd_buf_transfer();
        }


	
}
static void hx8346a_power_on(void)
{

/*************************************Power Supply Setting***********************************

WriteRegister(0x19,0x007f);  OSCADJ=010000, OSD_EN=1;41

Delay(10);
********************for the setting before power supply startup************
WriteRegister(0x20,0x0040);  BT=0100;40
Delay(2);
WriteRegister(0x1d,0x0047);  VC2=100, VC1=100
WriteRegister(0x1e,0x0000);  VC3=000
WriteRegister(0x1f,0x0003);  VRH=0110
WriteRegister(0x44,0x0020);  VCM=101 1010,VCOMH=VREG1*0.845.20
WriteRegister(0x45,0x000c); VDV=1 0001,VCOM=1.08*VREG1£¬0e
Delay(1);

********************for power supply setting********************************

WriteRegister(0x1c,0x0004);  AP=100
Delay(2);
WriteRegister(0x1b,0x0018);  GASENB=0, PON=1, DK=1, XDK=0, DDVDH_TRI=0, STB=0
Delay(4);
WriteRegister(0x1b,0x0010);  GASENB=0, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0
Delay(4);
WriteRegister(0x43,0x0080);  VCOMG=1
Delay(1);
*/



	write_hx8346a_reg( 0x20, 0x40 );
	mdelay(10);
	write_hx8346a_reg( 0x1d, 0x47 );
	write_hx8346a_reg( 0x1e, 0x00 );
	write_hx8346a_reg( 0x1f, 0x03 );
	write_hx8346a_reg( 0x44, 0x20 );
	write_hx8346a_reg( 0x45, 0x0c );
	mdelay(10);
	write_hx8346a_reg( 0x1c, 0x04 );
	mdelay(20);
	write_hx8346a_reg( 0x1b, 0x18 );
	mdelay(40);
	write_hx8346a_reg( 0x1b, 0x10 );
	mdelay(40);
	write_hx8346a_reg( 0x43, 0x80 );
	mdelay(10);

}

static void hx8346a_power_off(void)
{
	write_hx8346a_reg( 0x43, 0x00 );
	mdelay(10);
	write_hx8346a_reg( 0x1b, 0x18 );
	mdelay(10);
	write_hx8346a_reg( 0x1b, 0x08 );
	mdelay(10);

	write_hx8346a_reg( 0x1c, 0x00 );
	mdelay(10);
	write_hx8346a_reg( 0x30, 0x00 );
	mdelay(10);

}

static void  set_lcd_display_off(void) //set display off
{
    if ( curr_lcd_type == HX8346A_LCD )
    {
	write_hx8346a_reg( 0x26, 0x38 );
	mdelay(40);
	write_hx8346a_reg( 0x26, 0x28 );
	mdelay(40);
	write_hx8346a_reg( 0x26, 0x00 );

	hx8346a_power_off();
	printk("HX8346A_LCD:display off\n");

    }
    else
    {
        Com_Out(0xAE);
    }
}

static void  set_lcd_display_on(void) //set display on after init(let display off in init stage to avod display flash)
{
    if ( curr_lcd_type == HX8346A_LCD )
    {

	hx8346a_power_on();

/****** Display ON Setting******

WriteRegister(0x30,0x0008);  SAPS1=1000
Delay(4);
WriteRegister(0x26,0x0004); GON=0, DTE=0, D=01
Delay(4);
WriteRegister(0x26,0x0024); GON=1, DTE=0, D=01
WriteRegister(0x26,0x002c); GON=1, DTE=0, D=11
Delay(4);
WriteRegister(0x26,0x003c); GON=1, DTE=1, D=11
*/
	write_hx8346a_reg( 0x30, 0x08 );
	mdelay(40);
	write_hx8346a_reg( 0x26, 0x04 );
	mdelay(40);
	write_hx8346a_reg( 0x26, 0x24 );
	write_hx8346a_reg( 0x26, 0x2c );
	mdelay(40);
	write_hx8346a_reg( 0x26, 0x3c );
	printk("HX8346A_LCD:display on\n");
    }
    else
    {
        Com_Out(0xAF);
    }
}

static void  set_lcd_display_setting(void) //set display on after init(let display off in init stage to avod display flash)
{
/**************************************Display Setting*************************************
	{0x01,0},{0x06,1},	// IDMON=0, INVON=1, NORON=1, PTLON=0

	{0x16,0},{0xf8,1},	// MY=1, MX=1, MV=1, ML=1, BGR=0, TEON=0

	{0x23,0},{0x95,1},	// N_DC=1001 0101
	{0x24,0},{0x95,1},	// P_DC=1001 0101
	{0x25,0},{0xff,1},	 // I_DC=1111 1111

	{0x28,0},{0x02,1},	// N_BP=0000 0010
	{0x29,0},{0x02,1},	// N_FP=0000 0010
	{0x2a,0},{0x02,1},	// P_BP=0000 0010
	{0x2b,0},{0x02,1},	// P_FP=0000 0010
	{0x2c,0},{0x02,1},	// I_BP=0000 0010
	{0x2d,0},{0x02,1},	// I_FP=0000 0010

	{0x3a,0},{0x01,1},	// N_RTN=0000, N_NW=001
	{0x3b,0},{0x01,1}, 	// P_RTN=0000, P_NW=001
	{0x3c,0},{0xf0,1},	// I_RTN=1111, I_NW=000
	{0x3d,0},{0x00,1},	// DIV=00
*/
	write_hx8346a_reg( 0x44, 0x20 );
	write_hx8346a_reg( 0x45, 0x0c );
	mdelay(40);
	
	write_hx8346a_reg( 0x01, 0x06 );
//	if (read_hx8346a_reg(0x67)==0x47) //is it HX8347A?
	if (curr_tft_type==0x47) //new algorithm --keith
	{
	 	printk("\n HX8347A detected");
		write_hx8346a_reg( 0x16, 0xa8 ); //correct 8347-A register setting
	} 
	else
	{
	 	printk("\n HX8346A detected");
		write_hx8346a_reg( 0x16, 0xf8 ); //for 8346A
	}

}
static void lock_lcdctrl_access(void)
{
    down(&AccessLcdController_sem);
    if ( curr_lcd_type == HX8346A_LCD )
        return;

    gpio_set_low(LCD_CS);
}

static void release_lcdctrl_access(void)
{
    up(&AccessLcdController_sem);
}

static void init_lcd_cfg_resend_timer(void)
{
    #define RESEND_LCD_CFG_INTERVAL (jiffies +HZ)
    init_timer(&timer_resend_lcd_cfg);
    timer_resend_lcd_cfg.function=lcd_cfg_resend_timer_func;
    timer_resend_lcd_cfg.data=0;
    timer_resend_lcd_cfg.expires = RESEND_LCD_CFG_INTERVAL;
    add_timer(&timer_resend_lcd_cfg);
}
static void lcd_cfg_resend_timer_func(unsigned long unused)
{
    need_resend_lcd_cfg=1;
    resend_lcd_cfg_count++;
    call_lcd_buf_transfer();//should re send current display data after re send cfg data
    timer_resend_lcd_cfg.expires = RESEND_LCD_CFG_INTERVAL;
    add_timer(&timer_resend_lcd_cfg);
}
static void del_resend_lcd_cfg_timer(void)
{
    del_timer(&timer_resend_lcd_cfg);
}

static void config_lcdif_pins() //config LCD interface related pins
{
    if ( curr_lcd_type == HX8346A_LCD )  // used in WAC7K5 color lcd ,parallel 8 bit interface
    {
        printk("\n config pin for HX8346A lcd ");
        gpio_set_as_function(LCD_CD);
        gpio_set_as_function(LCD_CS);
        gpio_set_as_output (GPIO_UART0_NCTS, 1);//reset pin of lcd controller

        //gpio_set_as_output(LCD_CD,0);
        //gpio_set_as_output(LCD_CS,0);
    }
    else // others , serial interface
    {
        gpio_set_as_function(LCD_CLK); // config clk/bit/cd pin in function mode
        gpio_set_as_function(LCD_BIT);
        gpio_set_as_function(LCD_CD);

        gpio_set_as_output(LCD_CS,1);
        gpio_set_as_output (GPIO_UART0_NCTS, 1);//reset pin of lcd controller
    }
}

static void init_lcd_buf_sdma_tx_table(void)
{
    int i;

    switch(curr_lcd_type) // type of lcd controller
       {
          case     UC1601_LCD:// used in WACS3k/5k
                    lcd_sdma_table.total_items= (LCD_HEIGHT/8);
                    for(i=0;i<lcd_sdma_table.total_items;i++)
                       {
                         lcd_sdma_table.config_list[i].src_addr = __pa(pLcdVideoBuf+i*LCD_WIDTH*LCD_BPP);
	             lcd_sdma_table.config_list[i].dst_addr = (LCDIF_REGS_BASE + LCD_HW_IF_DATA_BYTE);
	             lcd_sdma_table.config_list[i].len =LCD_WIDTH+4;//4 =132 (lcd controller scan width) - 128 (display width)
	             lcd_sdma_table.config_list[i].size = TRANFER_SIZE_8;	// 8,16,32 bits
	             lcd_sdma_table.config_list[i].src_name=0;//0 means read-addr is incremented each read-cycle
	             lcd_sdma_table.config_list[i].dst_name=DEVICE_LCD; //polling full status pin before transfer
	             lcd_sdma_table.config_list[i].endian=ENDIAN_NORMAL;
	             lcd_sdma_table.config_list[i].circ_buf=NO_CIRC_BUF;
                       }
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    lcd_sdma_table.total_items= 1;
                    lcd_sdma_table.config_list[0].src_addr = __pa(pLcdVideoBuf);
	        lcd_sdma_table.config_list[0].dst_addr = (LCDIF_REGS_BASE + LCD_HW_IF_DATA_BYTE);
	        lcd_sdma_table.config_list[0].len =LCD_RAMBUF_LEN;
	        lcd_sdma_table.config_list[0].size = TRANFER_SIZE_8;	// 8,16,32 bits
	        lcd_sdma_table.config_list[0].src_name=0;//0 means read-addr is incremented each read-cycle
	        lcd_sdma_table.config_list[0].dst_name=DEVICE_LCD; //polling full status pin before transfer
	        lcd_sdma_table.config_list[0].endian=ENDIAN_NORMAL;
	        lcd_sdma_table.config_list[0].circ_buf=NO_CIRC_BUF;
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    lcd_sdma_table.total_items= (LCD_HEIGHT/8);
                    for(i=0;i<lcd_sdma_table.total_items;i++)
                       {
                         lcd_sdma_table.config_list[i].src_addr = __pa(pLcdSdmaTxBufWas7k+i*LCD_WIDTH*LCD_BPP*2);
	             lcd_sdma_table.config_list[i].dst_addr = (LCDIF_REGS_BASE + LCD_HW_IF_DATA_BYTE);
	             lcd_sdma_table.config_list[i].len =LCD_WIDTH*2;
	             lcd_sdma_table.config_list[i].size = TRANFER_SIZE_8;	// 8,16,32 bits
	             lcd_sdma_table.config_list[i].src_name=0;//0 means read-addr is incremented each read-cycle
	             lcd_sdma_table.config_list[i].dst_name=DEVICE_LCD; //polling full status pin before transfer
	             lcd_sdma_table.config_list[i].endian=ENDIAN_NORMAL;
	             lcd_sdma_table.config_list[i].circ_buf=NO_CIRC_BUF;
                       }
                    break;
          case     HX8346A_LCD:
                    lcd_sdma_table.total_items= 1;
                    lcd_sdma_table.config_list[0].src_addr = __pa(pLcdVideoBuf);
		lcd_sdma_table.config_list[0].dst_addr = (LCDIF_REGS_BASE + LCD_HW_IF_DATA_WORD);//modified by keith
	         lcd_sdma_table.config_list[0].len =LCD_RAMBUF_LEN/4; //word mode --keith
	         lcd_sdma_table.config_list[0].size = TRANFER_SIZE_32;	// 8,16,32 bits modified by keith for 32bit DMA
	         lcd_sdma_table.config_list[0].src_name=0;//0 means read-addr is incremented each read-cycle
	         lcd_sdma_table.config_list[0].dst_name=DEVICE_LCD; //polling full status pin before transfer
	         lcd_sdma_table.config_list[0].endian=ENDIAN_NORMAL;
	         lcd_sdma_table.config_list[0].circ_buf=NO_CIRC_BUF;
                    break;
          default:
                    printk("\n current board type not known,should not happen!");
                    break;
       }
}

static void set_lcdctrl_start_page_addr(unsigned char page_addr)
{
    switch(curr_lcd_type) // set page and col addr before transfer data
       {
            case     UC1601_LCD:// used in WACS3k/5k
                      if(page_addr==0) //only need to set page addr once before transfer
                         {
                           Com_Out(0xB0); //reset the diaplay data write page and col addr to 0
                           Com_Out(0x00);
                           Com_Out(0x10);
                        }
                      break;
            case     S1D15E06_LCD:// used in WAC7K
                      if(page_addr==0) //only need to set page addr once before transfer
                        {
                           Com_Out(0xb1);
	               Data_Out(1); // page 1
	               Com_Out(0x13);
	               Data_Out(0x00); // col 0
                           Com_Out(0x1d); //display data write command
                        }
                      break;
            case     S6B0741_LCD:// used in WAS7K
                      //need to set page addr for each page
                      Com_Out(0xb0+page_addr); // SET PAGE ADDRESS
                      Com_Out(0x10); // SET COL ADDRESS H
                      Com_Out(0x00); // SET COL ADDRESS L
                      break;
            case     HX8346A_LCD:

/***********************************240x320 window setting*******************************

WriteRegister(0x02,0x0000);  Column address start2

WriteRegister(0x03,0x0000);  Column address start1

WriteRegister(0x04,0x0000);  Column address end2

WriteRegister(0x05,0x00ef);  Column address end1

WriteRegister(0x06,0x0000);  Row address start2

WriteRegister(0x07,0x0000);  Row address start1

WriteRegister(0x08,0x0001);  Row address end2

WriteRegister(0x09,0x003f);  Row address end1   //240*320;
*/

                      if(page_addr==0) //only need to set addr once before transfer
                      {
                          // reset x and y addr accord to current  mapping setting 320*240
				write_hx8346a_reg(0x02,0x00);
				write_hx8346a_reg(0x03,0x00);

				write_hx8346a_reg(0x04,0x01);
				write_hx8346a_reg(0x05,0x3f);

				write_hx8346a_reg(0x06,0x00);
				write_hx8346a_reg(0x07,0x00);

				write_hx8346a_reg(0x08,0x00);
				write_hx8346a_reg(0x09,0xef);

                      }
                      break;
            default:
                      printk("\n current board type not known!");
                      break;
            }
}

static void prepare_for_lcdbuf_sdma(void)
{
    int i;
    cpu_cache_clean_invalidate_range((unsigned int)pLcdVideoBuf,(unsigned int)(pLcdVideoBuf+LCD_RAMBUF_LEN),0);//flush cache of lcd data
    if (curr_lcd_type==S6B0741_LCD)
      {
          for(i=0;i<LCD_RAMBUF_LEN;i++)
             {
                 *(unsigned char *)(pLcdSdmaTxBufWas7k+(i*2)) = *(unsigned char *)(pLcdVideoBuf+i );
                 *(unsigned char *)(pLcdSdmaTxBufWas7k+(i*2)+1) = *(unsigned char *)(pLcdVideoBuf+i );
             }
          cpu_cache_clean_invalidate_range((unsigned int)pLcdSdmaTxBufWas7k,(unsigned int)(pLcdSdmaTxBufWas7k+(LCD_RAMBUF_LEN*2)),0);//flush cache of lcd data
      }
    set_lcdctrl_start_page_addr(0);
    lcdif_poll_for_fifo_space(LCD_FIFO_LEN);//make sure fifo is empty
    lcdbuf_sdma_int_count=0;//clear sdma int counter;
}

static void LcdSdmaChannelIntHandler(int para)
{
    previous_fifo_contents = FIFO_CONTENTS_DATA;
    if(para&0x1) //check channel finish irq status
       lcdbuf_sdma_int_count++;

    if( lcdbuf_sdma_int_count == lcd_sdma_table.total_items ) //transfer complete
      {
          wake_up( &lcd_sdma_tx_wq );
      }
    else //prepare for tx next block
      {
         set_lcdctrl_start_page_addr(lcdbuf_sdma_int_count);
         lcdif_poll_for_fifo_space(LCD_FIFO_LEN);//make sure fifo is empty
         if( lcdbuf_sdma_int_count < lcd_sdma_table.total_items )
           {
             sdma_setup_channel (sdma_channel_for_lcdbuf,&lcd_sdma_table.config_list[lcdbuf_sdma_int_count],(void *)LcdSdmaChannelIntHandler);
             sdma_config_irq(sdma_channel_for_lcdbuf,1);//enable channel finished irq
             sdma_start_channel(sdma_channel_for_lcdbuf);//start sdma transfer
           }
      }

    return;
}

static void reset_and_init_lcd()
{
    //reset lcd controller and init lcd interface of melody
    if ( curr_lcd_type == HX8346A_LCD )  // used in WAC7K5 color lcd ,parallel 8 bit interface
    {
        gpio_set_low (GPIO_UART0_NCTS);
        InitLCDInterfaceOfMelody_Parallel();
        mdelay(2);
        gpio_set_high (GPIO_UART0_NCTS);
        mdelay(2);

        // perform soft reset of color lcd controller
 /*       write_hx8346a_reg(RESET_REG_ADD, 0x01);
        mdelay(10);
        write_hx8346a_reg(RESET_REG_ADD, 0x00);
	 mdelay(10);
*/
//        set_lcd_display_off();
        InitLCDController();
    }
    else
    {
        gpio_set_low (GPIO_UART0_NCTS);
        InitLCDInterfaceOfMelody();
        Com_Out(0);/* LCD SCLK idles low from reset instead of high. A dummy write fixes it... */
        udelay(10);
        gpio_set_high (GPIO_UART0_NCTS);
        udelay(1024);
        set_lcd_display_off();
        InitLCDController(); //init lcd controller
        Additional_Treat_For_LcdCtrl();
    }
    lcd_hw_just_init_flag=1;
}

static int HardwareInitForLCD(void)
{
    reset_and_init_lcd();
	if ( curr_lcd_type != HX8346A_LCD )
	{
    Fill_LCDRamBufArray_With_Byte(0);//clear screen
	}
	else
	{
		Fill_LCDRamBufArray_With_Byte(0xff);
	}
    printk("\n lcd controller init done.");
    return 0;
}

static void InitLCDInterfaceOfMelody_Parallel(void) // init the LCD interface of melody
{
    LcdControlUnion32bitReg    tLcdControlValue;
    LcdIntMaskUnion32bitReg   tLcdIntMaskValue;

    tLcdControlValue.regval_32bit=lcd_hw_if_read(LCD_HW_IF_CONTROL);
    tLcdControlValue.RegField.ps=PARALLEL_MODE; // config LCD interface in parallel mode
    tLcdControlValue.RegField.mi = 0; // 8080 mode
    tLcdControlValue.RegField.if_bitmode = 0; // 8 bit mode
    tLcdControlValue.RegField.busy_flag_check=DISABLE_BUSYFLAGCHECK;//config the busy flag check,default value after reset is DISABLE_BUSYFLAGCHECK
    tLcdControlValue.RegField.inverted_cs=CS_ACTIVE_LOW;// config the invert_cs,default value after reset is  CS_ACTIVE_HIGH
    tLcdControlValue.RegField.loopback = 0;
    lcd_hw_if_write(LCD_HW_IF_CONTROL,tLcdControlValue.regval_32bit);//write the config value

    //mask all interrupt of LCD interface,default value after reset is mask all int
    tLcdIntMaskValue.regval_32bit=0;//clear all bit;
    tLcdIntMaskValue.RegField.lcd_fifo_empty_mask=INT_MASK;
    tLcdIntMaskValue.RegField.lcd_fifo_half_empty_mask=INT_MASK;
    tLcdIntMaskValue.RegField.lcd_fifo_overrun_mask=INT_MASK;
    tLcdIntMaskValue.RegField.lcd_read_valid_mask=INT_MASK;
    lcd_hw_if_write(LCD_HW_IF_INT_MASK,tLcdIntMaskValue.regval_32bit);
    printk("\n lcd if of melody init done (parallel 8 bit 8080 mode).");
}

static void InitLCDInterfaceOfMelody(void) // init the LCD interface of melody
{
    LcdControlUnion32bitReg    tLcdControlValue;
    LcdIntMaskUnion32bitReg   tLcdIntMaskValue;

    tLcdControlValue.regval_32bit=lcd_hw_if_read(LCD_HW_IF_CONTROL);
    tLcdControlValue.RegField.ps=SERIAL_MODE; // config LCD interface in serial mode
    tLcdControlValue.RegField.serial_clk_shift=CLK_MDOE_3;//config the serial clk shit,default value after reset is CLK_MDOE_3
    tLcdControlValue.RegField.serial_read_pos=READ_POS_3; //config the serial read position,default value after reset is READ_POS_3
    tLcdControlValue.RegField.busy_flag_check=DISABLE_BUSYFLAGCHECK;//config the busy flag check,default value after reset is DISABLE_BUSYFLAGCHECK
    tLcdControlValue.RegField.inverted_cs=CS_ACTIVE_LOW;// config the invert_cs,default value after reset is  CS_ACTIVE_HIGH
    tLcdControlValue.RegField.msb_first=BIT7_FIRST; //config which bit of a byte will be transmitted first
    lcd_hw_if_write(LCD_HW_IF_CONTROL,tLcdControlValue.regval_32bit);//write the config value

    //mask all interrupt of LCD interface,default value after reset is mask all int
    tLcdIntMaskValue.regval_32bit=0;//clear all bit;
    tLcdIntMaskValue.RegField.lcd_fifo_empty_mask=INT_MASK;
    tLcdIntMaskValue.RegField.lcd_fifo_half_empty_mask=INT_MASK;
    tLcdIntMaskValue.RegField.lcd_fifo_overrun_mask=INT_MASK;
    tLcdIntMaskValue.RegField.lcd_read_valid_mask=INT_MASK;
    lcd_hw_if_write(LCD_HW_IF_INT_MASK,tLcdIntMaskValue.regval_32bit);
    printk("\n lcd if of melody init done.");
}

static void InitLCDController(void)
{
	unsigned char i, result;
	switch(curr_lcd_type)
      {
          case     UC1601_LCD:// used in WACS3k/5k
                    write_batch_data_to_lcd_ctrl(atLcdCtrl_InitData_UC1601);
                    break;
          case     S1D15E06_LCD:// used in WAC7K
                    write_batch_data_to_lcd_ctrl(atLcdCtrl_InitData_S1D15E06);
                    break;
          case     S6B0741_LCD:// used in WAS7K
                    write_batch_data_to_lcd_ctrl(atLcdCtrl_InitData_S6B0741);
                    break;
          case     HX8346A_LCD:
       			//result=read_hx8346a_reg(0x67); //0x47=8347, 0x46=8346
       			if (curr_tft_type==0x46) //hx8346a?
			{
	 			curr_tft_type=0x46; //0x46=HX8346, 0x47=HX8347 --keith
				printk("\nHX8346A is detected..");
	                	write_batch_data_to_lcd_ctrl(atLcdCtrl_InitData_hx8346A);
	                	break;
			}
			
			if (curr_tft_type==0x47) //hx8347a?
			{
	 			curr_tft_type=0x47; //0x46=HX8346, 0x47=HX8347 --keith
				printk("\nHX8347A is detected\n");
	                	write_batch_data_to_lcd_ctrl(atLcdCtrl_InitData_hx8347A);
	                	break;
	        	}
	
                    //break;
          default:
                    printk("\n current board type not known!");
                    break;
      }

   SetLcdDisplayContrast(curr_lcd_contrast);
}

static void Fill_LCDRamBufArray_With_Byte(unsigned char byData)
{
   memset (pLcdVideoBuf, byData, LCD_RAMBUF_LEN);
   call_lcd_buf_transfer();
}

/*********************************kernel thread related functions ******************************/
static void kthread_launcher(void *data)
{
        kthread_t *kthread = data;
        kernel_thread((int (*)(void *))kthread->function, (void *)kthread, 0);
}

static void start_kthread(void (*func)(kthread_t *), kthread_t *kthread)/* create a new kernel thread. Called by the creator. */
{
        init_MUTEX_LOCKED(&kthread->startstop_sem);
        kthread->function=func; //store the function to be executed in the data passed to the launcher
        /* initialize the task queue structure ,create the new thread my running a task through keventd */
        kthread->tq.sync = 0;
        INIT_LIST_HEAD(&kthread->tq.list);
        kthread->tq.routine =  kthread_launcher;
        kthread->tq.data = kthread;
        schedule_task(&kthread->tq); // schedule it for execution
        down(&kthread->startstop_sem); // wait till it has reached the setup_thread routine
}

static void stop_kthread(kthread_t *kthread) // stop a kernel thread. Called by the removing instance
{
        if (kthread->thread == NULL)
           {
              printk("stop_kthread: killing non existing thread!\n");
              return;
           }
        lock_kernel(); // be protected with the big kernel lock
        init_MUTEX_LOCKED(&kthread->startstop_sem);
        mb(); // do a memory barrier
        kthread->terminate = 1; // set flag to request thread termination
        mb();// do a memory barrier
        kill_proc(kthread->thread->pid, SIGKILL, 1);
        down(&kthread->startstop_sem); // block till thread terminated
        unlock_kernel(); // release the big kernel lock
        kill_proc(2, SIGCHLD, 1); // notify keventd to clean the process up
}

static void init_kthread(kthread_t *kthread, char *name) // initialize new created thread. Called by the new thread
{
        lock_kernel();
        kthread->thread = current; // fill in thread structure
        siginitsetinv(&current->blocked, sigmask(SIGKILL)|sigmask(SIGINT)|sigmask(SIGTERM)); // set signal mask to what we want to respond
        init_waitqueue_head(&kthread->queue);//initialise wait queue
        kthread->terminate = 0; // initialise termination flag
        sprintf(current->comm, name); // set name of this process (max 15 chars + 0 !)
        unlock_kernel();
        up(&kthread->startstop_sem); // tell the creator that we are ready and let him continue
}

static void exit_kthread(kthread_t *kthread) // cleanup of thread. Called by the exiting thread
{
        lock_kernel(); // lock the kernel, the exit will unlock it
        kthread->thread = NULL;
        mb();
        up(&kthread->startstop_sem); // notify the stop_kthread() routine that we are terminating
}

static void LCDBufTransThread(kthread_t *kthread) //thread used to transfer lcd buf
{
    siginfo_t info;
    unsigned long signr;

    init_kthread(kthread, "lcd_buf_tx");
    printk("\nStarting LCD buf transfer thread... ");
    for(;;) // an endless loop in which we are doing our work
      {
        mb();//do a memory barrier
        if (kthread->terminate) // we received a request to terminate ourself
           break;

        down_interruptible(&LcdRamBufTrans_sem);
        if(signal_pending(current)) //check if some signal are pending for process
          {
             spin_lock_irq(&current->sigmask_lock);
             signr = dequeue_signal(&current->blocked, &info);
             spin_unlock_irq(&current->sigmask_lock);
             if((signr==SIGKILL)||(signr==SIGTERM))
                {
                    printk("\n lcd buf thread  killed ");
                    break;
                }
          }

        #if 1 // temp mask for debugging color lcd driver of WAC7K5
        lock_lcdctrl_access();

	if ( curr_lcd_type != HX8346A_LCD)
	{
		if(need_resend_lcd_cfg) //check if need resend lcd cfg data now
		{
			need_resend_lcd_cfg=0;
			//check if need reset lcd controller,will do reset every 900 seconds
			if ( resend_lcd_cfg_count == 900) // update display after hw reset
			{
				resend_lcd_cfg_count=0;
				set_lcd_display_off();
				reset_and_init_lcd();  //reset lcd controller and init lcd interface of melody
			}
			else
			{
				// re setting will cause flash,so not re send it for HX8346a
				if ( curr_lcd_type != HX8346A_LCD)
				InitLCDController(); // re send lcd config data
				goto lcd_next_display_round; // do not update display in this case
			}
		} // if(need_resend_lcd_cfg)
	}

        prepare_for_lcdbuf_sdma();
        sdma_setup_channel (sdma_channel_for_lcdbuf,&lcd_sdma_table.config_list[0],(void *)LcdSdmaChannelIntHandler);
	if ( curr_lcd_type == HX8346A_LCD)
	{
		Com_Out( 0x22 );
//		printk("start DMA transfer\n");
	}
        sdma_config_irq(sdma_channel_for_lcdbuf,1);//enable channel finished irq
        sdma_start_channel(sdma_channel_for_lcdbuf);//start sdma transfer

        //it is not normal if not woke up after 1s,maybe the sdma int not occured as
        //expected,to avoid block,we use ...timeout
        interruptible_sleep_on_timeout(&lcd_sdma_tx_wq,HZ);
        if( lcdbuf_sdma_int_count != lcd_sdma_table.total_items )
           printk("\n some thing wrong in sdma transfer for lcd buf?");//give a tip

        if(lcd_hw_just_init_flag)
            {
               // enable display just after hw init and a valid frame was transferred to avoid display flash in init step
               mdelay(10);
		if (curr_lcd_type!=HX8346A_LCD)
		{
			set_lcd_display_on();
		}
		lcd_hw_just_init_flag=0;

            }

        //set cs high after a frame transmitted
        lcd_next_display_round:
        lcdif_poll_for_fifo_space(LCD_FIFO_LEN);
        if ( curr_lcd_type != HX8346A_LCD)
            gpio_set_high(LCD_CS);

        release_lcdctrl_access();
        #endif

        lcd_buf_send_finish_flag=1;
      } // end of for(...
     /* here we go only in case of termination of the thread */
     exit_kthread(kthread); /* cleanup the thread, leave */
     printk("\n LCDBufTransThread killed ok.");
}

/************************** below is lcd driver file operations functions **************************/
static ssize_t lcddev_read(
    struct file *file,
    char *buffer, 		/* The buffer to fill with the data */
    size_t length,     	/* The length of the buffer */
    loff_t *offset) 	/* offset to the file */
{

	loff_t r_offset = *offset;
	size_t readlength = length;
	if ((r_offset>LCD_RAMBUF_LEN)||(r_offset<0))
	{
		printk("\n offset parameter error!");
		return -1;
	}
	if((r_offset+length)>LCD_RAMBUF_LEN)
	{
		printk("\n read length is too large,will only read LCD_RAMBUF_LEN bytes!");
		readlength = LCD_RAMBUF_LEN - r_offset;
	}
	copy_to_user( (char *)buffer, (char *)(pLcdVideoBuf+r_offset), readlength );
	*offset = *offset+readlength;
	return readlength;
}

static ssize_t lcddev_write(struct file *file, const char *buffer, size_t length, loff_t *offset)
{
	loff_t w_offset = *offset;
	size_t writelength = length;
	if ((w_offset>LCD_RAMBUF_LEN)||(w_offset<0))
	{
		printk("\n offset parameter error!");
		return -1;
	}
	if((w_offset+length)>LCD_RAMBUF_LEN)
	{
		printk("\n write length is too large,will only write LCD_RAMBUF_LEN bytes!");
		writelength = LCD_RAMBUF_LEN - w_offset;
	}

	copy_from_user( (char *)(pLcdVideoBuf+w_offset), (char *)buffer, writelength );
	call_lcd_buf_transfer();
	*offset = *offset+writelength;
   return writelength;
}

static int lcddev_ioctl(
    struct inode *inode,
    struct file *file,
    unsigned int ioctl_num,/* The number of the ioctl */
    unsigned long ioctl_param) /* The parameter to it */
{
   int parabuf[100];
   unsigned char  cmdByte,dispByte;
   unsigned char  addr, data, data1, data2, data3;
   int i;

   switch (ioctl_num)
   {
      case IOCTRL_SEND_LCDCMD:
              copy_from_user((char *)parabuf,(char *)ioctl_param,4);
              cmdByte=parabuf[0]&0xFF;
              lock_lcdctrl_access();
              Com_Out(cmdByte);
              release_lcdctrl_access();
              break;
      case IOCTRL_SEND_LCDDATABYTE:
              copy_from_user((char *)parabuf,(char *)ioctl_param,4);
              cmdByte=parabuf[0]&0xFF;
              lock_lcdctrl_access();
              Data_Out(cmdByte);
              release_lcdctrl_access();
              break;
      case IOCTRL_CLEAR_LCD:

              Fill_LCDRamBufArray_With_Byte(0);
              break;
      case IOCTRL_UPDATE_NOW:

              call_lcd_buf_transfer();
              break;
      case IOCTRL_GET_TFT_TYPE:
              parabuf[0]=(int) curr_tft_type;//0x46=HX8346, 0x47=HX8347 --keith
              copy_to_user((char *)ioctl_param,(char *)parabuf,4);
              break;
      case IOCTRL_GET_LCD_WIDTH:
              parabuf[0]=LCD_WIDTH;
              copy_to_user((char *)ioctl_param,(char *)parabuf,4);
              break;
      case IOCTRL_GET_LCD_HEIGHT:
              parabuf[0]=LCD_HEIGHT;
              copy_to_user((char *)ioctl_param,(char *)parabuf,4);
              break;
      case IOCTRL_GET_LCD_BPP:
              parabuf[0]=LCD_BPP;
              copy_to_user((char *)ioctl_param,(char *)parabuf,4);
              break;
      case IOCTRL_RESET_LCD:
              lock_lcdctrl_access();
              set_lcd_display_off();
              HardwareInitForLCD();//re init the hardware of lcd interface and lcd controller
              release_lcdctrl_access();
              break;
      case IOCTRL_INC_LCD_CONTRAST:
              lock_lcdctrl_access();
              IncLcdContrast();
              release_lcdctrl_access();
              break;
      case IOCTRL_DEC_LCD_CONTRAST:
              lock_lcdctrl_access();
              DecLcdContrast();
              release_lcdctrl_access();
              break;
      case IOCTRL_SET_LCD_CONTRAST: // only for debug and test
              copy_from_user((char *)parabuf,(char *)ioctl_param,4);
              lock_lcdctrl_access();
              SetLcdDisplayContrast(parabuf[0]&0xFF);//set the contrast of lcd pannel
              release_lcdctrl_access();
              break;
      case IOCTRL_GET_LCD_SEND_FINISH_FLAG:
             parabuf[0]=lcd_buf_send_finish_flag;
             copy_to_user((char *)ioctl_param,(char *)parabuf,4);
             break;
      case IOCTRL_SET_LIGHTPIN_HIGH:
             gpio_set_as_output (GPIO_LCD_BACKLIGHT, 1);		/* wac3k5: high == on, wacs7k5: high == off */
             printk("\n now drive lcd back light pin to high level");
             break;
      case IOCTRL_SET_LIGHTPIN_LOW:
             gpio_set_as_output (GPIO_LCD_BACKLIGHT, 0);		/* wac3k5: low == off, wacs7k5: low == on */
             printk("\n now drive lcd back light pin to low level");
             break;
      case IOCTRL_GET_DISPLAY_REVERSE_FLAG:
             parabuf[0]=lcd_disp_invert_flg;
             copy_to_user((char *)ioctl_param,(char *)parabuf,4);
             break;
      case IOCTRL_DISP_DATA://for debug and test
             copy_from_user((char *)parabuf,(char *)ioctl_param,1*4);
             dispByte=parabuf[0]&0xff;
             printk("\n display data 0x%02x\n",dispByte);
             Fill_LCDRamBufArray_With_Byte(dispByte);
             break;
      case IOCTRL_SET_PPTR_FLAG:// for compatible with lcd in wacs7k pptr
             printk("\n perform additional treat for lcd used in ptr board...");
             pptr_board_flag=1;
             lock_lcdctrl_access();
             Additional_Treat_For_LcdCtrl();
             release_lcdctrl_access();
             printk("done");
             break;
      case IOCTRL_DISPLAY_ENABLE:
//             set_lcd_display_on();
             break;
      case IOCTRL_DISPLAY_DISABLE:
//             set_lcd_display_off();
             break;

      /**************** below is for debug and test of color lcd HX8346A **********************/
      case IOCTRL_WRITE_LCDCTRL_REG:
             copy_from_user((char *)parabuf,(char *)ioctl_param,2*4);
             addr = parabuf[0]&0xff;
             data = parabuf[1]&0xff;
             write_hx8346a_reg(addr,data);
             printk("\n write reg[0x%02x] with 0x%02x \n", addr, data);
             break;
      case IOCTRL_READ_LCDCTRL_REG:
             copy_from_user((char *)parabuf,(char *)ioctl_param,1*4);
             addr = parabuf[0]&0xff;
             parabuf[0] = read_hx8346a_reg(addr);
             copy_to_user((char *)ioctl_param,(char *)parabuf,4);
             break;
      case IOCTRL_WRITE_LCDCTRL_DATA:
             copy_from_user((char *)parabuf,(char *)ioctl_param,3*4);
             data1 = parabuf[0]&0xff;
             data2 = parabuf[1]&0xff;
             data3 = parabuf[2]&0xff;
             printk("\n write data 0x%02x,0x%02x,0x%02x \n", data1, data2, data3);
             for (i = 0; i < LCD_WIDTH*LCD_HEIGHT*3; i += 3)
             {
                 *(unsigned char *)(pLcdVideoBuf + i + 0) = data1;
                 *(unsigned char *)(pLcdVideoBuf + i + 1) = data2;
                 *(unsigned char *)(pLcdVideoBuf + i + 2) = data3;
             }
             call_lcd_buf_transfer();
             break;
      default:
             break;
   }

  return 0;
};

static int lcddev_mmap(struct file * file, struct vm_area_struct * vma)
{
    unsigned int offset;
    unsigned int len,physicalstart;

    offset = vma->vm_pgoff << PAGE_SHIFT;
    len = PAGE_ALIGN(LCD_RAMBUF_LEN);
    if ((vma->vm_end - vma->vm_start + offset) > len)
       {
          printk("\n vma range	too large!");
          return -EINVAL;
       }

    vma->vm_flags |= VM_LOCKED |VM_RESERVED;
    physicalstart = __pa(pLcdVideoBuf + offset);// virtual address to physical address
    if (remap_page_range(vma->vm_start, physicalstart, vma->vm_end-vma->vm_start,PAGE_SHARED))
       {
           printk("\n remap_page_range error! ");
           return -EAGAIN;
       }

    return 0;
}

static int __init lcd_module_init( void ) //Initialize the module - Register the character device
{
    int ret_val;

    printk("\n Start init LCD module...");
    curr_lcd_type= get_curr_lcd_type();//get lcd type
    if(curr_lcd_type==0xFF)
      {
           printk("\n Can not identify the type of board,lcd init aborted!");
           return 1;
      }

    init_lcddrv_para();
    ret_val=AllocateMemForLcdModule();//allocate memory needed for lcd module
    if(ret_val)
      {
          printk("\nFATAL ERROR !  failed to allocate memory for lcd module \n" );
          return 2;
      }

    sdma_channel_for_lcdbuf=sdma_allocate_channel();
    printk("\n allocated sdma channel for lcd is %d",sdma_channel_for_lcdbuf);
    if ( sdma_channel_for_lcdbuf == (-1) )
      {
          printk("\n FATAL ERROR ! lcddrv failed to request sdma channel !");
          return 3;
      }

    init_lcd_buf_sdma_tx_table();

    init_MUTEX(&AccessLcdController_sem); //for access lcd controller
    init_MUTEX_LOCKED(&LcdRamBufTrans_sem); // for transfer LCD ram buf

    start_kthread(LCDBufTransThread,&tLCDModuleThread[0]);  /*start thread for LCD module*/

	 if ( curr_lcd_type!=HX8346A_LCD )
 	{
		config_lcdif_pins();
		lock_lcdctrl_access();
		ret_val=HardwareInitForLCD();
		if(ret_val)
		{
			printk("\nFATAL ERROR ! failed init hardware for LCD \n" );
			return 4;
		}

		release_lcdctrl_access();
 	}
	else
	{


		config_lcdif_pins();
		lock_lcdctrl_access();
             gpio_set_as_output (GPIO_LCD_BACKLIGHT, 1);		/* wac3k5: high == on, wacs7k5: high == off */
		ret_val=HardwareInitForLCD();
		if(ret_val)
		{
			printk("\nFATAL ERROR ! failed init hardware for LCD \n" );
			return 4;
		}
		release_lcdctrl_access();

//temporarily disable the full init sequence of lcd controller --keith

//		set_lcd_display_setting();
	}
	
    display_startup_logo(); //display startup logo

    ret_val = register_chrdev(MAJOR_NUM, DEVICE_NAME, &lcd_fops);  //register lcd dev
    if (ret_val < 0)
      {
         printk ("%s failed with %d\n","Sorry, registering the character device ",ret_val);
         return ret_val;
      }

//    init_lcd_cfg_resend_timer(); remove the init the lcd resend lcd cfg timer
    show_lcd_type_ver_info();
    return 0;
}

static void __exit lcd_module_cleanup(void) //Cleanup - unregister the LCD device module
{
    int ret;
    int i;

    ret = unregister_chrdev(MAJOR_NUM, DEVICE_NAME);/* Unregister the device */
    if (ret < 0)
      printk("Error in module_unregister_chrdev: %d\n", ret);

    del_resend_lcd_cfg_timer(); //del the lcd resend cfg timer
    /* terminate the kernel threads created by LCD module */
    for (i=0; i<NTHREADS; i++)
        stop_kthread(&tLCDModuleThread[i]);

    free_pages((int)pLcdVideoBuf, get_order(LCD_RAMBUF_LEN));

    if( sdma_channel_for_lcdbuf != (-1) )
       sdma_free_channel(sdma_channel_for_lcdbuf); // free the sdma channel
    printk("\n lcd drv module removed.\n");
}

module_init(lcd_module_init);
module_exit(lcd_module_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wxl");
MODULE_DESCRIPTION("LCD driver(all in one) in melody");

