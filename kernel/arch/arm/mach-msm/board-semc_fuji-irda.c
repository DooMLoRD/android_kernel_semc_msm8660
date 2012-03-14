/* arch/arm/mach-msm/board-semc_fuji-irda.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/mfd/pmic8058.h>
#include <linux/delay.h>
#include <linux/err.h>

#define PM_GPIO_IRDA_M_RX   36
#define PM_GPIO_IRDA_M_TX   35
#define PM_GPIO_IRDA_RX     34
#define PM_GPIO_IRDA_TX     22

#define MSM_GPIO_IRDA_PWDOWN 103

static int __init semc_irda_pm_gpio_configure(void)
{
	int ret;
	int i, len;
	struct pm8058_gpio_config {
		int                gpio;
		struct pm8058_gpio cfg;
	};

	static struct pm8058_gpio_config irda_pm_gpio[] = {
		{
			PM_GPIO_IRDA_TX,
			{
				.direction      = PM_GPIO_DIR_OUT,
				.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_L2,
				.out_strength   = PM_GPIO_STRENGTH_MED,
				.function       = PM_GPIO_FUNC_2,
			},
		},
		{
			PM_GPIO_IRDA_RX,
			{
				.direction      = PM_GPIO_DIR_IN,
				.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_L2,
				.out_strength   = PM_GPIO_STRENGTH_NO,
				.function       = PM_GPIO_FUNC_NORMAL,
			},
		},
		{
			PM_GPIO_IRDA_M_TX,
			{
				.direction      = PM_GPIO_DIR_IN,
				.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_S3,
				.out_strength   = PM_GPIO_STRENGTH_NO,
				.function       = PM_GPIO_FUNC_NORMAL,
			},
		},
		{
			PM_GPIO_IRDA_M_RX,
			{
				.direction      = PM_GPIO_DIR_OUT,
				.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_S3,
				.out_strength   = PM_GPIO_STRENGTH_MED,
				.function       = PM_GPIO_FUNC_2,
			},
		},
	};

	len = ARRAY_SIZE(irda_pm_gpio);
	for (i = 0; i < len; i++) {
		ret = pm8058_gpio_config(irda_pm_gpio[i].gpio,
						&irda_pm_gpio[i].cfg);
		if (ret) {
			pr_err("%s pm gpio enable[%d] config failed\n",
				__func__, i);
			return ret;
		}
	}

	return 0;
}

static int semc_irda_power(int enable)
{
	unsigned long usec;

	if (enable)
		usec = 200;
	else
		usec = 1;
	gpio_set_value(MSM_GPIO_IRDA_PWDOWN, enable);
	udelay(usec);

	return 0;
}

static ssize_t irda_msm_ctrl_store(
			struct class *class,
			struct class_attribute *attr,
			const char *buf,
			size_t count)
{
	unsigned long value;
	int ret = 0;

	if (strict_strtoul(buf, 0, &value)) {
		pr_err("%s: Invalid value\n", __func__);
		goto err_out;
	}
	if (value)
		ret = semc_irda_power(1);
	else
		ret = semc_irda_power(0);

	return count;

err_out:
	pr_err("%s: Invalid value %lu\n", __func__, value);
	return 0;
}

static CLASS_ATTR(irda, S_IRUGO | S_IWUGO,
			NULL, irda_msm_ctrl_store);
static struct class irda_class = {
	.name = "irda",
};

static int __init irda_class_init(void)
{
	int error;

	semc_irda_pm_gpio_configure();
	error = class_register(&irda_class);
	if (error) {
		pr_err("%s: class_register failed\n", __func__);
		goto error_class_register;
	}
	error = gpio_request(MSM_GPIO_IRDA_PWDOWN, "IRDA_PWDOWN");
	if (error) {
		pr_err("%s: GPIO %d: gpio_request failed\n", __func__,
			MSM_GPIO_IRDA_PWDOWN);
		goto error_gpio_request;
	}
	error = pm8058_misc_control(NULL, PM8058_UART_MUX_MASK,
					PM8058_UART_MUX_3);
	if (error) {
		pr_err("%s: cannot set UART MUX 3.\n",	__func__);
		goto error_mux_select;
	}
	error = class_create_file(&irda_class, &class_attr_irda);
	if (error) {
		pr_err("%s: class_create_file failed\n", __func__);
		goto error_class_create_file;
	}

	return 0;

error_class_create_file:
	pm8058_misc_control(NULL, PM8058_UART_MUX_MASK, PM8058_UART_MUX_NO);
error_mux_select:
	gpio_free(MSM_GPIO_IRDA_PWDOWN);
error_gpio_request:
	class_unregister(&irda_class);
error_class_register:
	return error;
}

static void __exit irda_class_exit(void)
{
	class_remove_file(&irda_class, &class_attr_irda);
	pm8058_misc_control(NULL, PM8058_UART_MUX_MASK, PM8058_UART_MUX_NO);
	gpio_free(MSM_GPIO_IRDA_PWDOWN);
	class_unregister(&irda_class);
}

module_init(irda_class_init);
module_exit(irda_class_exit);
