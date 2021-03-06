/*
 *  Copyright (c) 2009, 2010 zmiq2
 *
 *  Driver for Fujitsu USB Touchscreen for U810, P1620
 *
 *  Derived from USB Acecad "Acecad Flair" tablet driver (acecad.c)
 *  Derived from USB U810 tablet driver from Julian Brown
 *
 * 0.3.9 - 2011.05.21
 *  - Updated to work on ubuntu natty (Linux 2.6.38-8) (cybergene)
 *
 * 0.3.8 - 2010.10.04
 *  - Updated to work on Linux 2.6.35 (nerd65536)
 *
 * 0.3.7 - 2010.07.20
 *  - changed module load procedure to allow working on kernels with
 *    module usb_hid compiled in
 *
 * 0.3.5 - 2009.07.15
 *  - fixed bug to allow calibration on a T1010 IA64 (zmiq2)
 *
 * 0.3.4 - 2009.05.18
 *  - redone coordinate calculation when screen is rotated (zmiq2)
 *
 * 0.3.3 - 2009.05.16
 *  - fixed to compile under 2.6.30rc5 (zmiq2)
 *
 * 0.3.2 - 2009.05.14
 *  - fixed bug on coordinate calculation when screen is rotated (zmiq2)
 *
 * 0.3.1 - 2009.05.10
 *  - added parameters to allow screen calibration (zmiq2)
 *  
 * 0.3.0 - 2009.04.20
 *  - added parameters to upload min/max values (zmiq2)
 *  - added parameter to allow screen rotation (zmiq2)
 *                   
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>

/*
 * Version Information
 */
#define DRIVER_VERSION "v0.3.9"
#define DRIVER_DESC    "Fujitsu usb touchscreen driver for u810, u820, p1620, t1010"
#define DRIVER_LICENSE "GPL"
#define DRIVER_AUTHOR  "zmiq2 <zzmiq2@gmail.com>"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE(DRIVER_LICENSE);

#define USB_VENDOR_ID_FUJITSU		0x0430
#define USB_DEVICE_ID_U810TABLET	0x0530

#define kres	10

/* default calibration values */
#define TOUCH_MINX	130
#define TOUCH_MINY	250
#define TOUCH_MAXX	3820
#define TOUCH_MAXY	3920

#define TOUCH_MINX_HELP "MinX value from calibration (default:" "TOUCH_MINX" ")"
#define TOUCH_MINY_HELP "MinY value from calibration (default:" "TOUCH_MINY" ")"
#define TOUCH_MAXX_HELP "MaxX value from calibration (default:" "TOUCH_MAXX" ")"
#define TOUCH_MAXY_HELP "MaxY value from calibration (default:" "TOUCH_MAXY" ")"

/* module parameters */
static int printpos=0;
static int orientation=0;
static int touch_minx=TOUCH_MINX;
static int touch_miny=TOUCH_MINY;
static int touch_maxx=TOUCH_MAXX;
static int touch_maxy=TOUCH_MAXY;
static int calibrate=0;
static int calib_minx=0;
static int calib_miny=0;
static int calib_maxx=0;
static int calib_maxy=0;

struct usb_u810_tablet {
	char name[128];
	char phys[64];
	struct usb_device *usbdev;
	struct usb_interface *intf;
	struct input_dev *input;
	struct urb *irq;

	unsigned char *data;
	dma_addr_t data_dma;
};

