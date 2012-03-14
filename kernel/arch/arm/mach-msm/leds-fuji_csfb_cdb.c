/* arch/arm/mach-msm/leds-fuji_csfb_cdb.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/leds-as3676.h>
#include <linux/leds.h>

static struct as3676_platform_led as3676_leds_mapping[] = {
	{
		.name = "lcd-backlight",
		.sinks = BIT(AS3676_SINK_01) | BIT(AS3676_SINK_02)
		| BIT(AS3676_SINK_06),
		.flags = AS3676_FLAG_ALS,
		.max_current = 10000,
		.default_brightness = LED_FULL,
	},
	{
		.name = "button-backlight",
		.sinks = BIT(AS3676_SINK_RGB1) | BIT(AS3676_SINK_RGB2)
		| BIT(AS3676_SINK_RGB3),
		.flags = AS3676_FLAG_BLINK,
		.max_current = 500,
	},
	{
		.name = "keyboard-backlight",
		.sinks = BIT(AS3676_SINK_30) | BIT(AS3676_SINK_31)
		| BIT(AS3676_SINK_32) | BIT(AS3676_SINK_33),
		.max_current = 500,
	},
	{
		.name = "red",
		.sinks = BIT(AS3676_SINK_41),
		.flags = AS3676_FLAG_BLINK,
		.max_current = 500,
	},
	{
		.name = "green",
		.sinks = BIT(AS3676_SINK_42),
		.flags = AS3676_FLAG_BLINK,
		.max_current = 500,
	},
	{
		.name = "blue",
		.sinks = BIT(AS3676_SINK_43),
		.flags = AS3676_FLAG_BLINK,
		.max_current = 500,
	},
};

struct as3676_platform_data as3676_platform_data = {
	.leds = as3676_leds_mapping,
	.num_leds = ARRAY_SIZE(as3676_leds_mapping),
	.als_connected = 1,
	.als_wait = 100,
	.dls_connected = false,
	.softdim_enable = true,
	.ldo_mV = 2500,
};
