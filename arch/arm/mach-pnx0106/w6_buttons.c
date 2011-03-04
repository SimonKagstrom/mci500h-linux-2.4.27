/*
 * buttons_dev.c - Unified input device driver for Philips PNX0106
 *
 * Copyright (C) 2005-2006 PDCC, Philips CE.
 *
 * Revision History :
 * ----------------
 *
 *   Version             DATE          NAME
 *   Initial version     01-Dec-2005   Alex Xu       initial version
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/config.h>
#include <linux/fcntl.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <asm/fiq.h>
#include <asm/hardware.h>
#include <asm/uaccess.h>        // for copy_from_user, etc.
#include <asm/arch/adc.h>
#include <asm/arch/tsc.h>	//for ReadTSC();
#include <asm/arch/w6_buttons.h>

#define KE_EVENT_KEY    1
#define KE_EVENT_WAVE   2
#define WAVE_TIMING_SIZE 64
//============Interrupt related definition====================
#define VA_INTC_REQUEST(irq_number)             (IO_ADDRESS_INTC_BASE + 0x400 + (4 * (irq_number)))

#define INTC_REQUEST_MASTER_ENABLE              ((1 << 16) | (1 << 26))
#define INTC_REQUEST_MASTER_DISABLE             ((0 << 16) | (1 << 26))
#define INTC_REQUEST_TARGET_IRQ                 ((0 <<  8) | (1 << 27))
#define INTC_REQUEST_TARGET_FIQ                 ((1 <<  8) | (1 << 27))

#define RC6_1T_STANDARD		5333		/* 444.444us */
#define RC6_1T_MIN			2989		/* 249.111us */
#define RC6_1T_MAX			7895		/* 658.000us */
#define RC6_2T_MIN			8055		/* 671.333us */
#define RC6_2T_MAX			13495		/* 1124.667us */
#define RC6_3T_MIN			13498		/* 1125.000us */
#define RC6_3T_MAX			18875		/* 1573.111us */
#define RC6_6T_MIN			26142		/* 2178.667us */
#define RC6_6T_MAX			40316		/* 3360.000us */
#define RC6_FREE_MIN		40316		/* 3360.000us */
#define RC6_SPIKE_MAX		2400		/* 200.000us  */

#define RC6_1T 1
#define RC6_2T 2
#define RC6_3T 3
#define RC6_6T 6
#define RC6_SFT 10
#define RC6_ERROR 11

#define ADC_CHANNEL_0 0
#define ADC_CHANNEL_1 1
#define ADC_CHANNEL_2 2
#define ADC_CHANNEL_3 3
#define ADC_CHANNEL_4 4

#define ADC_FILTER_SAMPLES  1		// Alex change to 1

//global variable definition
unsigned long long g_wave_captured = 0;
unsigned  int a_wave_timing[WAVE_TIMING_SIZE];
unsigned  int a_pc_table[WAVE_TIMING_SIZE];	//alex: for test purpose, the fiq will save PC value here!!
unsigned int wIndex=0;	//write index for a_wave_timing
unsigned short rIndex=0;	//read index for a_wave_timing
int rc6_needs_to_send_tmout=0;	//a flag to indicate whether a timeout is needed
unsigned int button_flags=0;
struct timer_list key_Timer;
int gDebugFlag=0;	//0--disable debug output
static int bit=0;
static int T_cnt=0;	//count T
extern unsigned long long g_wave_captured ;
struct timer_list rc6_Timer;


int  key_test_mode = 0;            //0 for normal mode,1 for test mode
int  key_press_time = 0;


static unsigned long long buttons_wave_captured;
static unsigned int buttons_key_scanned;
//function declaration
enum key_t_ key_t_GetPhysicalKeyScanStatus(void);
int test_rc6_input(unsigned long long rc6_wave);
int test_key_input(unsigned int key_scan_value);
int key_set_type(char *name);

static signed char key_scanner_filter_index;	// -1:not yet started, else: index to the filter buffer which are scanned and filled
static unsigned short key_scanner_filters[3];	// filter buffer for all of the key lines
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#define KEY_SCANNER_DEBOUNCING  4		// 3 samples to be used to detect bouncing
static unsigned char key_scanner_sample_index;	// sample buffer index
static unsigned char key_scanner_samples[3][KEY_SCANNER_DEBOUNCING];
static unsigned char key_scanner_keys_captured[3];
static unsigned int keyIdleCnt=0;
extern void key_event_indication(int , void *);
char KeyConvertADValue(unsigned short ADValue);

