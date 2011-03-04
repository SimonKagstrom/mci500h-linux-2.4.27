/* Mass storage device driver using the ISP1581 USB 2.0 device controller
 * on the HAS 7752 CPU.
 * (C) 2004 Philips Semiconductor Inc.
 * Author: Ranjit Deshpande
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <scsi/scsi.h>		/* For CDB command definitions */
#include <linux/hdreg.h>	/* For ATA command definitions */
#include <linux/usb.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include "has7752_usbdev.h"

#undef ONE_SECTOR_PER_COMMAND	/* Define this to operate on a single
				 * sector per command.
				 */
#undef CHECK_DRQ		/* Define this to check DRQ bit before
				 * issuing a command.
				 */
#undef DUMP_FIFO		/* Define this to dump the first 16 bytes
				 * of a READ/WRITE transactions. If you
				 * define this, please define 
				 * ONE_SECTOR_PER_COMMAND as well.
				 */

static union isp1581_state st;
static u8 isp1581_config_value;
static struct hd_driveid isp1581_driveid;
static void (*isp1581_ata_handler)(void);
static void (*isp1581_do_ata_cmd)(struct isp1581_cbw *, u16 sectors, u32 lba);
static struct isp1581_cbw cur_cbw;
static u32 drive_capacity;
static struct timer_list tx_timer;

#ifdef ONE_SECTOR_PER_COMMAND
static int sector_count;
#endif

struct isp1581_error_code {
	u8		sense_key;
	u8		asc;
	u8		ascq;
} ec;

static void isp1581_do_data_transfer(void);

#define FIXUP_DRIVEID_16(x)	isp1581_driveid.x = \
					le16_to_cpu(isp1581_driveid.x)

static void isp1581_stall_ep(int endpoint, int dir)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_dma_ep_reg depreg;
	union isp1581_control_reg creg;

	endpoint 		&= 0x7;
	epireg.val		= 0;
	epireg.bits.endpidx	= endpoint;
	epireg.bits.dir	= dir;

	depreg.val = isp1581_inb(dma_ep);
	if (depreg.bits.epidx == endpoint) {
		isp1581_outb(dcmd_reset_dma, dma_cmd);
		depreg.bits.epidx = 1;
		isp1581_outb(depreg.val, dma_ep);
	}

	isp1581_outb(epireg.val, ep_index);

	creg.val	= 0;
	creg.bits.stall = 1;
	st.bits.state = st_stalled;
}
	
static u16 isp1581_read_reg(u8 reg)
{
	int timeout = TIMEOUT;
	unsigned long flags;
	u16 val;

	local_irq_save(flags);

	/* Perform a dummy read from the page frame first */
	val = epld_isp1581_regs(regs, (reg << 1));

	/* Wait till the read completes */
	while ((epld_isp1581_regs(micro_valid, 0) & isp1581_micro_if_active) &&
			timeout--);

	if (timeout < 0) {
		printk(KERN_ERR "%s: Read from register %02x timed out\n",
				__FUNCTION__, reg);
		val = 0xffff;
	} else {
		/* Read data from data port */
		val = epld_isp1581_regs(data, 0);
		//printk("%s: read %x from reg %x\n", __FUNCTION__, val, reg);
	}

	local_irq_restore(flags);
	return val;
}

static void isp1581_write_reg(u16 val, u8 reg)
{
	int timeout = TIMEOUT;
	unsigned long flags;

	local_irq_save(flags);

	//printk("%s: Writing %x to reg %x\n", __FUNCTION__, val, reg);
	/* Perform the write to the page frame first */
	epld_isp1581_regs(regs, (reg << 1)) = val;

	/* Wait till the write completes */
	while ((epld_isp1581_regs(micro_valid, 0) & isp1581_micro_if_active) &&
			timeout--);

	if (timeout < 0) {
		printk(KERN_ERR "%s: Write to register %02x timed out\n",
					__FUNCTION__, reg);
		val = 0xffff;
	} 

	local_irq_restore(flags);
	return;
}

static void isp1581_tx_timer(unsigned long endpoint)
{
	union isp1581_int_enable_reg ireg;

	ireg.val = isp1581_inl(intr_cr);
	if (ireg.bits.iebrst || ireg.bits.ieresm || 
			(ireg.val & (1 << (11 + endpoint*2)))) {
		enable_irq(INT_ISP1581);
		return;
	}
	tx_timer.expires = jiffies + 1;	/* 10 ms */
	tx_timer.data = endpoint;
	tx_timer.function = isp1581_tx_timer;
	add_timer(&tx_timer);
}

static int isp1581_wait_for_tx(int endpoint)
{
	union isp1581_int_enable_reg ireg;
	int timeout = 10;

	ireg.val = 0;
	while (!(ireg.val & (1 << (11 + endpoint*2))) && timeout--) {
		udelay(10);
		ireg.val = isp1581_inl(intr_cr);
		if (ireg.bits.iebrst || ireg.bits.ieresm)
			return 0;
	}

	if (timeout < 0) {
		/* Disable this interrupt */
		disable_irq(INT_ISP1581);

		/* Add a timer */
		tx_timer.expires = jiffies + 1;	/* 10 ms */
		tx_timer.data = endpoint;
		tx_timer.function = isp1581_tx_timer;
		add_timer(&tx_timer);
		return -1;
	}
	ireg.val = (1 << (11 + endpoint*2)); /* Clear EP intr bit */
	isp1581_outl(ireg.val, intr_cr);
	return 0;
}

static void isp1581_fill_idesc(struct usb_interface_descriptor *idesc)
{
	idesc->bLength			= IFACE_DESC_LENGTH;
	idesc->bDescriptorType		= USB_DT_INTERFACE;
	idesc->bInterfaceNumber		= 0;
	idesc->bAlternateSetting	= 0;
	idesc->bNumEndpoints		= 2;
	idesc->bInterfaceClass		= USB_CLASS_MASS_STORAGE;
	idesc->bInterfaceSubClass 	= 0x06;	/* No translation of mode 
						   sense/select */
	idesc->bInterfaceProtocol	= 0x50;	/* Bulk only */
	idesc->iInterface		= 0;	/* Index of interface string */
}

static void isp1581_fill_epdesc(struct usb_endpoint_descriptor *epdesc, int in)
{
	int len = (st.bits.speed == high_speed) ? 0x200 : 0x40;

	epdesc->bLength			= ENDPOINT_DESC_LENGTH;
	epdesc->bDescriptorType		= USB_DT_ENDPOINT;
	epdesc->bEndpointAddress	= in == 1 ? 0x82 : 0x02;
	epdesc->bmAttributes		= 0x02;
	epdesc-> wMaxPacketSize		= cpu_to_le16(len);
	epdesc->bInterval		= 0;
}

static void isp1581_send_status_common(struct usb_ctrlrequest *req, u16 val)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	/* Send data on EP0 */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	isp1581_outw(2, buffer_len);
	isp1581_outb((u8)val, data_port);
	isp1581_outb(0, data_port);

	isp1581_wait_for_tx(0);

	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	/* Send ACK on EP0 */
	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);

}

static void isp1581_do_vendor_class_req(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	/* Send data on EP0 */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	switch (req->bRequest) {
	case 0xfe:
		isp1581_outw(1, buffer_len);
		isp1581_outb(0, data_port);
		isp1581_wait_for_tx(0);

		epireg.bits.dir		= epdir_out;
		isp1581_outb(epireg.val, ep_index);

		/* Send ACK on EP0 */
		creg.val		= 0;
		creg.bits.status	= 1;
		isp1581_outb(creg.val, control);
		break;
	case 0xff:
		creg.val		= 0;
		creg.bits.status	= 1;
		isp1581_outb(creg.val, control);
		isp1581_wait_for_tx(0);
		break;
	default:
		printk("Unhandled Vendor/Class specific request %x\n",
				req->bRequest);
	}
}

static void isp1581_get_status(struct usb_ctrlrequest *req)
{
	/* See if the chip has been provided with an address */
	if (st.bits.state == st_addressed && req->wIndex != 0) {
		/* We have to stall */
		isp1581_stall_ep(0, epdir_in);
		return;
	}

	switch (req->bRequestType) {
	case 0x80:	/* Device Status */
		isp1581_send_status_common(req, 1);
		break;
	case 0x81:	/* Interface Status */
		isp1581_send_status_common(req, 0);
		break;
	case 0x82:	/* Endpoint Status */
		isp1581_send_status_common(req, (u16)st.bits.halt);
		break;
	default:
		printk("%s: USB_REQ_GET_STATUS: type %x not understood.\n",
				__FUNCTION__, req->bRequestType);
	}
}

