/*
	libptouch - functions to help accessing a brother ptouch

	Copyright (C) 2013-2020 Dominic Radermacher <blip@mockmoon-cybernetics.ch>

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License version 3 as
	published by the Free Software Foundation

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software Foundation,
	Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#define _POSIX_C_SOURCE	199309L	/* needed for nanosleep() when using -std=c11 */

#include <stdio.h>
#include <stdlib.h>	/* malloc() */
#include <string.h>	/* memcmp()  */
#include <sys/types.h>	/* open() */
#include <sys/stat.h>	/* open() */
#include <fcntl.h>	/* open() */
#include <time.h>	/* nanosleep(), struct timespec */
#include "config.h"
#include "gettext.h"	/* gettext(), ngettext() */
#include "ptouch.h"

#define _(s) gettext(s)

/* Print area width in 180 DPI pixels */
struct _pt_tape_info tape_info[]= {
	{ 6, 32, 1.0},	/* 6 mm tape */
	{ 9, 52, 1.0},	/* 9 mm tape */
	{12, 76, 2.0},	/* 12 mm tape */
	{18, 120, 3.0},	/* 18 mm tape */
	{24, 128, 3.0},	/* 24 mm tape */
	{36, 192, 4.5},	/* 36 mm tape */
	{ 0, 0, 0.0}		/* terminating entry */
};

struct _pt_dev_info ptdevs[] = {
	{0x04f9, 0x2007, "PT-2420PC", 128, 180, FLAG_RASTER_PACKBITS},	/* 180dpi, 128px, maximum tape width 24mm, must send TIFF compressed pixel data */
	{0x04f9, 0x2011, "PT-2450PC", 128, 180, FLAG_RASTER_PACKBITS},
	{0x04f9, 0x2019, "PT-1950", 128, 180, FLAG_RASTER_PACKBITS},	/* 180dpi, apparently 112px printhead ?, maximum tape width 18mm - unconfirmed if it works */
	{0x04f9, 0x202c, "PT-1230PC", 128, 180, FLAG_NONE},		/* 180dpi, supports tapes up to 12mm - I don't know how much pixels it can print! */
	/* Notes about the PT-1230PC: While it is true that this printer supports
	   max 12mm tapes, it apparently expects > 76px data - the first 32px
	   must be blank. */
	{0x04f9, 0x202d, "PT-2430PC", 128, 180, FLAG_NONE},		/* 180dpi, maximum 128px */
	{0x04f9, 0x2030, "PT-1230PC (PLite Mode)", 128, 180, FLAG_PLITE},
	{0x04f9, 0x2031, "PT-2430PC (PLite Mode)", 128, 180, FLAG_PLITE},
	{0x04f9, 0x2041, "PT-2730", 128, 180, FLAG_NONE},		/* 180dpi, maximum 128px, max tape width 24mm - reported to work with some quirks */
	/* Notes about the PT-2730: was reported to need 48px whitespace
	   within png-images before content is actually printed - can not check this */
	{0x04f9, 0x205e, "PT-H500", 128, 180, FLAG_RASTER_PACKBITS},
	/* Note about the PT-H500: was reported by Eike with the remark that
	   it might need some trailing padding */
	{0x04f9, 0x205f, "PT-E500", 128, 180, FLAG_RASTER_PACKBITS},
	/* Note about the PT-E500: was reported by Jesse Becker with the
	   remark that it also needs some padding (white pixels) */
	{0x04f9, 0x2061, "PT-P700", 128, 180, FLAG_RASTER_PACKBITS|FLAG_P700_INIT},
	{0x04f9, 0x2064, "PT-P700 (PLite Mode)", 128, 180, FLAG_PLITE},
	{0x04f9, 0x2062, "PT-P750W", 128, 180, FLAG_RASTER_PACKBITS|FLAG_P700_INIT},
	{0x04f9, 0x2065, "PT-P750W (PLite Mode)", 128, 180, FLAG_PLITE},
	{0x04f9, 0x2073, "PT-D450", 128, 180, FLAG_RASTER_PACKBITS},
	/* Notes about the PT-D450: I'm unsure if print width really is 128px */
	{0x04f9, 0x2074, "PT-D600", 128, 180, FLAG_RASTER_PACKBITS},
	/* PT-D600 was reported to work, but with some quirks (premature
	   cutting of tape, printing maximum of 73mm length) */
	//{0x04f9, 0x200d, "PT-3600", 384, 360, FLAG_RASTER_PACKBITS},
	{0,0,"",0,0,0}
};

void ptouch_rawstatus(uint8_t raw[32]);

