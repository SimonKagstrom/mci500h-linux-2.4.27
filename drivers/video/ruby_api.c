//-----------------------------------------------------------------------------
//  
// ruby_api.c -- driver for Philips Ruby LCD
//
// Copyright(c) 2005, Philips Semiconductors  
// All rights reserved.
//
//---------------------------------------------------------------------------- 

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/wrapper.h>

#include <asm/uaccess.h>

#include <video/fbcon.h>
#include <video/fbcon-cfb4.h>
#include <video/fbcon-cfb8.h>
#include <video/fbcon-cfb16.h>
#include <video/fbcon-cfb24.h>
#include <video/fbcon-cfb32.h>

#include "ruby_api.h"
#include "ruby_local.h"
#include "ruby_export.h"

// Check if at least one colourdepth was configured
#if !defined (FBCON_HAS_CFB4)  && !defined (FBCON_HAS_CFB8)  && \
    !defined (FBCON_HAS_CFB16) && !defined (FBCON_HAS_CFB24) && \
    !defined (FBCON_HAS_CFB32)
#error No colourdepth specified ?
#endif

// For now, we'll only support 16bpp
#if !defined (FBCON_HAS_CFB16)
#error This driver only supports 16bpp. Sorry ...
#endif

// Linux has powermanagement. To disable support in this driver, uncomment this line:
#define NO_RABBIT_POWERMANAGEMENT

//----------------------------------------------------------------------------- 
//
// Local Definitions
// 
//-----------------------------------------------------------------------------  

typedef struct {
	struct fb_info_gen gen;
	unsigned char *FrameBufferAddress;
	dma_addr_t FrameBufferDmaAddress;
	struct timer_list ruby_timer;
} FB_INFO_RUBY;

int periodic_flush_enable = 0;

//-----------------------------------------------------------------------------
//
// Function Prototypes 
// 
//-----------------------------------------------------------------------------

int __init rubyfb_setup(char *dummy);
int __init rubyfb_init(void);

int ruby_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	       unsigned long arg, int con, struct fb_info *info);

static void ruby_detect(void);
static int ruby_encode_fix(struct fb_fix_screeninfo *fix, const void *par,
			   struct fb_info_gen *info);
static int ruby_decode_var(const struct fb_var_screeninfo *var, void *par,
			   struct fb_info_gen *info);
static int ruby_encode_var(struct fb_var_screeninfo *var, const void *par,
			   struct fb_info_gen *info);
static void ruby_get_par(void *par, struct fb_info_gen *info);
static void ruby_set_par(const void *par, struct fb_info_gen *info);
static int ruby_getcolreg(unsigned regno, unsigned *red, unsigned *green,
			  unsigned *blue, unsigned *transp,
			  struct fb_info *info);
static int ruby_setcolreg(unsigned regno, unsigned red, unsigned green,
			  unsigned blue, unsigned transp, struct fb_info *info);
static int ruby_pan_display(const struct fb_var_screeninfo *var,
			    struct fb_info_gen *info);
static int ruby_blank(int blank_mode, struct fb_info_gen *info);
static void ruby_set_disp(const void *par, struct display *disp,
			  struct fb_info_gen *info);
static void ruby_flush(unsigned long timeoutval);
static int ruby_mmap(struct fb_info *info, struct file *file, struct vm_area_struct *vma);

static void free_framebuffer(void);
static unsigned char *alloc_framebuffer(void);

//----------------------------------------------------------------------------- 
//
// Local globals
// 
//-----------------------------------------------------------------------------    

static struct fb_ops ruby_ops = {
      owner:THIS_MODULE,
      fb_get_fix:fbgen_get_fix,
      fb_get_var:fbgen_get_var,
      fb_set_var:fbgen_set_var,
      fb_get_cmap:fbgen_get_cmap,
      fb_set_cmap:fbgen_set_cmap,
      fb_ioctl:ruby_ioctl,
	  fb_mmap:ruby_mmap,
};

