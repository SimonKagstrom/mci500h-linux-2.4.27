/*
 * buttons_dev.h - Unified input device driver for Philips PNX0106
 *
 * Copyright (C) 2005-2006 PDCC, Philips CE.
 *
 * Revision History :
 * ----------------
 *
 *   Version             DATE          NAME
 *   Initial version     01-Dec-2005   James       initial version
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#ifndef _W6_BUTTONS_H_
#define _W6_BUTTONS_H_

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#include <asm/arch/hardware.h>
#include <asm/arch/event_router.h>

#if (GPIO_RC6RX == GPIO_IEEE1394_LPS)
#define ERID_RC6RX EVENT_ID_XT_IEEE1394_LPS
#elif (GPIO_RC6RX == GPIO_GPIO_6)
#define ERID_RC6RX EVENT_ID_XT_GPIO_6
#else
#error "RC6 input connected to unexpected GPIO pin..."
#endif

#define BUTTONS_MINOR		180

/*
 * Function command codes for io_ctl.
 */
#define BUTTONS_GET_WAVE	1
#define BUTTONS_SET_WAVE	2
#define BUTTONS_GET_KEY	        3
#define BUTTONS_SET_KEY	        4

#define BUTTONS_SET_SIGNAL      5
#define BUTTONS_GET_FLAGS		6
#define BUTTONS_SET_KEYPAD_TYPE 7
#define BUTTONS_SET_RAW_KEY_MAPTABLE 8
#define BUTTONS_GET_RAW_KEY_MAPTABLE 9
#define BUTTONS_GET_KEY_MAPTBL_SIZE 10	//get maptable size
#define BUTTONS_SET_DEBUG_FLAG 11

#define BUTTONS_FLAGS_WAVE	0x01	//RC6 wave got
#define BUTTONS_FLAGS_KEY		0x02	//Key event!!


#define GPIO_CALIBRATION_PIN    GPIO_GPIO_18

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
#ifndef __ASSEMBLY__
extern void wave_capturer(void);
extern unsigned long long g_wave_captured;
extern void *p_wave_capturer_end;
extern unsigned int a_wave_timing[64];
extern unsigned long g_wave_timing_restore;

enum key_t_{
	Virtual_Key_Idle,		// means no key press

	// HAS module
	/* 2.		| HAS-keys:|*/									/* |			|				|			|||-----|||			|			 |		  |		 |				|		  |				*/
	/* KEY-1	| line		|*/									/* |			|				|			|||-----|||			|			 |		  |		 |				|		  |				*/
	/* No.		| Key		|*/									/* | Primary	| function		| of		|||-----||| key		| VNOM		 | VMIN	  |	VMAX | STEPNOM		| STEPMIN |	STEPMAX		*/
	/* [#]		| [			|*/									/* | [V]		| [decimal]		| [decimal]	|||-----||| [V]		| [V]		 |		  |		 |				|		  |				*/

	/* 1		| 1014		|*/ physical_KEY_DBB,				/* | 0.000		| 0.010			| 0.100		|||-----||| 0		| 0		| 8	  |	8	 | 0.100		| 0.100	  |				*/
	/* 2		| 1015		|*/ physical_KEY_IS,				/* | 0.359		| 0.174			| 0.393		|||-----||| 28		| 13	| 31	  |	6	 | 0.229		| 0.064	  |				*/
	/* 3		| 1016		|*/ physical_KEY_MUTE,				/* | 0.648		| 0.389			| 0.702		|||-----||| 50		| 31	| 55	  |	9	 | 0.205		| 0.104	  |				*/
	/* 4		| 1017		|*/ physical_key_Left_Reverse,		/* | 0.974		| 0.614			| 1.044		|||-----||| 76		| 48	| 81	  |	11	 | 0.204		| 0.137	  |				*/
	/* 5		| 1018		|*/ physical_KEY_PLAY,				/* | 1.324		| 0.883			| 1.404		|||-----||| 103		| 70	| 109	  |	13	 | 0.202		| 0.159	  |				*/
	/* 6		| 1019		|*/ physical_key_Right_Forward,		/* | 1.683		| 1.194			| 1.765		|||-----||| 131		| 95	| 137	  |	13	 | 0.196		| 0.165	  |				*/
	/* 7		| 1020		|*/ physical_KEY_STOP,				/* | 2.029		| 1.518			| 2.106		|||-----||| 157		| 120	| 164	  |	13	 | 0.185		| 0.156	  |				*/


