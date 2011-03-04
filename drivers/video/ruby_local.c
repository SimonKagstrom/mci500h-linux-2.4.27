//
// Copyright 2005, Philips Semiconductors
// Created by Srinivasa Mylarappa
//

#include <linux/module.h>
    
#include <asm/io.h>			// for ioremap- & write-functions
#include <linux/ioport.h>	// for mem_region functions
#include <linux/delay.h>	// for udelay
#include <linux/kernel.h>	// for printk
#include <linux/mm.h>
#include <asm/delay.h>
#include <asm/hardware.h>
#include <asm/arch/gpio.h>
    
#include "ruby_api.h"
#include "ruby_local.h"
#include "PhilipsLogo.h"
    
#define	TWO_BYTE_PIXELS	1 /* we support only packed 2 bytes per pixel mode */
#if CONFIG_SSA_TAHOE
#define LCDIF	((LCDIFRegs_t *)g_virt_lcd_intf)
#endif
    
static int lcdif_init(void);
static void lcdif_poll_for_fifo_space(unsigned int space_required);

// Globals
static void *g_virt_lcd_intf = NULL;
static unsigned char *g_virAddressFB = NULL;
static int isprevcmd = 0;
#if CONFIG_PNX0106_LAGUNA_REVA
static uint8_t *LCDIF_REG = NULL;
static uint8_t *LCDIF_DATA = NULL;
#endif

/*
 * Internals
 */ 
void
ruby_init_display(void) 
{
	int i;
//	mdelay(1000);

#ifdef CONFIG_SSA_TAHOE
    // Prepare our memory-maps
    if (!request_mem_region (LCDIF_REGS_BASE, sizeof (LCDIFRegs_t), "Ruby LCD INTF")) {
		printk(KERN_ERR "RUBY_LCDIF: Unable to reserve the EBI command memory region.\n");
		return;
	}
	
    // Map the physical address to a virtual address
    g_virt_lcd_intf =
    ioremap_nocache(LCDIF_REGS_BASE, sizeof (LCDIFRegs_t));
	if (!g_virt_lcd_intf) {
		printk(KERN_ERR "RABBIT_LCD: Unable to map the LCD interface memory region.\n");
		return;
	}
#else
    if (!request_mem_region (0x24080000, 256, "Ruby LCD INTF")) {
		printk(KERN_ERR "RUBY_LCDIF: Unable to reserve the LCDIF REG memory region.\n");
		return;
	}
    LCDIF_REG = ioremap_nocache(0x24080000, 256);
    if (!request_mem_region (0x240C0000, 256, "Ruby LCD INTF")) {
		printk(KERN_ERR "RUBY_LCDIF: Unable to reserve the LCDIF DATA memory region.\n");
		return;
	}
    LCDIF_DATA = ioremap_nocache(0x240C0000, 256);
	printk(KERN_CRIT "RUBY LCD: LCDIF_REG=%8.8x  LCDIF_DATA=%8.8x\n", LCDIF_REG, LCDIF_DATA);
#endif
	gpio_set_as_output(GPIO_LCD_RESET, 0);	/* drive low to assert LCD conrtoller reset */
	udelay(20);		/* min assertion time 20 usec */
	lcdif_init();	/* initialize lcd controller control reg */
	gpio_set_high(GPIO_LCD_RESET);	/* drive high to release LCD controller reset */
	gpio_set_as_output(GPIO_LCD_BACKLIGHT, 1);	/* drive high to turn on LCD backlight */
	mdelay(100); /* wait 100 msec before first access to Ruby LCD controller registers */

	lcd_register_write(0x03, 0x01);	/* Issue SW reset */
	mdelay(100); /* wait 100 msec before first access to Ruby LCD controller registers */

	/* Stand-by to normal mode( wake up ) but display is still off all white */
	lcd_register_write(0x00, 0x80);	
	lcd_register_write(0x4A, 0x00);	/* setup EEPROM address */
	lcd_register_write(0x43, 0x06);	/* start to read EEPROM */
	mdelay(500);	/* wait for display module power supplies to stabilise .. */

	lcd_register_write(0x02, 0x00);	/* setup CPU interface as data write source */
	
#ifdef TWO_BYTE_PIXELS
    lcd_register_write(0x04, 0x02);	/* Pixel data write format 5-6-5 (2-byte) */
#else
    lcd_register_write(0x04, 0x01);	/* Pixel data write format 6-6-6 (3-byte) */
#endif
	    
    /* set lcd window size */ 
    lcd_set_display_window(0, 0, RUBY_DISPLAY_WIDTH - 1, RUBY_DISPLAY_HEIGHT - 1);
	lcd_register_write(0x0E, 0x00);
	lcd_register_write(0x0F, 0xF0);
	lcd_register_write(0x10, 0x00);
	lcd_register_write(0x11, 0x00);

	lcd_register_write(0x5D, 0x00);	/* Select full colour mode */
	lcd_register_write(0x00, 0x00);	/* Enable display */
	
    /* clear display */ 
    ruby_clear_display();

	//printk( "RUBY LCDIF: ... finished low-level hardware initialisation.\n" ) ;
}

