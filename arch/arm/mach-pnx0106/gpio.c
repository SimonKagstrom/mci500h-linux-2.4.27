/*
 *  linux/arch/arm/mach-pnx0106/gpio.c
 *
 *  Copyright (C) 2003-2007 Andre McCurdy, NXP Semiconductors.
 *
 *  This source code is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  version 2 as published by the Free Software Foundation.
 */

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/arch/gpio.h>


/****************************************************************************
****************************************************************************/

/*
    The operation of each GPIO pin is controlled as follows:

        Mode1 bit   Mode0 bit

            0           0       : Input (output driver disabled)
            0           1       : Controlled associated HW block (no longer GPIO)
            1           0       : Output driven low
            1           1       : Output driven high

    In all cases, the physical state of each pin can be read via
    the PINS registers.
*/

#define VA_GPIO_PINS(slice)         (IO_ADDRESS_IOCONF_BASE + 0x00 + (0x40 * (slice)))
#define VA_GPIO_MODE_0(slice)       (IO_ADDRESS_IOCONF_BASE + 0x10 + (0x40 * (slice)))
#define VA_GPIO_MODE_0_SET(slice)   (IO_ADDRESS_IOCONF_BASE + 0x14 + (0x40 * (slice)))
#define VA_GPIO_MODE_0_CLEAR(slice) (IO_ADDRESS_IOCONF_BASE + 0x18 + (0x40 * (slice)))
#define VA_GPIO_MODE_1(slice)       (IO_ADDRESS_IOCONF_BASE + 0x20 + (0x40 * (slice)))
#define VA_GPIO_MODE_1_SET(slice)   (IO_ADDRESS_IOCONF_BASE + 0x24 + (0x40 * (slice)))
#define VA_GPIO_MODE_1_CLEAR(slice) (IO_ADDRESS_IOCONF_BASE + 0x28 + (0x40 * (slice)))

#define GPIO_PIN2SLICE(pin)         ((pin) >> 5)
#define GPIO_PIN2BITMASK(pin)       (1 << ((pin) & 0x1F))


/****************************************************************************
****************************************************************************/

void gpio_set_as_input (unsigned int pin)
{
    unsigned long flags;
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    /* fixme: should check existing level so that we can change to an input without glitching... */

    local_irq_save (flags);
    writel (bitmask, VA_GPIO_MODE_0_CLEAR(slice));
    writel (bitmask, VA_GPIO_MODE_1_CLEAR(slice));
    local_irq_restore (flags);
}

void gpio_set_as_output (unsigned int pin, unsigned int initial_value)
{
    unsigned long flags;
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    /* fixme: should check existing level so that we can change to an output without glitching... */

    local_irq_save (flags);
    writel (bitmask, (initial_value) ? VA_GPIO_MODE_0_SET(slice) : VA_GPIO_MODE_0_CLEAR(slice));
    writel (bitmask, VA_GPIO_MODE_1_SET(slice));
    local_irq_restore (flags);
}

void gpio_set_as_function (unsigned int pin)
{
    unsigned long flags;
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    /* fixme: should check existing level so that we can change to function mode without glitching... */

    local_irq_save (flags);
    writel (bitmask, VA_GPIO_MODE_0_SET(slice));
    writel (bitmask, VA_GPIO_MODE_1_CLEAR(slice));
    local_irq_restore (flags);
}

int gpio_get_value (unsigned int pin)
{
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    return (readl (VA_GPIO_PINS(slice)) & bitmask) ? 1 : 0;
}

void gpio_set_high (unsigned int pin)
{
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    writel (bitmask, VA_GPIO_MODE_0_SET(slice));
}

void gpio_set_low (unsigned int pin)
{
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    writel (bitmask, VA_GPIO_MODE_0_CLEAR(slice));
}

void gpio_set_output (unsigned int pin, unsigned int value)
{
    if (value)
        gpio_set_high (pin);
    else
        gpio_set_low (pin);
}

int gpio_toggle_output (unsigned int pin)
{
    unsigned long flags;
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);
    unsigned int mode0;

    local_irq_save (flags);
    mode0 = readl (VA_GPIO_MODE_0(slice));
    writel ((mode0 ^ bitmask), VA_GPIO_MODE_0(slice));
    local_irq_restore (flags);

    return (mode0 & bitmask) ? 1 : 0;
}