void rc6_indication(int flag, void *data)
{
    unsigned long *ptr;
    if (flag == BUTTONS_FLAGS_WAVE) {
			ptr=(unsigned long *)data;
         if(ptr[0]+ptr[1]!=0){
         		buttons_wave_captured = *(unsigned long long*)data;

 		 				button_flags|=BUTTONS_FLAGS_WAVE;
//		 printk("rc6 wave: 0x%x--0x%x\n",ptr[1],ptr[0]);
	 			}
	 			else	//something wrong here, zero should not indicate to up layer
	 				return;
    	}
}

void key_event_indication(int flag, void *data)
{
    if(flag == KE_EVENT_KEY){
      	buttons_key_scanned=*(unsigned int *)data;
	button_flags|=BUTTONS_FLAGS_KEY;
    }

}
static inline unsigned int get_timer_value(void)
{
	return 1-ReadTSC();	//please see the implemenation of ReadTSC, then you can understand why 1- is needed
}

int check_timing(unsigned int pulse_time)
{
	int retval;

	/*--- compare pulse width ---*/
	if( (RC6_1T_MIN <= pulse_time) && (pulse_time <= RC6_1T_MAX) )
	{
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		T_cnt++;
		retval=RC6_1T;
	}
	else if( (RC6_2T_MIN <= pulse_time) && (pulse_time <= RC6_2T_MAX) )
	{
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		T_cnt+=2;

		retval=RC6_2T;

	}
	else if( (RC6_3T_MIN <= pulse_time) && (pulse_time <= RC6_3T_MAX) )
	{
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;

		T_cnt+=3;
		retval=RC6_3T;
	}
	else if( (RC6_6T_MIN <= pulse_time) && (pulse_time <= RC6_6T_MAX) )
	{
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;

		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		T_cnt+=6;
		retval=RC6_6T;

	}
	else if( pulse_time <= RC6_SPIKE_MAX )		//how to deal with spike??
	{
	//toggle bit
	bit^=0x01;
		retval=RC6_ERROR;
	}
	else
	{
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;

		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		g_wave_captured<<=1;
		g_wave_captured|=bit;
		if(T_cnt==51){
			g_wave_captured<<=1;
			g_wave_captured|=bit;
			}
		/* Error pulse width */
		retval=RC6_SFT;
		rc6_indication(BUTTONS_FLAGS_WAVE,&g_wave_captured);
		g_wave_captured=0;
		bit=0;	//reset bit
		T_cnt=0;

	}
	//toggle bit
	bit^=0x01;
	return retval;
}

void RC6_TimerRtn(unsigned long unused)
{
	unsigned int wIdx=wIndex;	//save the wIndex, which is updated by FIQ handler, should check whether this is an atomic operation!!
	int i;
	static unsigned int pre=0;	//check please!!!, potential something wrong here!!!

	//read the timing info from the wave timing buffer
	wIdx/=4;
	if(rIndex<wIdx){
		for(i=rIndex;i<wIdx;i++){
			check_timing(pre-a_wave_timing[i]);
			pre=a_wave_timing[i];
			}
		}
	else if(rIndex>wIdx){
		for(i=rIndex;i<WAVE_TIMING_SIZE;i++){
			check_timing(pre-a_wave_timing[i]);
			pre=a_wave_timing[i];

			}
		for(i=0;i<wIdx;i++){
			check_timing(pre-a_wave_timing[i]);
			pre=a_wave_timing[i];

			}

		}
	else{	//rIndex==wIdx, the buffer is empty!!!!
		if(g_wave_captured==0)
		{
			if(rc6_needs_to_send_tmout){
				rc6_needs_to_send_tmout--;
				if(rc6_needs_to_send_tmout==0){
					//Timeout should be sent!
					g_wave_captured=0x01;
					rc6_indication(BUTTONS_FLAGS_WAVE,&g_wave_captured);
					g_wave_captured=0;
					bit=0;	//reset bit
					T_cnt=0;
					}
				}

		}
		else
		{
			pre=get_timer_value();
			g_wave_captured<<=1;
			g_wave_captured|=bit;
			g_wave_captured<<=1;
			g_wave_captured|=bit;
			g_wave_captured<<=1;
			g_wave_captured|=bit;

			g_wave_captured<<=1;
			g_wave_captured|=bit;
			g_wave_captured<<=1;
			g_wave_captured|=bit;
			g_wave_captured<<=1;
			g_wave_captured|=bit;

			if(T_cnt==51){
				g_wave_captured<<=1;
				g_wave_captured|=bit;
				}

			rc6_indication(BUTTONS_FLAGS_WAVE,&g_wave_captured);
			g_wave_captured=0;
			bit=0;	//reset bit
			T_cnt=0;
			rc6_needs_to_send_tmout=18;
		}
	}
	rIndex=wIdx;

	//Alex: I don't know why sometimes the Enable bit is cleared? it's just a workaround here
	*(volatile unsigned long*)VA_INTC_REQUEST(IRQ_RC6RX)=INTC_REQUEST_TARGET_FIQ;
	*(volatile unsigned long*)VA_INTC_REQUEST(IRQ_RC6RX)=INTC_REQUEST_MASTER_ENABLE;
	rc6_Timer.expires = 0;//jiffies +HZ/1000;
      add_timer(&rc6_Timer);
}

