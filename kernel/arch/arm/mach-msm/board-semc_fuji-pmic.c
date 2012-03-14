/* arch/arm/mach-msm/board-semc_fuji-pmic.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/mfd/pmic8058.h>
#include <linux/input/pmic8058-keypad.h>
#include <linux/pmic8058-pwrkey.h>
#include <linux/rtc/rtc-pm8058.h>
#include <linux/pmic8058-vibrator.h>
#include <mach/mpp.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/pmic8058-xoadc.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/leds-pmic8058.h>
#include <linux/msm_adc.h>
#include <linux/m_adcproc.h>
#include <mach/board.h>
#include <mach/simple_remote_msm8x60_pf.h>
#include <mach/rpm-regulator.h>
#include <linux/i2c.h>
#include "devices.h"
#include "devices-msm8x60.h"
#include "gpiomux-semc_fuji.h"
#include "keypad-semc_fuji.h"
#include "keypad-pmic-fuji.h"
#include "gpiomux-fuji_pm8058.h"

/*
 * The UI_INTx_N lines are pmic gpio lines which connect i2c
 * gpio expanders to the pm8058.
 */
#define UI_INT1_N 25
#define UI_INT2_N 34
#define UI_INT3_N 14

#ifdef CONFIG_SENSORS_MSM_ADC
static struct resource resources_adc[] = {
	{
		.start = PM8058_ADC_IRQ(PM8058_IRQ_BASE),
		.end   = PM8058_ADC_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
};

static struct adc_access_fn xoadc_fn = {
	pm8058_xoadc_select_chan_and_start_conv,
	pm8058_xoadc_read_adc_code,
	pm8058_xoadc_get_properties,
	pm8058_xoadc_slot_request,
	pm8058_xoadc_restore_slot,
	pm8058_xoadc_calibrate,
};

static struct msm_adc_channels msm_adc_channels_data[] = {
	{"vbatt", CHANNEL_ADC_VBATT, 0, &xoadc_fn, CHAN_PATH_TYPE2,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"vcoin", CHANNEL_ADC_VCOIN, 0, &xoadc_fn, CHAN_PATH_TYPE1,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
	{"vcharger_channel", CHANNEL_ADC_VCHG, 0, &xoadc_fn, CHAN_PATH_TYPE3,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE4, scale_default},
	{"charger_current_monitor", CHANNEL_ADC_CHG_MONITOR, 0, &xoadc_fn,
		CHAN_PATH_TYPE4,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"vph_pwr", CHANNEL_ADC_VPH_PWR, 0, &xoadc_fn, CHAN_PATH_TYPE5,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"usb_vbus", CHANNEL_ADC_USB_VBUS, 0, &xoadc_fn, CHAN_PATH_TYPE11,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE3, scale_default},
	{"pmic_therm", CHANNEL_ADC_DIE_TEMP, 0, &xoadc_fn, CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_pmic_therm},
	{"pmic_therm_4K", CHANNEL_ADC_DIE_TEMP_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE12,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE7, scale_pmic_therm},
	{"xo_therm", CHANNEL_ADC_XOTHERM, 0, &xoadc_fn, CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE5, tdkntcgtherm},
	{"xo_therm_4K", CHANNEL_ADC_XOTHERM_4K, 0, &xoadc_fn,
		CHAN_PATH_TYPE_NONE,
		ADC_CONFIG_TYPE1, ADC_CALIB_CONFIG_TYPE6, tdkntcgtherm},
	{"hdset_detect", CHANNEL_ADC_HDSET, 0, &xoadc_fn, CHAN_PATH_TYPE6,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1, scale_default},
	{"chg_batt_amon", CHANNEL_ADC_BATT_AMON, 0, &xoadc_fn, CHAN_PATH_TYPE10,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE1,
		scale_xtern_chgr_cur},
	{"msm_therm", CHANNEL_ADC_MSM_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE8,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_msm_therm},
	{"batt_therm", CHANNEL_ADC_BATT_THERM, 0, &xoadc_fn, CHAN_PATH_TYPE7,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_batt_therm},
	{"batt_id", CHANNEL_ADC_BATT_ID, 0, &xoadc_fn, CHAN_PATH_TYPE9,
		ADC_CONFIG_TYPE2, ADC_CALIB_CONFIG_TYPE2, scale_default},
};

static struct msm_adc_platform_data msm_adc_pdata = {
	.channel = msm_adc_channels_data,
	.num_chan_supported = ARRAY_SIZE(msm_adc_channels_data),
#ifdef CONFIG_SENSORS_MSM_ADC
	.target_hw = MSM_8x60,
#endif
};

struct platform_device semc_fuji_adc_device = {
	.name   = "msm_adc",
	.id = -1,
	.dev = {
		.platform_data = &msm_adc_pdata,
	},
};

static void pmic8058_xoadc_mpp_config(void)
{
	int rc;

	rc = pm8901_mpp_config_digital_out(XOADC_MPP_4,
			PM8901_MPP_DIG_LEVEL_S4, PM_MPP_DOUT_CTL_LOW);
	if (rc)
		pr_err("%s: Config mpp4 on pmic 8901 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_3,
			PM_MPP_AIN_AMUX_CH5, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp3 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_5,
			PM_MPP_AIN_AMUX_CH9, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp5 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_7,
			PM_MPP_AIN_AMUX_CH6, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp7 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_8,
			PM_MPP_AIN_AMUX_CH8, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp8 on pmic 8058 failed\n", __func__);

	rc = pm8058_mpp_config_analog_input(XOADC_MPP_10,
			PM_MPP_AIN_AMUX_CH7, PM_MPP_AOUT_CTL_DISABLE);
	if (rc)
		pr_err("%s: Config mpp10 on pmic 8058 failed\n", __func__);

	rc = pm8901_mpp_config_digital_in(XOADC_MPP_2,
			PM8901_MPP_DIG_LEVEL_MSMIO, PM_MPP_DIN_TO_INT);
	if (rc)
		pr_err("%s: Config mpp2 on pmic 8901 failed\n", __func__);

	rc = pm8901_mpp_config_digital_in(XOADC_MPP_3,
			PM8901_MPP_DIG_LEVEL_MSMIO, PM_MPP_DIN_TO_INT);
	if (rc)
		pr_err("%s: Config mpp3 on pmic 8901 failed\n", __func__);

}

static int pmic8058_xoadc_rpm_vreg_config(int on)
{
	int rc;

	rc = rpm_vreg_set_voltage(RPM_VREG_ID_PM8058_L18,
				RPM_VREG_VOTER3, (on ? 2200000 : 0), 0);

	return rc;
}

/* usec. For this ADC,
 * this time represents clk rate @ txco w/ 1024 decimation ratio.
 * Each channel has different configuration, thus at the time of starting
 * the conversion, xoadc will return actual conversion time
 * */
static struct adc_properties pm8058_xoadc_data = {
	.adc_reference          = 2200, /* milli-voltage for this adc */
	.bitresolution         = 15,
	.bipolar                = 0,
	.conversiontime         = 54,
};

static struct xoadc_platform_data xoadc_pdata = {
	.xoadc_prop = &pm8058_xoadc_data,
	.xoadc_mpp_config = pmic8058_xoadc_mpp_config,
	.xoadc_vreg_set = pmic8058_xoadc_rpm_vreg_config,
	.xoadc_num = XOADC_PMIC_0,
};
#endif

#define PM8058_GPIO_PIN_DISABLE 0x01

static int pm8058_gpios_init(void)
{
	int i;
	int rc;
	struct pm8058_gpio_cfg {
		int                gpio;
		struct pm8058_gpio cfg;
	};

	struct pm8058_gpio disable_cfg = {
		.disable_pin = PM8058_GPIO_PIN_DISABLE,
	};

	struct pm8058_gpio_cfg gpio_cfgs[] = {
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
		{
			PMIC_GPIO_SDC3_DET - 1,
			{
				.direction      = PM_GPIO_DIR_IN,
				.pull           = PM_GPIO_PULL_UP_30,
				.vin_sel        = 2,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			},
		},
#endif
		{ /* Timpani Reset */
			20,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_value	= 1,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.pull		= PM_GPIO_PULL_DN,
				.out_strength	= PM_GPIO_STRENGTH_HIGH,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
		{ /* PMIC ID interrupt */
			30,
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_NO,
				.function	= PM_GPIO_FUNC_NORMAL,
				.vin_sel	= 2,
				.inv_int_pol	= 0,
			}
		},
		{ /* BCM4330 SLEEP CLOCK - GPIO38*/
			37,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.output_buffer	= PM_GPIO_OUT_BUF_CMOS,
				.output_value	= 1,
				.pull		= PM_GPIO_PULL_NO,
				.out_strength	= PM_GPIO_STRENGTH_LOW,
				.function	= PM_GPIO_FUNC_2,
				.vin_sel	= PM_GPIO_VIN_S3,
				.inv_int_pol	= 0,
			}
		},
#ifdef CONFIG_PMIC8058_MDM_ANTENA_SWITCH
		{/*PM8058 GPIO 33*/
			32,
			{
				.direction = PM_GPIO_DIR_IN,
				.pull = PM_GPIO_PULL_NO,
				.vin_sel = PM_GPIO_VIN_L7,
				.function = PM_GPIO_FUNC_NORMAL,
			}
		},
		{/*PM8058 GPIO 34*/
			33,
			{
				.direction = PM_GPIO_DIR_IN,
				.pull = PM_GPIO_PULL_NO,
				.vin_sel = PM_GPIO_VIN_L7,
				.function = PM_GPIO_FUNC_NORMAL,
			}
		},
		{/*PM8058 GPIO 35*/
			34,
			{
				.direction = PM_GPIO_DIR_IN,
				.pull = PM_GPIO_PULL_NO,
				.vin_sel = PM_GPIO_VIN_L7,
				.function = PM_GPIO_FUNC_NORMAL,
			}
		},
		{/*PM8058 GPIO 36*/
			35,
			{
				.direction = PM_GPIO_DIR_IN,
				.pull = PM_GPIO_PULL_NO,
				.vin_sel = PM_GPIO_VIN_L7,
				.function = PM_GPIO_FUNC_NORMAL,
			}
		},
		{/*PM8058 GPIO 37*/
			36,
			{
				.direction = PM_GPIO_DIR_OUT,
				.pull = PM_GPIO_PULL_NO,
				.vin_sel = PM_GPIO_VIN_L6,
				.function = PM_GPIO_FUNC_1,
				.output_buffer = PM_GPIO_OUT_BUF_CMOS,
				.out_strength = PM_GPIO_STRENGTH_NO,
			}
		},
#endif
#if defined(CONFIG_NFC_PN544) && !defined(CONFIG_SEMC_FELICA_SUPPORT)
		{ /* pn544 NFC_EN - GPIO17 */
			16,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.pull		= PM_GPIO_PULL_NO,
#ifdef CONFIG_MACH_SEMC_FUJI_NOZOMI
				.output_buffer  = PM_GPIO_OUT_BUF_OPEN_DRAIN,
				.vin_sel	= PM_GPIO_VIN_VPH,
#else
				.vin_sel	= PM_GPIO_VIN_S3,
#endif
				.out_strength	= PM_GPIO_STRENGTH_MED,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
		{ /* pn544 NFC_FWDL_EN - GPIO27 */
			26,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.pull		= PM_GPIO_PULL_NO,
				.vin_sel	= PM_GPIO_VIN_S3,
				.out_strength	= PM_GPIO_STRENGTH_NO,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
		{ /* pn544 NFC_IRQ - GPIO28 */
			27,
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_DN,
				.vin_sel	= PM_GPIO_VIN_S3,
				.out_strength	= PM_GPIO_STRENGTH_NO,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
#endif
#if !defined(CONFIG_NFC_PN544) && defined(CONFIG_SEMC_FELICA_SUPPORT)
		{ /* FELICA_PON - GPIO17 */
			16,
			{
				.direction	= PM_GPIO_DIR_OUT,
				.pull		= PM_GPIO_PULL_NO,
				.vin_sel	= PM_GPIO_VIN_S3,
				.out_strength	= PM_GPIO_STRENGTH_MED,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
		{ /* FELICA_RFS - GPIO27 */
			26,
			{
				.direction	= PM_GPIO_DIR_IN,
				.pull		= PM_GPIO_PULL_NO,
				.vin_sel	= PM_GPIO_VIN_S3,
				.out_strength	= PM_GPIO_STRENGTH_NO,
				.function	= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
		{ /* FELICA_INT - GPIO28 */
			27,
			{
				.direction		= PM_GPIO_DIR_IN,
				.pull			= PM_GPIO_PULL_NO,
				.vin_sel		= PM_GPIO_VIN_S3,
				.out_strength	= PM_GPIO_STRENGTH_NO,
				.function		= PM_GPIO_FUNC_NORMAL,
				.inv_int_pol	= 0,
			}
		},
#endif
		{ /* VOLT_DET_CTRL - GPIO_40 */
			39,
			{
				.direction      = PM_GPIO_DIR_OUT,
				.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
				.output_value   = 1,
				.pull           = PM_GPIO_PULL_NO,
				.vin_sel        = PM_GPIO_VIN_VPH,
				.out_strength   = PM_GPIO_STRENGTH_MED,
				.function       = PM_GPIO_FUNC_NORMAL,
				.inv_int_pol    = 0,
			}
		},
	};

	for (i = 0; i < pmic8058_unused_gpios.unused_gpio_num; ++i) {
		rc = pm8058_gpio_config(
			pmic8058_unused_gpios.unused_gpio[i] - 1,
			&disable_cfg);
		if (rc < 0)
			pr_err("%s Failed to disable pmic gpio\n", __func__);
	}

	for (i = 0; i < ARRAY_SIZE(gpio_cfgs); ++i) {
		rc = pm8058_gpio_config(gpio_cfgs[i].gpio,
				&gpio_cfgs[i].cfg);
		if (rc < 0) {
			pr_err("%s pmic gpio config failed\n",
				__func__);
			return rc;
		}
	}

	return 0;
}


static struct resource resources_keypad[] = {
	{
		.start	= PM8058_KEYPAD_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_KEYPAD_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_KEYSTUCK_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_KEYSTUCK_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};


static struct resource resources_pwrkey[] = {
	{
		.start	= PM8058_PWRKEY_REL_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_PWRKEY_REL_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= PM8058_PWRKEY_PRESS_IRQ(PM8058_IRQ_BASE),
		.end	= PM8058_PWRKEY_PRESS_IRQ(PM8058_IRQ_BASE),
		.flags	= IORESOURCE_IRQ,
	},
};

static struct pmic8058_pwrkey_pdata pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us   = 970,
	.wakeup			= 1,
};

static struct pmic8058_vibrator_pdata pmic_vib_pdata = {
	.initial_vibrate_ms  = 0,
	.level_mV = CONFIG_PMIC8058_VIBRATOR_ON_VOLTAGE,
	.max_timeout_ms = 15000,
};

static int pm8058_pwm_config(struct pwm_device *pwm, int ch, int on)
{
	struct pm8058_gpio pwm_gpio_config = {
		.direction      = PM_GPIO_DIR_OUT,
		.output_buffer  = PM_GPIO_OUT_BUF_CMOS,
		.output_value   = 0,
		.pull           = PM_GPIO_PULL_NO,
		.vin_sel        = PM_GPIO_VIN_VPH,
		.out_strength   = PM_GPIO_STRENGTH_HIGH,
		.function       = PM_GPIO_FUNC_2,
	};

	int rc = -EINVAL;
	int id, mode, max_mA;

	id = mode = max_mA = 0;
	switch (ch) {
	case 0:
	case 1:
	case 2:
		if (on) {
			id = 24 + ch;
			rc = pm8058_gpio_config(id - 1, &pwm_gpio_config);
			if (rc)
				pr_err("%s: pm8058_gpio_config(%d): rc=%d\n",
					__func__, id, rc);
		}
		break;

	case 6:
		id = PM_PWM_LED_FLASH;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	case 7:
		id = PM_PWM_LED_FLASH1;
		mode = PM_PWM_CONF_PWM1;
		max_mA = 300;
		break;

	default:
		break;
	}

	if (ch >= 6 && ch <= 7) {
		if (!on) {
			mode = PM_PWM_CONF_NONE;
			max_mA = 0;
		}
		rc = pm8058_pwm_config_led(pwm, id, mode, max_mA);
		if (rc)
			pr_err("%s: pm8058_pwm_config_led(ch=%d): rc=%d\n",
			       __func__, ch, rc);
	}
	return rc;

}

static struct pm8058_pwm_pdata pm8058_pwm_data = {
	.config		= pm8058_pwm_config,
};

static struct pm8058_gpio_platform_data pm8058_gpio_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 0),
	.init		= pm8058_gpios_init,
};

static struct pm8058_gpio_platform_data pm8058_mpp_data = {
	.gpio_base	= PM8058_GPIO_PM_TO_SYS(PM8058_GPIOS),
	.irq_base	= PM8058_MPP_IRQ(PM8058_IRQ_BASE, 0),
};

static struct resource resources_rtc[] = {
       {
		.start  = PM8058_RTC_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_RTC_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
       {
		.start  = PM8058_RTC_ALARM_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_RTC_ALARM_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

static struct pm8058_rtc_platform_data pm8058_rtc_pdata = {
    .rtc_alarm_powerup      = false,
};

#ifdef CONFIG_PMIC8058_LEDS
static struct pmic8058_led pmic8058_leds = {
	.name		= "tally-light",
	.max_brightness = 20,
	.id		= PMIC8058_ID_LED_0,
};

static struct pmic8058_leds_platform_data pm8058_leds_data = {
	.num_leds = 1,
	.leds	= &pmic8058_leds,
};
#endif


static struct resource resources_temp_alarm[] = {
       {
		.start  = PM8058_TEMP_ALARM_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_TEMP_ALARM_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

static struct resource resources_pm8058_misc[] = {
       {
		.start  = PM8058_OSCHALT_IRQ(PM8058_IRQ_BASE),
		.end    = PM8058_OSCHALT_IRQ(PM8058_IRQ_BASE),
		.flags  = IORESOURCE_IRQ,
       },
};

static struct resource resources_simple_remote[] = {
	{
		.start = PM8058_SW_1_IRQ(PM8058_IRQ_BASE),
		.end   = PM8058_SW_1_IRQ(PM8058_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
};

#define PM8058_SUBDEV_LED 1

static struct mfd_cell pm8058_subdevs[] = {
	{
		.name = "pm8058-keypad",
		.id		= -1,
		.num_resources	= ARRAY_SIZE(resources_keypad),
		.resources	= resources_keypad,
		.platform_data	= &fuji_keypad_data,
		.data_size = sizeof(fuji_keypad_data),
	},
#ifdef CONFIG_PMIC8058_LEDS
	{	.name = "pm8058-led",
		.id		= -1,
		.platform_data	= &pm8058_leds_data,
		.data_size	= sizeof(pm8058_leds_data),
	},
#endif
	{	.name = "pm8058-gpio",
		.id		= -1,
		.platform_data	= &pm8058_gpio_data,
		.data_size	= sizeof(pm8058_gpio_data),
	},
	{	.name = "pm8058-mpp",
		.id		= -1,
		.platform_data	= &pm8058_mpp_data,
		.data_size	= sizeof(pm8058_mpp_data),
	},
#if defined(CONFIG_SEMC_FELICA_SUPPORT) || defined(CONFIG_NFC_PN544)
	{	.name = "pm8058-nfc",
		.id		= -1,
	},
#endif
	{	.name = "pm8058-pwrkey",
		.id	= -1,
		.resources = resources_pwrkey,
		.num_resources = ARRAY_SIZE(resources_pwrkey),
		.platform_data = &pwrkey_pdata,
		.data_size = sizeof(pwrkey_pdata),
	},
	{
		.name = "pm8058-vib",
		.id = -1,
		.platform_data = &pmic_vib_pdata,
		.data_size     = sizeof(pmic_vib_pdata),
	},
	{
		.name = "pm8058-pwm",
		.id = -1,
		.platform_data = &pm8058_pwm_data,
		.data_size = sizeof(pm8058_pwm_data),
	},
#ifdef CONFIG_SENSORS_MSM_ADC
	{
		.name = "pm8058-xoadc",
		.id = -1,
		.num_resources = ARRAY_SIZE(resources_adc),
		.resources = resources_adc,
		.platform_data = &xoadc_pdata,
		.data_size = sizeof(xoadc_pdata),
	},
#endif
	{
		.name = "pm8058-rtc",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_rtc),
		.resources      = resources_rtc,
		.platform_data = &pm8058_rtc_pdata,
		.data_size = sizeof(pm8058_rtc_pdata),
	},
	{
		.name = "pm8058-tm",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_temp_alarm),
		.resources      = resources_temp_alarm,
	},
	{	.name = "pm8058-upl",
		.id		= -1,
	},
	{
		.name = "pm8058-misc",
		.id = -1,
		.num_resources  = ARRAY_SIZE(resources_pm8058_misc),
		.resources      = resources_pm8058_misc,
	},
	{
		.name = SIMPLE_REMOTE_PF_NAME,
		.id = -1,
		.num_resources = ARRAY_SIZE(resources_simple_remote),
		.resources = resources_simple_remote,
		.platform_data	= &simple_remote_pf_data,
		.data_size	= sizeof(simple_remote_pf_data),
	},
#ifdef CONFIG_FUJI_PMIC_KEYPAD
	{
		.name = KP_NAME,
		.id = -1,
		.platform_data	= &keypad_pmic_platform_data,
		.data_size	= sizeof(keypad_pmic_platform_data),
	},
#endif /* CONFIG_FUJI_PMIC_KEYPAD */
};

static struct pm8058_platform_data pm8058_platform_data = {
	.irq_base = PM8058_IRQ_BASE,

	.num_subdevs = ARRAY_SIZE(pm8058_subdevs),
	.sub_devices = pm8058_subdevs,
	.irq_trigger_flags = IRQF_TRIGGER_LOW,
};

static struct i2c_board_info pm8058_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8058-core", 0x55),
		.irq = MSM_GPIO_TO_INT(PM8058_GPIO_INT),
		.platform_data = &pm8058_platform_data,
	},
};

static struct pm8901_gpio_platform_data pm8901_mpp_data = {
	.gpio_base	= PM8901_GPIO_PM_TO_SYS(0),
	.irq_base	= PM8901_MPP_IRQ(PM8901_IRQ_BASE, 0),
};

static struct resource pm8901_temp_alarm[] = {
	{
		.start = PM8901_TEMP_ALARM_IRQ(PM8901_IRQ_BASE),
		.end = PM8901_TEMP_ALARM_IRQ(PM8901_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
	{
		.start = PM8901_TEMP_HI_ALARM_IRQ(PM8901_IRQ_BASE),
		.end = PM8901_TEMP_HI_ALARM_IRQ(PM8901_IRQ_BASE),
		.flags = IORESOURCE_IRQ,
	},
};

static struct regulator_consumer_supply pm8901_vreg_supply[PM8901_VREG_MAX] = {
	[PM8901_VREG_ID_MPP0] =     REGULATOR_SUPPLY("8901_mpp0",     NULL),
	[PM8901_VREG_ID_HDMI_MVS] = REGULATOR_SUPPLY("8901_hdmi_mvs", NULL),
};

#define PM8901_VREG_INIT(_id, _min_uV, _max_uV, _modes, _ops, _apply_uV, \
			 _always_on, _active_high, _level) \
	[_id] = { \
		.init_data = { \
			.constraints = { \
				.valid_modes_mask = _modes, \
				.valid_ops_mask = _ops, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
				.input_uV = _min_uV, \
				.apply_uV = _apply_uV, \
				.always_on = _always_on, \
			}, \
			.num_consumer_supplies = 1, \
			.consumer_supplies = &pm8901_vreg_supply[_id], \
		}, \
		.active_high = _active_high, \
		.level = _level, \
	}

#define PM8901_VREG_INIT_MPP(_id, _active_high, _level) \
	PM8901_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL, \
			REGULATOR_CHANGE_STATUS, 0, 0, _active_high, _level)

#define PM8901_VREG_INIT_VS(_id) \
	PM8901_VREG_INIT(_id, 0, 0, REGULATOR_MODE_NORMAL, \
			REGULATOR_CHANGE_STATUS, 0, 0, 0, 0)

static struct pm8901_vreg_pdata pm8901_vreg_init_pdata[PM8901_VREG_MAX] = {
	PM8901_VREG_INIT_MPP(PM8901_VREG_ID_MPP0, 1,
				PM8901_MPP_DIG_LEVEL_MSMIO),

	PM8901_VREG_INIT_VS(PM8901_VREG_ID_HDMI_MVS),
};

#define PM8901_VREG(_id) { \
	.name = "pm8901-regulator", \
	.id = _id, \
	.platform_data = &pm8901_vreg_init_pdata[_id], \
	.data_size = sizeof(pm8901_vreg_init_pdata[_id]), \
}

static struct mfd_cell pm8901_subdevs[] = {
	{	.name = "pm8901-mpp",
		.id		= -1,
		.platform_data	= &pm8901_mpp_data,
		.data_size	= sizeof(pm8901_mpp_data),
	},
	{	.name = "pm8901-tm",
		.id		= -1,
		.num_resources  = ARRAY_SIZE(pm8901_temp_alarm),
		.resources      = pm8901_temp_alarm,
	},
	PM8901_VREG(PM8901_VREG_ID_MPP0),
	PM8901_VREG(PM8901_VREG_ID_HDMI_MVS),
};

static struct pm8901_platform_data pm8901_platform_data = {
	.irq_base = PM8901_IRQ_BASE,
	.num_subdevs = ARRAY_SIZE(pm8901_subdevs),
	.sub_devices = pm8901_subdevs,
	.irq_trigger_flags = IRQF_TRIGGER_LOW,
};

static struct i2c_board_info pm8901_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("pm8901-core", 0x55),
		.irq = MSM_GPIO_TO_INT(PM8901_GPIO_INT),
		.platform_data = &pm8901_platform_data,
	},
};

void __init semc_fuji_pmic_register(void) {
	i2c_register_board_info(MSM_SSBI1_I2C_BUS_ID,
				pm8058_boardinfo,
				ARRAY_SIZE(pm8058_boardinfo));
	i2c_register_board_info(MSM_SSBI2_I2C_BUS_ID,
				pm8901_boardinfo,
				ARRAY_SIZE(pm8901_boardinfo));
}