static void isp1581_set_feature(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	/* Send data on EP0 */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	switch (st.bits.state) {
	case st_default:
		/* Stall */
		isp1581_stall_ep(0, epdir_in);
		break;
	case st_addressed:
	case st_configured:
		if (req->wLength != 0)
			break;
		if ((req->wIndex & 0xff) == 0) {
			/* Stall */
			isp1581_stall_ep(0, epdir_in);
			break;
		}
		switch (req->wValue) {
		case 0:	/* Endpoint Halt */
			st.bits.halt 		= 1;
			creg.val		= 0;
			creg.bits.status	= 1;
			isp1581_outb(creg.val, control);
			isp1581_wait_for_tx(0);
		default:
			/* Stall */
			isp1581_stall_ep(0, epdir_in);
			break;
		}
		break;
	}
}

static void isp1581_clear_feature(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;
	union isp1581_endpoint_type_reg eptreg;

	/* Send data on EP0 */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	creg.val		= 0;
	eptreg.val		= 0;
	
	switch (st.bits.state) {
	case st_addressed:
		if (req->wLength != 0)
			break;
		if (req->wIndex != 0) {
			/* Stall */
			isp1581_stall_ep(0, epdir_in);
			break;
		}
		switch (req->wValue) {
		case 0:	/* Endpoint Halt */
			st.bits.halt 		= 0;
			creg.bits.status	= 1;
			isp1581_outb(creg.val, control);
			isp1581_wait_for_tx(0);
		default:
			/* Stall */
			isp1581_stall_ep(0, epdir_in);
			break;
		}
		break;

	case st_configured:
		if (req->wLength != 0)
			break;

		epireg.bits.endpidx = 0x0f;
		if (req->wValue == 0) {
			/* Endpoint Halt */
			epireg.bits.endpidx	= req->wIndex & 0x0f;
			epireg.bits.dir		= req->wIndex & 0x80 ?
							epdir_in : epdir_out;
			if (epireg.bits.endpidx == 0 || 
					epireg.bits.endpidx == 2) {
				isp1581_outb(epireg.val, ep_index);
				
				/* Clear Stall */
				creg.val 	= isp1581_inb(control);
				creg.bits.stall	= 0;
				isp1581_outb(creg.val, control);

				/* Disable Endpoint */
				eptreg.val = isp1581_inw(ep_type);
				eptreg.bits.enable = 0;
				isp1581_outw(eptreg.val, ep_type);

				/* Enable Endpoint */
				eptreg.bits.enable = 1;
				isp1581_outw(eptreg.val, ep_type);

				epireg.bits.endpidx	= 0;
				epireg.bits.dir		= epdir_in;
				isp1581_outb(epireg.val, ep_index);
	
				creg.val		= 0;
				creg.bits.status 	= 1;
				isp1581_outb(creg.val, control);
				isp1581_wait_for_tx(0);
	
				st.bits.halt		= 0;
			} else {
				/* Stall */
				isp1581_stall_ep(0, epdir_in);
			}
		}
		break;

	default:
		/* Stall */
		isp1581_stall_ep(0, epdir_in);
		break;
	}
}

static void isp1581_set_address(struct usb_ctrlrequest *req)
{
	union isp1581_address_reg areg;
	union isp1581_control_reg creg;
	union isp1581_endpoint_index_reg epireg;

	if (req->wValue == 0)
		st.bits.state = st_default;
	else
		st.bits.state = st_addressed;
	areg.val		= isp1581_inb(address);
	areg.bits.devaddr	= req->wValue;
	isp1581_outb(areg.val, address);

	/* Send empty packet */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);

	isp1581_wait_for_tx(0);
}

static void isp1581_send_device_desc(struct usb_ctrlrequest *req)
{
	struct usb_device_descriptor ddesc;
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;
	int len;

	/* Send data on EP0 */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	ddesc.bLength 			= sizeof(ddesc);
	ddesc.bDescriptorType		= USB_DT_DEVICE;
	ddesc.bcdUSB			= st.bits.speed == high_speed ? 
					  cpu_to_le16(0x0200) : /* USB 2.0 */
					  cpu_to_le16(0x0110);  /* USB 1.1 */
	ddesc.bDeviceClass		= 0;
	ddesc.bDeviceSubClass		= 0;
	ddesc.bDeviceProtocol		= 0;
	ddesc.bMaxPacketSize0		= 0x40;
	ddesc.idVendor			= cpu_to_le16(1137);
	ddesc.idProduct			= cpu_to_le16(0x7752);
	ddesc.bcdDevice			= cpu_to_le16(0x0100);
	ddesc.iManufacturer		= 1;
	ddesc.iProduct			= 2;
	ddesc.iSerialNumber		= 3;
	ddesc.bNumConfigurations	= 1;

	len = req->wLength < sizeof(ddesc) ? req->wLength : sizeof(ddesc);
	isp1581_outw((u16)len, buffer_len);
	isp1581_send_data((u8 *)&ddesc, len);

	isp1581_wait_for_tx(0);

	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	/* Send ACK on EP0 */
	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);
}

static void isp1581_send_config_desc(struct usb_ctrlrequest *req)
{
	struct usb_config_descriptor cdesc;
	struct usb_endpoint_descriptor epdesc;
	struct usb_interface_descriptor idesc;
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;
	int i, len, save_speed = 0, os;
	char buf[32];

	os = (req->wValue >> 8) == USB_DT_OTHER_SPEED ? 1 : 0;

	/* Send data on EP0 */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	cdesc.bLength 		= CONFIG_DESC_LENGTH;
	cdesc.bDescriptorType	= (req->wValue >> 8);
	cdesc.wTotalLength	= CONFIG_DESC_LENGTH + 
				(HAS7752_USBDEV_IFACES * IFACE_DESC_LENGTH) +
				(HAS7752_USBDEV_ENDPOINTS *
				 		ENDPOINT_DESC_LENGTH);
	cdesc.bNumInterfaces	= 1;
	cdesc.bConfigurationValue = os ? 2 : 1;
	cdesc.iConfiguration	= os ? HAS7752_CONFIG2_INDEX : 
						HAS7752_CONFIG1_INDEX;
	cdesc.bmAttributes	= 0xc0;
	cdesc.MaxPower		= 0x32;

	len = req->wLength < CONFIG_DESC_LENGTH ? req->wLength : 
						CONFIG_DESC_LENGTH;
	memcpy(buf, &cdesc, len);
	i = len;

	if (req->wLength > CONFIG_DESC_LENGTH) {
		/* Fill endpoint and interface descriptors */
		isp1581_fill_idesc(&idesc);

		req->wLength -= len;
		len = req->wLength < IFACE_DESC_LENGTH ? req->wLength : 
					IFACE_DESC_LENGTH;
		memcpy(&buf[i], &idesc, len);
		i += len;

		/* Cheat a little and set speed to full_speed for Other
		 * Speed Config Descriptor.
		 */
		if (os) {
			save_speed = st.bits.speed;
			st.bits.speed = full_speed;
		}

		/* Bulk IN endpoint */
		isp1581_fill_epdesc(&epdesc, 1);
		req->wLength -= len;
		len = req->wLength < ENDPOINT_DESC_LENGTH ? req->wLength :
					ENDPOINT_DESC_LENGTH;
		memcpy(&buf[i], &epdesc, len);
		i += len;

		/* Bulk OUT endpoint */
		isp1581_fill_epdesc(&epdesc, 0);
		req->wLength -= len;
		len = req->wLength < ENDPOINT_DESC_LENGTH ? req->wLength :
					ENDPOINT_DESC_LENGTH;
		memcpy(&buf[i], &epdesc, len);
		i += len;

		/* Reset speed */
		if (os) 
			st.bits.speed = save_speed;
	}

	/* Set buffer length */
	isp1581_outw(i, buffer_len);
	isp1581_send_data((u8 *)buf, i);
	isp1581_wait_for_tx(0);

	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	/* Send ACK on EP0 */
	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);
}