int ptouch_open(ptouch_dev *ptdev)
{
	libusb_device **devs;
	libusb_device *dev;
	libusb_device_handle *handle = NULL;
	struct libusb_device_descriptor desc;
	ssize_t cnt;
	int r,i=0;

	if ((*ptdev=malloc(sizeof(struct _ptouch_dev))) == NULL) {
		fprintf(stderr, _("out of memory\n"));
		return -1;
	}
	if (((*ptdev)->devinfo=malloc(sizeof(struct _pt_dev_info))) == NULL) {
		fprintf(stderr, _("out of memory\n"));
		return -1;
	}
	if (((*ptdev)->status=malloc(sizeof(struct _ptouch_stat))) == NULL) {
		fprintf(stderr, _("out of memory\n"));
		return -1;
	}
	if ((libusb_init(NULL)) < 0) {
		fprintf(stderr, _("libusb_init() failed\n"));
		return -1;
	}
//	libusb_set_debug(NULL, 3);
	if ((cnt=libusb_get_device_list(NULL, &devs)) < 0) {
		return -1;
	}
	while ((dev=devs[i++]) != NULL) {
		if ((r=libusb_get_device_descriptor(dev, &desc)) < 0) {
			fprintf(stderr, _("failed to get device descriptor"));
			libusb_free_device_list(devs, 1);
			return -1;
		}
		for (int k=0; ptdevs[k].vid > 0; k++) {
			if ((desc.idVendor == ptdevs[k].vid) && (desc.idProduct == ptdevs[k].pid) && (ptdevs[k].flags >= 0)) {
				fprintf(stderr, _("%s found on USB bus %d, device %d\n"),
					ptdevs[k].name,
					libusb_get_bus_number(dev),
					libusb_get_device_address(dev));
				if (ptdevs[k].flags & FLAG_PLITE) {
					printf("Printer is in P-Lite Mode, which is unsupported\n\n");
					printf("Turn off P-Lite mode by changing switch from position EL to position E\n");
					printf("or by pressing the PLite button for ~ 2 seconds (or consult the manual)\n");
					return -1;
				}
				if (ptdevs[k].flags & FLAG_UNSUP_RASTER) {
					printf("Unfortunately, that printer currently is unsupported (it has a different raster data transfer)\n");
					return -1;
				}
				if ((r=libusb_open(dev, &handle)) != 0) {
					fprintf(stderr, _("libusb_open error :%s\n"), libusb_error_name(r));
					return -1;
				}
				libusb_free_device_list(devs, 1);
				if ((r=libusb_kernel_driver_active(handle, 0)) == 1) {
					if ((r=libusb_detach_kernel_driver(handle, 0)) != 0) {
						fprintf(stderr, _("error while detaching kernel driver: %s\n"), libusb_error_name(r));
					}
				}
				if ((r=libusb_claim_interface(handle, 0)) != 0) {
					fprintf(stderr, _("interface claim error: %s\n"), libusb_error_name(r));
					return -1;
				}
				(*ptdev)->h=handle;
				(*ptdev)->devinfo->dpi=ptdevs[k].dpi;
				(*ptdev)->devinfo->max_px=ptdevs[k].max_px;
				(*ptdev)->devinfo->flags=ptdevs[k].flags;
				return 0;
			}
		}
	}
	fprintf(stderr, _("No P-Touch printer found on USB (remember to put switch to position E)\n"));
	libusb_free_device_list(devs, 1);
	return -1;
}

int ptouch_close(ptouch_dev ptdev)
{
	libusb_release_interface(ptdev->h, 0);
	libusb_close(ptdev->h);
	return 0;
}

int ptouch_send(ptouch_dev ptdev, uint8_t *data, size_t len)
{
	int r, tx;

	if ((ptdev == NULL) || (len > 128)) {
		return -1;
	}
	if ((r=libusb_bulk_transfer(ptdev->h, 0x02, data, (int)len, &tx, 0)) != 0) {
		fprintf(stderr, _("write error: %s\n"), libusb_error_name(r));
		return -1;
	}
	if (tx != (int)len) {
		fprintf(stderr, _("write error: could send only %i of %ld bytes\n"), tx, len);
		return -1;
	}
	return 0;
}

int ptouch_init(ptouch_dev ptdev)
{
	char cmd[]="\x1b\x40";		/* 1B 40 = ESC @ = INIT */
	return ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
}

int ptouch_enable_packbits(ptouch_dev ptdev)
{				/* 4D 00 = disable compression */
	char cmd[] = "M\x02";	/* 4D 02 = enable packbits compression mode */
	return ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
}