static void usb_u810_tablet_irq(struct urb *urb)
{
	struct usb_u810_tablet *u810_tablet = urb->context;
	unsigned char *data = u810_tablet->data;
	struct input_dev *dev = u810_tablet->input;
	struct usb_interface *intf;
	int status;

        unsigned int x, y, touch;
	int sx, sy;

	switch (urb->status) {
		case 0:
			/* success */
			break;
		case -ECONNRESET:
		case -ENOENT:
		case -ESHUTDOWN:
			/* this urb is terminated, clean up */
			dev_dbg(&intf->dev, "%s - urb shutting down with status: %d", __func__, urb->status);
			return;
		default:
			dev_dbg(&intf->dev, "%s - nonzero urb status received: %d", __func__, urb->status);
			goto resubmit;
	}

	x = data[1] | (data[2] << 8);
	y = data[3] | (data[4] << 8);
	touch = data[0] & 0x01;

	if (calibrate) {
		calib_minx = (calib_minx==0)?x:((x<calib_minx)?x:calib_minx);
		calib_miny = (calib_miny==0)?y:((y<calib_miny)?y:calib_miny);
		calib_maxx = (x>calib_maxx)?x:calib_maxx;
		calib_maxy = (y>calib_maxy)?y:calib_maxy;
        }
        else {
                switch(orientation) {
			case 1: /* left */
				sx = touch_maxx - touch_minx;
				sy = touch_maxy - touch_miny;
				input_report_abs(dev, ABS_X, touch_maxx - ((( y - touch_miny ) * kres * sx / sy) / kres));
				input_report_abs(dev, ABS_Y, touch_miny + ((( x - touch_minx ) * kres * sy / sx) / kres));
				break;

			case 2: /* inverted */
				input_report_abs(dev, ABS_X, touch_maxx + touch_minx - x);
				input_report_abs(dev, ABS_Y, touch_maxy + touch_miny - y);
				break;

			case 3: /* right */
				sx = touch_maxx - touch_minx;
				sy = touch_maxy - touch_miny;
				input_report_abs(dev, ABS_X, touch_minx + ((( y - touch_miny ) * kres * sx / sy) / kres));
				input_report_abs(dev, ABS_Y, touch_maxy - ((( x - touch_minx ) * kres * sy / sx) / kres));
				break;

			default:
				input_report_abs(dev, ABS_X, x);
				input_report_abs(dev, ABS_Y, y);
				break;
                }

		input_report_key(dev, BTN_TOUCH, touch);
		input_sync(dev);
	}

	if(printpos)
		pr_err("fujitsu_touchscreen::x:[%d] y:[%d] t:[%d]", x, y, touch);

resubmit:
	status = usb_submit_urb(urb, GFP_ATOMIC);
	if (status)
		dev_err(&intf->dev,"can't resubmit intr, %s-%s/input0, status %d",
		    u810_tablet->usbdev->bus->bus_name, u810_tablet->usbdev->devpath, status);
}

static int usb_u810_tablet_open(struct input_dev *dev)
{
	struct usb_u810_tablet *u810_tablet = input_get_drvdata(dev);

	u810_tablet->irq->dev = u810_tablet->usbdev;
	if (usb_submit_urb(u810_tablet->irq, GFP_KERNEL))
		return -EIO;

	return 0;
}

static void usb_u810_tablet_close(struct input_dev *dev)
{
	struct usb_u810_tablet *u810_tablet = input_get_drvdata(dev);

	usb_kill_urb(u810_tablet->irq);
}

/* Send some control messages to the device. FIXME: These are
   reverse-engineered from examining USB traces from the Windows driver.
   YMMV!  */
static void usb_u810_tablet_configure(struct usb_device *udev)
{
	int ret;
	
	/* Inhibit reporting from interface until a change is detected.  */
	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0),
			      0x0a, /* SET_IDLE */
			      0x21, /* Class Interface */
			      0x0000, 0x0000, NULL, 0x0000,
			      USB_CTRL_SET_TIMEOUT);
	if (ret < 0)
		pr_err("%s failed performing SET_IDLE for class interface",
		    __FUNCTION__);

	/* Vendor device */
	ret = usb_control_msg(udev, usb_rcvctrlpipe(udev, 0), 0x05, 0x40,
			      0x0016, 0x0000, NULL, 0x0000,
			      USB_CTRL_SET_TIMEOUT);
	if (ret < 0)
		pr_err("%s failed sending magic control message 0x05 0x40",
		    __FUNCTION__);
}