static void isp1581_send_string_desc(struct usb_ctrlrequest *req)
{
	int index = req->wValue & 0xff, i;
	struct usb_string_descriptor sdesc;
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;
	u8 *ptr = NULL;

	epireg.val = 0;
	epireg.bits.dir		= epdir_in;
	epireg.bits.endpidx	= 0;
	isp1581_outb(epireg.val, ep_index);

	creg.val 		= 0;
	creg.bits.status	= 1;

	sdesc.bLength		= sizeof(sdesc.bLength) + 
					sizeof(sdesc.bDescriptorType);
	sdesc.bDescriptorType	= USB_DT_STRING;
	sdesc.wData[0]		= 0x0;	
	
	switch (index) {
	case 0:
		sdesc.bLength	+= 2;
		ptr = (u8 *)sdesc.wData;
		break;
	case HAS7752_MANF_INDEX:
		sdesc.bLength	+= strlen(MANF_ID_STRING) * 2;
		ptr		= MANF_ID_STRING;
		break;
	case HAS7752_PROD_INDEX:
		sdesc.bLength	+= strlen(PROD_ID_STRING) * 2;
		ptr		= PROD_ID_STRING;
		break;
	case HAS7752_SERIAL_INDEX:
		sdesc.bLength	+= strlen(SERIAL_ID_STRING) * 2;
		ptr		= SERIAL_ID_STRING;
		break;
	case HAS7752_CONFIG1_INDEX:
		ptr = st.bits.speed == high_speed ? CONFIG1_ID_STRING_HS :
				CONFIG1_ID_STRING_FS;
		sdesc.bLength	+= strlen(ptr) * 2;
		break;
	case HAS7752_CONFIG2_INDEX:
		sdesc.bLength	+= strlen(CONFIG2_ID_STRING) * 2;
		ptr		= CONFIG2_ID_STRING;
		break;
	}

	if (sdesc.bLength > req->wLength)
		sdesc.bLength = req->wLength;
	isp1581_outw((u16)sdesc.bLength, buffer_len);

	isp1581_outb(sdesc.bLength, data_port);
	isp1581_outb(sdesc.bDescriptorType, data_port);

	sdesc.bLength -= 2;

	if (ptr != NULL) {
		for (i = 0; i < sdesc.bLength; i++) {
			isp1581_outb(ptr[i], data_port);
			isp1581_outb(0, data_port);
		}
	}
	isp1581_wait_for_tx(0);

	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	/* Send ACK on EP0 */
	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);
}

static void isp1581_send_endpoint_desc(struct usb_ctrlrequest *req)
{
	struct usb_endpoint_descriptor epdesc;
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;
	int len;
	char buf[ENDPOINT_DESC_LENGTH*2];

	if (req->wValue > 1) {
		/* Stall */
		isp1581_stall_ep(0, epdir_in);
		return;
	}

	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	/* Bulk IN Endpoint */
	isp1581_fill_epdesc(&epdesc, 1);
	memcpy(buf, &epdesc, ENDPOINT_DESC_LENGTH);

	/* Bulk OUT Endpoint */
	isp1581_fill_epdesc(&epdesc, 0);
	memcpy(&buf[ENDPOINT_DESC_LENGTH], &epdesc, ENDPOINT_DESC_LENGTH);
	len = ENDPOINT_DESC_LENGTH * 2;

	if (len > req->wLength)
		len = req->wLength;

	/* Set buffer length */
	isp1581_outw(len, buffer_len);
	isp1581_send_data((u8 *)buf, len);
	isp1581_wait_for_tx(0);

	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	/* Send ACK on EP0 */
	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);
}

static void isp1581_send_dev_qual_desc(struct usb_ctrlrequest *req)
{
	struct usb_device_qualifier_descriptor dqdesc;
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;
	int len;

	epireg.val		= 0;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	dqdesc.bLength		= sizeof(dqdesc);
	dqdesc.bDescriptorType	= USB_DT_DEV_QUALIFIER;
	dqdesc.wVersion		= cpu_to_le16(0x200); /* USB 2.0 */
	dqdesc.bDeviceClass	= 0;
	dqdesc.bDeviceSubClass	= 0;
	dqdesc.bProtocol	= 0;
	dqdesc.bMaxPktSize	= 0x40;
	dqdesc.bOtherConfig	= 1;
	dqdesc.bReserved	= 0;
	
	len = (req->wLength < sizeof(dqdesc)) ? req->wLength : sizeof(dqdesc);
	isp1581_outw(len, buffer_len);
	isp1581_send_data((u8 *)&dqdesc, len);
	isp1581_wait_for_tx(0);

	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	/* Send ACK on EP0 */
	creg.val		= 0;
	creg.bits.status	= 1;
	isp1581_outb(creg.val, control);
}

static void isp1581_get_config(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	switch (st.bits.state) {
	case st_default:
		break;
	case st_addressed:
	case st_configured:
		epireg.val		= 0;
		epireg.bits.dir		= epdir_in;
		isp1581_outb(epireg.val, ep_index);
		isp1581_outw(1, buffer_len);
		isp1581_outb(st.bits.state == st_addressed ? 0 :
				isp1581_config_value, data_port);
		isp1581_wait_for_tx(0);
		epireg.bits.dir		= epdir_out;
		isp1581_outb(epireg.val, ep_index);
		/* Send ACK on EP0 */
		creg.val		= 0;
		creg.bits.status	= 1;
		isp1581_outb(creg.val, control);
		break;
	}
}

static void isp1581_set_config(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	epireg.val = 0;
	epireg.bits.dir		= epdir_in;
	epireg.bits.endpidx	= 0;
	isp1581_outb(epireg.val, ep_index);

	creg.val 		= 0;
	creg.bits.status	= 1;
	switch(st.bits.state) {
	case st_default:
		break;
	case st_addressed:
	case st_configured:
		if (req->wValue == 0) {
			if (st.bits.state == st_configured)
				st.bits.state = st_addressed;
			isp1581_config_value = req->wValue;
			/* Send ACK */
			isp1581_outb(creg.val, control);
			isp1581_wait_for_tx(0);
		} else if (req->wValue == 0x01 || req->wValue == 0x02) {
			isp1581_config_value = req->wValue;
			st.bits.state = st_configured;
			/* Send ACK */
			isp1581_outb(creg.val, control);
			isp1581_wait_for_tx(0);
		} else {
			/* Stall */
			isp1581_stall_ep(0, epdir_in);
		}
		break;
	}
}

static void isp1581_get_iface(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	epireg.val = 0;
	epireg.bits.dir		= epdir_in;
	epireg.bits.endpidx	= 0;
	isp1581_outb(epireg.val, ep_index);

	switch (st.bits.state) {
	case st_default:
	case st_addressed:
		/* Stall */
		isp1581_stall_ep(0, epdir_in);
		return;
	case st_configured:
		if (req->wValue != 0) {
			/* Stall */
			isp1581_stall_ep(0, epdir_in);
		} else {
			isp1581_outw(1, buffer_len);
			isp1581_outb((u8)st.bits.iface, data_port);
			isp1581_wait_for_tx(0);
			epireg.bits.dir		= epdir_out;
			isp1581_outb(epireg.val, ep_index);
			/* Send ACK on EP0 */
			creg.val		= 0;
			creg.bits.status	= 1;
			isp1581_outb(creg.val, control);
		}
		break;
	}
}

static void isp1581_set_iface(struct usb_ctrlrequest *req)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_control_reg creg;

	epireg.val = 0;
	epireg.bits.dir		= epdir_in;
	epireg.bits.endpidx	= 0;
	isp1581_outb(epireg.val, ep_index);

	switch (st.bits.state) {
	case st_default:
	case st_addressed:
		/* Stall */
		isp1581_stall_ep(0, epdir_in);
		return;
	case st_configured:
		if (req->wIndex == 0 && req->wValue == 1)
			st.bits.iface = 1;
		else if (req->wIndex == 1 && req->wValue == 0)
			st.bits.iface = 0;
		else if (req->wIndex == 0 && req->wValue == 0)
			st.bits.iface = 0;
		/* Send ACK on EP0 */
		creg.val		= 0;
		creg.bits.status	= 1;
		isp1581_outb(creg.val, control);
		isp1581_wait_for_tx(0);
	}
}

