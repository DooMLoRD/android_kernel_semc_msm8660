/* arch/arm/mach-msm/board-semc_fuji_csfb-usb.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/platform_device.h>
#include <mach/mpp.h>
#include <linux/gpio.h>
#include <linux/usb/android_composite.h>
#include <linux/mfd/pmic8058.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/msm-charger.h>
#include <mach/socinfo.h>
#include "board-semc_fuji-usb.h"
#include <mach/semc_charger_usb.h>
#ifdef CONFIG_CHARGER_BQ24185
#include <linux/i2c/bq24185_charger.h>
#endif
#ifdef CONFIG_CHARGER_BQ24160
#include <linux/i2c/bq24160_charger.h>
#endif

#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define PM8058_IRQ_BASE				(NR_MSM_IRQS + NR_GPIO_IRQS)

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
#if defined(CONFIG_CHARGER_BQ24185)

	if (on)
		(void)bq24185_set_opa_mode(CHARGER_BOOST_MODE);
	else
		(void)bq24185_set_opa_mode(CHARGER_CHARGER_MODE);

#elif defined(CONFIG_CHARGER_BQ24160)

	(void)bq24160_set_otg_lock(on);

#else
	static struct regulator *votg_5v_switch;
	static struct regulator *ext_5v_reg;
	static int vbus_is_on;

	/* If VBUS is already on (or off), do nothing. */
	if (on == vbus_is_on)
		return;

	if (!votg_5v_switch) {
		votg_5v_switch = regulator_get(NULL, "8901_usb_otg");
		if (IS_ERR(votg_5v_switch)) {
			pr_err("%s: unable to get votg_5v_switch\n", __func__);
			return;
		}
	}
	if (!ext_5v_reg) {
		ext_5v_reg = regulator_get(NULL, "8901_mpp0");
		if (IS_ERR(ext_5v_reg)) {
			pr_err("%s: unable to get ext_5v_reg\n", __func__);
			return;
		}
	}
	if (on) {
		if (regulator_enable(ext_5v_reg)) {
			pr_err("%s: Unable to enable the regulator:"
					" ext_5v_reg\n", __func__);
			return;
		}
		if (regulator_enable(votg_5v_switch)) {
			pr_err("%s: Unable to enable the regulator:"
					" votg_5v_switch\n", __func__);
			return;
		}
	} else {
		if (regulator_disable(votg_5v_switch))
			pr_err("%s: Unable to enable the regulator:"
				" votg_5v_switch\n", __func__);
		if (regulator_disable(ext_5v_reg))
			pr_err("%s: Unable to enable the regulator:"
				" ext_5v_reg\n", __func__);
	}

	vbus_is_on = on;
#endif
}

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
static struct regulator *ldo6_3p3;
static struct regulator *ldo7_1p8;
static struct regulator *vdd_cx;
#define PMICID_INT		PM8058_GPIO_IRQ(PM8058_IRQ_BASE, 30)
notify_vbus_state notify_vbus_state_func_ptr;

#ifdef CONFIG_USB_EHCI_MSM_72K
#define USB_PMIC_ID_DET_DELAY	msecs_to_jiffies(100)
struct delayed_work pmic_id_det;
static int pmic_id_notif_supported;
static int usb_phy_susp_dig_vol = 750000;

static int __init usb_id_pin_rework_setup(char *support)
{
	if (strncmp(support, "true", 4) == 0)
		pmic_id_notif_supported = 1;

	return 1;
}
__setup("usb_id_pin_rework=", usb_id_pin_rework_setup);

static void pmic_id_detect(struct work_struct *w)
{
	int val = gpio_get_value_cansleep(PM8058_GPIO_PM_TO_SYS(30));
	pr_info("%s(): gpio_read_value = %d\n", __func__, val);

	if (notify_vbus_state_func_ptr)
		(*notify_vbus_state_func_ptr) (val);
}

static irqreturn_t pmic_id_on_irq(int irq, void *data)
{
	/*
	 * Spurious interrupts are observed on pmic gpio line
	 * even though there is no state change on USB ID. Schedule the
	 * work to to allow debounce on gpio
	 */
	schedule_delayed_work(&pmic_id_det, USB_PMIC_ID_DET_DELAY);

	return IRQ_HANDLED;
}