/*
 *  In most cases the `generic' routines (fbgen_*) should be satisfactory.
 *  However, you're free to fill in your own replacements.
 */
static struct fbgen_hwswitch ruby_hwswitch = {
	ruby_detect,
	ruby_encode_fix,
	ruby_decode_var,
	ruby_encode_var,
	ruby_get_par,
	ruby_set_par,
	ruby_getcolreg,
	ruby_setcolreg,
	ruby_pan_display,
	ruby_blank,
	ruby_set_disp
};

// More locals
static FB_INFO_RUBY fb_info;
static struct display disp;
static char __initdata default_fontname[40] = { 0 };

//----------------------------------------------------------------------------- 
//
// Function implementations
// 
//-----------------------------------------------------------------------------   

// The Ruby LCD is not a real framebuffer device. It cannot directly expose
// its video-memory like for instance (most) PCI framebuffer devices can.
// Therefore we've used a normal buffer allocated in the kernel.
// This buffer has now been changed and must therefore be 'flushed'.
void __init
ruby_flush_framebuffer(void)
{
	ruby_send_data(0, RUBY_DISPLAY_WIDTH - 1, 0, RUBY_DISPLAY_HEIGHT - 1);
}

//----------------------------------------------------------------------------- 
// 
// This function should detect the current video mode settings and store  
// it as the default video mode 
//  
//----------------------------------------------------------------------------- 
static void
ruby_detect(void)
{
	// Nice and simple. :-)
	// This is because for now we only support one mode.
}

//----------------------------------------------------------------------------- 
// 
// This function should fill in the 'fix' structure based on the values  
// in the `par' structure.  
//  
//----------------------------------------------------------------------------- 
static int
ruby_encode_fix(struct fb_fix_screeninfo *fix, const void *par,
		struct fb_info_gen *info)
{
	//printk( "ruby_encode_fix: Called!\n" ) ;

	memset(fix, 0, sizeof (struct fb_fix_screeninfo));
	strcpy(fix->id, fb_info.gen.info.modename);

	// For now we only support 16 bpp
	fix->visual = FB_VISUAL_DIRECTCOLOR;
/*  
#if defined FBCON_HAS_CFB4 
    fix->visual = FB_VISUAL_STATIC_PSEUDOCOLOR ;
#elif defined FBCON_HAS_CFB8
    fix->visual = FB_VISUAL_PSEUDOCOLOR ;
#elif defined FBCON_HAS_CFB16
    fix->visual = FB_VISUAL_DIRECTCOLOR ;
#elif defined FBCON_HAS_CFB24 
    fix->visual = FB_VISUAL_DIRECTCOLOR ;
#elif defined FBCON_HAS_CFB32
    fix->visual = FB_VISUAL_DIRECTCOLOR ;
#endif
*/

	// Start and length of frame buffer memory
	fix->smem_start = fb_info.FrameBufferDmaAddress;
	fix->smem_len = RUBY_PHYSICAL_MEM_SIZE;

	// Some more initialisation ...
	fix->type = FB_TYPE_PACKED_PIXELS;	// See FB_TYPE_* in fb.h
	fix->accel = FB_ACCEL_NONE;

	// Set the line length
	fix->line_length = RUBY_DISPLAY_SCANLINE_BYTES;

	return 0;
}

//----------------------------------------------------------------------------- 
// 
//  Get the video params out of 'var'. If a value doesn't fit, round it up,  
//  if it's too big, return -EINVAL.  
//  
//  Suggestion: Round up in the following order: bits_per_pixel, xres,  
//  yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,  
//  bitfields, horizontal timing, vertical timing.  
//  
//----------------------------------------------------------------------------- 
static int
ruby_decode_var(const struct fb_var_screeninfo *var, void *fb_par,
		struct fb_info_gen *info)
{
	//printk( "ruby_decode_var: Called!\n" ) ;