static void isp1581_do_usb_setup(void)
{
	struct usb_ctrlrequest req;
	union isp1581_endpoint_index_reg epireg;

	/* User Endpoint 0 */
	epireg.val		= 0;
	epireg.bits.ep0setup	= 1;
	isp1581_outb(epireg.val, ep_index);

	isp1581_recv_data((u8 *)&req, sizeof(req));
	req.wValue	= le16_to_cpu(req.wValue);
	req.wIndex	= le16_to_cpu(req.wIndex);
	req.wLength	= le16_to_cpu(req.wLength);

#if 0	
	printk("USB req received: bRequestType = %02x, bRequest = %02x\n",
			req.bRequestType, req.bRequest);
	printk("wValue = %04x, wIndex = %04x, wLength = %04x\n",
			req.wValue, req.wIndex, req.wLength);
#endif

	if (req.bRequestType == 0xc0 || req.bRequestType == 0x40 ||
			req.bRequestType == 0xA1 || req.bRequestType == 0x21) {
		/* Vendor specific or class specific requests */
		isp1581_do_vendor_class_req(&req);
		return;
	}

	switch (req.bRequest) {
	case USB_REQ_GET_STATUS:
		isp1581_get_status(&req);
		break;
	case USB_REQ_CLEAR_FEATURE:
		isp1581_clear_feature(&req);
		break;
	case USB_REQ_SET_FEATURE:
		isp1581_set_feature(&req);
		break;
	case USB_REQ_SET_ADDRESS:
		isp1581_set_address(&req);
		break;
	case USB_REQ_GET_DESCRIPTOR:
		switch (req.wValue >> 8) {
		case USB_DT_DEVICE:
			isp1581_send_device_desc(&req);
			break;
		case USB_DT_CONFIG:
			isp1581_send_config_desc(&req);
			break;
		case USB_DT_STRING:
			isp1581_send_string_desc(&req);
			break;
		case USB_DT_ENDPOINT:
			isp1581_send_endpoint_desc(&req);
			break;
		case USB_DT_DEV_QUALIFIER:
			isp1581_send_dev_qual_desc(&req);
			break;
		case USB_DT_OTHER_SPEED:
			isp1581_send_config_desc(&req);
			break;
		case USB_DT_INTERFACE:
		case USB_DT_IFACE_POWER:
		default:
			printk("%s: USB_REQ_GET_DESCRIPTOR: type %x not "
					"implemented.\n", __FILE__, req.wValue);
			panic("Unsupported USB command");
			break;
		}
		break;
	case USB_REQ_GET_CONFIGURATION:
		isp1581_get_config(&req);
		break;
	case USB_REQ_SET_CONFIGURATION:
		isp1581_set_config(&req);
		break;
	case USB_REQ_GET_INTERFACE:
		isp1581_get_iface(&req);
		break;
	case USB_REQ_SET_INTERFACE:
		isp1581_set_iface(&req);
		break;
	case USB_REQ_SET_DESCRIPTOR:
	default:
		printk("%s: Request %x not implemented.\n", __FILE__, 
				req.bRequest);
		panic("Unsupported USB command");
		break;
	}
}

#ifdef CHECK_DRQ
static void isp1581_wait_for_drq(void)
{
	union isp1581_dma_int_enable_reg diereg;
	int timeout = 1000;

	diereg.val		= isp1581_inw(dma_intr_enable);
	diereg.bits.bsy_done	= 1;
	isp1581_outw(diereg.val, dma_intr_enable);

	isp1581_outb(dcmd_poll_bsy, dma_cmd);
	do {
		diereg.val = isp1581_inw(dma_intr_cause);
		udelay(10);
	} while (!diereg.bits.bsy_done);

	do {
		timeout = 10;
		/* Read taskfiles to check status */
		isp1581_outb(dcmd_read_tf, dma_cmd);
		do {
			diereg.val = isp1581_inw(dma_intr_cause);
			udelay(10);
		} while (!diereg.bits.tf_rd_done && timeout--);
	
		if (timeout < 0) {
			printk("%s: ATA Read Taskfile Command timed out.\n", 
					__FUNCTION__);
			break;
		}
	} while (!(isp1581_inb(tf_control) & DRQ_STAT));

	isp1581_outw(diereg.val, dma_intr_cause);

	diereg.val		= isp1581_inw(dma_intr_enable);
	diereg.bits.bsy_done	= 0;
	isp1581_outw(diereg.val, dma_intr_enable);
}
#endif

static void isp1581_do_48bit_cmd(struct isp1581_cbw *cbw, u16 nsectors, u32 lba)
{
	u16 cmd;

	switch (cbw->bCBWCB[0]) {
	case READ_10:
		cmd = WIN_READ_EXT;
		break;
	case WRITE_10:
		cmd = WIN_WRITE_EXT;
		break;
	case VERIFY:
		cmd = WIN_VERIFY_EXT;
		break;
	default:
		return;
	}

	isp1581_outb((u8)nsectors, tf_nsector);
	isp1581_outb((u8)lba, tf_sector);
	isp1581_outb((u8)(lba>>8), tf_lcyl);
	isp1581_outb((u8)(lba>>16), tf_hcyl);
	isp1581_outb(0xa0 | 0x40 | (u8)((lba>>24) & 0xf), tf_select);

	epld_isp1581_regs(epld_nsector, 0) 	= (u8)(nsectors >> 8);
	epld_isp1581_regs(epld_nsector, 0) 	= (u8)nsectors;
	epld_isp1581_regs(epld_hcyl, 0)		= 0;	/* LBA[47:40] */
	epld_isp1581_regs(epld_lcyl, 0)		= 0;	/* LBA[39:32] */
	epld_isp1581_regs(epld_sector, 0)	= (u8)(lba >> 24); /* [31:24] */
	epld_isp1581_regs(epld_hcyl, 0)		= (u8)(lba >> 16); /* [23:16] */
	epld_isp1581_regs(epld_lcyl, 0)		= (u8)(lba >> 8);  /* [15:8] */
	epld_isp1581_regs(epld_sector, 0)	= (u8)(lba); 	   /* [7:0] */
	epld_isp1581_regs(epld_select, 0)	= 0xa0 | 0x40;

	if (epld_isp1581_regs(fifo_count, 0) != 0)
		panic("FIFO count != 0");

#ifdef DUMP_FIFO
	epld_isp1581_regs(fifo_reset, 0) = 0x02;
	*(u16 *)(IO_ADDRESS_ATA_REGS_BASE + 0x06) = 0;
#endif
	/* Send command */
	isp1581_outb(cmd, tf_cmd_status);
	epld_isp1581_regs(epld_cmd_status, 0) = cmd;
}

static void isp1581_do_28bit_cmd(struct isp1581_cbw *cbw, u16 nsectors, u32 lba)
{
	u16 cmd;

	switch (cbw->bCBWCB[0]) {
	case READ_10:
		cmd = WIN_READ;
		break;
	case WRITE_10:
		cmd = WIN_WRITE;
		break;
	case VERIFY:
		cmd = WIN_VERIFY;
		break;
	default:
		return;
	}

	if (nsectors == 256)
		nsectors = 0;

	isp1581_outb((u8)nsectors, tf_nsector);
	isp1581_outb((u8)lba, tf_sector);
	isp1581_outb((u8)(lba>>8), tf_lcyl);
	isp1581_outb((u8)(lba>>16), tf_hcyl);
	isp1581_outb(0xa0 | 0x40 | (u8)((lba>>24) & 0xf), tf_select);

	epld_isp1581_regs(epld_nsector, 0) 	= nsectors;
	epld_isp1581_regs(epld_sector, 0)	= (u8)lba;
	epld_isp1581_regs(epld_lcyl, 0)		= (u8)(lba>>8);
	epld_isp1581_regs(epld_hcyl, 0)		= (u8)(lba>>16);
	epld_isp1581_regs(epld_select, 0)	= 0xa0 | 0x40 | 
							(u8)((lba>>24) & 0xf);

#ifdef DUMP_FIFO
	epld_isp1581_regs(fifo_reset, 0) = 0x02;
	*(u16 *)(IO_ADDRESS_ATA_REGS_BASE + 0x06) = 0;
#endif
	/* Send command */
	isp1581_outb(cmd, tf_cmd_status);
	epld_isp1581_regs(epld_cmd_status, 0) = cmd;
}

static void isp1581_do_csw_status(struct isp1581_cbw *cbw, int len, int status)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_dma_ep_reg depreg;
	struct isp1581_csw csw;

	/* Sanity check */
	if (cbw->dCBWDataTransferLen < len)
		BUG();
	depreg.val		= 0;
	depreg.bits.epidx	= 1;
	depreg.bits.dmadir	= epdir_in;
	isp1581_outb(depreg.val, dma_ep);

	epireg.val		= 0;
	epireg.bits.endpidx	= 2;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	csw.dCSWSignature	= cpu_to_le32(CSW_SIG);
	csw.dCSWTag		= cpu_to_le32(cbw->dCBWTag);
	csw.dCSWDataResidue	= cpu_to_le32(cbw->dCBWDataTransferLen - len);
	csw.bCSWStatus		= status;

	isp1581_outw(sizeof(csw), buffer_len);
	isp1581_send_data((u8 *)&csw, sizeof(csw));
	isp1581_wait_for_tx(2);
}

static void isp1581_handle_verify(void)
{
	struct isp1581_cbw *cbw = &cur_cbw;

	isp1581_do_csw_status(cbw, 0, 
		(epld_isp1581_regs(epld_control, 0) & ERR_STAT) ? 1 : 0);
	memset(&ec, 0, sizeof(ec));
	memset(cbw, 0, sizeof(*cbw));
}

