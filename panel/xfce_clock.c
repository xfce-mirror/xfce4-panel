/*  xfce4
 *  Copyright (C) 1999 Olivier Fourdan (fourdan@xfce.org)
 *  Copyright (C) 2002 Jasper Huijsmans (j.b.huijsmans@hetnet.nl)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* Original code by Olivier Fourdan,
   port to gtk2 by Jasper Huijsmans
   improvements and maintenance by Xavier Maillard
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PANGO_ENABLE_BACKEND 1

#include <gtk/gtkmain.h>
#include <pango/pango.h>
#include "xfce_clock.h"

#define DIGITS_WIDTH 234
#define DIGITS_HEIGHT 74
static unsigned char digits_bits[] = {
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x38, 0x8e, 0xe3, 0x38, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x04, 0x41, 0x51, 0x10, 0x40,
    0x51, 0x14, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x11, 0x04, 0x41, 0x51, 0x10, 0x40, 0x51, 0x14, 0x45, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x04, 0x41, 0x51, 0x10, 0x40,
    0x51, 0x14, 0x45, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc1, 0x18, 0x43,
    0x70, 0x02, 0x00, 0xc0, 0x3b, 0x91, 0xf3, 0x10, 0x11, 0x00, 0x00, 0x00,
    0x00, 0xe0, 0x38, 0x8e, 0xe3, 0x00, 0x8e, 0xe3, 0x38, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x62, 0x22, 0xa5, 0x64, 0x88, 0x2a, 0x25, 0x45, 0x44, 0x91,
    0x14, 0x11, 0x13, 0x00, 0x00, 0x00, 0x11, 0x14, 0x40, 0x10, 0x14, 0x41,
    0x11, 0x14, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x02, 0xa5, 0x44,
    0x88, 0x2a, 0x55, 0xc3, 0x45, 0x91, 0x14, 0x29, 0x15, 0x00, 0x00, 0x00,
    0x11, 0x14, 0x40, 0x10, 0x14, 0x41, 0x11, 0x14, 0x05, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x12, 0x82, 0xa4, 0x44, 0x88, 0x2a, 0x75, 0x41, 0x44, 0x91,
    0x13, 0x39, 0x15, 0x00, 0x00, 0x00, 0x11, 0x14, 0x40, 0x10, 0x14, 0x41,
    0x11, 0x14, 0x05, 0x04, 0x00, 0x00, 0x00, 0x00, 0x12, 0x42, 0xa4, 0x44,
    0x88, 0xaa, 0x14, 0x41, 0x44, 0x91, 0x14, 0x45, 0x19, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x62, 0xe2, 0x19, 0x43, 0x70, 0x4a, 0x64, 0x41, 0x38, 0x8e,
    0xf4, 0x44, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x3e, 0x3e, 0x00, 0x3e,
    0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x5d, 0x40, 0x5c, 0x5c, 0x41, 0x1d, 0x1d, 0x5c, 0x5d, 0x5d, 0x5d, 0x5d,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x60, 0x60, 0x60, 0x63, 0x03,
    0x03, 0x60, 0x63, 0x63, 0x63, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x63, 0x60, 0x60, 0x60, 0x63, 0x03, 0x03, 0x60, 0x63, 0x63, 0x63, 0x63,
    0x18, 0x00, 0x00, 0x08, 0x05, 0x01, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x60, 0x60, 0x60, 0x63, 0x03,
    0x03, 0x60, 0x63, 0x63, 0x63, 0x63, 0x18, 0x00, 0x1c, 0x08, 0x01, 0x01,
    0x10, 0x00, 0x10, 0x00, 0xc7, 0x11, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x41, 0x40, 0x5e, 0x5e, 0x5d, 0x3d, 0x3f, 0x40, 0x5d, 0x5d, 0x5d, 0x5d,
    0x18, 0x00, 0xa4, 0x3a, 0x65, 0x47, 0x1c, 0x75, 0x9c, 0x94, 0x40, 0x12,
    0x94, 0xc9, 0x31, 0x01, 0x00, 0x00, 0x41, 0x40, 0x3d, 0x7e, 0x7e, 0x5e,
    0x5d, 0x40, 0x7f, 0x7e, 0x7f, 0x3f, 0x00, 0x00, 0xa4, 0x4a, 0x15, 0xa9,
    0x12, 0x95, 0x52, 0x8d, 0x4c, 0x12, 0x54, 0x54, 0x8a, 0x02, 0x00, 0x00,
    0x63, 0x60, 0x03, 0x60, 0x60, 0x60, 0x63, 0x60, 0x63, 0x60, 0x63, 0x03,
    0x18, 0x00, 0x9c, 0x4a, 0x65, 0xe9, 0x12, 0x95, 0xd2, 0x85, 0xc8, 0x11,
    0x54, 0x5c, 0xb2, 0x03, 0x00, 0x00, 0x63, 0x60, 0x03, 0x60, 0x60, 0x60,
    0x63, 0x60, 0x63, 0x60, 0x63, 0x03, 0x18, 0x00, 0x84, 0x4a, 0x45, 0x29,
    0x12, 0x95, 0x52, 0x84, 0x48, 0x10, 0x54, 0x44, 0xa2, 0x00, 0x00, 0x00,
    0x63, 0x60, 0x03, 0x60, 0x60, 0x60, 0x63, 0x60, 0x63, 0x60, 0x63, 0x03,
    0x18, 0x00, 0x04, 0x3b, 0x35, 0xc9, 0x1c, 0x96, 0x9c, 0x05, 0x47, 0x70,
    0x94, 0x59, 0x1a, 0x03, 0x00, 0x00, 0x5d, 0x40, 0x1d, 0x5c, 0x40, 0x5c,
    0x5d, 0x40, 0x5d, 0x5c, 0x41, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3e, 0x00, 0x3e, 0x3e, 0x00, 0x3e, 0x3e, 0x00, 0x3e, 0x3e, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0xf8, 0x81, 0x1f,
    0x00, 0x80, 0x1f, 0xf8, 0x81, 0x1f, 0xf8, 0x81, 0x1f, 0xf8, 0x81, 0x1f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf8, 0x01, 0x00, 0xf8, 0x81, 0x1f, 0x00, 0x80, 0x1f, 0xf8, 0x81, 0x1f,
    0xf8, 0x81, 0x1f, 0xf8, 0x81, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x00, 0x06, 0x60,
    0x06, 0x66, 0x00, 0x06, 0x00, 0x60, 0x06, 0x66, 0x60, 0x06, 0x66, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x60, 0x00, 0x06, 0x60, 0x06, 0x66, 0x00, 0x06, 0x00, 0x60,
    0x06, 0x66, 0x60, 0x06, 0x66, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x00, 0x06, 0x60,
    0x06, 0x66, 0x00, 0x06, 0x00, 0x60, 0x06, 0x66, 0x60, 0x06, 0x66, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x60, 0x00, 0x06, 0x60, 0x06, 0x66, 0x00, 0x06, 0x00, 0x60,
    0x06, 0x66, 0x60, 0x06, 0x66, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x00, 0x06, 0x60,
    0x06, 0x66, 0x00, 0x06, 0x00, 0x60, 0x06, 0x66, 0x60, 0x06, 0x66, 0x60,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x60, 0x00, 0x06, 0x60, 0x06, 0x66, 0x00, 0x06, 0x00, 0x60,
    0x06, 0x66, 0x60, 0x06, 0x66, 0x60, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x81, 0x1f,
    0xf8, 0x81, 0x1f, 0xf8, 0x01, 0x00, 0xf8, 0x81, 0x1f, 0xf8, 0x81, 0x1f,
    0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xf8, 0x81, 0x1f, 0xf8, 0x81, 0x1f, 0xf8, 0x01, 0x00,
    0xf8, 0x81, 0x1f, 0xf8, 0x81, 0x1f, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x06, 0x00, 0x60,
    0x00, 0x06, 0x60, 0x06, 0x06, 0x60, 0x06, 0x06, 0x60, 0x06, 0x66, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x60, 0x06, 0x00, 0x60, 0x00, 0x06, 0x60, 0x06, 0x06, 0x60,
    0x06, 0x06, 0x60, 0x06, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x06, 0x00, 0x60,
    0x00, 0x06, 0x60, 0x06, 0x06, 0x60, 0x06, 0x06, 0x60, 0x06, 0x66, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x60, 0x06, 0x00, 0x60, 0x00, 0x06, 0x60, 0x06, 0x06, 0x60,
    0x06, 0x06, 0x60, 0x06, 0x66, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06, 0x60, 0x06, 0x00, 0x60,
    0x00, 0x06, 0x60, 0x06, 0x06, 0x60, 0x06, 0x06, 0x60, 0x06, 0x66, 0x00,
    0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x06, 0x60, 0x06, 0x00, 0x60, 0x00, 0x06, 0x60, 0x06, 0x06, 0x60,
    0x06, 0x06, 0x60, 0x06, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x01, 0x00, 0xf8, 0x81, 0x1f,
    0x00, 0x80, 0x1f, 0xf8, 0x01, 0x00, 0xf8, 0x81, 0x1f, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf8, 0x01, 0x00, 0xf8, 0x81, 0x1f, 0x00, 0x80, 0x1f, 0xf8, 0x01, 0x00,
    0xf8, 0x81, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0x1f, 0x00, 0x00, 0x00, 0xff, 0x01, 0xfc, 0x07, 0x00, 0x00, 0xc0,
    0x7f, 0x00, 0xff, 0x01, 0xfc, 0x07, 0xf0, 0x1f, 0xc0, 0x7f, 0x00, 0xff,
    0x01, 0xfc, 0x07, 0x00, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0x80, 0xff,
    0x03, 0xfe, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0x80, 0xff, 0x03, 0xfe, 0x0f,
    0xf8, 0x3f, 0xe0, 0xff, 0x80, 0xff, 0x03, 0xfe, 0x0f, 0x00, 0x00, 0x00,
    0xf4, 0x5f, 0x00, 0x00, 0x01, 0xff, 0x05, 0xfc, 0x17, 0x04, 0x40, 0xd0,
    0x7f, 0x40, 0xff, 0x01, 0xfc, 0x17, 0xf4, 0x5f, 0xd0, 0x7f, 0x41, 0xff,
    0x05, 0xfd, 0x17, 0x00, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x38,
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x38, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38,
    0x00, 0xe0, 0x00, 0x00, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x38, 0x00, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x38,
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x38, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38,
    0x00, 0xe0, 0x00, 0x00, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x38, 0x00, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x38,
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x38, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38,
    0x00, 0xe0, 0x00, 0x00, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x38, 0x00, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x38,
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x38, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38,
    0x00, 0xe0, 0x00, 0x00, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x38, 0x00, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x38,
    0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x38, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0x03, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x38,
    0x00, 0xe0, 0x00, 0x00, 0x00, 0x38, 0x0e, 0xe0, 0x38, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x38, 0x80, 0x03, 0x00, 0x04, 0x40, 0x00, 0x00, 0x01, 0xff,
    0x05, 0xfc, 0x17, 0xf4, 0x5f, 0xd0, 0x7f, 0x40, 0xff, 0x01, 0x00, 0x10,
    0xf4, 0x5f, 0xd0, 0x7f, 0x41, 0xff, 0x05, 0xfd, 0x17, 0x80, 0x03, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0x03, 0xfe, 0x0f, 0xf8, 0x3f, 0xe0,
    0xff, 0x80, 0xff, 0x03, 0x00, 0x00, 0xf8, 0x3f, 0xe0, 0xff, 0x80, 0xff,
    0x03, 0xfe, 0x0f, 0x80, 0x03, 0x00, 0x04, 0x40, 0x00, 0x00, 0x41, 0xff,
    0x01, 0xfc, 0x17, 0xf0, 0x5f, 0xc0, 0x7f, 0x41, 0xff, 0x05, 0x00, 0x10,
    0xf4, 0x5f, 0xc0, 0x7f, 0x41, 0xff, 0x05, 0xfd, 0x07, 0x80, 0x03, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x00, 0x00, 0x38, 0x00, 0xe0, 0x00,
    0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x00, 0x80, 0x03, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x00, 0x00, 0x38, 0x00, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x00, 0x00, 0x38, 0x00, 0xe0, 0x00,
    0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x00, 0x00, 0x38, 0x00, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x00, 0x00, 0x38, 0x00, 0xe0, 0x00,
    0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x00, 0x80, 0x03, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x00, 0x00, 0x38, 0x00, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x00, 0x00, 0x38, 0x00, 0xe0, 0x00,
    0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x00, 0x80, 0x03, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x00, 0x00, 0x38, 0x00, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x00, 0x80, 0x03, 0x00,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x00, 0x00, 0x38, 0x00, 0xe0, 0x00,
    0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x8e, 0x03, 0x00, 0x80, 0x03, 0x00, 0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00,
    0x00, 0x00, 0x38, 0x00, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x0e, 0x00, 0x38,
    0x0e, 0xe0, 0x00, 0x80, 0xe3, 0x00, 0x8e, 0x03, 0x00, 0x00, 0x00, 0x00,
    0xf4, 0x5f, 0x00, 0x00, 0x41, 0xff, 0x01, 0xfc, 0x17, 0x00, 0x40, 0xc0,
    0x7f, 0x41, 0xff, 0x05, 0x00, 0x10, 0xf4, 0x5f, 0xc0, 0x7f, 0x41, 0x00,
    0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x3f, 0x00, 0x00, 0x80, 0xff,
    0x03, 0xfe, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0x80, 0xff, 0x03, 0x00, 0x00,
    0xf8, 0x3f, 0xe0, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0x1f, 0x00, 0x00, 0x00, 0xff, 0x01, 0xfc, 0x07, 0x00, 0x00, 0xc0,
    0x7f, 0x00, 0xff, 0x01, 0x00, 0x00, 0xf0, 0x1f, 0xc0, 0x7f, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define MY_CLOCK_DEFAULT_SIZE 50

#ifndef M_PI
#define M_PI 3.141592654
#endif

#define DIGITS_SMALL_WIDTH    6
#define DIGITS_SMALL_HEIGHT  10

#define DIGITS_MEDIUM_WIDTH    8
#define DIGITS_MEDIUM_HEIGHT  14

#define DIGITS_LARGE_WIDTH  12
#define DIGITS_LARGE_HEIGHT 20

#define DIGITS_HUGE_WIDTH   18
#define DIGITS_HUGE_HEIGHT  30



/* Forward declarations */