	return 0;
}

//----------------------------------------------------------------------------- 
//
// Fill the 'var' structure based on the values in 'par' and maybe other 
// values read out of the hardware. 
// 
//----------------------------------------------------------------------------- 
static int
ruby_encode_var(struct fb_var_screeninfo *var, const void *fb_par,
		struct fb_info_gen *info)
{
	//printk( "ruby_encode_var: Called!\n" ) ;

	memset(var, 0, sizeof (struct fb_var_screeninfo));

	// Visible resolution
	var->xres = RUBY_DISPLAY_WIDTH;
	var->yres = RUBY_DISPLAY_HEIGHT;

	// Lots of other parameters
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres;
	var->xoffset = 0;
	var->yoffset = 0;
	var->bits_per_pixel = RUBY_DISPLAY_BPP;
	var->grayscale = 0;
	var->nonstd = 0;	// != 0 Non standard pixel format
	var->activate = FB_ACTIVATE_NOW;	// see FB_ACTIVATE_*
	var->height = -1;	// height of picture in mm
	var->width = -1;	// width of picture in mm
	var->accel_flags = 0;	// acceleration flags
	var->pixclock = 20000;	// (AAG:) What does this _exactely_ do?
	var->right_margin = 0;
	var->lower_margin = 0;
	var->hsync_len = 0;
	var->vsync_len = 0;
	var->left_margin = 0;
	var->upper_margin = 0;
	var->sync = 0;
	var->vmode = FB_VMODE_NONINTERLACED;

	// Colour offsets: { A:R:G:B } = { 0:5:6:5 }
	var->red.offset = 11;
	var->red.length = 5;
	var->green.offset = 5;
	var->green.length = 6;
	var->blue.offset = 0;
	var->blue.length = 5;
	var->transp.offset = 0;
	var->transp.length = 0;

	return 0;
}

//----------------------------------------------------------------------------- 
//
// Set the 'par' according to hardware
//
//----------------------------------------------------------------------------- 
static void
ruby_get_par(void *fb_par, struct fb_info_gen *info)
{
	// Nice and simple. :-)
}

//----------------------------------------------------------------------------- 
//
// Set the hardware according to 'par'. 
//
//----------------------------------------------------------------------------- 
static void
ruby_set_par(const void *fb_par, struct fb_info_gen *info)
{
	// Nice and simple. :-)
}

//----------------------------------------------------------------------------- 
// 
// Read a single colour register and split it into colours/transparent.  
// The return values must have a 16 bit magnitude.  
// Return != 0 for invalid regno.  
// NOTE: ruby don't have any color registers
// 
//----------------------------------------------------------------------------- 
static int
ruby_getcolreg(unsigned regno, unsigned *red, unsigned *green,
	       unsigned *blue, unsigned *transp, struct fb_info *info)
{
	//printk( "ruby_getcolreg: Called!\n" ) ;

	// Third, pack the values into 16bit integers.
	*red = 0;
	*green = 0;
	*blue = 0;
	*transp = 0;

	return 0;
}

//---------------------------------------------------------------------------- 
// 
// Set a single colour register.
// The values supplied have a 16 bit magnitude.  
// Return != 0 for invalid regno.  
//
// We get called even if we specified that we don't have a programmable palette
// or in direct/true colour modes!
//---------------------------------------------------------------------------- 
static int
ruby_setcolreg(unsigned regno, unsigned red, unsigned green,
	       unsigned blue, unsigned transp, struct fb_info *info)
{
	//printk( "ruby_setcolreg: Called for register %u.\n", regno ) ;
	//printk( "Values are : %u %u %u %u.\n", red, green, blue, transp ) ;

	// This is not needed for normal framebuffer operations.
	// I suspect Linux uses this to display their pinguin?
	// So for now, just skip it.
	/* 
	   // Sanity check ...
	   if ( regno > RABBIT_PHYSICAL_PIXELS )
	   {
	   return -EINVAL ;
	   }
	   // ToDo: Actual implementation. :-)
	 */

	return 0;
}

