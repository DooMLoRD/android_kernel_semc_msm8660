/* Copyright (c) 2011, Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#ifndef __ARCH_ARM_MACH_MSM_KEYPAD_PMIC_FUJI_H
#define __ARCH_ARM_MACH_MSM_KEYPAD_PMIC_FUJI_H

#define KP_NAME "keypad-pmic"
#define KP_DEVICE "dev/keypad-pmic"

struct keypad_pmic_fuji_key {
	unsigned int code;
	int irq;
	int gpio;
	int wake;
	ktime_t debounce_time;
};

struct keypad_pmic_fuji_platform_data {
	struct keypad_pmic_fuji_key *keymap;
	int keymap_size;
	char *input_name;
	struct pm8058_gpio *pm_gpio_config;
};

extern struct keypad_pmic_fuji_platform_data keypad_pmic_platform_data;
#endif
