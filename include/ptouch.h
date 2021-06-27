/*
	ptouch-print - Print labels with images or text on a Brother P-Touch

	Copyright (C) 2015-2021 Dominic Radermacher <dominic@familie-radermacher.ch>

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

#include <stdint.h>
#include <libusb-1.0/libusb.h>

struct _pt_tape_info {
	uint8_t mm;		/* Tape width in mm */
	uint16_t px;		/* Printing area in px */
	double margins;		/* default tape margins in mm */
};

#define FLAG_NONE		(0)
#define FLAG_UNSUP_RASTER	(1 << 0)
#define FLAG_RASTER_PACKBITS	(1 << 1)
#define FLAG_PLITE		(1 << 2)
#define FLAG_P700_INIT		(1 << 3)

typedef enum _pt_page_flags {
	FEED_NONE	= 0x0,
	FEED_SMALL	= 0x08,
	FEED_MEDIUM	= 0x0c,
	FEED_LARGE	= 0x1a,
	AUTO_CUT	= (1 << 6),
	MIRROR		= (1 << 7),
} pt_page_flags;

struct _pt_dev_info {
	int vid;		/* USB vendor ID */
	int pid;		/* USB product ID */
	char *name;
	int max_px;		/* Maximum pixel width that can be printed */
	int dpi;		/* Dots per inch of the printhead */
	//size_t bytes_per_line;
	int flags;
};
typedef struct _pt_dev_info *pt_dev_info;

struct __attribute__((packed, aligned(4))) _ptouch_stat {
	uint8_t printheadmark;	// 0x80
	uint8_t size;		// 0x20
	uint8_t brother_code;	// "B"
	uint8_t series_code;	// "0"
	uint8_t model;
	uint8_t country;	// "0"
	uint16_t reserved_1;
	uint16_t error;		// table 1 and 2
	uint8_t media_width;	// tape width in mm
	uint8_t media_type;	// table 4
	uint8_t ncol;		// 0
	uint8_t fonts;		// 0
	uint8_t jp_fonts;	// 0
	uint8_t mode;
	uint8_t density;	// 0
	uint8_t media_len;	// table length, always 0
	uint8_t status_type;	// table 5
	uint8_t phase_type;
	uint16_t phase_number;	// table 6
	uint8_t notif_number;
	uint8_t exp;		// 0
	uint8_t tape_color;	// table 8
	uint8_t text_color;	// table 9
	uint32_t hw_setting;
	uint16_t reserved_2;
};
typedef struct _ptouch_stat *pt_dev_stat;

struct _ptouch_dev {
	libusb_device_handle *h;
	pt_dev_info devinfo;
	pt_dev_stat status;
	uint16_t tape_width_px;
};
typedef struct _ptouch_dev *ptouch_dev;

int ptouch_open(ptouch_dev *ptdev);
int ptouch_close(ptouch_dev ptdev);
int ptouch_send(ptouch_dev ptdev, uint8_t *data, size_t len);
int ptouch_init(ptouch_dev ptdev);
int ptouch_lf(ptouch_dev ptdev);
int ptouch_ff(ptouch_dev ptdev);
size_t ptouch_get_max_pixel_width(ptouch_dev ptdev);
size_t ptouch_get_tape_width(ptouch_dev ptdev);
int ptouch_page_flags(ptouch_dev ptdev, uint8_t page_flags);
int ptouch_eject(ptouch_dev ptdev);
int ptouch_getstatus(ptouch_dev ptdev);
int ptouch_getmaxwidth(ptouch_dev ptdev);
int ptouch_enable_packbits(ptouch_dev ptdev);
int ptouch_rasterstart(ptouch_dev ptdev);
int ptouch_sendraster(ptouch_dev ptdev, uint8_t *data, size_t len);
void ptouch_list_supported();