//-----------------------------------------------------------------------------
// 
// Pan (or wrap, depending on the `vmode' field) the display using the 
// `xoffset' and `yoffset' fields of the `var' structure. 
// If the values don't fit, return -EINVAL. 
// 
//-----------------------------------------------------------------------------
static int
ruby_pan_display(const struct fb_var_screeninfo *var, struct fb_info_gen *info)
{
	// We should never be called!

	//return 0 ;
	return -EINVAL;
}

//-----------------------------------------------------------------------------
//     
// Blank the screen if blank_mode != 0, else unblank. If blank == NULL  
// then the caller blanks by setting the CLUT (Colour Look Up Table) to all  
// black. Return 0 if blanking succeeded, != 0 if un-/blanking failed due  
// to e.g. a video mode which doesn't support it. Implements VESA suspend  
// and powerdown modes on hardware that supports disabling hsync/vsync:  
//     
// 0 = unblank 
// 1 = blank 
// 2 = suspend vsync 
// 3 = suspend hsync 
// 4 = off 
//
//----------------------------------------------------------------------------- 
static int
ruby_blank(int blank_mode, struct fb_info_gen *info)
{
	ruby_clear_display();
	return 0;
}

//----------------------------------------------------------------------------- 
//
// Fill in a pointer with the virtual address of the mapped frame buffer.  
// Fill in a pointer to appropriate low level text console operations (and  
// optionally a pointer to help data) for the video mode `par' of your  
// video hardware. These can be generic software routines, or hardware  
// accelerated routines specifically tailored for your hardware.  
// If you don't have any appropriate operations, you must fill in a  
// pointer to dummy operations, and there will be no text output.  
//
//----------------------------------------------------------------------------- 
static void
ruby_set_disp(const void *par, struct display *disp, struct fb_info_gen *info)
{
	//printk( "ruby_set_disp: Called!\n" ) ;

	disp->screen_base = fb_info.FrameBufferAddress;
	disp->dispsw = &fbcon_dummy;	// (AAG:) Dummy (for now).
	disp->scrollmode = SCROLL_YREDRAW;
}

//----------------------------------------------------------------------------- 
//
// Initialise the chip and the frame buffer driver. 
//
//----------------------------------------------------------------------------- 
int __init
rubyfb_init(void)
{

	fb_info.FrameBufferAddress = alloc_framebuffer();
	if (!fb_info.FrameBufferAddress) {
		printk
		    ("rubyfb_init: Failed to allocate memory for the framebuffer.\n");
		return -EINVAL;
	}
	// Clear the allocated memory
	memset(fb_info.FrameBufferAddress, 0, RUBY_PHYSICAL_MEM_SIZE);

	// Tell the lower levels what our addresses are
	ruby_set_addresses(fb_info.FrameBufferAddress,
			   fb_info.FrameBufferDmaAddress);

	ruby_init_display();

	strcpy(fb_info.gen.info.modename, "ruby");
	fb_info.gen.info.changevar = NULL;
	fb_info.gen.info.node = -1;
	fb_info.gen.info.fbops = &ruby_ops;
	fb_info.gen.info.disp = &disp;

	fb_info.gen.info.switch_con = &fbgen_switch;
	fb_info.gen.info.updatevar = &fbgen_update_var;
	fb_info.gen.info.blank = &fbgen_blank;

	strcpy(fb_info.gen.info.fontname, default_fontname);
	fb_info.gen.parsize = 0;
	fb_info.gen.info.flags = FBINFO_FLAG_DEFAULT;
	fb_info.gen.fbhw = &ruby_hwswitch;
	fb_info.gen.fbhw->detect();

	fbgen_get_var(&disp.var, -1, &fb_info.gen.info);
	disp.var.activate = FB_ACTIVATE_NOW;
	fbgen_do_set_var(&disp.var, 1, &fb_info.gen);
	fbgen_set_disp(-1, &fb_info.gen);
	fbgen_install_cmap(0, &fb_info.gen);

	if (register_framebuffer(&fb_info.gen.info) < 0) {
		printk("Failed to register the %s framebuffer device.\n",
		       fb_info.gen.info.modename);
		free_framebuffer();
		return -EINVAL;
	}
	if (periodic_flush_enable) {
		init_timer(&fb_info.ruby_timer);
		fb_info.ruby_timer.function = ruby_flush;
		fb_info.ruby_timer.data = 0;
		fb_info.ruby_timer.expires = (unsigned long) jiffies + HZ + 100;
		add_timer(&fb_info.ruby_timer);
	}

	ruby_display_logo();

	printk("fb%d: %s framebuffer device installed.\n",
	       GET_FB_IDX(fb_info.gen.info.node), fb_info.gen.info.modename);

	printk("Display %s features %d x %d at %d bpp.\n",
	       fb_info.gen.info.modename,
	       RUBY_DISPLAY_WIDTH, RUBY_DISPLAY_HEIGHT, RUBY_DISPLAY_BPP);

	return 0;
}