int test_rc6_input(unsigned long long rc6_wave)
{
	g_wave_captured=0;
	rc6_indication(BUTTONS_FLAGS_WAVE,&rc6_wave);
	bit=0;	//reset bit
	T_cnt=0;
	rc6_needs_to_send_tmout=18;
	return 0;
}

//rc6-related initialization
int rc6_init(void)
{
	struct pt_regs regs;
	int length;

	er_add_gpio_input_event(IRQ_RC6RX, GPIO_RC6RX,EVENT_TYPE_FALLING_EDGE_TRIGGERED);

	memset (&regs, 0, sizeof(regs));	/* initialise all regsiters to zero */

	set_fiq_regs(&regs);
	length=(unsigned char *)&p_wave_capturer_end-(unsigned char *)wave_capturer;
	if(length>=0x200-0x1c){
		printk("fiq handler size too large, abort copy\n");
	}

	set_fiq_handler(wave_capturer, length);
	//start a timer for rc6
  init_timer(&rc6_Timer);
  rc6_Timer.function=RC6_TimerRtn;
  rc6_Timer.data=0;
  rc6_Timer.expires = 0;//jiffies +HZ/1000;
  add_timer(&rc6_Timer);

	*(volatile unsigned long*)VA_INTC_REQUEST(IRQ_RC6RX)=INTC_REQUEST_TARGET_FIQ;
	*(volatile unsigned long*)VA_INTC_REQUEST(IRQ_RC6RX)=INTC_REQUEST_MASTER_ENABLE;
	 return 0;
}

int rc6_exit(void)
{
	del_timer(&rc6_Timer);
	*(volatile unsigned long*)VA_INTC_REQUEST(IRQ_RC6RX)=INTC_REQUEST_MASTER_DISABLE;

	return 0;
}

#if defined (CONFIG_PNX0106_W6) || \
	defined (CONFIG_PNX0106_MCIH) || \
	defined (CONFIG_PNX0106_MCI) || \
    defined (CONFIG_PNX0106_HASLI7) || \
    defined (CONFIG_PNX0106_HACLI7)		/* fixme... hasli7 should be a separate table !! */

unsigned char wac5000_key_mapping_table[2][14]=	//key matrix for WAC5000
{
    {
    	Virtual_Key_Idle,	// 0
    	physical_key_EJECT,
    	physical_key_Music_follows,
    	physical_key_Music_broadcast,
    	physical_key_Left_Reverse,
    	physical_key_Down_Next,	//5
    	physical_KEY_PLAY,
    	physical_key_Up_Previous,	//HSI error!!!
    	physical_key_Right_Forward,	//HSI error!!!
    	physical_KEY_STOP,
    	physical_key_REC,	//10
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13
    },

    {
	Virtual_Key_Idle,	// 0
	physical_KEY_POWER_STANDBY,
	physical_key_Menu,
	physical_KEY_SOURCE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,	//5
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,	//10
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13

    }
};
unsigned char was5000_key_mapping_table[2][14]=	//key matrix for WAS5000
{
    {
	Virtual_Key_Idle,	// 0
	physical_KEY_POWER_STANDBY,
	physical_key_Menu,
	physical_key_Music_follows,
    	physical_KEY_STOP,
    	physical_key_ERROR_ADVALUE,	//5
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,	//10
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13
    },

    {
	Virtual_Key_Idle,	// 0
	physical_key_Right_Forward,
	physical_key_Down_Next,
	physical_key_Left_Reverse,
    	physical_KEY_PLAY,
    	physical_key_Up_Previous,	//5
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,	//10
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13

    }
};