static int msm_hsusb_pmic_id_notif_init(void (*callback)(int online), int init)
{
	unsigned ret = -ENODEV;

	if (!callback)
		return -EINVAL;

	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 2) {
		pr_debug("%s: USB_ID pin is not routed to PMIC"
					"on V1 surf/ffa\n", __func__);
		return -ENOTSUPP;
	}

	usb_phy_susp_dig_vol = 500000;

	if (init) {
		notify_vbus_state_func_ptr = callback;
		ret = pm8901_mpp_config_digital_out(1,
			PM8901_MPP_DIG_LEVEL_L5, 1);
		if (ret) {
			pr_err("%s: MPP2 configuration failed\n", __func__);
			return -ENODEV;
		}
		INIT_DELAYED_WORK(&pmic_id_det, pmic_id_detect);
		ret = request_threaded_irq(PMICID_INT, NULL, pmic_id_on_irq,
			(IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
						"msm_otg_id", NULL);
		if (ret) {
			pm8901_mpp_config_digital_out(1,
					PM8901_MPP_DIG_LEVEL_L5, 0);
			pr_err("%s:pmic_usb_id interrupt registration failed",
					__func__);
			return ret;
		}
		/* Notify the initial Id status */
		pmic_id_detect(&pmic_id_det.work);
	} else {
		free_irq(PMICID_INT, 0);
		cancel_delayed_work_sync(&pmic_id_det);
		notify_vbus_state_func_ptr = NULL;
		ret = pm8901_mpp_config_digital_out(1,
			PM8901_MPP_DIG_LEVEL_L5, 0);
		if (ret) {
			pr_err("%s:MPP2 configuration failed\n", __func__);
			return -ENODEV;
		}
	}
	return 0;
}
#endif

#define USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL	1000000
#define USB_PHY_MAX_VDD_DIG_VOL			1320000
static int msm_hsusb_init_vddcx(int init)
{
	int ret = 0;

	if (init) {
		vdd_cx = regulator_get(NULL, "8058_s1");
		if (IS_ERR(vdd_cx))
			return PTR_ERR(vdd_cx);

		ret = regulator_set_voltage(vdd_cx,
				USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL,
				USB_PHY_MAX_VDD_DIG_VOL);
		if (ret) {
			pr_err("%s: unable to set the voltage for regulator"
				"vdd_cx\n", __func__);
			regulator_put(vdd_cx);
			return ret;
		}

		ret = regulator_enable(vdd_cx);
		if (ret) {
			pr_err("%s: unable to enable regulator"
				"vdd_cx\n", __func__);
			regulator_put(vdd_cx);
		}
	} else {
		ret = regulator_disable(vdd_cx);
		if (ret) {
			pr_err("%s: Unable to disable the regulator:"
				"vdd_cx\n", __func__);
			return ret;
		}

		regulator_put(vdd_cx);
	}

	return ret;
}

static int msm_hsusb_config_vddcx(int high)
{
	int max_vol = USB_PHY_MAX_VDD_DIG_VOL;
	int min_vol;
	int ret;

	if (high)
		min_vol = USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL;
	else
		min_vol = usb_phy_susp_dig_vol;

	ret = regulator_set_voltage(vdd_cx, min_vol, max_vol);
	if (ret) {
		pr_err("%s: unable to set the voltage for regulator"
			"vdd_cx\n", __func__);
		return ret;
	}

	pr_debug("%s: min_vol:%d max_vol:%d\n", __func__, min_vol, max_vol);

	return ret;
}

#define USB_PHY_3P3_VOL_MIN	3075000 /* uV */
#define USB_PHY_3P3_VOL_MAX	3075000 /* uV */
#define USB_PHY_3P3_HPM_LOAD	50000	/* uA */
#define USB_PHY_3P3_LPM_LOAD	4000	/* uA */