	/* KEY-2	| line		|*/									/* |			|				|			|||-----|||			|		|		  |		 |				|		  |				*/
	/* No.		| Key		|*/									/* | Primary	| function		| of		|||-----||| key		| VNOM	| VMIN	  |	VMAX | STEPNOM		| STEPMIN |	STEPMAX		*/
	/* [#]		| [			|*/									/* | [V]		| [decimal]		| [decimal]	|||-----||| [V]		| [V]	|		  |		 |				|		  |				*/
	/* 1		| 1007		|*/ physical_KEY_SOURCE,			/* | 0.000		| 0.010			| 0.100		|||-----||| 0		| 0		| 8	  |	8	 | 0.100		| 0.100	  |				*/
	/* 2		| 1008		|*/ physical_key_Down_Next,			/* | 0.359		| 0.174			| 0.393		|||-----||| 28		| 13	| 31	  |	6	 | 0.229		| 0.064	  |				*/
	/* 3		| 1009		|*/ physical_key_Mark,				/* | 0.648		| 0.389			| 0.702		|||-----||| 50		| 31	| 55	  |	9	 | 0.205		| 0.104	  |				*/
	/* 4		| 1010		|*/ physical_KEY_VOLUME_UP,			/* | 0.974		| 0.614			| 1.044		|||-----||| 76		| 48	| 81	  |	11	 | 0.204		| 0.137	  |				*/
	/* 5		| 1011		|*/ physical_KEY_VOLUME_DOWN,		/* | 1.324		| 0.883			| 1.404		|||-----||| 103		| 70	| 109	  |	13	 | 0.202		| 0.159	  |				*/
	/* 6		| 1012		|*/ physical_key_Music_broadcast,	/* | 1.683		| 1.194			| 1.765		|||-----||| 131		| 95	| 137	  |	13	 | 0.196		| 0.165	  |				*/
	/* 7		| 1013		|*/ physical_key_Music_follows,		/* | 2.029		| 1.518			| 2.106		|||-----||| 157		| 120	| 164	  |	13	 | 0.185		| 0.156	  |				*/
	/* 8		| 1047		|*/ physical_key_REC,				/* | 2.382		| 1.841			| 2.447		|||-----||| 185		| 146	| 190	  |	11	 | 0.208		| 0.133	  |				*/


	/* KEY-3	| line		|*/									/* |			|				|			|||-----|||			|		|		  |		 |				|		  |				*/
	/* No.		| Key		|*/									/* | Primary	| function		| of		|||-----||| key		| VNOM	| VMIN	  |	VMAX | STEPNOM		| STEPMIN |	STEPMAX		*/
	/* [#]		| [			|*/									/* | [V]		| [decimal]		| [decimal]	|||-----||| [V]		| [V]	|		  |		 |				|		  |				*/
	/* 1		| 1025		|*/ physical_KEY_POWER_STANDBY,		/* | 0.000		| 0.010			| 0.100		|||-----||| 0		| 0		| 8	  |	8	 | 0.100		| 0.100	  |				*/
	/* 2		| 1001		|*/ physical_key_Like_Artist,		/* | 0.000		| 0.174			| 0.100		|||-----||| 0		| 13	| 8	  |	8	 | 0.100		| 0.100	  |				*/
	/* 3		| 1002		|*/ physical_key_Like_Genre,		/* | 0.359		| 0.389			| 0.393		|||-----||| 28		| 31	| 31	  |	6	 | 0.229		| 0.064	  |				*/
	/* 4		| 1003		|*/ physical_key_Match_Genre,		/* | 0.648		| 0.614			| 0.702		|||-----||| 50		| 48	| 55	  |	9	 | 0.205		| 0.104	  |				*/
	/* 5		| 1004		|*/ physical_key_Menu,				/* | 0.974		| 0.883			| 1.044		|||-----||| 76		| 70	| 81	  |	11	 | 0.204		| 0.137	  |				*/
	/* 6		| 1005		|*/ physical_key_Up_Previous,		/* | 1.324		| 1.194			| 1.404		|||-----||| 103		| 95	| 109	  |	13	 | 0.202		| 0.159	  |				*/
	/* 7		| 1006		|*/ physical_key_View,				/* | 1.683		| 1.518			| 1.765		|||-----||| 131		| 120	| 137	  |	13	 | 0.196		| 0.165	  |				*/
	/* 8		| 1021		|*/ physical_key_Dim,				/* | 2.029		| 1.841			| 2.106		|||-----||| 157		| 146	| 164	  |	13	 | 0.185		| 0.156	  |				*/


	virtual_VIEW_UP,
	virtual_VIEW_DOWN,
	virtual_PLAY_UP,
	virtual_PLAY_DOWN,
	virtual_VIEW_SOURCE,	// add this def under Vincent's demand
	virtual_DBB_SOURCE,

	physical_key_EJECT,
	physical_key_ERROR_ADVALUE	//add by alex
};
#endif  // __ASSEMBLY__
#endif // BUTTONS_DEV_INCLUDED