#if defined (CONFIG_PNX0106_LPAS1)
void gpio_get_config (unsigned int pin) 	/* LTPE Joe for getting gpio config of pin */
{
    unsigned long flags;
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);
    unsigned int mode1,mode0;

    local_irq_save (flags);
    mode1 = readl (VA_GPIO_MODE_1(slice));
    mode0 = readl (VA_GPIO_MODE_0(slice));
    printk(KERN_INFO "bitmask = %x\n",bitmask);
    printk(KERN_INFO "Mode0[%x]=%x,Addr=%x\n",pin,(mode0 & bitmask),VA_GPIO_MODE_0(slice));
    printk(KERN_INFO "Mode1[%x]=%x,Addr=%x\n",pin,(mode1 & bitmask),VA_GPIO_MODE_1(slice));
    local_irq_restore (flags);
}
#endif


/****************************************************************************
****************************************************************************/

static const struct
{
    char pad_letter;
    unsigned char pad_number;
    unsigned short pin;
}
padname_table[] =
{
    { 'D',  2, GPIO_MPMC_D0           },
    { 'C',  6, GPIO_MPMC_A0           },
    { 'D',  1, GPIO_MPMC_D1           },
    { 'B',  6, GPIO_MPMC_A1           },
    { 'E',  4, GPIO_MPMC_D2           },
    { 'A',  6, GPIO_MPMC_A2           },
    { 'E',  3, GPIO_MPMC_D3           },
    { 'D',  5, GPIO_MPMC_A3           },
    { 'E',  2, GPIO_MPMC_D4           },
    { 'C',  5, GPIO_MPMC_A4           },
    { 'E',  1, GPIO_MPMC_D5           },
    { 'B',  5, GPIO_MPMC_A5           },
    { 'F',  4, GPIO_MPMC_D6           },
    { 'A',  5, GPIO_MPMC_A6           },
    { 'F',  3, GPIO_MPMC_D7           },
    { 'C',  4, GPIO_MPMC_A7           },
    { 'F',  2, GPIO_MPMC_D8           },
    { 'B',  4, GPIO_MPMC_A8           },
    { 'F',  1, GPIO_MPMC_D9           },
    { 'A',  4, GPIO_MPMC_A9           },
    { 'G',  4, GPIO_MPMC_D10          },
    { 'C',  3, GPIO_MPMC_A10          },
    { 'G',  3, GPIO_MPMC_D11          },
    { 'B',  3, GPIO_MPMC_A11          },
    { 'G',  2, GPIO_MPMC_D12          },
    { 'A',  3, GPIO_MPMC_A12          },
    { 'G',  1, GPIO_MPMC_D13          },
    { 'A',  2, GPIO_MPMC_A13          },
    { 'H',  3, GPIO_MPMC_D14          },
    { 'A',  1, GPIO_MPMC_A14          },
    { 'H',  2, GPIO_MPMC_D15          },
    { 'K',  3, GPIO_MPMC_NDYCS        },

    { 'L',  3, GPIO_MPMC_CKE          },
    { 'J',  1, GPIO_MPMC_DQM0         },
    { 'K',  4, GPIO_MPMC_DQM1         },
    { 'J',  2, GPIO_MPMC_CLKOUT       },
    { 'L',  4, GPIO_MPMC_NWE          },
    { 'K',  1, GPIO_MPMC_NCAS         },
    { 'K',  2, GPIO_MPMC_NRAS         },

    { 'E', 14, GPIO_SPI0_S_N          },
    { 'F', 17, GPIO_SPI0_MISO         },
    { 'E', 13, GPIO_SPI0_MOSI         },
    { 'F', 16, GPIO_SPI0_SCK          },
    { 'F', 15, GPIO_SPI0_EXT          },
    { 'B', 17, GPIO_PCCARDA_NRESET    },
    { 'B', 16, GPIO_PCCARDA_NCS0      },
    { 'A', 16, GPIO_PCCARDA_NCS1      },
    { 'B', 15, GPIO_PCCARDA_INTRQ     },
    { 'C', 14, GPIO_PCCARDA_IORDY     },
    { 'A', 15, GPIO_PCCARDA_NDMACK    },
    { 'D',  9, GPIO_PCCARD_A0         },
    { 'C',  9, GPIO_PCCARD_A1         },
    { 'B',  9, GPIO_PCCARD_A2         },
    { 'D', 14, GPIO_PCCARDA_DMARQ     },
    { 'B', 14, GPIO_PCCARD_NDIOW      },
    { 'A', 14, GPIO_PCCARD_NDIOR      },
    { 'D', 13, GPIO_PCCARD_DD0        },
    { 'C', 13, GPIO_PCCARD_DD1        },
    { 'B', 13, GPIO_PCCARD_DD2        },
    { 'A', 13, GPIO_PCCARD_DD3        },
    { 'D', 12, GPIO_PCCARD_DD4        },
    { 'C', 12, GPIO_PCCARD_DD5        },
    { 'B', 12, GPIO_PCCARD_DD6        },
    { 'A', 12, GPIO_PCCARD_DD7        },
    { 'D', 11, GPIO_PCCARD_DD8        },
    { 'C', 11, GPIO_PCCARD_DD9        },
    { 'B', 11, GPIO_PCCARD_DD10       },
    { 'A', 11, GPIO_PCCARD_DD11       },
    { 'D', 10, GPIO_PCCARD_DD12       },
    { 'C', 10, GPIO_PCCARD_DD13       },
    { 'B', 10, GPIO_PCCARD_DD14       },

    { 'A', 10, GPIO_PCCARD_DD15       },
    { 'M', 13, GPIO_LCD_RS            },
    { 'N', 17, GPIO_LCD_CSB           },
    { 'P', 14, GPIO_LCD_DB1           },
    { 'R', 17, GPIO_LCD_DB2           },
    { 'R', 16, GPIO_LCD_DB3           },
    { 'T', 17, GPIO_LCD_DB4           },
    { 'T', 16, GPIO_LCD_DB5           },
    { 'M', 15, GPIO_LCD_DB6           },
    { 'M', 14, GPIO_LCD_DB7           },
    { 'P', 15, GPIO_LCD_DB0           },
    { 'P', 16, GPIO_LCD_RW_WR         },
    { 'P', 17, GPIO_LCD_E_RD          },
    { 'N', 14, GPIO_DAI_DATA          },
    { 'N', 15, GPIO_DAI_WS            },
    { 'N', 16, GPIO_DAI_BCK           },
    { 'E', 17, GPIO_UART0_TXD         },
    { 'E', 16, GPIO_UART0_RXD         },
    { 'M', 17, GPIO_UART1_TXD         },
    { 'M', 16, GPIO_UART1_RXD         },
    { 'L', 13, GPIO_DAO_WS            },
    { 'L', 14, GPIO_DAO_BCK           },
    { 'L', 15, GPIO_DAO_DATA          },
    { 'L', 16, GPIO_DAO_CLK           },
    { 'B',  7, GPIO_GPIO_22           },
    { 'C',  7, GPIO_GPIO_21           },
    { 'A',  8, GPIO_GPIO_20           },
    { 'B',  8, GPIO_GPIO_19           },
    { 'D', 15, GPIO_UART0_NCTS        },
    { 'D', 16, GPIO_SPI1_SCK          },
    { 'D', 17, GPIO_SPI1_MISO         },
    { 'C', 15, GPIO_SPI1_MOSI         },

    { 'C', 16, GPIO_SPI1_EXT          },
    { 'C', 17, GPIO_SPI1_S_N          },
    { 'E', 15, GPIO_UART0_NRTS        },
    { 'K', 14, GPIO_IEEE1394_PD       },
    { 'K', 15, GPIO_IEEE1394_NRESET   },
    { 'K', 16, GPIO_IEEE1394_CTL0     },
    { 'K', 17, GPIO_IEEE1394_CTL1     },
    { 'J', 15, GPIO_IEEE1394_LREQ     },
    { 'J', 16, GPIO_IEEE1394_SYSCLK   },
    { 'J', 17, GPIO_IEEE1394_D0       },
    { 'H', 15, GPIO_IEEE1394_D1       },
    { 'H', 16, GPIO_IEEE1394_D2       },
    { 'H', 17, GPIO_IEEE1394_D3       },
    { 'G', 14, GPIO_IEEE1394_D4       },
    { 'G', 15, GPIO_IEEE1394_D5       },
    { 'G', 16, GPIO_IEEE1394_D6       },
    { 'G', 17, GPIO_IEEE1394_D7       },
    { 'F', 13, GPIO_IEEE1394_CNA      },
    { 'L', 17, GPIO_IEEE1394_LPS      },
    { 'F', 14, GPIO_IEEE1394_CPS      },
    { 'T',  9, GPIO_GPIO_0            },
    { 'U',  9, GPIO_GPIO_1            },
    { 'A',  7, GPIO_GPIO_2            },
    { 'N', 13, GPIO_GPIO_3            },
    { 'U', 12, GPIO_GPIO_4            },
    { 'T', 12, GPIO_GPIO_5            },
    { 'R', 12, GPIO_GPIO_6            },
    { 'P', 12, GPIO_GPIO_7            },
    { 'C',  8, GPIO_GPIO_18           },
    { 'P', 10, GPIO_GPIO_17           },
    { 'R', 10, GPIO_GPIO_16           },
    { 'T', 10, GPIO_GPIO_15           },

    { 'U', 10, GPIO_GPIO_14           },
    { 'N', 11, GPIO_GPIO_13           },
    { 'P', 11, GPIO_GPIO_12           },
    { 'R', 11, GPIO_GPIO_11           },
    { 'T', 11, GPIO_GPIO_10           },
    { 'U', 11, GPIO_GPIO_9            },
    { 'N', 12, GPIO_GPIO_8            },

    { 'B',  1, GPIO_MPMC_A15          },
    { 'B',  2, GPIO_MPMC_A16          },
    { 'C',  1, GPIO_MPMC_A17          },
    { 'C',  2, GPIO_MPMC_A18          },
    { 'D',  4, GPIO_MPMC_A19          },
    { 'D',  3, GPIO_MPMC_A20          },
    { 'L',  2, GPIO_MPMC_NSTCS0       },
    { 'L',  1, GPIO_MPMC_NSTCS1       },
    { 'M',  5, GPIO_MPMC_NSTCS2       },
    { 'H',  1, GPIO_MPMC_BLOUT0       },
    { 'J',  4, GPIO_MPMC_BLOUT1       },
    { 'J',  3, GPIO_MPMC_NOE          },

    { 'D',  6, GPIO_SPDIF_OUT         }
};

