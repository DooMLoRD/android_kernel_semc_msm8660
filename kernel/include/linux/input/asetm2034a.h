/* include/linux/input/asetm2034a.h
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Yukito Naganuma <Yukito.X.Naganuma@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef LINUX_INPUT_ASETM2034A_H
#define LINUX_INPUT_ASETM2034A_H

/* device name */
#define ASETM2034A_NAME	"asetm2034a"

/* switch bit */
#define ASETM2034A_SW1		(1 << 0)
#define ASETM2034A_SW2		(1 << 1)
#define ASETM2034A_SW3		(1 << 2)
#define ASETM2034A_IDLE2	(1 << 7)

struct asetm2034a_key {
	u8 sw_bit;
	int keycode;
};

struct asetm2034a_keymap {
	struct asetm2034a_key *keys;
	int num_keys;
};

struct asetm2034a_platform_data {
	int (*gpio_setup)(void);
	void (*gpio_shutdown)(void);
	int (*hw_reset)(void);
	struct asetm2034a_keymap *keymap;
};

#endif /* LINUX_INPUT_ASETM2034A_H */


