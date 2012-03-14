#ifndef __ARCH_ARM_MACH_MSM_KEYPAD_PMIC_ZEUS_H
#define __ARCH_ARM_MACH_MSM_KEYPAD_PMIC_ZEUS_H

struct keypad_pmic_zeus_key {
	unsigned int code;
	int irq;
	int gpio;
	int wake;
};

struct keypad_pmic_zeus_platform_data {
	struct keypad_pmic_zeus_key *keymap;
	int keymap_size;
};

#endif