static void xfce_clock_class_init(XfceClockClass * klass);
static void xfce_clock_init(XfceClock * clock);
static void xfce_clock_destroy(GtkObject * object);
static void xfce_clock_finalize(GObject * object);
static void xfce_clock_realize(GtkWidget * widget);
static void xfce_clock_size_request(GtkWidget * widget,
                                    GtkRequisition * requisition);
static void xfce_clock_size_allocate(GtkWidget * widget,
                                     GtkAllocation * allocation);
static gint xfce_clock_expose(GtkWidget * widget, GdkEventExpose * event);
static void xfce_clock_draw(GtkWidget * widget, GdkRectangle * area);
static gint xfce_clock_timer(XfceClock * clock);
static void xfce_clock_draw_internal(GtkWidget * widget);

static void draw_digits(XfceClock * clock, GdkGC * gc, gint x, gint y, gchar c);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

GtkType xfce_clock_get_type(void)
{
    static GtkType clock_type = 0;

    if(!clock_type)
    {
        static const GTypeInfo clock_info = {
            sizeof(XfceClockClass),
            NULL,               /* base_init */
            NULL,               /* base_finalize */
            (GClassInitFunc) xfce_clock_class_init,
            NULL,               /* class_finalize */
            NULL,               /* class_data */
            sizeof(XfceClock),
            0,                  /* n_preallocs */
            (GInstanceInitFunc) xfce_clock_init
        };

        clock_type =
            g_type_register_static(GTK_TYPE_WIDGET, "XfceClock", &clock_info,
                                   0);
    }

    return clock_type;
}