/*
    Note: result is returned in a static buffer...
*/
char *gpio_pin_to_padname (unsigned int pin)
{
    int i;
    static char padname[8];

    memcpy (padname, "UNKNOWN", 8);

    for (i = 0; i < (sizeof(padname_table) / sizeof(padname_table[0])); i++) {
        if (padname_table[i].pin == pin) {
            padname[0] = padname_table[i].pad_letter;
            sprintf (&padname[1], "%d", padname_table[i].pad_number);
            break;
        }
    }

    return padname;
}

void gpio_dump_state (unsigned int pin)
{
    unsigned long flags;
    unsigned int mode0;
    unsigned int mode1;
    unsigned int value;
    unsigned int slice = GPIO_PIN2SLICE(pin);
    unsigned int bitmask = GPIO_PIN2BITMASK(pin);

    local_irq_save (flags);
    mode0 = readl (VA_GPIO_MODE_0(slice));
    mode1 = readl (VA_GPIO_MODE_1(slice));
    value = readl (VA_GPIO_PINS(slice));
    local_irq_restore (flags);

    mode0 = (mode0 & bitmask) ? 1 : 0;
    mode1 = (mode1 & bitmask) ? 1 : 0;
    value = (value & bitmask) ? 1 : 0;

    printk ("%-4s: %s (pin state: %d)%s\n",
            gpio_pin_to_padname (pin),
            ((mode1 == 0) && (mode0 == 0)) ? "Input   " :
            ((mode1 == 0) && (mode0 == 1)) ? "Function" :
            ((mode1 == 1) && (mode0 == 0)) ? "Output 0" : "Output 1", value,
            ((mode1 == 1) && (value != mode0)) ? " <-- Conflict !!" : "");
}

void gpio_dump_state_all (void)
{
    int i;

    for (i = 0; i < (sizeof(padname_table) / sizeof(padname_table[0])); i++)
        gpio_dump_state (padname_table[i].pin);
}


#if 1
EXPORT_SYMBOL(gpio_set_as_input);
EXPORT_SYMBOL(gpio_set_as_output);
EXPORT_SYMBOL(gpio_set_as_function);
EXPORT_SYMBOL(gpio_get_value);
EXPORT_SYMBOL(gpio_set_high);
EXPORT_SYMBOL(gpio_set_low);
EXPORT_SYMBOL(gpio_set_output);
EXPORT_SYMBOL(gpio_toggle_output);
#if defined (CONFIG_PNX0106_LPAS1)
EXPORT_SYMBOL(gpio_get_config);
#endif
#if 0
EXPORT_SYMBOL(gpio_pin_to_padname);
EXPORT_SYMBOL(gpio_dump_state);
EXPORT_SYMBOL(gpio_dump_state_all);
#endif
#endif