unsigned char wac7000_key_mapping_table[2][14]=	//key matrix for WAS5000
{
    {
	Virtual_Key_Idle,	// 0
	physical_KEY_POWER_STANDBY,
	physical_KEY_IS,
	physical_KEY_MUTE,
    	physical_key_Left_Reverse,
    	physical_key_Down_Next,	//5
    	physical_key_Up_Previous,
    	physical_key_Right_Forward,
    	physical_KEY_PLAY,
    	physical_KEY_STOP,
    	physical_key_Mark,	//10
    	physical_KEY_VOLUME_DOWN,
    	physical_KEY_VOLUME_UP,
    	physical_key_ERROR_ADVALUE	//13
    },

    {
	Virtual_Key_Idle,	// 0
	physical_key_REC,
	physical_KEY_SOURCE,
	physical_key_Menu,
    	physical_key_Match_Genre,	//smart equalizer--same as match genre in 700
    	physical_key_Like_Genre,	//same Genre--same as like genre in 700
    	physical_key_Like_Artist,	//same Artist--same as like artist in 700
    	physical_key_View,
    	physical_KEY_DBB,
    	physical_key_Music_broadcast,
    	physical_key_Music_follows,	//10
    	physical_key_EJECT,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13

    }
};
unsigned char was7000_key_mapping_table[2][14]=	//key matrix for WAS5000
{
    {
	Virtual_Key_Idle,	// 0
	physical_key_Music_follows,
	physical_KEY_DBB,
	physical_KEY_IS,
    	physical_key_View,
    	physical_KEY_VOLUME_UP,	//5
    	physical_KEY_VOLUME_DOWN,
    	physical_KEY_STOP,
    	physical_key_Right_Forward,
    	physical_key_Up_Previous,
    	physical_key_ERROR_ADVALUE,	//10
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13
    },

    {
	Virtual_Key_Idle,	// 0
	physical_KEY_POWER_STANDBY,
	physical_KEY_SOURCE,
	physical_KEY_MUTE,
    	physical_key_Down_Next,
    	physical_KEY_PLAY,	//5
    	physical_key_Left_Reverse,
    	physical_key_Menu,
    	physical_key_Match_Genre,	//smart equalizer
    	physical_key_Like_Genre,	//same Genre
    	physical_key_Like_Artist,	//Same Artist
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE,
    	physical_key_ERROR_ADVALUE	//13

    }
};

unsigned char key_mapping_table [2][14];

int UI1_Init(void)
{
    key_scanner_filter_index=-1;					// -1:not yet started
    key_scanner_sample_index=0;	// sample buffer index

   //initialize every key to idle key
    memset(key_mapping_table,Virtual_Key_Idle,sizeof(key_mapping_table));

    key_scanner_keys_captured[0]=KeyConvertADValue(adc_get_value(ADC_CHANNEL_0));
    key_scanner_keys_captured[1]=KeyConvertADValue(adc_get_value(ADC_CHANNEL_1));
    return 0;
}

int key_set_type(char *name)
{
	//it's better to disable key scan timer first, then again enable key scan timer
	//anyway, this should be ok, this function is supposed to be called only once
	printk("key driver: set key pad to %s\n",name);
	if(strcmp(name,"wac5000")==0){
		memcpy(key_mapping_table,wac5000_key_mapping_table,sizeof(key_mapping_table));
		return 0;
		}
	if(strcmp(name,"was5000")==0){
		memcpy(key_mapping_table,was5000_key_mapping_table,sizeof(key_mapping_table));
		return 0;
		}
	if(strcmp(name,"wac7000")==0){
		memcpy(key_mapping_table,wac7000_key_mapping_table,sizeof(key_mapping_table));
		return 0;
		}
	if(strcmp(name,"was7000")==0){
		memcpy(key_mapping_table,was7000_key_mapping_table,sizeof(key_mapping_table));
		return 0;
		}
	printk("key_set_type: unknown keypad name\n");
	return 1;	//error

}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
enum key_t_ key_t_GetPhysicalKeyScanStatus(void)
{
	enum key_t_ keyArray[2];
	enum key_t_ retKey=Virtual_Key_Idle;