void
ruby_deinit_display(void) 
{
	// Cleanup the mapping
#if CONFIG_SSA_TAHOE
	iounmap(g_virt_lcd_intf);
	release_mem_region(LCDIF_REGS_BASE, sizeof (LCDIFRegs_t));
	g_virt_lcd_intf = NULL;
#endif
}

void
lcdif_write_reg_single(unsigned int value) 
{
#ifdef CONFIG_SSA_TAHOE
	if (isprevcmd)
		lcdif_poll_for_fifo_space(1);
	else
		lcdif_poll_for_fifo_space(LCDIF_FIFO_SIZE);
	LCDIF->inst_byte = value;
	isprevcmd = 1;
#else
	//uint8_t *addr =  (uint8_t *)LCDIF_REG;
	//*addr = value & 0xFF;
	writeb(value, LCDIF_REG);
	//printk(KERN_DEBUG "Writing LCD reg : %x Register : %x after : %x\n", addr, value, *addr);
#endif
}

void
lcdif_write_data_single(unsigned int value) 
{
#if CONFIG_SSA_TAHOE
	if (isprevcmd)
		lcdif_poll_for_fifo_space(LCDIF_FIFO_SIZE);
	else
		lcdif_poll_for_fifo_space(1);
	LCDIF->data_byte = value;
	isprevcmd = 0;
#else
	//uint8_t *addr =  (uint8_t *)LCDIF_DATA;
	//*addr = value & 0xFF;
	writeb(value, LCDIF_DATA);
	//printk(KERN_DEBUG "Writing LCD data : %x value to write: %x after : %x\n", addr, value, *addr);
#endif
}

void
lcdif_write_data_word(unsigned int value) 
{
#if CONFIG_SSA_TAHOE
	if (isprevcmd)
		lcdif_poll_for_fifo_space(LCDIF_FIFO_SIZE);
	
	else
		lcdif_poll_for_fifo_space(4);
	LCDIF->data_word = value;
	isprevcmd = 0;
#else
	uint16_t *addr =  (uint16_t *)LCDIF_DATA;
    *addr = value;
	writew(value, LCDIF_DATA);
#endif
}

void
lcd_register_write(unsigned int addr, unsigned int value) 
{
	lcdif_write_reg_single(addr);
	lcdif_write_data_single(value);
}

void
lcdif_write_data_multi(unsigned short *src, unsigned int bytecount) 
{
#if 0
    while ((((unsigned long) src) & 0x03) && bytecount) {
		lcdif_write_data_single(*src++);
		bytecount--;
	} 
	while (bytecount >= 4) {
		LCDIF->data_word = *((unsigned int *) src);
		src += 4;
		bytecount -= 4;
	} 
	unsigned short *ptr = src;
	unsigned short val = 0;
	while (bytecount) {
		val = *ptr;
		lcdif_write_data_single(val >> 8);
		lcdif_write_data_single(val & 0xff);
		ptr++;
		bytecount -= 2;
	}
#endif	
	char *ptr = (char *)src;
	while (bytecount) {
		lcdif_write_data_single(*ptr);
		ptr++;
		bytecount--;
	}
}

void
lcd_set_display_window(unsigned int tl_x, unsigned int tl_y, unsigned int br_x,
		       unsigned int br_y) 
{
	lcd_register_write(8, tl_x);
	lcd_register_write(10, tl_y + 4);
	lcd_register_write(9, br_x);
	lcd_register_write(11, br_y + 4);
} 

void
lcd_write_pixel_data(unsigned int red, unsigned int green, unsigned int blue,
		     unsigned int repeat_count) 
{
	unsigned int temp;
	if (red > 0x3F)
		red = 0x3F;
	if (green > 0x3F)
		green = 0x3F;
	if (blue > 0x3F)
		blue = 0x3F;
	lcdif_write_reg_single(0x06);
	
#ifdef TWO_BYTE_PIXELS
	    temp = ((red >> 1) << 11) | (green << 5) | ((blue >> 1) << 0);
	while (repeat_count--) {
		lcdif_write_data_single((temp >> 8) & 0xFF);
		lcdif_write_data_single((temp >> 0) & 0xFF);
	}
	
#else
	while (repeat_count--) {
		lcdif_write_data_single(red);
		lcdif_write_data_single(green);
		lcdif_write_data_single(blue);
	}
#endif	
}

