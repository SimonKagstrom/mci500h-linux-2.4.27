
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/delay.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>

#include <asm/arch/has7752_pccard.h>

static void pccard_reset (void)
{
   /*
      Generate pccard HW reset cycle
   */
   has7752_pccard_reset_assert();
   udelay (10);
   has7752_pccard_reset_deassert();

   mdelay (200);     /* fixme: try not to wait so long */
}

static void write_tst (unsigned int addr, unsigned int value)
{
   printk ("Read from offset 0x%04x : 0x%08x\n", addr, has7752_pccard_io_read (addr));

   if (has7752_pccard_io_write (addr, value) != 0)
      printk ("write failed...\n");
   
   udelay (1000);
   
   printk ("Read from offset 0x%04x : 0x%08x\n", addr, has7752_pccard_io_read (addr));
}


static int __init pccard_test_init (void)
{
   int i;
   int result = 0;

   printk ("HAS7752 simple pccard interface test module begin\n");

   /*
      initialise the pccard interface hardware
   */
   if ((result = has7752_pccard_init()) != 0) {
      printk ("Error initialising pccard interface\n");
      goto exit;
   }

   /*
      Setup waitstates for memory (ie attribute/config) and IO spaces.
      Waitstate settings are address setup, data strobe and address hold.
      All waitstates are measured in HCLK clock cycles.
   */
   printk ("\nSetting up waitstates for %d MHz bus\n", HCLK_MHZ);
   has7752_pccard_set_mem_waitstates (15,31,7);
   has7752_pccard_set_io_waitstates (15,31,7);

   /*
      Generate pccard HW reset cycle
   */
   pccard_reset();

// gpio_set_as_output (GPIO_PCMCIA_RESET, 0);

#if 0

   /*
      Read from attribute space
   */
   printk ("\nAttribute Read test...\n");
   for (i = 0; i < 0x100; i += 2) {
      if ((result = has7752_pccard_attribute_read (i)) == -ETIMEDOUT)
         goto exit;
      printk ("Attribute space 0x%04x: 0x%02x\n", i, has7752_pccard_attribute_read (i));
   }
      
   /*
      Write to config space
   */
   printk ("\nPerforming config Write...\n");
   if ((result = has7752_pccard_attribute_write (0x1fe, 0x01)) != 0)
      goto exit;

#endif

   /*
      IO space access tests... (fixme: need to know card details !)
   */

#if 0
   printk ("\nIO Read test...\n");
   for (i = 0; i < 0x30; i += 2) {
      if ((result = has7752_pccard_io_read (i)) == -ETIMEDOUT)
         goto exit;
      printk ("IO space 0x%04x: 0x%02x\n", i, result);
   }
#endif


   if (has7752_pccard_io_write (0x00388, 0x6789) != 0)
      printk ("write failed...\n");

#if 1

   printk ("\n");
   printk ("Read from offset 0x24   : 0x%08x\n", has7752_pccard_io_read (0x24));
   printk ("Read from offset 0x1000 : 0x%08x\n", has7752_pccard_io_read (0x1000));
   printk ("Read from offset 0x1002 : 0x%08x\n", has7752_pccard_io_read (0x1002));

// write_tst (0x0020, 0x0000);
   write_tst (0x0388, 0x1234);
   write_tst (0x0004, 0x1234);

#endif

   /*
   printk ("\nIO Write test...\n");
   for (i = 0x00; i < 0x30; i += 2)
      if ((result = has7752_pccard_io_write (i, i)) == -ETIMEDOUT)
         goto exit;

   printk ("\nIO Read-back test...\n");
   for (i = 0x1000; i < 0x1030; i += 2) {
      if ((result = has7752_pccard_io_read (i)) == -ETIMEDOUT)
         continue;
      printk ("IO space 0x%04x: 0x%02x\n", i, result);
   }

   */
   
exit:

   /*
      Generate pccard HW reset cycle
   */
   pccard_reset();
   printk ("\nTests complete\n");
   return -1;
}

static void __exit pccard_test_exit (void)
{
   printk ("HAS7752 simple pccard interface test module exit\n");
}

EXPORT_NO_SYMBOLS;

MODULE_AUTHOR ("Andre McCurdy");
MODULE_DESCRIPTION ("HAS7752 pccard interface debug/test module");
MODULE_LICENSE ("GPL");

module_init (pccard_test_init);
module_exit (pccard_test_exit);