static void isp1581_handle_data_transfer(void)
{
	struct isp1581_cbw *cbw = &cur_cbw;
	union isp1581_int_enable_reg iereg;
	int count;
	u16 sectors;

	/* Make sure that the DMA count is zero */
#if 1
	count = isp1581_inl(dma_count);
	if (count != 0) 
		panic("DMA not complete, count = %d.\n", count);
#else
	do {
		count = isp1581_inl(dma_count);
	} while (count != 0);
#endif

	if (epld_isp1581_regs(fifo_count, 0) != 0)
		panic("FIFO count != 0 and DMA count is 0");

#ifdef DUMP_FIFO
	if (!cbw->bmCBWFlags) {
		printk("EPLD FIFO: ");
		for (sectors = 0; sectors < 16; sectors += 2) {
			u16 data = *(u16 *)(IO_ADDRESS_ATA_BUF_BASE+sectors);
			printk("%02x %02x ", data & 0xff, data >> 8);
		}
		printk("\n");
	}
#endif

	sectors = cbw->bCBWCB[7] << 8 | cbw->bCBWCB[8];
#ifdef ONE_SECTOR_PER_COMMAND
	if (--sectors) {
		u32 lba;
		
		memcpy(&lba, &cbw->bCBWCB[2], sizeof(lba));

		cbw->bCBWCB[7] = (u8)(sectors >> 8);
		cbw->bCBWCB[8] = (u8)(sectors & 0xff);

		isp1581_do_ata_cmd(cbw, 1, ++lba);

#ifdef CHECK_DRQ
		isp1581_wait_for_drq();
#endif
		memcpy(&cbw->bCBWCB[2], &lba, sizeof(lba));
		cur_cbw = *cbw;

		isp1581_do_data_transfer();

		return;
	}
#endif
	/* Transfer done */
	//printk("Signalling successful transfer of %d bytes\n",
	//		sectors * 512);
	count = 0;
	if (epld_isp1581_regs(epld_control, 0) & (ERR_STAT | WRERR_STAT)) {
		count = 1;
		/* Set Error Sense here */
	} else {
		memset(&ec, 0, sizeof(ec));
	}

	/* Turn on EP2 TX/RX interrupts */
	iereg.val		= isp1581_inl(intr_enable);
	iereg.bits.iep2rx	= 1;
	iereg.bits.iep2tx 	= 1;
	isp1581_outw(iereg.val, intr_enable);

#ifdef ONE_SECTOR_PER_COMMAND
	isp1581_do_csw_status(cbw, sector_count * SECTOR_SIZE, count);
#else
	isp1581_do_csw_status(cbw, sectors * SECTOR_SIZE, count);
#endif
	memset(cbw, 0, sizeof(*cbw));
}

static void isp1581_do_data_transfer(void)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_int_enable_reg iereg;
	union isp1581_dma_ep_reg depreg;
	struct isp1581_cbw *cbw = &cur_cbw;
	u16 sectors;

	sectors = cbw->bCBWCB[7] << 8 | cbw->bCBWCB[8];

	epireg.val		= 0;
#if 0
	epireg.bits.endpidx	= 2;
	epireg.bits.dir		= cbw->bmCBWFlags ? epdir_in : epdir_out;
	isp1581_outb(epireg.val, ep_index);

	if (cbw->bmCBWFlags)
		isp1581_outw(st.bits.speed == high_speed ? 512 : 64,
				buffer_len);

	if (cbw->bmCBWFlags == 0) {
		int i;

		depreg.val = isp1581_inb(dma_ep);
		printk("DMA EP = 0x%02x, buffer_len = %d\n", depreg.val,
				isp1581_inw(buffer_len));
		for (i = 0; i < 16; i++)
			printk("0x%02x ", isp1581_inb(data_port));
		printk("\n");
		for (i = 0; i < sectors*512-16; i++)
			isp1581_inb(data_port);
		isp1581_do_csw_status(cbw, sectors*512, 0);
		return;
	}
	//printk("Buffer length is %d\n", isp1581_inw(buffer_len));
#endif

	/* Read/Write data from/to Host */
	epireg.bits.endpidx	= 3;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	depreg.val		= 0;
	depreg.bits.dmadir 	= cbw->bmCBWFlags ? epdir_in : epdir_out;
	depreg.bits.epidx 	= 2;
	isp1581_outb(depreg.val, dma_ep);

	/* Transfer data to/from drive */
#ifdef ONE_SECTOR_PER_COMMAND
	isp1581_outl(512, dma_count);
#else
	isp1581_outl(sectors*512, dma_count);
#endif
	/* Disable EP2RX & EP2TX interrupt */
	iereg.val		= isp1581_inl(intr_enable);
	iereg.bits.iep2rx	= 0;
	iereg.bits.iep2tx	= 0;
	isp1581_outw(iereg.val, intr_enable);
	
	/* Send DMA command */
	isp1581_outb(cbw->bmCBWFlags ? dcmd_pio_read : dcmd_pio_write, dma_cmd);

	/* Set handler to handle data transfer completetion interrupt. */
	cur_cbw = *cbw;
	isp1581_ata_handler = isp1581_handle_data_transfer;
}

static void isp1581_do_mode_sense(struct isp1581_cbw *cbw)
{
	char page_code = cbw->bCBWCB[2] & 0x3f;
	struct isp1581_mode_param_hdr mph;
	struct isp1581_timer_protect_page tpp;
	union isp1581_mode_param_hdr_wp wp;

	switch (page_code) {
	case 0x1c:
	case 0x3f:

		memset(&mph, 0, sizeof(mph));
		memset(&tpp, 0, sizeof(tpp));

		mph.mdl 	= cpu_to_be16(sizeof(mph) - sizeof(mph.mdl) + 
					sizeof(tpp));
		mph.mtc		= 0;	/* Currently mounted medium */
		wp.val		= 0;
		mph.wp		= wp.val;
		
		tpp.page_code	= 0x1c;
		tpp.page_len	= 0x06;
		tpp.timer	= 0;	/* Device remains in current state
					 * infinitely after a seek.
					 */

		isp1581_outw(sizeof(mph) + sizeof(tpp), buffer_len);
		isp1581_send_data((u8 *)&mph, sizeof(mph));
		isp1581_send_data((u8 *)&tpp, sizeof(tpp));
		isp1581_wait_for_tx(2);
		isp1581_do_csw_status(cbw, sizeof(mph) + sizeof(tpp), 0);
		break;
	default:
		isp1581_do_csw_status(cbw, 0, 1);
		break;
	}
}