	keyArray[0]=key_scanner_keys_captured[0];
	keyArray[1]=key_scanner_keys_captured[1];

	keyArray[0] = key_mapping_table[0][keyArray[0]];// key_1
	keyArray[1] = key_mapping_table[1][keyArray[1]];// key_2


	if(Virtual_Key_Idle == keyArray[0] && Virtual_Key_Idle == keyArray[1] )
		retKey=Virtual_Key_Idle;

	if(Virtual_Key_Idle != keyArray[0] && Virtual_Key_Idle == keyArray[1])
		retKey= keyArray[0];
	if(Virtual_Key_Idle == keyArray[0] && Virtual_Key_Idle != keyArray[1])
		retKey= keyArray[1];

	return retKey;
}
//Alex: an interface for automatic test!!
int test_key_input(unsigned int key_scan_value)
{
       key_press_time = (key_scan_value >> 16) & 0x7fff;     //get the press time.
       key_press_time = abs(key_press_time/20);
       key_test_mode = (key_scan_value >> 31) & 0xff;                       //get test mode value

	if (!key_test_mode)
       	
		  return 1;
       	
	   
	enum key_t_ key;

	key_scanner_keys_captured[0]=key_scan_value&0xff;
	key_scanner_keys_captured[1]=(key_scan_value>>8)&0xff;

	key=key_t_GetPhysicalKeyScanStatus();
	if(key==Virtual_Key_Idle){	//something wrong here
		printk("key pressed, but get Virtual_Key_Idle, perhaps key map table is not loaded\n");
		}
	else if(key==physical_key_ERROR_ADVALUE){
		printk("key:wrong input value, can not find a valid key, check your test code please\n");
		}
	else{
		//encode key_scanner_keys_captured info in the return value will increase flexibility
		key=(key_scanner_keys_captured[0]<<16)|(key_scanner_keys_captured[1]<<8)|key;
		key_event_indication(KE_EVENT_KEY,(void*)&key);
		keyIdleCnt=0;		//reset key idle count
		printk("test key_scanner_keys_captured(0,1)=%d:%d\n",key_scanner_keys_captured[0],\
			key_scanner_keys_captured[1]);
		}
       
 	   
	return 0;
}


void key_testmode_timer(void)
{
  key_press_time--;
  if ( key_press_time == 0)
  	 
	     key_test_mode = 0;                            //get out of test mode
  	 
}