static void xfce_clock_class_init(XfceClockClass * class)
{
    GtkObjectClass *object_class;
    GObjectClass *gobject_class;
    GtkWidgetClass *widget_class;

    object_class = (GtkObjectClass *) class;
    widget_class = (GtkWidgetClass *) class;
    gobject_class = G_OBJECT_CLASS (class);
    
    parent_class = gtk_type_class(gtk_widget_get_type());

    object_class->destroy = xfce_clock_destroy;
    gobject_class->finalize = xfce_clock_finalize;

    widget_class->realize = xfce_clock_realize;
    widget_class->expose_event = xfce_clock_expose;
    widget_class->size_request = xfce_clock_size_request;
    widget_class->size_allocate = xfce_clock_size_allocate;
}

static void xfce_clock_realize(GtkWidget * widget)
{
    XfceClock *clock;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);
    clock = XFCE_CLOCK(widget);

    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
    widget->window =
        gdk_window_new(widget->parent->window, &attributes, attributes_mask);

    widget->style = gtk_style_attach(widget->style, widget->window);

    gdk_window_set_user_data(widget->window, widget);

    gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
    
    if (!(clock->digits_bmap))
    {
        clock->digits_bmap = gdk_bitmap_create_from_data(widget->window, digits_bits, DIGITS_WIDTH, DIGITS_HEIGHT);
    }
    if (!(clock->timer))
    {
        clock->timer = gtk_timeout_add(clock->interval, (GtkFunction) xfce_clock_timer, (gpointer) clock);
    }
}