static void isp1581_do_cdb(struct isp1581_cbw *cbw)
{
	union isp1581_endpoint_index_reg	epireg;
	struct isp1581_inquiry_data		id;
	struct isp1581_capacity_list_header	clh;
	struct isp1581_capacity_desc		cd;
	struct isp1581_read_capacity_data	rcd;
	struct isp1581_request_sense_data	rsd;
	union isp1581_inquiry_data_vers		vers;
	union isp1581_request_sense_errcode	errcode;
	int 					i;
	u32 					lba;

	epireg.val		= 0;
	epireg.bits.endpidx	= 2;
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	/* This is basically an CDB to ATA command translator */
	switch (cbw->bCBWCB[0]) {
	case INQUIRY:
		//printk("CDB Command: INQUIRY\n");
		id.pdt		= 0;	/* Direct access device */
		id.rmb		= 0;	/* This is non-removable media */
		vers.val	= 0;	/* All versions set to 0 */
		id.vers		= vers.val;
		id.rdf		= 1;	/* Always set to 1 */
		id.add_len	= 31;	/* Always set to 31 */
		id.reserved[0]	= 0;
		id.reserved[1]	= 0;
		memset(&id.vendor, 0x20, sizeof(id.vendor) + 
						sizeof(id.product));

		i = strlen(MANF_ID_STRING);
		i = i < sizeof(id.vendor) ? i : sizeof(id.vendor);
		memcpy(id.vendor, MANF_ID_STRING, i);

		i = strlen(PROD_ID_STRING);
		i = i < sizeof(id.product) ? i : sizeof(id.product);
		memcpy(id.product, PROD_ID_STRING, i);
		
		i = strlen(PROD_VERS_STRING);
		i = i < sizeof(id.version) ? i : sizeof(id.version);
		memcpy(id.version, PROD_VERS_STRING, i);

		isp1581_outw(sizeof(id), buffer_len); 
		isp1581_send_data((u8 *)&id, sizeof(id));
		isp1581_wait_for_tx(2);

		isp1581_do_csw_status(cbw, sizeof(id), 0);
		break;

	case READ_FORMAT_CAPACITIES:		
		//printk("CDB Command: READ_FORMAT_CAPACITIES\n");
		clh.len		= sizeof(cd);
		cd.blocks	= cpu_to_be32(drive_capacity);
		cd.desc_code	= 0x02;	/* Formatted media */
		cd.blen_high	= 0x00;
		cd.blen_low	= cpu_to_be16(SECTOR_SIZE);

		isp1581_outw(sizeof(clh) + sizeof(cd), buffer_len);
		isp1581_send_data((u8 *)&clh, sizeof(clh));
		isp1581_send_data((u8 *)&cd, sizeof(cd));
		isp1581_wait_for_tx(2);
		isp1581_do_csw_status(cbw, sizeof(clh) + sizeof(cd), 0);
		break;

	case READ_CAPACITY:
		//printk("CDB Command: READ_CAPACITY\n");

		rcd.last_lba	= cpu_to_be32(drive_capacity - 1);
		rcd.blen	= cpu_to_be32(SECTOR_SIZE);
		
		isp1581_outw(sizeof(rcd), buffer_len);
		isp1581_send_data((u8 *)&rcd, sizeof(rcd));
		isp1581_wait_for_tx(2);
		isp1581_do_csw_status(cbw, sizeof(rcd), 0);
		break;

	case READ_10:
	case WRITE_10:
		//printk("CDB Command: %s ", cbw->bCBWCB[0] == READ_10 ? 
		//			"READ_10" : "WRITE_10"); 
		i = cbw->bCBWCB[7] << 8 | cbw->bCBWCB[8];
		lba = (cbw->bCBWCB[2] << 24) | (cbw->bCBWCB[3] << 16) | 
			(cbw->bCBWCB[4] << 8) | (cbw->bCBWCB[5] << 0);
		//printk(" sector count: %d, lba: %d\n", i, lba);
		
		/* Setup Taskfile and send command */
#ifdef ONE_SECTOR_PER_COMMAND
		sector_count = i;
		isp1581_do_ata_cmd(cbw, 1, lba);
		memcpy(&cbw->bCBWCB[2], &lba, sizeof(lba));
#else
		isp1581_do_ata_cmd(cbw, i, lba);
#endif

#ifdef CHECK_DRQ
		isp1581_wait_for_drq();
#endif
		cbw->bCBWCB[7] = (u8)(i >> 8);
		cbw->bCBWCB[8] = (u8)(i & 0xff);

		cur_cbw = *cbw;

		isp1581_do_data_transfer();
		break;

	case REQUEST_SENSE:
		//printk("CDB Command: REQUEST_SENSE\n");

		memset(&rsd, 0, sizeof(rsd));
		errcode.val		= 0;
		errcode.bits.code	= 0x70;
		errcode.bits.valid	= 1;	/* Info field is valid */
		rsd.errcode		= errcode.val;
		rsd.sense_key		= ec.sense_key;
		rsd.info1		= 0;
		rsd.info2		= 0;
		rsd.info3		= 0;
		rsd.info4		= 0;
		rsd.add_len		= 10;
		rsd.asc			= ec.asc;
		rsd.ascq		= ec.ascq;

		isp1581_outw(sizeof(rsd), buffer_len);
		isp1581_send_data((u8 *)&rsd, sizeof(rsd));
		isp1581_wait_for_tx(2);
		isp1581_do_csw_status(cbw, sizeof(rsd), 0);
		break;

	case TEST_UNIT_READY:				/* Test Unit Ready */
		//printk("CDB Command: TEST_UNIT_READY\n");
		memset(&ec, 0, sizeof(ec));
		isp1581_do_csw_status(cbw, 0, 0);
		break;

	case MODE_SENSE:
		//printk("CDB Command: MODE_SENSE page code %#x\n", 
		//					cbw->bCBWCB[2]);
		isp1581_do_mode_sense(cbw);
		break;

	case START_STOP:
		//printk("CDB Command: START_STOP\n");
		ec.sense_key	= ILLEGAL_REQUEST;
		ec.asc		= 0x24;
		ec.ascq		= 0;
		isp1581_do_csw_status(cbw, 0, 1);
		break;

	case VERIFY:
		//printk("CDB Command: VERIFY\n");
		i = cbw->bCBWCB[7] << 8 | cbw->bCBWCB[8];
		lba = (cbw->bCBWCB[2] << 24) | (cbw->bCBWCB[3] << 16) | 
			(cbw->bCBWCB[4] << 8) | (cbw->bCBWCB[5] << 0);
		
		//printk(" sector count: %d, lba: %d\n", i, lba);

		cur_cbw = *cbw;
		isp1581_ata_handler = isp1581_handle_verify;

		/* Setup Taskfile and send command */
		isp1581_do_ata_cmd(cbw, i, lba);
		break;

	case ALLOW_MEDIUM_REMOVAL:
		//printk("CDB Command: ALLOW_MEDIUM_REMOVAL\n");
		memset(&ec, 0, sizeof(ec));
		isp1581_do_csw_status(cbw, 0, 0);
		break;

	default:
		printk("CDB command: UNKNOWN (%#x)\n", cbw->bCBWCB[0]);
		ec.sense_key	= ILLEGAL_REQUEST;
		ec.asc		= 0x24;
		ec.ascq		= 0;
		isp1581_do_csw_status(cbw, 0, 1);
	}
}

static void isp1581_do_ep2rx(void)
{
	int count;
	union isp1581_endpoint_index_reg epireg;
	union isp1581_dma_ep_reg depreg;
	struct isp1581_cbw cbw;
	unsigned long flags;
	static int max_tsc;

	/* First see if a DMA operation is in progress */
	depreg.val = isp1581_inb(dma_ep);
	if (depreg.bits.epidx == 2)
		return;

	/* See how much data is present on EP2 */
	epireg.val		= 0;
	epireg.bits.endpidx	= 2;
	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	count = isp1581_inw(buffer_len);
	if (count == sizeof(cbw)) {
		int tsc = ReadTSC();
		local_irq_save(flags);
		isp1581_recv_data((u8 *)&cbw, count);

		cbw.dCBWSignature	= le32_to_cpu(cbw.dCBWSignature);
		cbw.dCBWTag		= le32_to_cpu(cbw.dCBWTag);
		cbw.dCBWDataTransferLen	= le32_to_cpu(cbw.dCBWDataTransferLen);
		
#if 0
		do {
			int i;
		printk("CBW Signature: %#x, CBW TAG: %#x, CBW length: %d\n",
				cbw.dCBWSignature, cbw.dCBWTag, 
				cbw.dCBWDataTransferLen);
		printk("CBW Flags: %#x, CBW LUN: %#x, CDB length: %d\n",
				cbw.bmCBWFlags, cbw.bCBWLun, cbw.bCBWCBLength);
		printk("CBW_CDB: ");
		for (i = 0; i < cbw.bCBWCBLength; i++) 
			printk("%#x ", cbw.bCBWCB[i]);
		printk("\n");
		} while (0);
#endif

		if (cbw.dCBWSignature != CBW_SIG) {
			isp1581_stall_ep(2, epdir_in);
			isp1581_stall_ep(2, epdir_out);
			local_irq_restore(flags);
			return;
		}

		if (cbw.bCBWCB[0] != WRITE_10) {
			local_irq_restore(flags);
			tsc = (ReadTSC() - tsc) / ReadTSCMHz();
		}

		isp1581_do_cdb(&cbw);

		if (cbw.bCBWCB[0] == WRITE_10) {
			local_irq_restore(flags);
			tsc = (ReadTSC() - tsc) / ReadTSCMHz();
		}
		if (tsc > max_tsc) {
			printk("Interrupts off for %d us\n", tsc);
			max_tsc = tsc;
		}
	} 
#if 1
	else {
		printk("%d bytes of data received on EP2\n", count);
	}
#endif
}