static int usb_u810_tablet_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface = intf->cur_altsetting;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_u810_tablet *u810_tablet;
	struct input_dev *input_dev;
	int pipe, maxp;
	int err;

	if (interface->desc.bNumEndpoints != 1) {
		pr_err("fujitsu_touchscreen::probing::no device found::bNumEndpoints!=1");
		return -ENODEV;
        }

	endpoint = &interface->endpoint[0].desc;

	if (!usb_endpoint_is_int_in(endpoint)) {
		pr_err("fujitsu_touchscreen::probing::no device found::endpoint not int");
		return -ENODEV;
        }

	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(dev, pipe, usb_pipeout(pipe));

	u810_tablet = kzalloc(sizeof(struct usb_u810_tablet), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!u810_tablet || !input_dev) {
		pr_err("fujitsu_touchscreen::probing::no device found::cannot allocate device");
		err = -ENOMEM;
		goto fail1;
	}

	u810_tablet->data = usb_alloc_coherent(dev, 8, GFP_KERNEL, &u810_tablet->data_dma);
	if (!u810_tablet->data) {
		pr_err("fujitsu_touchscreen::probing::no device found::cannot allocate device data");
		err= -ENOMEM;
		goto fail1;
	}

	u810_tablet->irq = usb_alloc_urb(0, GFP_KERNEL);
	if (!u810_tablet->irq) {
		pr_err("fujitsu_touchscreen::probing::no device found::cannot allocate device irq");
		err = -ENOMEM;
		goto fail2;
	}

	u810_tablet->usbdev = dev;
	u810_tablet->intf = intf;
	u810_tablet->input = input_dev;

	if (dev->manufacturer)
		strlcpy(u810_tablet->name, dev->manufacturer, sizeof(u810_tablet->name));

	if (dev->product) {
		if (dev->manufacturer)
			strlcat(u810_tablet->name, " ", sizeof(u810_tablet->name));
		strlcat(u810_tablet->name, dev->product, sizeof(u810_tablet->name));
	}

	usb_make_path(dev, u810_tablet->phys, sizeof(u810_tablet->phys));
	strlcat(u810_tablet->phys, "/input0", sizeof(u810_tablet->phys));

	input_dev->name = u810_tablet->name;
	input_dev->phys = u810_tablet->phys;
	usb_to_input_id(dev, &input_dev->id);
	input_dev->dev.parent = &intf->dev;

	pr_err("fujitsu_touchscreen::probing::device found::[%s][%s]", u810_tablet->name, u810_tablet->phys);

	input_set_drvdata(input_dev, u810_tablet);

	input_dev->open = usb_u810_tablet_open;
	input_dev->close = usb_u810_tablet_close;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->relbit[0] = 0;
	input_dev->absbit[0] = BIT_MASK(ABS_X) | BIT_MASK(ABS_Y);
        input_dev->keybit[BIT_WORD(BTN_DIGI)] = BIT_MASK(BTN_TOOL_PEN) |
                BIT_MASK(BTN_TOUCH) | BIT_MASK(BTN_STYLUS) |
                BIT_MASK(BTN_STYLUS2);

        input_alloc_absinfo(input_dev);

	switch (id->driver_info) {
		case 0:
			input_dev->absinfo[ABS_X].minimum = touch_minx;
			input_dev->absinfo[ABS_Y].minimum = touch_miny;
			input_dev->absinfo[ABS_X].maximum = touch_maxx;
			input_dev->absinfo[ABS_Y].maximum = touch_maxy;
			if (!strlen(u810_tablet->name))
				snprintf(u810_tablet->name,
					sizeof(u810_tablet->name),
					"USB Fujitsu U810 Tablet %04x:%04x",
					le16_to_cpu(dev->descriptor.idVendor),
					le16_to_cpu(dev->descriptor.idProduct));
			break;
		default:
			/* Badness?  */
			;
	}

	/* 6 pixels of fuzz.  The touch panel appears to be quite noisy.  */
	input_dev->absinfo[ABS_X].fuzz = 6;
	input_dev->absinfo[ABS_Y].fuzz = 6;

	usb_fill_int_urb(u810_tablet->irq, dev, pipe,
			u810_tablet->data, maxp > 8 ? 8 : maxp,
			usb_u810_tablet_irq, u810_tablet, endpoint->bInterval);
	u810_tablet->irq->transfer_dma = u810_tablet->data_dma;
	u810_tablet->irq->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	err = input_register_device(u810_tablet->input);
	if (err)
		goto fail3;

	usb_set_intfdata(intf, u810_tablet);

	usb_u810_tablet_configure(dev);
	pr_err("fujitsu_touchscreen::loaded::[%s]::min-max values::x[%d,%d] y[%d,%d]", DRIVER_VERSION, touch_minx, touch_maxx, touch_miny, touch_maxy);

	return 0;

 fail3: usb_free_urb(u810_tablet->irq);
 fail2:	usb_free_coherent(dev, 8, u810_tablet->data, u810_tablet->data_dma);
 fail1: kfree(input_dev->absinfo);
	input_free_device(input_dev);
	kfree(u810_tablet);
	pr_err("fujitsu_touchscreen::probing::no device found");
	return err;
}