#define USB_PHY_1P8_VOL_MIN	1800000 /* uV */
#define USB_PHY_1P8_VOL_MAX	1800000 /* uV */
#define USB_PHY_1P8_HPM_LOAD	50000	/* uA */
#define USB_PHY_1P8_LPM_LOAD	4000	/* uA */
static int msm_hsusb_ldo_init(int init)
{
	int rc = 0;

	if (init) {
		ldo6_3p3 = regulator_get(NULL, "8058_l6");
		if (IS_ERR(ldo6_3p3))
			return PTR_ERR(ldo6_3p3);

		ldo7_1p8 = regulator_get(NULL, "8058_l7");
		if (IS_ERR(ldo7_1p8)) {
			rc = PTR_ERR(ldo7_1p8);
			goto put_3p3;
		}

		rc = regulator_set_voltage(ldo6_3p3, USB_PHY_3P3_VOL_MIN,
				USB_PHY_3P3_VOL_MAX);
		if (rc) {
			pr_err("%s: Unable to set voltage level for"
				"ldo6_3p3 regulator\n", __func__);
			goto put_1p8;
		}
		rc = regulator_enable(ldo6_3p3);
		if (rc) {
			pr_err("%s: Unable to enable the regulator:"
				"ldo6_3p3\n", __func__);
			goto put_1p8;
		}
		rc = regulator_set_voltage(ldo7_1p8, USB_PHY_1P8_VOL_MIN,
				USB_PHY_1P8_VOL_MAX);
		if (rc) {
			pr_err("%s: Unable to set voltage level for"
				"ldo7_1p8 regulator\n", __func__);
			goto disable_3p3;
		}
		rc = regulator_enable(ldo7_1p8);
		if (rc) {
			pr_err("%s: Unable to enable the regulator:"
				"ldo7_1p8\n", __func__);
			goto disable_3p3;
		}

		return 0;
	}

	regulator_disable(ldo7_1p8);
disable_3p3:
	regulator_disable(ldo6_3p3);
put_1p8:
	regulator_put(ldo7_1p8);
put_3p3:
	regulator_put(ldo6_3p3);
	return rc;
}

static int msm_hsusb_ldo_enable(int on)
{
	int ret = 0;

	if (!ldo7_1p8 || IS_ERR(ldo7_1p8)) {
		pr_err("%s: ldo7_1p8 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (!ldo6_3p3 || IS_ERR(ldo6_3p3)) {
		pr_err("%s: ldo6_3p3 is not initialized\n", __func__);
		return -ENODEV;
	}

	if (on) {
		ret = regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_HPM_LOAD);
		if (ret < 0) {
			pr_err("%s: Unable to set HPM of the regulator:"
				"ldo7_1p8\n", __func__);
			return ret;
		}
		ret = regulator_set_optimum_mode(ldo6_3p3,
				USB_PHY_3P3_HPM_LOAD);
		if (ret < 0) {
			pr_err("%s: Unable to set HPM of the regulator:"
				"ldo6_3p3\n", __func__);
			regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_LPM_LOAD);
			return ret;
		}
	} else {
		ret = regulator_set_optimum_mode(ldo7_1p8,
				USB_PHY_1P8_LPM_LOAD);
		if (ret < 0)
			pr_err("%s: Unable to set LPM of the regulator:"
				"ldo7_1p8\n", __func__);
		ret = regulator_set_optimum_mode(ldo6_3p3,
				USB_PHY_3P3_LPM_LOAD);
		if (ret < 0)
			pr_err("%s: Unable to set LPM of the regulator:"
				"ldo6_3p3\n", __func__);
	}

	pr_debug("reg (%s)\n", on ? "HPM" : "LPM");
	return ret < 0 ? ret : 0;
 }
#endif

struct msm_usb_host_platform_data semc_fuji_usb_host_pdata = {
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
	.power_budget	= 390,
};
#endif

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
struct msm_otg_platform_data semc_fuji_otg_pdata = {
	/* if usb link is in sps there is no need for
	 * usb pclk as dayatona fabric clock will be
	 * used instead
	 */
	.pclk_src_name		 = "dfab_usb_hs_clk",
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.se1_gating		 = SE1_GATING_DISABLE,
#ifdef CONFIG_USB_EHCI_MSM_72K
	.pmic_id_notif_init = msm_hsusb_pmic_id_notif_init,
#endif
#ifdef CONFIG_USB_EHCI_MSM_72K
	.vbus_power = msm_hsusb_vbus_power,
#endif
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.config_vddcx            = msm_hsusb_config_vddcx,
	.init_vddcx              = msm_hsusb_init_vddcx,
#ifdef CONFIG_SEMC_CHARGER_USB_ARCH
	.chg_vbus_draw		 = semc_charger_usb_vbus_draw,
	.chg_connected		 = semc_charger_usb_connected,
	.chg_init		 = semc_charger_usb_init,
#endif
};
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
struct msm_hsusb_gadget_platform_data semc_fuji_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};
#endif