static int ruby_mmap(struct fb_info *info, struct file *file, struct vm_area_struct *vma) 
{
	int ret;
	unsigned long offset = vma->vm_pgoff<<PAGE_SHIFT;
	unsigned long size = vma->vm_end - vma->vm_start;

	if(offset & ~PAGE_MASK) {
		printk("Offset not aligned: %ld\n", offset);
		return -ENXIO;
	}
	
	/*
	if(size > RUBY_PHYSICAL_MEM_SIZE) {
		printk("size too big\n");
		return(-ENXIO);
	}
	*/
	if( (vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		printk("writeable mappings must be shared, rejecting\n");
		return(-EINVAL);
	}

	/* we do not want to have this area swapped out, lock it */
	vma->vm_flags |= VM_LOCKED;

	if(remap_page_range(vma->vm_start, 
					virt_to_phys((void *)(unsigned long)fb_info.FrameBufferAddress), 
							size, PAGE_SHARED))
	{
		printk("remap page range failed\n");
		return -ENXIO;
	}

	return 0;
}

int
ruby_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg, int con, struct fb_info *info)
{
	//printk(KERN_ERR "%s val = %x\n", __FUNCTION__, cmd);
	if (FB_RUBY_FLUSH == cmd) {
		// The Rabbit LCD is not a real framebuffer device. It cannot
		// directly expose its video-memory like for instance (most) PCI
		// framebuffer devices can.
		// Therefore we've used a normal buffer allocated in the kernel.
		// This can flush the buffer manually to the LCD-device.

		struct screenlocation pos;

		copy_from_user((void *) &pos, (void *) arg,
				       sizeof (struct screenlocation));
		ruby_send_data(pos.xstart, pos.xend, pos.ystart, pos.yend);
		return 0;
	}

	if (FB_RUBY_BLANK == cmd) {
		// Clear the allocated memory
		memset(fb_info.FrameBufferAddress, 0, RUBY_PHYSICAL_MEM_SIZE);
		ruby_clear_display();
		return 0;
	}

	if (FB_RUBY_LOGO == cmd) {
		ruby_display_logo();
		return 0;
	}

	if (FB_RUBY_DRAW_PIXEL == cmd) {
		struct pixel *pix = (struct pixel *) arg;
		ruby_draw_pixel(pix->x, pix->y, pix->val);
		return 0;
	}

	if (FB_RUBY_DRAW_HLINE == cmd) {
		struct horzline *line = (struct horzline *) arg;
		ruby_draw_horzline(line->xstart, line->xend, line->y,
				   line->val);
		return 0;
	}

	if (FB_RUBY_DRAW_VLINE == cmd) {
		struct vertline *line = (struct vertline *) arg;
		ruby_draw_vertline(line->x, line->ystart, line->yend,
				   line->val);
		return 0;
	}

	if (FB_RUBY_DRAW_RECT == cmd) {
		struct fillrect *rect = (struct fillrect *) arg;
		//printk("drawRect: x1: %d, y1:%d x2:%d, y2:%d\n", rect->xstart, rect->ystart, rect->xend, rect->yend);
		ruby_draw_fillrect(rect->xstart, rect->ystart, rect->xend,
				   rect->yend, rect->val);
		return 0;
	}

	if (FB_RUBY_DRAW_TEXT == cmd) {
		struct textDraw *in = (struct textDraw *) arg;
		//printk(KERN_DEBUG "FB_RUBY_DRAW_TEXT ioctl Text : %s count = %d x:%d y:%d color:%d\n",
		//						in->text, in->count, in->x, in->y, in->fgcolor);
		ruby_draw_text(in->text, in->count, in->x, in->y, in->fgcolor, in->bgcolor);
		return 0;
	}


	return -EINVAL;
}

