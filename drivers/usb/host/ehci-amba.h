#ifndef _EHCI_AMBA_H_
#define _EHCI_AMBA_H_
#include <linux/ioport.h>

#define AMBA_NR_IRQS    2

struct amba_device { 
	struct usb_hcd dev;
	struct resource res;
	u64                     dma_mask;
	unsigned int            periphid;
	unsigned int            irq[AMBA_NR_IRQS];
        char            	name[90];       /* device name */
        char            	slot_name[8];   /* slot name */
};

struct amba_id {
        unsigned int            id;
        unsigned int            mask;
        void                    *data;
};

struct amba_driver {
	char 			*name;
        int                     (*probe)(struct amba_device *, void *);
        int                     (*remove)(struct amba_device *);
        void                    (*shutdown)(struct amba_device *);
#ifdef CONFIG_PM
        int                     (*suspend)(struct amba_device *, u32 state);
        int                     (*resume)(struct amba_device *);
#endif
        struct amba_id          *id_table;
};

#define amba_get_drvdata(d)     dev_get_drvdata(&d->dev)
#define amba_set_drvdata(d,p)   dev_set_drvdata(&d->dev, p)

//int amba_driver_register(struct amba_driver *);
//void amba_driver_unregister(struct amba_driver *);
int amba_device_register(struct amba_device *, struct resource *);
void amba_device_unregister(struct amba_device *);
//struct amba_device *amba_find_device(const char *, struct device *, unsigned int, unsigned int);
int amba_request_regions(struct amba_device *, const char *);
void amba_release_regions(struct amba_device *);

#define amba_config(d)  (((d)->periphid >> 24) & 0xff)
#define amba_rev(d)     (((d)->periphid >> 20) & 0x0f)
#define amba_manf(d)    (((d)->periphid >> 12) & 0xff)
#define amba_part(d)    ((d)->periphid & 0xfff)

#endif

