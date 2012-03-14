/* arch/arm/mach-msm/touch-semc_fuji_cdb.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Yusuke Yoshimura <Yusuke.Yoshimura@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/input.h>
#include <linux/clearpad.h>

struct synaptics_button synaptics_back_key = {
	.code = KEY_BACK,
};

struct synaptics_button synaptics_home_key = {
	.code = KEY_HOME,
};

struct synaptics_button synaptics_menu_key = {
	.code = KEY_MENU,
};

struct synaptics_funcarea clearpad_funcarea_array[] = {
	{
		{ 0, 0, 719, 1279 }, { 0, 0, 719, 1299 },
		SYN_FUNCAREA_POINTER, NULL
	},
	{
		{ 0, 1310, 219, 1327 }, { 0, 1290, 239, 1327 },
		SYN_FUNCAREA_BUTTON, &synaptics_back_key
	},
	{
		{ 260, 1310, 459, 1327 }, { 240, 1290, 479, 1327 },
		SYN_FUNCAREA_BUTTON, &synaptics_home_key
	},
	{
		{ 500, 1310, 719, 1327 }, { 480, 1290, 719, 1327 },
		SYN_FUNCAREA_BUTTON, &synaptics_menu_key
	},
	{ .func = SYN_FUNCAREA_END }
};