static int
lcdif_init(void)
{
#if CONFIG_SSA_TAHOE

    /* Power-on defaults apart from CS set active high.  */ 
    LCDIF->control = ((0 << 0) | /* bit  0     : reserved */ 
	      (0 << 1) | /* bit  1     : (0 == parallel mode), (1 == serial mode) */ 
	      (0 << 2) | /* bit  2     : (0 == 8080 mode), (1 == 6800 mode) */ 
	      (0 << 3) | /* bit  3     : (0 == 8bit mode), (1 == 4bit mode) */ 
	      (3 << 4) | /* bits 4..5  : serial clock shift (see datasheet..) */ 
	      (3 << 6) | /* bits 6..7  : serial read position (see datasheet...) */ 
	      (0 << 8) | /* bit  8     : busy flag check (0 == disable), (1 == enable) */ 
	      (0 << 9) | /* bit  9     : busy value (??) */ 
	      (7 << 10) | /* bit 10..12 : busy bit number */ 
	      (0 << 13) | /* bit 13     : busy rs value */ 
	      (1 << 14) | /* bit 14     : (0 == CS active low), (1 == CS active high) */ 
	      (0 << 15) | /* bit 15     : (0 == E_RD active low), (1 == E_RD active high) */ 
	      (0 << 16) | /* bit 16     : msb first */ 
	      (0 << 17)); /* bit 17     : (0 == normal operation), (1 == loopback mode) */

	    /* clean FIFO buffer */ 
	    // LCDIF->data_word = 0;
	    // LCDIF->data_word = 0;
	    // LCDIF->data_word = 0;
	    // LCDIF->data_word = 0;
	    // LCDIF->data_word = 0;
	    // LCDIF->data_word = 0;
#endif
	return 0;
}

static void
lcdif_poll_for_fifo_space(unsigned int space_required) 
{
#if CONFIG_SSA_TAHOE
	unsigned int space_available;
	while (1) {
		space_available =
		    (LCDIF_FIFO_SIZE - ((LCDIF->status >> 5) & 0x1F));
		if (space_available >= space_required)
			break;
	}
#endif
}


/*
 * Externals ...
 */ 
void
ruby_clear_display(void) 
{
	
	//printk("Inside %s function \n", __FUNCTION__);
	memset(g_virAddressFB, 0, RUBY_PHYSICAL_MEM_SIZE);
	lcd_set_display_window(0, 0, RUBY_DISPLAY_WIDTH - 1, RUBY_DISPLAY_HEIGHT - 1);
	lcdif_write_reg_single(0x06);
	lcdif_write_data_multi((unsigned short *) g_virAddressFB, RUBY_PHYSICAL_MEM_SIZE);
}

void
ruby_display_logo(void) 
{
    //printk("Inside %s function \n", __FUNCTION__);
	lcd_set_display_window(0, 0, RUBY_DISPLAY_WIDTH - 1, RUBY_DISPLAY_HEIGHT - 1);
	lcdif_write_reg_single(0x06);
	lcdif_write_data_multi((unsigned short *) PhilipsLogo, RUBY_PHYSICAL_MEM_SIZE);
} 

void
ruby_set_addresses(unsigned char *pAddress, dma_addr_t pDmaAddress) 
{
	g_virAddressFB = pAddress;

} 

void
ruby_send_data(unsigned char xstart, unsigned char xend, unsigned char ystart,
	       unsigned char yend) 
{
	unsigned short *addr = (unsigned short *)g_virAddressFB;
	int x, y;
	unsigned short val;
	
	lcd_set_display_window(xstart, ystart , xend -1  , yend -1);
	lcdif_write_reg_single(0x06);
	for(y = ystart; y < yend; y++)
		for(x = xstart; x < xend; x++) {
			val = *(addr+x + (y * RUBY_DISPLAY_WIDTH - 1));
			lcdif_write_data_single(val >> 8);
			lcdif_write_data_single(val & 0xff);
		}
} 

void
ruby_draw_pixel(unsigned char x, unsigned char y, unsigned short val) 
{
	lcd_set_display_window(x, y, x, y);
	lcdif_write_reg_single(0x06);
	lcdif_write_data_single(val >> 8);
	lcdif_write_data_single(val & 0xff);
}

void
ruby_draw_horzline(unsigned char xstart, unsigned char xend, unsigned char y,
		   unsigned short val) 
{
	int count = xend - xstart + 1;
	lcd_set_display_window(xstart, y, xend, y);
	lcdif_write_reg_single(0x06);
	while (count--) {
		lcdif_write_data_single(val >> 8);
		lcdif_write_data_single(val & 0xff);
	}
}

void
ruby_draw_vertline(unsigned char x, unsigned char ystart, unsigned char yend,
		   unsigned short val) 
{
	int count = yend - ystart + 1;
	lcd_set_display_window(x, ystart, x, yend);
	lcdif_write_reg_single(0x06);
	while (count--) {
		lcdif_write_data_single(val >> 8);
		lcdif_write_data_single(val & 0xff);
	}
}

void
ruby_draw_fillrect(unsigned char xstart, unsigned char ystart,
		   unsigned char xend, unsigned char yend,
		   unsigned short val) 
{
	int linecount = yend - ystart + 1;
	lcd_set_display_window(xstart, ystart, xend, yend);
	lcdif_write_reg_single(0x06);
	while (linecount--) {
		int count = xend - xstart + 1;
		while (count--) {
			lcdif_write_data_single(val >> 8);
			lcdif_write_data_single(val & 0xff);
		}
	}
}