static void
free_framebuffer()
{
	/*  consistent_free(fb_info.FrameBufferAddress,
			RUBY_PHYSICAL_MEM_SIZE, fb_info.FrameBufferDmaAddress);
	*/
	kfree(fb_info.FrameBufferAddress);
}

static unsigned char *
alloc_framebuffer()
{
	
	/*return (unsigned char *) consistent_alloc(GFP_KERNEL,
						  PAGE_ALIGN(RUBY_PHYSICAL_MEM_SIZE + PAGE_SIZE),
						  &fb_info.
						  FrameBufferDmaAddress);
	*/
	int *kmalloc_ptr = NULL;
	int *kmalloc_area = NULL;
	unsigned long virt_addr;
	kmalloc_ptr = kmalloc(RUBY_PHYSICAL_MEM_SIZE + 2 * PAGE_SIZE, GFP_KERNEL);
	kmalloc_area = (int *)(((unsigned long)kmalloc_ptr + PAGE_SIZE -1) & PAGE_MASK);
	for(virt_addr = (unsigned long)kmalloc_area; virt_addr<(unsigned long)kmalloc_area
							+ RUBY_PHYSICAL_MEM_SIZE; virt_addr += PAGE_SIZE)
	{
			/* reserver all pages to make them remapable */
			mem_map_reserve(virt_to_page(virt_addr));
	}

	return (unsigned char *)kmalloc_area;

}

//----------------------------------------------------------------------------- 
//
// Parse user speficied options (`video=ruby:')  
//
//----------------------------------------------------------------------------- 
int __init
rubyfb_setup(char *options)
{
	// We don't support options (yet) ...
	return 0;
}

static void
ruby_flush(unsigned long timeoutval)
{
	ruby_flush_framebuffer();
	if (periodic_flush_enable)
		mod_timer(&fb_info.ruby_timer, jiffies + HZ / 2);	// half sec
}

/*
 * For usage as a module ... 
 */
#ifdef MODULE
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Srinivasa Mylarappa");
MODULE_DESCRIPTION("Linux device driver for Philips ruby lcd module");
MODULE_PARM(periodic_flush_enable, "i");

static unsigned int bModInit = 0;

int
init_module(void)
{
	if (!bModInit) {
		rubyfb_init();
		bModInit = 1;
	}

	return 0;
}

void
cleanup_module(void)
{
	int status = 0;

	if (bModInit) {
		status = unregister_framebuffer(&fb_info.gen.info);
		if (status < 0) {
			printk
			    ("Failed to unregister the framebuffer device (status=%d).\n",
			     status);
		} else {
			ruby_deinit_display();
			free_framebuffer();
			if (periodic_flush_enable)
				del_timer(&fb_info.ruby_timer);
			bModInit = 0;
		}
	}
}
#endif