static void xfce_clock_init(XfceClock * clock)
{
    time_t ticks;
    struct tm *tm;
    gint h, m, s;

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;
    clock->hrs_angle = 2.5 * M_PI - (h % 12) * M_PI / 6 - m * M_PI / 360;
    clock->min_angle = 2.5 * M_PI - m * M_PI / 30;
    clock->sec_angle = 2.5 * M_PI - s * M_PI / 30;
    clock->timer = 0;
    clock->radius = 0;
    clock->pointer_width = 0;
    clock->mode = XFCE_CLOCK_ANALOG;
    clock->military_time = 0;   /* use 12 hour mode by default */
    clock->display_am_pm = 1;   /* display am or pm by default. */
    clock->display_secs = 0;    /* do not display secs by default */
    clock->interval = 100;      /* 1/10 seconds update interval by default */
    clock->led_size = DIGIT_MEDIUM;
    clock->digits_bmap = NULL;
    clock->timer = 0;
    clock->old_hour = 0;
    clock->old_minute = 0;
    clock->old_second = 0;
}

GtkWidget *xfce_clock_new(void)
{
    return GTK_WIDGET (g_object_new (xfce_clock_get_type (), NULL));
}

static void xfce_clock_destroy(GtkObject * object)
{
    g_return_if_fail(object != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(object));

    if(GTK_OBJECT_CLASS(parent_class)->destroy)
        (*GTK_OBJECT_CLASS(parent_class)->destroy) (object);
}

static void xfce_clock_finalize (GObject *object)
{
    XfceClock *clock;

    g_return_if_fail(object != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(object));

    clock = XFCE_CLOCK(object);

 
    if(clock->digits_bmap)
    {
        gdk_bitmap_unref(clock->digits_bmap);
	clock->digits_bmap = NULL;
    }
    if(clock->timer)
    {
        gtk_timeout_remove(clock->timer);
	clock->timer = 0;
    }

    G_OBJECT_CLASS (parent_class)->finalize (object);
}

void xfce_clock_show_ampm(XfceClock * clock, gboolean show)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    clock->display_am_pm = show;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_ampm_toggle(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->display_am_pm)
        clock->display_am_pm = 0;
    else
        clock->display_am_pm = 1;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

gboolean xfce_clock_ampm_shown(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);
    return (clock->display_am_pm);
}

void xfce_clock_show_secs(XfceClock * clock, gboolean show)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->display_secs = show;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_secs_toggle(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->display_secs)
        clock->display_secs = 0;
    else
        clock->display_secs = 1;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

gboolean xfce_clock_secs_shown(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);
    return (clock->display_secs);
}

void xfce_clock_show_military(XfceClock * clock, gboolean show)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->military_time = show;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_military_toggle(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->military_time)
    {
        clock->military_time = 0;
        xfce_clock_show_ampm(clock, 0);
    }
    else
        clock->military_time = 1;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

gboolean xfce_clock_military_shown(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);
    return (clock->military_time);
}

void xfce_clock_set_interval(XfceClock * clock, guint interval)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->interval = interval;

    if(clock->timer)
    {
        gtk_timeout_remove(clock->timer);
        clock->timer = gtk_timeout_add(clock->interval, (GtkFunction) xfce_clock_timer, (gpointer) clock);
    }
}

guint xfce_clock_get_interval(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, 0);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), 0);
    return (clock->interval);
}

