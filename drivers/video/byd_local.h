//
// Copyright 2004, Philips Semiconductors BV, ASL
// Created by Armin Gerritsen
//

#ifndef BYD_LCDDRV_H
#define BYD_LCDDRV_H
    
/*
 * Function exports
 */ 
void ruby_init_display(void);
void ruby_deinit_display(void);
void ruby_clear_display(void);
void ruby_display_logo(void);
void ruby_send_data(unsigned char xstart, unsigned char xend,
		     unsigned char ystart, unsigned char yend);
void ruby_set_addresses(unsigned char *pAddress, dma_addr_t pDmaAddress);
void ruby_draw_pixel(unsigned char x, unsigned char y, unsigned short val);
unsigned short ruby_read_pixel(unsigned char x, unsigned char y);
void ruby_draw_horzline(unsigned char xstart, unsigned char xend,
			 unsigned char y, unsigned short val);
void ruby_draw_vertline(unsigned char x, unsigned char ystart,
			 unsigned char yend, unsigned short val);
void ruby_draw_fillrect(unsigned char xstart, unsigned char ystart,
			 unsigned char xend, unsigned char yend,
			 unsigned short val);
void lcd_set_display_window(unsigned int tl_x, unsigned int tl_y,
				    unsigned int br_x, unsigned int br_y);
void lcdif_write_reg_single(unsigned int value);
void lcdif_write_data_single(unsigned int value);
void lcdif_write_data_word(unsigned int value);
void lcd_register_write(unsigned int addr, unsigned int value);
void lcdif_write_data_multi(unsigned short *src,
				    unsigned int bytecount);
void lcd_write_pixel_data(unsigned int red, unsigned int green,
				  unsigned int blue, unsigned int repeat_count);
#ifdef CONFIG_RUBY_MELODY
#define LCDIF_REGS_BASE 	(0x8E000000UL)
#endif

////// for pnx0106 laguna_lpas1
#ifdef CONFIG_RUBY_MELODY
#define LCDIF_REGS_BASE 	(0x15000800)
#endif

#define FBCON_HAS_CFB16 1
    
#ifndef CONFIG_RUBY_MELODY
#define LCDIF_FIFO_SIZE   (16)
typedef struct  {
	volatile unsigned int status;	/* R- 0x00  */
	volatile unsigned int control;	/* RW 0x04  */
	volatile unsigned int int_raw;	/* R- 0x08  */
	volatile unsigned int int_clear;	/* -W 0x0C  */
	volatile unsigned int int_mask;	/* RW 0x10  */
	volatile unsigned int read_cmd;	/* -W 0x14  */
	volatile unsigned int p1[2];
	volatile unsigned int inst_byte;	/* RW 0x20  */
	volatile unsigned int p2[3];
	volatile unsigned int data_byte;	/* RW 0x30  */
	volatile unsigned int p3[3];
	volatile unsigned int inst_word;	/* -W 0x40  */
	volatile unsigned int p4[15];
	volatile unsigned int data_word;	/* -W 0x80  */
} LCDIFRegs_t;
#endif

/*
 * Debug macros
 */
#undef RUBY_DEBUG
#ifdef RUBY_DEBUG
static int dbg_level = 3;
#define DBG(level, args...)	if (level <= dbg_level) printk(args)
#else
#define DBG(level, args...)
#endif

#endif	/* BYD_LCDDRV_H */
    