/* dynamic composition */
static char *usb_functions_msc[] = {
	"usb_mass_storage",
};

static char *usb_functions_msc_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_msc_adb_eng[] = {
	"usb_mass_storage",
	"adb",
	"modem",
	"nmea",
	"diag",
};

#if defined(CONFIG_USB_ANDROID_MTP_ARICENT)
static char *usb_functions_mtp[] = {
	"mtp",
};

static char *usb_functions_mtp_adb[] = {
	"mtp",
	"adb",
};

static char *usb_functions_mtp_msc[] = {
	"mtp",
	"usb_mass_storage",
};

static char *usb_functions_mtp_adb_eng[] = {
	"mtp",
	"adb",
	"modem",
	"nmea",
	"diag",
};
#endif

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
	"diag_mdm",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET_SDIO
	"rmnet_sdio",
#endif
	"usb_mass_storage",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};

static char *csfb_usb_functions_default_adb[] = {
	"diag",
	"diag_mdm",
	"adb",
	"modem",
	"nmea",
	"rmnet_sdio",
	"usb_mass_storage",
};


static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0x9031,
		.num_functions	= ARRAY_SIZE(csfb_usb_functions_default_adb),
		.functions	= csfb_usb_functions_default_adb,
	},
	{
		.product_id	= 0xE000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_msc),
		.functions	= usb_functions_msc,
	},
	{
		.product_id	= 0x6000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_msc_adb),
		.functions	= usb_functions_msc_adb,
	},
#if defined(CONFIG_USB_ANDROID_MTP_ARICENT)
	{
		.product_id	= 0x0000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp),
		.functions	= usb_functions_mtp,
	},
	{
		.product_id	= 0x5000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb),
		.functions	= usb_functions_mtp_adb,
	},
	{
		.product_id	= 0x4000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc),
		.functions	= usb_functions_mtp_msc,
	},
#endif
	{
		.product_id	= 0x7000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= 0x8000 |  CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
#if defined(CONFIG_USB_ANDROID_MTP_ARICENT)
	{
		.product_id	= 0x5146,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb_eng),
		.functions	= usb_functions_mtp_adb_eng,
	},
#endif
	{
		.product_id	= 0x6146,
		.num_functions	= ARRAY_SIZE(usb_functions_msc_adb_eng),
		.functions	= usb_functions_msc_adb_eng,
	},
};
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "SEMC",
	.product        = "Mass storage",
	.release = 0x0100,

	.cdrom_nluns = 1,
	.cdrom_vendor = "SEMC",
	.cdrom_product = "CD-ROM",
	.cdrom_release = 0x0100,
};

struct platform_device semc_fuji_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= 0x0FCE,
	.vendorDescr	= "SEMC",
};

struct platform_device semc_fuji_rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.product_id	= 0x9031,
	.version	= 0x0100,
	.product_name		= "Qualcomm HSUSB Device",
	.manufacturer_name	= "Qualcomm Incorporated",
	.num_products = ARRAY_SIZE(usb_products),

	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	/* Once we get S1boot, it will be set by board_serialno_setup */
	.serial_number = "1234567890ABCDEF",
};

struct platform_device semc_fuji_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
	int ix, len, i;
	static char usb_serial_number[21];
	char *src = serialno;

	/* create a MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	/* The USB mass storage spec states in section 4.1.2 that
	 * the serial number may only contain characters '0' to '9'
	 * and 'A' to 'F'. With this change the serial is instead
	 * generated from the hex value of the individual chars
	 * in the phone's serial number.
	 */
	len = strlen(serialno);
	ix = 0;
	while (ix < 20) {
		if (*serialno && ix >= 20 - (len << 1)) {
			sprintf(&usb_serial_number[ix], "%02X",
					(unsigned char)*serialno);
			serialno++;
		} else {
			sprintf(&usb_serial_number[ix], "%02X", 0);
		}
		ix += 2;
	}
	usb_serial_number[20] = '\0';
	android_usb_pdata.serial_number = usb_serial_number;

	printk(KERN_INFO "USB serial number: %s\n",
			android_usb_pdata.serial_number);
	return 1;
}
__setup("serialno=", board_serialno_setup);