void key_scanner_timer_callback(void)
{
	unsigned int keyScanned;
	enum key_t_ key;

	if (key_scanner_filter_index==0)
	{
		key_scanner_filters[0]=adc_get_value(ADC_CHANNEL_0);    		// line 1
		key_scanner_filters[1]=adc_get_value(ADC_CHANNEL_1);    		// line 2
	}
	else
	{
		key_scanner_filters[0]+=adc_get_value(ADC_CHANNEL_0);    		// line 1
		key_scanner_filters[1]+=adc_get_value(ADC_CHANNEL_1);    		// line 2
	}
	key_scanner_filter_index++;

	if (key_scanner_filter_index>=ADC_FILTER_SAMPLES)	// all filter buffer filled ?
	{
		key_scanner_filter_index=0;			// reset the filter buffer index
		/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
		// average the samples and convert to key index
		 key_scanner_samples[0][key_scanner_sample_index]=KeyConvertADValue(key_scanner_filters[0]/ADC_FILTER_SAMPLES);
		key_scanner_samples[1][key_scanner_sample_index]=KeyConvertADValue(key_scanner_filters[1]/ADC_FILTER_SAMPLES);
		// advance to next debouncing buffer
		key_scanner_sample_index++;
		if (key_scanner_sample_index>=KEY_SCANNER_DEBOUNCING)
		{
			int i;
			for (i=1;i<KEY_SCANNER_DEBOUNCING;i++)	// check if all samples are the same (smooth?)
			{
				if (key_scanner_samples[0][0] != key_scanner_samples[0][i]) break;
				if (key_scanner_samples[1][0] != key_scanner_samples[1][i]) break;
			}
			if (i==KEY_SCANNER_DEBOUNCING)	// All the A/D values are the same, a valid key
			{
				key_scanner_keys_captured[0]=key_scanner_samples[0][0];
				key_scanner_keys_captured[1]=key_scanner_samples[1][0];
				//Alex: Here got the scan code, it's time to notify the user to read the key
				keyScanned=(key_scanner_keys_captured[1]<<8)|key_scanner_keys_captured[0];
				if(keyScanned!=0)
				{
					key=key_t_GetPhysicalKeyScanStatus();
					if(key==Virtual_Key_Idle){	//something wrong here
						printk("key pressed, but get Virtual_Key_Idle, perhaps key map table is not loaded\n");
						}
					else if(key==physical_key_ERROR_ADVALUE){
						printk("key:wrong A/D value, can not find a valid key, check hardware please %d--%d\n",key_scanner_filters[0]/ADC_FILTER_SAMPLES,key_scanner_filters[1]/ADC_FILTER_SAMPLES);
						}
					else{
						key=(key_scanner_keys_captured[0]<<16)|(key_scanner_keys_captured[1]<<8)|key;
						key_event_indication(KE_EVENT_KEY,(void*)&key);
						keyIdleCnt=0;		//reset key idle count
						if(gDebugFlag){
							printk("key_scanner_keys_captured(0,1)=%d:%d\n",key_scanner_keys_captured[0],\
								key_scanner_keys_captured[1]);
							}
						}
				}
				else{	//if the idle key!!!
					keyIdleCnt++;
					keyIdleCnt%=50;	//indicate to user-space once every 50 idle keys
					if (keyIdleCnt==1){
						key=(key_scanner_keys_captured[0]<<16)|(key_scanner_keys_captured[1]<<8)|key;
						key=key_t_GetPhysicalKeyScanStatus();
						key_event_indication(KE_EVENT_KEY,(void*)&key);
						}
      				}
			}
			key_scanner_sample_index=0;
		}
	}
}
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
//Alex, modified to According to HSIv0.3
char KeyConvertADValue(unsigned short ADValue)
{
	if( ADValue > 933 )		// most of the cases--the program is optimiszed to put the statement here
	{
		return 0;
	}
	else if(ADValue  <= 32)
	{
		return 1;
	}
	else if( (46 <=ADValue) && (ADValue  <= 60))	//Thomas said will change to 46 at HSIv0.4
	{
		return 2;
	}
	else if( (112 <= ADValue) && (ADValue <= 135) )
	{
		return 3;
	}
	else if( (180 <= ADValue) && (ADValue <= 213) )
	{
		return 4;
	}
	else if( (262 <= ADValue) && (ADValue <= 304) )
	{
		return 5;
	}
	else if( (355 <= ADValue) && (ADValue <= 404)	)
	{
		return 6;
	}
	else if( (457 <= ADValue) && (ADValue <= 509)	)
	{
		return 7;
	}
	else if( (560 <= ADValue) && (ADValue <= 611)	)
	{
		return 8;
	}
	else if( (659 <= ADValue) && (ADValue <= 706)	)
	{
		return 9;
	}
	else if( (748 <= ADValue) && (ADValue <= 787)	)
	{
		return 10;
	}
	else if( (830 <= ADValue) && (ADValue <= 860)	)
	{
		return 11;
	}
	else if( (914 <= ADValue) && (ADValue <= 933)	)
	{
		return 12;
	}

	return 13;		//unrecongiszed AD value!!!, H/W maybe wrong here
}
#else
#error "needs a platform definition to buid the key and rc driver"
#endif

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static int buttons_ioctl(struct inode *inode, struct file *file,
                         unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
	case BUTTONS_GET_WAVE:
		if(copy_to_user((unsigned*)arg, &buttons_wave_captured, sizeof(buttons_wave_captured)))
			return -EFAULT;
		break;
	case BUTTONS_SET_WAVE:
	{
		unsigned long long rc_scan_code;
		if(copy_from_user(&rc_scan_code, (void*)arg, sizeof(rc_scan_code)))
			return -EFAULT;
		test_rc6_input(rc_scan_code);
		break;
	}
	case BUTTONS_GET_KEY:
		if(copy_to_user((unsigned*)arg, &buttons_key_scanned, sizeof(buttons_key_scanned)))
			return -EFAULT;
		break;
	case BUTTONS_SET_KEY:
   	{
		unsigned int key_scanned;
	         if(copy_from_user(&key_scanned, (void*)arg, sizeof(key_scanned)))
			return -EFAULT;
		test_key_input(key_scanned);
		break;
	}
	case BUTTONS_GET_FLAGS:
	{
            if(copy_to_user((unsigned*)arg, &button_flags, sizeof(button_flags)))
			return -EFAULT;
		button_flags=0;	//reset the button flags

		break;
	}
	case BUTTONS_SET_KEYPAD_TYPE:	//set key pad type to wac5000/was5000 or wac7000/was7000
	{
		char keyname[12];
		if(copy_from_user(keyname, (void*)arg, sizeof(keyname)))
			return -EFAULT;
		printk("set key pad to %s\n",keyname);
		key_set_type(keyname);
		break;
	}
	case BUTTONS_SET_RAW_KEY_MAPTABLE:	//load a raw key mapping table, it's rather dangerous to
	{										//call this ioctl, there is no way to verify the map table
		if(copy_from_user(key_mapping_table, (void*)arg, sizeof(key_mapping_table)))
			return -EFAULT;
		printk("raw key table loaded\n");
		break;
	}
	case BUTTONS_GET_RAW_KEY_MAPTABLE:
	{
            	if(copy_to_user((unsigned*)arg, &key_mapping_table, sizeof(key_mapping_table)))
			return -EFAULT;
		break;
	}
	case BUTTONS_GET_KEY_MAPTBL_SIZE:
	{
		int key_tbl_size=sizeof(key_mapping_table);
            	if(copy_to_user((unsigned*)arg, &key_tbl_size, sizeof(key_tbl_size)))
			return -EFAULT;
		break;
	}
	case BUTTONS_SET_DEBUG_FLAG:
	{
		printk("BUTTONS_SET_DEBUG_FLAG called\n");
		if(copy_from_user(&gDebugFlag, (void*)arg, sizeof(int)))
			return -EFAULT;
		printk("set debug flag to : 0x%x\n",gDebugFlag);
	}
	default:
		printk("unknown ioflags\n");
		break;
    }
    return 0;
}