void xfce_clock_set_led_size(XfceClock * clock, XfceClockLedSize size)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->led_size = size;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)) && (clock->mode == XFCE_CLOCK_LEDS))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

XfceClockLedSize xfce_clock_get_led_size(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, 0);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), 0);
    return (clock->led_size);
}

void xfce_clock_suspend(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->timer)
    {
        gtk_timeout_remove(clock->timer);
        clock->timer = 0;
    }
}

void xfce_clock_resume(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if(clock->timer)
        return;
    if(!(clock->interval))
        return;

    clock->timer = gtk_timeout_add(clock->interval, (GtkFunction) xfce_clock_timer, (gpointer) clock);
}

void xfce_clock_set_digit_size(XfceClock * clock, XfceClockMode mode)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->mode = mode;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_set_mode(XfceClock * clock, XfceClockMode mode)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    clock->mode = mode;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

void xfce_clock_toggle_mode(XfceClock * clock)
{
    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));
    switch (clock->mode)
    {
        case XFCE_CLOCK_ANALOG:
            clock->mode = XFCE_CLOCK_DIGITAL;
            break;
        case XFCE_CLOCK_DIGITAL:
            clock->mode = XFCE_CLOCK_LEDS;
            break;
        case XFCE_CLOCK_LEDS:
        default:
            clock->mode = XFCE_CLOCK_ANALOG;
    }
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(clock)))
        xfce_clock_draw(GTK_WIDGET(clock), NULL);
}

XfceClockMode xfce_clock_get_mode(XfceClock * clock)
{
    g_return_val_if_fail(clock != NULL, 0);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), 0);
    return (clock->mode);
}

static void xfce_clock_size_request(GtkWidget * widget,
                                    GtkRequisition * requisition)
{
    gchar buffer[16];
    XfceClock *clock;
    guint ln = 0;
    guint width = 0;
    guint height = 0;
    PangoLayout *layout = NULL;
    PangoRectangle logical_rect;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);
    
    switch (clock->mode)
    {
        case XFCE_CLOCK_DIGITAL:
	    if(clock->military_time)
	    {
	        if (clock->display_secs)
		{
		    strcpy(buffer, "88:88:88");
		}
		else
		{
		    strcpy(buffer, "88:88");
		}
            }
	    else if(clock->display_am_pm)
	    {
        	if(clock->display_secs)
		{
		    strcpy(buffer, "88:88:88XX");
        	}
		else
		{
		    strcpy(buffer, "88:88XX");
		}
	    }
	    else
	    {
        	if(clock->display_secs)
		{
		    strcpy(buffer, "88:88:88");
        	}
		else
		{
		    strcpy(buffer, "88:88");
		}
	    }
	    layout = gtk_widget_create_pango_layout(widget, buffer);
            pango_layout_get_pixel_extents(layout, NULL, &logical_rect);
    	    requisition->width = logical_rect.width + 6;
	    requisition->height = logical_rect.height + 6;
            g_object_unref(G_OBJECT(layout));
            break;
        case XFCE_CLOCK_LEDS:
	    if(clock->military_time)
	    {
	        if (clock->display_secs)
		{
		    ln = 8;
		}
		else
		{
		    ln = 5;
		}
            }
	    else if(clock->display_am_pm)
	    {
        	if(clock->display_secs)
		{
                    ln = 9;
        	}
		else
		{
		    ln = 6;
		}
	    }
	    else
	    {
        	if(clock->display_secs)
		{
                    ln = 8;
        	}
		else
		{
		    ln = 5;
		}
	    }
	    if (clock->led_size == DIGIT_HUGE)
	    {
		width = ln * DIGITS_HUGE_WIDTH;
		height = DIGITS_HUGE_HEIGHT;

	    }
	    else if(clock->led_size == DIGIT_LARGE)
	    {
		width = ln * DIGITS_LARGE_WIDTH;
		height = DIGITS_LARGE_HEIGHT;

	    }
	    else if(clock->led_size == DIGIT_MEDIUM)
	    {
		width = ln * DIGITS_MEDIUM_WIDTH;
		height = DIGITS_MEDIUM_HEIGHT;
	    }
            else
	    {
        	width = ln * DIGITS_SMALL_WIDTH;
        	height = DIGITS_SMALL_HEIGHT;
	    }
    	    requisition->width = width + 6;
	    requisition->height = height + 6;
            break;
        case XFCE_CLOCK_ANALOG:
        default:
	    requisition->width = MY_CLOCK_DEFAULT_SIZE;
	    requisition->height = MY_CLOCK_DEFAULT_SIZE;
    }
}

static void xfce_clock_size_allocate(GtkWidget * widget,
                                     GtkAllocation * allocation)
{
    XfceClock *clock;
    gint size;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(allocation != NULL);


    widget->allocation = *allocation;
    clock = XFCE_CLOCK(widget);

    if(GTK_WIDGET_REALIZED(widget))
        gdk_window_move_resize(widget->window, allocation->x, allocation->y,
                               allocation->width, allocation->height);


    size = MIN(allocation->width, allocation->height);

    clock->radius = size * 0.49;
    clock->internal = size * 0.5;
    clock->pointer_width = MAX(clock->radius / 5, 3);
}

