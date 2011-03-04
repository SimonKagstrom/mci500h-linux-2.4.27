//----------------------------------------------------------------------------
//
//  Copyright (c) 2004 Philips Semiconductors BV, ASL
//  All rights reserved.
//
//----------------------------------------------------------------------------

#if 0
#define RUBY_DISPLAY_WIDTH           176
#define RUBY_DISPLAY_HEIGHT          220
#define RUBY_DISPLAY_BPP             16
#define RUBY_DISPLAY_SCANLINE_BYTES  352	// = 176 * 16bits / 8bpB
#define RUBY_DISPLAY_FLIPLINE_BYTES  440	// = 220 * 16bits / 8bpB 
#define RUBY_PHYSICAL_MEM_SIZE       77440	// = 176x220x2 (565)
#define RUBY_PHYSICAL_PIXELS	     38720	// = 176x220 pixels
#endif 

//////////////////////////////////////////////
/////          For BYD LCM S6B33BC                    /////
//////////////////////////////////////////////
#if 1
#define RUBY_DISPLAY_WIDTH           160
#define RUBY_DISPLAY_HEIGHT          128
#define RUBY_DISPLAY_BPP             16
#define RUBY_DISPLAY_SCANLINE_BYTES  320	// = 160 * 16bits / 8bpB
#define RUBY_DISPLAY_FLIPLINE_BYTES  256	// = 128 * 16bits / 8bpB  
#define RUBY_PHYSICAL_MEM_SIZE       40960	// = 128x160x2 (565)
#define RUBY_PHYSICAL_PIXELS	     20480	// = 128x160 pixels
#endif