void key_TimerRtn(unsigned long unused)
{
      if (!key_test_mode)
	    key_scanner_timer_callback();
      else 
	    key_testmode_timer();	
      key_Timer.expires = 0;//jiffies +HZ/1000;
      add_timer(&key_Timer);
}
int Init_KeyTimer(void)
{
        init_timer(&key_Timer);
        key_Timer.function=key_TimerRtn;
        key_Timer.data=0;

        key_Timer.expires = 0;//jiffies +HZ/1000;
        add_timer(&key_Timer);
        return 0;
}
int Del_KeyTimer(void)
{
        del_timer(&key_Timer);
        return 0;
}

int key_init(void)
{

	UI1_Init();
	adcdrv_set_mode(1);
	Init_KeyTimer();
	return 0;
}
int key_exit(void)
{
	adcdrv_disable_adc();
	Del_KeyTimer();
	return 0;
}

static struct file_operations buttons_fops =
{
	ioctl:		buttons_ioctl,
};

static struct miscdevice buttons_dev =
{
	BUTTONS_MINOR,
	"buttons",
	&buttons_fops
};
static int __init buttons_dev_init(void)
{
    int ret;

    if (rc6_init())
		return -ENODEV;
    else {
		printk("RC init ok\n");
    	}
    if(key_init())
		return -ENODEV;
	else{
		printk("key init ok\n");
		}
    /* Register Buttons device driver */
    ret = misc_register(&buttons_dev);
    if (ret != 0)
    {
	printk(KERN_ERR "Unified input device driver: Error registering input device\n");
	return ret;
    }

    return 0;
}

static void __exit buttons_dev_exit(void)
{
	misc_deregister(&buttons_dev);
	key_exit();
	rc6_exit();
}

void show_pc_table (void)
{
	int i;
	for (i = 0; i < WAVE_TIMING_SIZE; i++)
		if (a_pc_table[i] != 0)
			printk ("0x%08x\n", a_pc_table[i]);
}

__initcall(buttons_dev_init);

