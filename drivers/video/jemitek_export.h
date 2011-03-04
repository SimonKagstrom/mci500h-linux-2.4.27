//
// Copyright 2005, Philips Semiconductors 
// Created by Srinivasa Mylarappa
//

#ifndef BYD_LCDDRV_EXPORT_H
#define BYD_LCDDRV_EXPORT_H
#include <linux/ioctl.h>

struct screenlocation {
	unsigned char xstart;
	unsigned char xend;
	unsigned char ystart;
	unsigned char yend;
};
typedef struct screenlocation screenlocation_t;

struct pixel {
	unsigned char x;
	unsigned char y;
	unsigned short val;
};
typedef struct pixel pixel_t;

struct horzline {
	unsigned char xstart;
	unsigned char xend;
	unsigned char y;
	unsigned short val;
};
typedef struct horzline horzline_t;

struct vertline {
	unsigned char x;
	unsigned char ystart;
	unsigned char yend;
	unsigned short val;
};
typedef struct vertline vertline_t;

struct fillrect {
	unsigned char xstart;
	unsigned char ystart;
	unsigned char xend;
	unsigned char yend;
	unsigned short val;
};
typedef struct fillrect fillrect_t;

struct textDraw {
	unsigned char x;
	unsigned char y;
	unsigned char count;
	char text[256];
	unsigned short fgcolor;
	unsigned short bgcolor;
};
typedef struct textDraw textDraw_t;

	

#define FB_BYD_CONTRAST	_IOW('F', 0x15, int)
#define FB_BYD_BACKLIGHT	_IOW('F', 0x16, int)

#define FB_RUBY_FLUSH    	_IOW('F',0x18, struct screenlocation)
#define FB_RUBY_BLANK    	_IO('F',0x19)
#define FB_RUBY_LOGO     	_IO('F', 0x1a)
#define FB_RUBY_READ_PIXEL	_IOW('F', 0x1b, struct pixel)
#define FB_RUBY_DRAW_PIXEL	_IOW('F', 0x1c, struct pixel)
#define FB_RUBY_DRAW_HLINE	_IOW('F', 0x1d, struct horzline)
#define FB_RUBY_DRAW_VLINE	_IOW('F', 0x1e, struct vertline)
#define FB_RUBY_DRAW_RECT	_IOW('F', 0x1f, struct fillrect)
#define FB_RUBY_DRAW_TEXT	_IOW('F', 0x20, struct textDraw)


#endif				/* BYD_LCDDRV_EXPORT_H */
