/* arch/arm/mach-msm/keypad-fuji_nozomi_mep.c
 *
 * Copyright (C) [2011] Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/input/pmic8058-keypad.h>
#ifdef CONFIG_INPUT_ASETM2034A
#include <linux/input/asetm2034a.h>
#endif /* CONFIG_INPUT_ASETM2034A */

static const unsigned int fuji_keymap[] = {
	KEY(1, 0, KEY_CAMERA_FOCUS),
	KEY(1, 1, KEY_CAMERA),
	KEY(1, 2, KEY_VOLUMEDOWN),
	KEY(1, 3, KEY_VOLUMEUP),
};

static struct matrix_keymap_data fuji_keymap_data = {
	.keymap_size	= ARRAY_SIZE(fuji_keymap),
	.keymap		= fuji_keymap,
};

struct pmic8058_keypad_data fuji_keypad_data = {
	.input_name		= "fuji-keypad",
	.input_phys_device	= "fuji-keypad/input0",
	.num_rows		= 5,
	.num_cols		= 5,
	.rows_gpio_start	= 8,
	.cols_gpio_start	= 0,
	.debounce_ms		= {8, 10},
	.scan_delay_ms		= 2,
	.row_hold_ns            = 122000,
	.wakeup			= 1,
	.keymap_data		= &fuji_keymap_data,
};

#ifdef CONFIG_INPUT_ASETM2034A
static struct asetm2034a_key asetm2034a_keys[] = {
	{ ASETM2034A_SW1,	KEY_MENU	},
	{ ASETM2034A_SW2,	KEY_HOME	},
	{ ASETM2034A_SW3,	KEY_BACK	},
	{ ASETM2034A_IDLE2,	KEY_WAKEUP	}, /* temporary */
};

struct asetm2034a_keymap asetm2034a_keymap = {
	.keys = asetm2034a_keys,
	.num_keys = ARRAY_SIZE(asetm2034a_keys),
};
#endif /* CONFIG_INPUT_ASETM2034A */
