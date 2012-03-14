/* arch/arm/mach-msm/leds-semc_fuji_cdb_v.c
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

struct as3676_platform_data as3676_platform_data = {
	.step_up_vtuning = 18,       /* 0 .. 31 uA on DCDC_FB */
	.audio_speed_down = 1,       /* 0..3 resp. 0, 200, 400, 800ms */
	.audio_speed_up = 4,         /* 0..7 resp. 0, 50, 100, 150,
						 200,250,400, 800ms */
	.audio_agc_ctrl = 1,         /* 0 .. 7: 0==no AGC, 7 very aggressive*/
	.audio_gain = 7,             /* 0..7: -12, -6,  0, 6
					       12, 18, 24, 30 dB */
	.audio_source = 2,           /* 0..3: 0=curr33, 1=DCDC_FB
					      2=GPIO1,  3=GPIO2 */
	.leds[0] = {
		.name = "curr1",
		.on_charge_pump = 0,
		.max_current_uA = 10000,
	},
	.leds[1] = {
		.name = "curr2",
		.on_charge_pump = 0,
		.max_current_uA = 10000,
	},
	.leds[2] = {
		.name = "curr6",
		.on_charge_pump = 0,
		.max_current_uA = 10000,
	},
	.leds[3] = {
		.name = "red",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[4] = {
		.name = "green",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[5] = {
		.name = "blue",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[6] = {
		.name = "curr41",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[7] = {
		.name = "curr42",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[8] = {
		.name = "curr43",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[9] = {
		.name = "curr30",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[10] = {
		.name = "curr31",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[11] = {
		.name = "curr32",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
	.leds[12] = {
		.name = "curr33",
		.on_charge_pump = 1,
		.max_current_uA = 500,
	},
};