static void draw_ticks(GtkWidget * widget, GdkGC * gc, gint xc, gint yc)
{
    XfceClock *clock;
    gint i;
    gdouble theta;
    gdouble s, c;
    GdkPoint points[5];

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    for(i = 0; i < 12; i++)
    {
        theta = (i * M_PI / 6.);
        s = sin(theta);
        c = cos(theta);

        points[0].x =
            xc + s * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[0].y =
            yc + c * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[1].x =
            xc + s * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[1].y =
            yc + c * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[2].x =
            xc + s * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[2].y =
            yc + c * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[3].x =
            xc + s * (clock->radius - clock->pointer_width / 2) +
            clock->pointer_width / 4;
        points[3].y =
            yc + c * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[4].x =
            xc + s * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;
        points[4].y =
            yc + c * (clock->radius - clock->pointer_width / 2) -
            clock->pointer_width / 4;

        gdk_draw_polygon(widget->window, gc, TRUE, points, 5);
    }

}

static void draw_sec_pointer(GtkWidget * widget, GdkGC * gc, gint xc, gint yc)
{
    XfceClock *clock;
    GdkPoint points[6];
    gdouble s, c;
    gdouble width;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(gc != NULL);

    clock = XFCE_CLOCK(widget);

    s = sin(clock->sec_angle);
    c = cos(clock->sec_angle);
    width = (gdouble) MAX (clock->pointer_width / 2, 1);

    points[0].x = xc + s * width;
    points[0].y = yc + c * width;
    points[1].x = xc + c * clock->radius + s * 0.5;
    points[1].y = yc - s * clock->radius + c * 0.5;
    points[2].x = xc + c * clock->radius - s * 0.5;
    points[2].y = yc - s * clock->radius - c * 0.5;
    points[3].x = xc - s * width;
    points[3].y = yc - c * width;
    points[4].x = xc - c * width;
    points[4].y = yc + s * width;
    points[5].x = xc + s * width;
    points[5].y = yc + c * width;

    gdk_draw_polygon(widget->window, gc, TRUE, points, 6);
}

static void draw_min_pointer(GtkWidget * widget, GdkGC * gc, gint xc, gint yc)
{
    XfceClock *clock;
    GdkPoint points[6];
    gdouble s, c;
    gdouble width;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(gc != NULL);

    clock = XFCE_CLOCK(widget);

    s = sin(clock->min_angle);
    c = cos(clock->min_angle);
    width = (gdouble) MAX (clock->pointer_width / 2, 1);

    points[0].x = xc + s * width;
    points[0].y = yc + c * width;
    points[1].x = xc + 3.0 * c * clock->radius / 4.0 + s * 0.5;
    points[1].y = yc - 3.0 * s * clock->radius / 4.0 + c * 0.5;
    points[2].x = xc + 3.0 * c * clock->radius / 4.0 - s * 0.5;
    points[2].y = yc - 3.0 * s * clock->radius / 4.0 - c * 0.5;
    points[3].x = xc - s * width;
    points[3].y = yc - c * width;
    points[4].x = xc - c * width;
    points[4].y = yc + s * width;
    points[5].x = xc + s * width;
    points[5].y = yc + c * width;

    gdk_draw_polygon(widget->window, gc, TRUE, points, 6);
}

static void draw_hrs_pointer(GtkWidget * widget, GdkGC * gc, gint xc, gint yc)
{
    XfceClock *clock;
    GdkPoint points[6];
    gdouble s, c;
    gdouble width;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(gc != NULL);

    clock = XFCE_CLOCK(widget);

    s = sin(clock->hrs_angle);
    c = cos(clock->hrs_angle);
    width = (gdouble) MAX (clock->pointer_width / 2, 1);

    points[0].x = xc + s * width;
    points[0].y = yc + c * width;
    points[1].x = xc + 3.0 * c * clock->radius / 5.0 + s * 0.5;
    points[1].y = yc - 3.0 * s * clock->radius / 5.0 + c * 0.5;
    points[2].x = xc + 3.0 * c * clock->radius / 5.0 - s * 0.5;
    points[2].y = yc - 3.0 * s * clock->radius / 5.0 - c * 0.5;
    points[3].x = xc - s * width;
    points[3].y = yc - c * width;
    points[4].x = xc - c * width;
    points[4].y = yc + s * width;
    points[5].x = xc + s * width;
    points[5].y = yc + c * width;

    gdk_draw_polygon(widget->window, gc, TRUE, points, 6);
}