static void isp1581_init_registers(void)
{
	union isp1581_mode_reg mreg;
	union isp1581_int_config_reg icreg;
	union isp1581_int_enable_reg iereg;
	union isp1581_dma_config_reg dcreg;
	union isp1581_dma_int_enable_reg diereg;
	union isp1581_dma_hw_reg dhwreg;

	/* Enable Soft Connect, Wakeup on CS, Clock always on */
	mreg.val		= 0;
	mreg.bits.softct 	= 1;
	mreg.bits.wkupcs 	= 1;
	mreg.bits.glintena	= 1;
	mreg.bits.clkaon	= 1;
	isp1581_outb(mreg.val, mode);

	/* Set all debug modes to "interrupt on ACK". Set level triggered,
	 * active low interrupt mode.
	 */
	icreg.val		= 0;
	icreg.bits.cdbgmod	= int_on_ack;
	icreg.bits.ddbgmodin	= int_on_ack;
	icreg.bits.ddbgmodout	= int_on_ack;
	icreg.bits.intlvl	= intlvl_level;
	icreg.bits.intpol	= intpol_active_low;
	isp1581_outb(icreg.val, intr_config);

	iereg.val		= 0;
	iereg.bits.iebrst	= 1;	/* Interrupt on bus reset */
	iereg.bits.iesusp	= 1;	/* Interrupt on Suspend */
	iereg.bits.ieresm	= 1;	/* Interrupt on Resume */
	iereg.bits.iehs_sta	= 1;	/* Interrupt on speed change */
	iereg.bits.iedma	= 1;	/* Interrupt on DMA status change */
	iereg.bits.iep0setup	= 1;	/* Interrupt on setup data on EP0 */
	iereg.bits.iep0tx	= 1;	/* Interrupt on Control OUT EP0 */
	iereg.bits.iep2tx	= 1;	/* Interrupt on Control OUT EP2 */
	iereg.bits.iep2rx	= 1;	/* Interrupt on Control IN EP2 */
	isp1581_outl(iereg.val, intr_enable);
	//printk("Interrupt enable reg = %#x\n", isp1581_inl(intr_enable));

	dcreg.val		= 0;
	dcreg.bits.width	= dcw_16_bit;		/* 16 bit bus */
	dcreg.bits.mode		= dcm_dior_diow;	/* Use DIOR/W */
	dcreg.bits.burst	= dcb_complete;		/* DREQ on complete */
	dcreg.bits.pio_mode	= dcpm_pio_4;		/* Set PIO mode 4 */
	dcreg.bits.ata_mode	= dcam_ata_mdma;	/* Set ATA mode */
	//printk("DMA config bits = %#x\n", dcreg.val);
	isp1581_outw(dcreg.val, dma_config);
	//printk("DMA config reg = %#x\n", isp1581_inw(dma_config));

	dhwreg.val		= 0;
	isp1581_outb(dhwreg.val, dma_hw);

	diereg.val		= 0;
	diereg.bits.cmd_intrq_ok= 1;	/* DMA xfer count == 0 and INTRQ */
	diereg.bits.tf_rd_done	= 1;	/* Interrupt on read taskfile done */
	//diereg.bits.bsy_done	= 1;	/* Interrupt on busy done */
	diereg.bits.read_1f0	= 1;	/* Interrupt when 1F0 FIFO has data */
	//diereg.bits.intrq_pending=1;	/* Interrupt pending on INTRQ */
	isp1581_outw(diereg.val, dma_intr_enable);
	//printk("DMA Interrupt enable reg = %#x\n", isp1581_inw(dma_intr_enable));
}

static void isp1581_init_endpoints(void)
{
	union isp1581_endpoint_index_reg epireg;
	union isp1581_endpoint_mps_reg epmpsreg;
	union isp1581_endpoint_type_reg eptreg;
	union isp1581_dma_ep_reg depreg;
	union isp1581_control_reg creg;
	union isp1581_address_reg areg;
	int fifo_size = 0;

	if (st.bits.speed == full_speed) {		/* USB 1.x */
		fifo_size = 64;
	} else if (st.bits.speed == high_speed) {	/* USB 2.0 */
		fifo_size = 512;
	}

	/* Set EP 2 Bulk OUT (receive data from host) */
	epireg.val		= 0;
	epireg.bits.dir		= epdir_out;
	epireg.bits.endpidx	= 2;
	isp1581_outb(epireg.val, ep_index);

	/* Set Max packet size */
	epmpsreg.val		= 0;
	epmpsreg.bits.ffosz	= fifo_size;
	epmpsreg.bits.ntrans	= epmps_ntrans_1pkt; /* 1 packet/xfer */
	isp1581_outw(epmpsreg.val, ep_max_ps);

	/* Set EP 2 Bulk IN (transmit data to host) */
	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	/* Set Max packet size */
	isp1581_outw(epmpsreg.val, ep_max_ps);

	/* Set endpoint type to Bulk */
	epireg.bits.dir		= epdir_out;
	isp1581_outb(epireg.val, ep_index);

	eptreg.val		= 0;
	eptreg.bits.endptyp	= eptype_bulk;
	eptreg.bits.dblbuf	= 0;
	eptreg.bits.enable	= 1;
	eptreg.bits.noempkt	= 1;
	isp1581_outw(eptreg.val, ep_type);

	epireg.bits.dir		= epdir_in;
	isp1581_outb(epireg.val, ep_index);

	/* Turn on Double-buffering for reads */
	eptreg.bits.dblbuf	= 1;
	isp1581_outw(eptreg.val, ep_type);

	/* Set EP 1 for DMA */
	depreg.val		= 0;
	depreg.bits.dmadir 	= epdir_out;
	depreg.bits.epidx 	= 1;
	isp1581_outb(depreg.val, dma_ep);

	/* Clear DMA IN(Tx) buffer */
	creg.val		= 0;
	creg.bits.clbuf		= 1;
	depreg.bits.dmadir 	= epdir_in;
	isp1581_outb(depreg.val, dma_ep);
	isp1581_outb(creg.val, control);

	/* Enable device and reset address */
	areg.val 		= 0;
	areg.bits.deven		= 1;
	isp1581_outb(areg.val, address);
}

static void isp1581_isr(int irq, void *context, struct pt_regs *regs)
{
	union isp1581_int_enable_reg ireg;
	union isp1581_mode_reg mreg;
	union isp1581_dma_int_enable_reg diereg;
	void (*temp_isp1581_ata_handler)(void) = NULL;
	u16 cause;
	static int state = 0;

	//cli();
//retry:
	cause = epld_isp1581_regs(intr_pending, 0);

	if (cause & 1) {
		state |= 1;
		writew (1, EPLD_ISP1581_INT_REGS_BASE);
	}

	if (cause & 0x02) {
		/* Get Interrupt Cause register */
		ireg.val = isp1581_inl(intr_cr);
	
		//printk("ISP1581 interrupt cause = %x\n", ireg.val); 
		
		/* Clear all interrupts */
		isp1581_outl(ireg.val, intr_cr);

		if (ireg.bits.iebrst) {
			//printk("Bus reset interrupt.\n");
			del_timer_sync(&tx_timer);
			st.val = 0;
			isp1581_init_registers();
			isp1581_init_endpoints();
			return;
			//goto retry;
		}
		
		if (ireg.bits.iehs_sta) {
			/* Speed changed */
			if (st.bits.speed != high_speed) {
				st.bits.speed = high_speed;
				isp1581_init_endpoints();
			}
		}

		if (ireg.bits.iesusp) {
			/* Activate suspend mode */
			del_timer_sync(&tx_timer);
			mreg.val = isp1581_inb(mode);
			mreg.bits.gosusp = 1;
			isp1581_outb(mreg.val, mode);
			mreg.bits.gosusp = 0;
			isp1581_outb(mreg.val, mode);
		}
	
		if (ireg.bits.ieresm)
			/* Write magic numbers to undocumented "unlock" 
			 * register 
			 */
			isp1581_outw(0xaa37, unlock);
	
		if (st.bits.state == st_stalled) {
			/* We are stalled. Wait till setup */
			printk("Stalled, waiting for EP0SETUP\n");
			return;
		}

		if (ireg.bits.iep2rx) 
			isp1581_do_ep2rx();


		if (ireg.bits.iep0setup) 
			isp1581_do_usb_setup();

		if (ireg.bits.iedma) {
			diereg.val = isp1581_inw(dma_intr_cause);
			isp1581_outw(diereg.val, dma_intr_cause);
			//printk("DMA interrupt %#x\n", diereg.val);
			if (diereg.bits.cmd_intrq_ok)
				state |= 2;
		}
	} 

	if (state == 3) {
		state = 0;
		if (isp1581_ata_handler) {
			temp_isp1581_ata_handler = isp1581_ata_handler;
			isp1581_ata_handler = NULL;
			temp_isp1581_ata_handler();
		}
	}
	//goto retry;
}