static void usb_u810_tablet_disconnect(struct usb_interface *intf)
{
	struct usb_u810_tablet *u810_tablet = usb_get_intfdata(intf);
//	struct input_dev *dev = u810_tablet->input;

	usb_set_intfdata(intf, NULL);

//	kfree(dev->absinfo);
	input_unregister_device(u810_tablet->input);
	usb_free_urb(u810_tablet->irq);
	usb_free_coherent(u810_tablet->usbdev, 8, u810_tablet->data, u810_tablet->data_dma);
	kfree(u810_tablet);
}

static struct usb_device_id usb_u810_tablet_id_table [] = {
	{ USB_DEVICE(USB_VENDOR_ID_FUJITSU, USB_DEVICE_ID_U810TABLET), .driver_info = 0 },
	{ }
};

MODULE_DEVICE_TABLE(usb, usb_u810_tablet_id_table);

static struct usb_driver usb_u810_tablet_driver = {
	.name =		"usb_u810_tablet",
	.probe =	usb_u810_tablet_probe,
	.disconnect =	usb_u810_tablet_disconnect,
	.id_table =	usb_u810_tablet_id_table
};

module_param(printpos, int, S_IRUSR | S_IWUSR | S_IRGRP |  S_IROTH);
MODULE_PARM_DESC(printpos, "0: do nothing, 1: print pen position on kern.log");

module_param(touch_minx, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(touch_miny, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(touch_maxx, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(touch_maxy, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(touch_minx, TOUCH_MINX_HELP);
MODULE_PARM_DESC(touch_miny, TOUCH_MINY_HELP);
MODULE_PARM_DESC(touch_maxx, TOUCH_MAXX_HELP);
MODULE_PARM_DESC(touch_maxy, TOUCH_MAXY_HELP);

module_param(orientation, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(orientation, "Screen orientation: 0:normal, 1:left, 2:inverted, 3:right");

module_param(calibrate, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(calibrate, "0: normal usage, 1: calibration mode");

module_param(calib_minx, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(calib_miny, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(calib_maxx, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
module_param(calib_maxy, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(calib_minx, "MinX value from calibration mode");
MODULE_PARM_DESC(calib_miny, "MinY value from calibration mode");
MODULE_PARM_DESC(calib_maxx, "MaxX value from calibration mode");
MODULE_PARM_DESC(calib_maxy, "MaxY value from calibration mode");

module_usb_driver(usb_u810_tablet_driver);