static void xfce_clock_draw_digital(GtkWidget * widget)
{
    XfceClock *clock;
    time_t ticks;
    struct tm *tm;
    gint h, m, s;
    gint x, y;
    gchar ampm[3] = "AM";
    gchar time_buf[24];
    gint width, height;         /* to measure out string. */
    PangoLayout *layout;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;

    if(h >= 12)
        ampm[0] = 'P';

    if(!(clock->military_time))
    {
        if(h > 12)
            h -= 12;
        if(h == 0)
            h = 12;
    }

    if(clock->military_time)
    {
        if(clock->display_secs)
            sprintf(time_buf, "%d:%02d:%02d", h, m, s);
        else
            sprintf(time_buf, "%d:%02d", h, m);
    }
    else if(clock->display_am_pm)
    {
        if(clock->display_secs)
            sprintf(time_buf, "%d:%02d:%02d %s", h, m, s, ampm);
        else
            sprintf(time_buf, "%d:%02d %s", h, m, ampm);
    }
    else
    {
        if(clock->display_secs)
            sprintf(time_buf, "%d:%02d:%02d", h, m, s);
        else
            sprintf(time_buf, "%d:%02d", h, m);
    }

    gdk_window_clear_area( widget->window,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
    layout = gtk_widget_create_pango_layout(widget, time_buf);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    pango_layout_get_pixel_size(layout, &width, &height);
    x = (widget->allocation.width - width) / 2;
    y = (widget->allocation.height - height) / 2;

    gdk_draw_layout(widget->window, widget->style->text_gc[GTK_STATE_NORMAL], x,
                    y, layout);
}

static void xfce_clock_draw_analog(GtkWidget * widget)
{
    XfceClock *clock;

    gint xc, yc;
    
    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    gdk_window_clear_area( widget->window,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
    
    xc = widget->allocation.width / 2 + 1;
    yc = widget->allocation.height / 2 + 1;

    /* 
     * Here we decide arbitrary that if the clock widget is smaller than 
     * 20 pixels, we don't draw the shadow.
     */
    if (MAX (xc, yc) >= 20)
    {
	draw_ticks(widget, widget->style->dark_gc[GTK_STATE_NORMAL], xc, yc);
	draw_hrs_pointer(widget, widget->style->dark_gc[GTK_STATE_NORMAL], xc, yc);
	draw_min_pointer(widget, widget->style->dark_gc[GTK_STATE_NORMAL], xc, yc);
	if(clock->display_secs)
	{
            draw_sec_pointer(widget, widget->style->dark_gc[GTK_STATE_NORMAL], xc, yc);
	}
    }
    draw_ticks(widget, widget->style->text_gc[GTK_STATE_NORMAL], xc - 1, yc - 1);
    draw_hrs_pointer(widget, widget->style->text_gc[GTK_STATE_NORMAL], xc - 1, yc - 1);
    draw_min_pointer(widget, widget->style->text_gc[GTK_STATE_NORMAL], xc - 1, yc - 1);
    if(clock->display_secs)
    {
        draw_sec_pointer(widget, widget->style->text_gc[GTK_STATE_NORMAL], xc - 1, yc - 1);
    }
}

static void xfce_clock_draw_leds(GtkWidget * widget)
{
    XfceClock *clock;
    time_t ticks;
    struct tm *tm;
    gint h, m, s;
    gint x, y;
    guint c_width = 0;
    guint c_height = 0;
    guint len, i;
    gchar ampm[2] = "a";
    gchar separator[2] = ":";
    gchar time_buf[12];

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;

    if(h >= 12)
        ampm[0] = 'p';

    if(s & 1)
        separator[0] = ':';
    else
        separator[0] = ' ';

    if(!(clock->military_time))
    {
        if(h > 12)
            h -= 12;
        if(h == 0)
            h = 12;
    }

    if(clock->military_time)
    {
        if(clock->display_secs)
            sprintf(time_buf, "%02d%s%02d%s%02d", h, separator, m, separator,
                    s);
        else
            sprintf(time_buf, "%02d%s%02d", h, separator, m);
    }
    else if(clock->display_am_pm)
    {

        if(clock->display_secs)
            sprintf(time_buf, "%02d%s%02d%s%02d%s", h, separator, m,
                    separator, s, ampm);
        else
            sprintf(time_buf, "%02d%s%02d%s", h, separator, m, ampm);
    }
    else
    {
        if(clock->display_secs)
            sprintf(time_buf, "%02d%s%02d%s%02d", h, separator, m,
                    separator, s);
        else
            sprintf(time_buf, "%02d%s%02d", h, separator, m);
    }
    if(time_buf[0] == '0')
        time_buf[0] = ' ';

    len = strlen(time_buf);


    if(clock->led_size == DIGIT_HUGE)
    {
        c_width = DIGITS_HUGE_WIDTH;
        c_height = DIGITS_HUGE_HEIGHT;
    }
    else if(clock->led_size == DIGIT_LARGE)
    {
        c_width = DIGITS_LARGE_WIDTH;
        c_height = DIGITS_LARGE_HEIGHT;
    }
    else if(clock->led_size == DIGIT_MEDIUM)
    {
        c_width = DIGITS_MEDIUM_WIDTH;
        c_height = DIGITS_MEDIUM_HEIGHT;
    }
    else
    {
        c_width = DIGITS_SMALL_WIDTH;
        c_height = DIGITS_SMALL_HEIGHT;

    }

    /* Center in the widget (if it fits in) */
    if( (x = (widget->allocation.width - (c_width * len))) <= 0 )
    {
         x = 0;
    }
    
    x = x / 2;
    y = (widget->allocation.height - c_height) / 2;


    gdk_window_clear_area (widget->window,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
    for(i = 0; i < len; i++)
    {
        draw_digits(clock, widget->style->dark_gc[GTK_STATE_NORMAL],
                    x + i * c_width + 1, y + 1, time_buf[i]);
        draw_digits(clock, widget->style->text_gc[GTK_STATE_NORMAL],
                    x + i * c_width, y, time_buf[i]);
    }
}

static void xfce_clock_draw_internal(GtkWidget * widget)
{
    XfceClock *clock;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));

    clock = XFCE_CLOCK(widget);

    if(GTK_WIDGET_DRAWABLE(widget))
    {
        switch (clock->mode)
        {
            case XFCE_CLOCK_ANALOG:
                xfce_clock_draw_analog(widget);
                break;
            case XFCE_CLOCK_LEDS:
                xfce_clock_draw_leds(widget);
                break;
            case XFCE_CLOCK_DIGITAL:
            default:
                xfce_clock_draw_digital(widget);
                break;
        }
    }
}

static gint xfce_clock_expose(GtkWidget * widget, GdkEventExpose * event)
{
    g_return_val_if_fail(widget != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(widget), FALSE);
    g_return_val_if_fail(event != NULL, FALSE);
    g_return_val_if_fail(GTK_WIDGET_DRAWABLE(widget), FALSE);
    g_return_val_if_fail(!GTK_WIDGET_NO_WINDOW(widget), FALSE);

    if(event->count > 0)
        return FALSE;

    xfce_clock_draw(widget, NULL);

    return FALSE;
}

static void xfce_clock_draw(GtkWidget * widget, GdkRectangle * area)
{
    XfceClock *clock;

    g_return_if_fail(widget != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(widget));
    g_return_if_fail(GTK_WIDGET_DRAWABLE(widget));
    g_return_if_fail(!GTK_WIDGET_NO_WINDOW(widget));

    clock = XFCE_CLOCK(widget);
     if(clock->mode == XFCE_CLOCK_ANALOG)
	gdk_window_clear_area (widget->window,
			       0, 0,
			       widget->allocation.width,
			       widget->allocation.height);

    xfce_clock_draw_internal(widget);
}

static gint xfce_clock_timer(XfceClock * clock)
{
    time_t ticks;
    struct tm *tm;
    gint h, m, s;

    g_return_val_if_fail(clock != NULL, FALSE);
    g_return_val_if_fail(XFCE_IS_CLOCK(clock), FALSE);

    GDK_THREADS_ENTER ();

    ticks = time(0);
    tm = localtime(&ticks);
    h = tm->tm_hour;
    m = tm->tm_min;
    s = tm->tm_sec;
    if(!(((!((clock->display_secs) || (clock->mode == XFCE_CLOCK_LEDS))) || (s == clock->old_second)) && (m == clock->old_minute) && (h == clock->old_hour)))
    {
        clock->old_hour   = h;
        clock->old_minute = m;
        clock->old_second = s;
        clock->hrs_angle = 2.5 * M_PI - (h % 12) * M_PI / 6 - m * M_PI / 360;
        clock->min_angle = 2.5 * M_PI - m * M_PI / 30;
        clock->sec_angle = 2.5 * M_PI - s * M_PI / 30;
        xfce_clock_draw_internal(GTK_WIDGET(clock));
    }

    GDK_THREADS_LEAVE ();
 
    return TRUE;
}

static void draw_digits(XfceClock * clock, GdkGC * gc, gint x, gint y, gchar c)
{
    gint tsx, tsy, tsw, tsh;
    guint tc = 0;

    g_return_if_fail(clock != NULL);
    g_return_if_fail(XFCE_IS_CLOCK(clock));

    if((c >= '0') && (c <= '9'))
        tc = c - '0';
    else if((c == 'A') || (c == 'a'))
        tc = 10;
    else if((c == 'P') || (c == 'p'))
        tc = 11;
    else if(c == ':')
        tc = 12;
    else
        return;

    switch (clock->led_size)
    {
        case DIGIT_HUGE:
            tsx = tc * DIGITS_HUGE_WIDTH;
            tsy = DIGITS_SMALL_HEIGHT + DIGITS_MEDIUM_HEIGHT + DIGITS_LARGE_HEIGHT;
            tsw = DIGITS_HUGE_WIDTH;
            tsh = DIGITS_HUGE_HEIGHT;
            break;
        case DIGIT_LARGE:
            tsx = tc * DIGITS_LARGE_WIDTH;
            tsy =  DIGITS_SMALL_HEIGHT + DIGITS_MEDIUM_HEIGHT;
            tsw = DIGITS_LARGE_WIDTH;
            tsh = DIGITS_LARGE_HEIGHT;
            break;
        case DIGIT_MEDIUM:
            tsx = tc * DIGITS_MEDIUM_WIDTH;
            tsy = DIGITS_SMALL_HEIGHT;
            tsw = DIGITS_MEDIUM_WIDTH;
            tsh = DIGITS_MEDIUM_HEIGHT;
            break;
        case DIGIT_SMALL:
        default:
	    tsx = tc * DIGITS_SMALL_WIDTH;
            tsy = 0;
            tsw = DIGITS_SMALL_WIDTH;
            tsh = DIGITS_SMALL_HEIGHT;
            break;
    }

    gdk_gc_set_stipple(gc, clock->digits_bmap);
    gdk_gc_set_fill(gc, GDK_STIPPLED);
    gdk_gc_set_ts_origin(gc, DIGITS_WIDTH - tsx + x, DIGITS_HEIGHT - tsy + y);

    gdk_draw_rectangle(GTK_WIDGET(clock)->window, gc, TRUE, x, y, tsw, tsh);

    gdk_gc_set_fill(gc, GDK_SOLID);

}