static int isp1581_get_device_info(void)
{
	int timeout = 1000;	/* Multiples of 10us */
	union isp1581_dma_int_enable_reg dicreg;
	int i;

	/* Reset FIFO */
	epld_isp1581_regs(fifo_reset, 0) = 1;

	/* Select Master drive */
	isp1581_outb(0xa0, tf_select);
	epld_isp1581_regs(epld_select, 0) = 0xa0;

	/* Send IDENTIFY command */
	isp1581_outb(WIN_IDENTIFY, tf_cmd_status);
	epld_isp1581_regs(epld_cmd_status, 0) = WIN_IDENTIFY;

	/* Turn on INTRQ */
	dicreg.val		= isp1581_inw(dma_intr_enable);
	dicreg.bits.intrq_pending=1;	/* Interrupt pending on INTRQ */
	isp1581_outw(dicreg.val, dma_intr_enable);

	/* Wait till identify completes */
	do {
		dicreg.val = isp1581_inw(dma_intr_cause);
		udelay(10);
	} while (!dicreg.bits.intrq_pending && timeout--);

	/* Turn off INTRQ */
	dicreg.val		= isp1581_inw(dma_intr_enable);
	dicreg.bits.intrq_pending=0;	/* Interrupt pending on INTRQ */
	isp1581_outw(dicreg.val, dma_intr_enable);

	if (timeout < 0) {
		printk("%s: ATA IDENTIFY command timed out.\n", __FUNCTION__);
		return -1;
	}

	timeout = 10;
	/* Read taskfiles to check status */
	isp1581_outb(dcmd_read_tf, dma_cmd);
	do {
		dicreg.val = isp1581_inw(dma_intr_cause);
		udelay(10);
	} while (!dicreg.bits.tf_rd_done && timeout--);

	if (timeout < 0) {
		printk("%s: ATA Read Taskfile Command timed out.\n", 
				__FUNCTION__);
		return -1;
	}

	/* Check status */
	if (isp1581_inb(tf_control) & ERR_STAT) {
		printk("%s: ATA Identify command returned error %#x\n",
				__FUNCTION__, isp1581_inb(tf_control));
		return -1;
	}

	/* Read 512 bytes from EPLD */
	isp1581_outw(512, dma_count);

	/* Taskfile registers 0x1f4 and 0x1f5 double as the byte
	 * count for reading data from ATA.
	 */
	isp1581_outw(512, tf_lcyl);
	isp1581_outb(dcmd_read_1f0, dma_cmd);

	timeout = 1000;
	do {
		dicreg.val = isp1581_inw(dma_intr_cause);
		udelay(10);
	} while (!dicreg.bits.read_1f0 && timeout--);

	if (timeout < 0) {
		printk("%s: ATA Read from DATA port command timed out.\n", 
				__FUNCTION__);
		return -1;
	}

	/* Now, read data */
	for (i = 0; i < 512; i++)
		((u8 *)&isp1581_driveid)[i] = isp1581_inb(tf_data);

	/* Fixup 16-bit fields */
	FIXUP_DRIVEID_16(config);
	FIXUP_DRIVEID_16(cyls);
	FIXUP_DRIVEID_16(heads);
	FIXUP_DRIVEID_16(track_bytes);
	FIXUP_DRIVEID_16(sector_bytes);
	FIXUP_DRIVEID_16(sectors);
	FIXUP_DRIVEID_16(dword_io);
	FIXUP_DRIVEID_16(field_valid);
	FIXUP_DRIVEID_16(eide_pio_modes);
	FIXUP_DRIVEID_16(eide_dma_min);
	FIXUP_DRIVEID_16(eide_dma_time);
	FIXUP_DRIVEID_16(eide_pio);
	FIXUP_DRIVEID_16(eide_pio_iordy);
	FIXUP_DRIVEID_16(eide_dma_time);
	FIXUP_DRIVEID_16(queue_depth);
	FIXUP_DRIVEID_16(command_set_1);
	FIXUP_DRIVEID_16(command_set_2);
	FIXUP_DRIVEID_16(cfsse);
	FIXUP_DRIVEID_16(cfs_enable_1);
	FIXUP_DRIVEID_16(cfs_enable_2);
	FIXUP_DRIVEID_16(csf_default);
	FIXUP_DRIVEID_16(hw_config);
	FIXUP_DRIVEID_16(cur_capacity0);
	FIXUP_DRIVEID_16(cur_capacity1);

	isp1581_driveid.lba_capacity_2 = 
				le64_to_cpu(isp1581_driveid.lba_capacity_2);

	/* Fixup model string */
	for (i = 0; i < sizeof(isp1581_driveid.model); i += 2) {
		char c;
		c = isp1581_driveid.model[i];
		isp1581_driveid.model[i] = isp1581_driveid.model[i+1];
		isp1581_driveid.model[i+1] = c;
	}

	for (i = sizeof(isp1581_driveid.model) - 2; i >= 0; i--) {
		if (isp1581_driveid.model[i] != ' ') {
			isp1581_driveid.model[i+1] = '\0';
			break;
		}
	}

	printk("%s: %s, ATA DISK drive\n", __FILE__,
			(isp1581_driveid.model[0] == '\0') ? "Unknown" :
						(char *)&isp1581_driveid.model);
	if (!(isp1581_driveid.capability & 0x02)) {
		printk("%s: ATA DISK drive does not support LBA mode. "
				"Driver disabled.\n", __FILE__);
		return -1;
	}


	/* Check for 48-bit LBA. Currently not supported. This limits
	 * drive size to 128GB.
	 */
	if (isp1581_driveid.lba_capacity == 0x0fffffff) {
		printk("%s: %lld sectors, CHS=%d/%d/%d, PIO-4 "
				"%d-bit LBA mode\n", __FILE__,
			isp1581_driveid.lba_capacity_2, isp1581_driveid.cyls,
			isp1581_driveid.heads, isp1581_driveid.sectors, 48);
		isp1581_do_ata_cmd = isp1581_do_48bit_cmd;
		drive_capacity = (u32)isp1581_driveid.lba_capacity_2;
	} else {
		printk("%s: %lld sectors, CHS=%d/%d/%d, PIO-4 "
				"%d-bit LBA mode\n", __FILE__,
			(u64)isp1581_driveid.lba_capacity, isp1581_driveid.cyls,
			isp1581_driveid.heads, isp1581_driveid.sectors, 28);
		isp1581_do_ata_cmd = isp1581_do_28bit_cmd;
		drive_capacity = isp1581_driveid.lba_capacity;
	}

	return 0;
}

static int __init has7752_usbdev_init(void)
{
#ifndef CONFIG_SSA_HAS7752
#error "This driver works only for the HAS 7752 configuration."
#else
	u16 chipid, version;

	/* Drive low to assert ISP1581 reset */
	gpio_set_as_output (GPIO_USB_RESET, 0); 

	/* Hold it low for 10us */
	udelay (10);

	/* drive high to deassert ISP1581 reset */
	gpio_set_as_output (GPIO_USB_RESET, 1);

	/* Do not reset the EPLD USB interface. The ATA and USB resets in
	 * the EPLD are closely tied to each other. We expect the bootloader
	 * to toggle these resets.
	 * writew (EPLD_SYSCONTROL_USB_RESET, IO_ADDRESS_SYSCONTROL_BASE);
	 */
	writew (EPLD_SYSCONTROL_ATA_RESET, IO_ADDRESS_SYSCONTROL_BASE);
	writew (EPLD_SYSCONTROL_USB_RESET, IO_ADDRESS_SYSCONTROL_BASE);

	/* Configure wait states */
	epld_isp1581_regs(micro_speed, 0) = 0x0;
	epld_isp1581_regs(micro_wait, 0) = 0x0;
	chipid = isp1581_inw(chip_id+1);
	if (chipid != 0x1581) {
		printk("ISP1581 not found\n");
		return -ENODEV;
	}
#ifdef MODULE
	MOD_INC_USE_COUNT;
#endif
	version = isp1581_inb(chip_id);
	printk(KERN_INFO "ISP1581 version %d found, "
			"HAS USB Mass Storage driver enabled\n", 
				version - 0x50);

	/* Set speed to USB 1.x */
	st.bits.speed = full_speed;

	/* Initialize ISP1581 initialization registers */
	isp1581_init_registers();

	/* Initialize endpoints */
	isp1581_init_endpoints();

	/* Read drive information. Done with ISP1581 interrupts off */
	if (isp1581_get_device_info())
		return -ENODEV;

	/* Reset all DMA interrupt bits */
	isp1581_outw(0x1fff, dma_intr_cause);

	/* Clear all interrupts */
	isp1581_outl(0x03fffd7f, intr_cr);

	/* Acknowledge any ATA interrupts signalled by the EPLD */
	epld_isp1581_regs(intr_pending, 0) = 1;

	/* Register interrupt */
	return request_irq(INT_ISP1581, isp1581_isr, 0, "ISP1581", NULL);
#endif
}

static void __init has7752_usbdev_exit(void)
{
	free_irq(INT_ISP1581, NULL);
#ifdef MODULE
	MOD_DEC_USE_COUNT
#endif
}


MODULE_AUTHOR("Ranjit Deshpande <ranjit@kenati.com>");
MODULE_DESCRIPTION("USB Mass Storage driver for ISP1581");
MODULE_LICENSE("GPL");
module_init(has7752_usbdev_init);
module_exit(has7752_usbdev_exit);