int ptouch_rasterstart(ptouch_dev ptdev)
{
	/* 1B 69 52 01 = Select graphics transfer mode = Raster */
	char cmd[] = "\x1b\x69\x52\x01";
	/* 1B 69 61 01 = switch mode (0=esc/p, 1=raster mode) */
	char cmd2[] = "\x1b\x69\x61\x01";
	if (ptdev->devinfo->flags & FLAG_P700_INIT) {
		return ptouch_send(ptdev, (uint8_t *)cmd2, strlen(cmd2));
	} /* else */
	return ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
}

/* print an empty line */
int ptouch_lf(ptouch_dev ptdev)
{
	char cmd[]="\x5a";
	return ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
}

/* print and advance tape, but do not cut */
int ptouch_ff(ptouch_dev ptdev)
{
	char cmd[]="\x0c";
	return ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
}

/* print and cut tape */
int ptouch_eject(ptouch_dev ptdev)
{
	char cmd[]="\x1a";
	return ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
}

void ptouch_rawstatus(uint8_t raw[32])
{
	fprintf(stderr, _("debug: dumping raw status bytes\n"));
	for (int i=0; i<32; i++) {
		fprintf(stderr, "%02x ", raw[i]);
		if (((i+1) % 16) == 0) {
			fprintf(stderr, "\n");
		}
	}
	fprintf(stderr, "\n");
	return;
}

int ptouch_getstatus(ptouch_dev ptdev)
{
	char cmd[]="\x1biS";	/* 1B 69 53 = ESC i S = Status info request */
	uint8_t buf[32];
	int i, r, tx=0, tries=0;
	struct timespec w;

	ptouch_send(ptdev, (uint8_t *)cmd, strlen(cmd));
	while (tx == 0) {
		w.tv_sec=0;
		w.tv_nsec=100000000;	/* 0.1 sec */
		r=nanosleep(&w, NULL);
		if ((r=libusb_bulk_transfer(ptdev->h, 0x81, buf, 32, &tx, 0)) != 0) {
			fprintf(stderr, _("read error: %s\n"), libusb_error_name(r));
			return -1;
		}
		tries++;
		if (tries > 10) {
			fprintf(stderr, _("timeout while waiting for status response\n"));
			return -1;
		}
	}
	if (tx == 32) {
		if (buf[0]==0x80 && buf[1]==0x20) {
			memcpy(ptdev->status, buf, 32);
			ptdev->tape_width_px=0;
			for (i=0; tape_info[i].mm > 0; i++) {
				if (tape_info[i].mm == buf[10]) {
					ptdev->tape_width_px=tape_info[i].px;
				}
			}
			if (ptdev->tape_width_px == 0) {
				fprintf(stderr, _("unknown tape width of %imm, please report this.\n"), buf[10]);
			}
			return 0;
		}
	}
	if (tx == 16) {
		fprintf(stderr, _("got only 16 bytes... wondering what they are:\n"));
		ptouch_rawstatus(buf);
	}
	if (tx != 32) {
		fprintf(stderr, _("read error: got %i instead of 32 bytes\n"), tx);
		return -1;
	}
	fprintf(stderr, _("strange status:\n"));
	ptouch_rawstatus(buf);
	fprintf(stderr, _("trying to flush junk\n"));
	if ((r=libusb_bulk_transfer(ptdev->h, 0x81, buf, 32, &tx, 0)) != 0) {
		fprintf(stderr, _("read error: %s\n"), libusb_error_name(r));
		return -1;
	}
	fprintf(stderr, _("got another %i bytes. now try again\n"), tx);
	return -1;
}

size_t ptouch_get_tape_width(ptouch_dev ptdev)
{
	return ptdev->tape_width_px;
}

size_t ptouch_get_max_width(ptouch_dev ptdev)
{
	return ptdev->devinfo->max_px;
}

int ptouch_sendraster(ptouch_dev ptdev, uint8_t *data, size_t len)
{
	uint8_t buf[64];
	int rc;

	if (len > (size_t)(ptdev->devinfo->max_px / 8)) {
		return -1;
	}
	buf[0]=0x47;
	if (ptdev->devinfo->flags & FLAG_RASTER_PACKBITS) {
		/* Fake compression by encoding a single uncompressed run */
		buf[1] = (uint8_t)(len + 1);
		buf[2] = 0;
		buf[3] = (uint8_t)(len - 1);
		memcpy(buf + 4, data, len);
		rc = ptouch_send(ptdev, buf, len + 4);
	} else {
		buf[1] = (uint8_t)len;
		buf[2] = 0;
		memcpy(buf + 3, data, len);
		rc = ptouch_send(ptdev, buf, len + 3);
	}
	return rc;
}
