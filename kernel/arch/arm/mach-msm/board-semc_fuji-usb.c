/* arch/arm/mach-msm/board-semc_fuji-usb.c
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
#ifdef CONFIG_USB_ANDROID_ACCESSORY
#include <linux/usb/f_accessory.h>
#endif
#include <linux/mfd/pmic8058.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/msm-charger.h>
#include <mach/socinfo.h>
#include "board-semc_fuji-usb.h"
#include <mach/semc_charger_usb.h>
#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
#include <mach/semc_charger_cradle.h>
#endif
#include <mach/msm72k_otg.h>
#ifdef CONFIG_CHARGER_BQ24185
#include <linux/i2c/bq24185_charger.h>
#endif
#ifdef CONFIG_CHARGER_BQ24160
#include <linux/i2c/bq24160_charger.h>
#endif
#ifdef CONFIG_USB_NCP373
#include <linux/usb/ncp373.h>
#endif
#include "gpiomux-semc_fuji.h"

#define PM8058_GPIO_BASE			NR_MSM_GPIOS
#define PM8058_GPIO_PM_TO_SYS(pm_gpio)		(pm_gpio + PM8058_GPIO_BASE)
#define PM8058_IRQ_BASE				(NR_MSM_IRQS + NR_GPIO_IRQS)

struct semc_hsusb_platform_data {
	struct wake_lock wlock;
};
static struct semc_hsusb_platform_data *the_semc_hsusb_platform_data;

static int semc_hsusb_platform_init(int init)
{
	struct semc_hsusb_platform_data *dev;

	if (init) {
		dev = kzalloc(sizeof(struct semc_hsusb_platform_data),
						GFP_KERNEL);
		if (!dev)
			return -ENOMEM;
		wake_lock_init(&dev->wlock, WAKE_LOCK_SUSPEND,
						"semc_hsusb_platform_data");
		the_semc_hsusb_platform_data = dev;
	} else {
		dev = the_semc_hsusb_platform_data;
		the_semc_hsusb_platform_data = NULL;
		wake_lock_destroy(&dev->wlock);
		kfree(dev);
	}

	return 0;
}

#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	struct semc_hsusb_platform_data *dev = the_semc_hsusb_platform_data;
#if defined(CONFIG_CHARGER_BQ24185)
	if (on) {
		if (dev && !wake_lock_active(&dev->wlock))
			wake_lock(&dev->wlock);
		(void)bq24185_set_opa_mode(CHARGER_BOOST_MODE);
	} else {
		(void)bq24185_set_opa_mode(CHARGER_CHARGER_MODE);
		if (dev && wake_lock_active(&dev->wlock))
			wake_unlock(&dev->wlock);
	}
#elif defined(CONFIG_CHARGER_BQ24160) && defined(CONFIG_USB_NCP373)
	if (on) {
		int ret;
		if (dev && !wake_lock_active(&dev->wlock))
			wake_lock(&dev->wlock);
		ret = bq24160_set_otg_lock(1);
		if (unlikely(ret < 0)) {
			pr_err("%s: failed to lock the bq24160 ret=%d\n",
								__func__, ret);
			goto do_vbus_off;
		}

		ret = ncp373_vbus_switch(1);
		if (unlikely(ret < 0)) {
			pr_err("%s: failed to switch the vbus load ret=%d\n",
								__func__, ret);
			goto do_vbus_off;
		}
		return;
	}
do_vbus_off:
	ncp373_vbus_switch(0);
	bq24160_set_otg_lock(0);
	if (!on && dev && wake_lock_active(&dev->wlock))
		wake_unlock(&dev->wlock);
#endif
}

#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_MSM_72K)
struct msm_otg_platform_data semc_fuji_otg_pdata;
static struct regulator *ldo6_3p3;
static struct regulator *ldo7_1p8;
static struct regulator *vdd_cx;

static int usb_phy_susp_dig_vol = 750000;

#ifdef CONFIG_USB_EHCI_MSM_72K
#define PMIC_USB_HS_ID_DET	30
#define PMICID_INT		PM8058_GPIO_IRQ(PM8058_IRQ_BASE,\
							PMIC_USB_HS_ID_DET)
#define PMICID_GPIO		PM8058_GPIO_PM_TO_SYS(PMIC_USB_HS_ID_DET)
#define USB_PMIC_ID_DET_DELAY	msecs_to_jiffies(100)
struct delayed_work pmic_id_det;
notify_vbus_state notify_id_state_func_ptr;

static void pmic_id_detect(struct work_struct *w)
{
	int val = gpio_get_value_cansleep(PMICID_GPIO);
	pr_debug("%s(): gpio_read_value = %d\n", __func__, val);

	if (notify_id_state_func_ptr)
		(*notify_id_state_func_ptr) (val);
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

	struct pm8058_gpio pmic_id_cfg = {
		.direction	= PM_GPIO_DIR_IN,
		.pull		= PM_GPIO_PULL_UP_1P5,
		.function	= PM_GPIO_FUNC_NORMAL,
		.vin_sel	= 2,
		.inv_int_pol	= 0,
	};
	struct pm8058_gpio pmic_id_uncfg = {
		.direction	= PM_GPIO_DIR_IN,
		.pull		= PM_GPIO_PULL_NO,
		.function	= PM_GPIO_FUNC_NORMAL,
		.vin_sel	= 2,
		.inv_int_pol	= 0,
	};
	if (!callback)
		return -EINVAL;

	usb_phy_susp_dig_vol = 500000;

	if (init) {
		notify_id_state_func_ptr = callback;
		INIT_DELAYED_WORK(&pmic_id_det, pmic_id_detect);
		ret = pm8058_gpio_config(PMIC_USB_HS_ID_DET, &pmic_id_cfg);
		if (ret)
			pr_err("%s:return val of pm8058_gpio_config: %d\n",
						__func__,  ret);
		ret = request_threaded_irq(PMICID_INT, NULL, pmic_id_on_irq,
			(IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
						"msm_otg_id", NULL);
		if (ret) {
			pr_err("%s:pmic_usb_id interrupt registration failed",
					__func__);
			return ret;
		}
		semc_fuji_otg_pdata.pmic_id_irq = PMICID_INT;
	} else {
		usb_phy_susp_dig_vol = 750000;
		free_irq(PMICID_INT, 0);
		ret = pm8058_gpio_config(PMIC_USB_HS_ID_DET, &pmic_id_uncfg);
		if (ret)
			pr_err("%s: return val of pm8058_gpio_config: %d\n",
						__func__,  ret);
		semc_fuji_otg_pdata.pmic_id_irq = 0;
		cancel_delayed_work_sync(&pmic_id_det);
		notify_id_state_func_ptr = NULL;
	}
	return 0;
}
#endif

#define PMIC_CBLPWR0_N		10
#define PMICVBUS_INT		PM8058_MPP_IRQ(PM8058_IRQ_BASE, PMIC_CBLPWR0_N)
#define PMICVBUS_GPIO		PM8058_MPP_PM_TO_SYS(PMIC_CBLPWR0_N)
#define USB_PMIC_VBUS_DET_DELAY	msecs_to_jiffies(100)
struct delayed_work pmic_vbus_det;
notify_vbus_state notify_vbus_state_func_ptr;

static void pmic_vbus_detect(struct work_struct *w)
{
	int val = !gpio_get_value_cansleep(PMICVBUS_GPIO);
	pr_debug("%s(): gpio_read_value = %d\n", __func__, val);

	if (notify_vbus_state_func_ptr)
		(*notify_vbus_state_func_ptr) (val);
}

static irqreturn_t pmic_vbus_on_irq(int irq, void *data)
{
	schedule_delayed_work(&pmic_vbus_det, USB_PMIC_VBUS_DET_DELAY);

	return IRQ_HANDLED;
}

static int msm_hsusb_pmic_vbus_notif_init(void (*callback)(int online),
								int init)
{
	unsigned ret = -ENODEV;

	if (!callback)
		return -EINVAL;

	if (init) {
		notify_vbus_state_func_ptr = callback;
		ret = pm8058_mpp_config_digital_in(PMIC_CBLPWR0_N,
			PM8058_MPP_DIG_LEVEL_S3, PM_MPP_DIN_TO_INT);
		if (ret) {
			pr_err("%s: PM8058 MPP%d configuration failed\n",
					__func__, PMIC_CBLPWR0_N + 1);
			return ret;
		}
		INIT_DELAYED_WORK(&pmic_vbus_det, pmic_vbus_detect);
		ret = request_threaded_irq(PMICVBUS_INT, NULL, pmic_vbus_on_irq,
			(IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
						"msm_otg_vbus", NULL);
		if (ret) {
			pr_err("%s:pmic_usb_vbus interrupt registration failed",
					__func__);
			return ret;
		}
		semc_fuji_otg_pdata.pmic_vbus_irq = PMICVBUS_INT;
	} else {
		free_irq(PMICVBUS_INT, 0);
		semc_fuji_otg_pdata.pmic_vbus_irq = 0;
		cancel_delayed_work_sync(&pmic_vbus_det);
		notify_vbus_state_func_ptr = NULL;
	}
	return 0;
}

#define USB_PHY_OPERATIONAL_MIN_VDD_DIG_VOL	1000000
#define USB_PHY_MAX_VDD_DIG_VOL			1320000
static int msm_hsusb_init_vddcx(int init)
{
	int ret = 0;

	if (init) {
		vdd_cx = regulator_get(NULL, "8058_s1");
		if (IS_ERR(vdd_cx)) {
			return PTR_ERR(vdd_cx);
		}

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
	.power_budget	= 300,
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
#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
	.is_cradle_connected	 = semc_charger_cradle_is_connected,
#endif
#if defined(CONFIG_CHARGER_BQ24185)
	.chg_is_initialized	 = bq24185_charger_initialized,
#elif defined(CONFIG_CHARGER_BQ24160)
	.chg_is_initialized	 = bq24160_charger_initialized,
#endif
	.pmic_vbus_notif_init	 = msm_hsusb_pmic_vbus_notif_init,
	.phy_can_powercollapse	 = 1,
	.chg_drawable_ida	 = USB_IDCHG_MAX,
	.platform_init		 = semc_hsusb_platform_init,
};
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
struct msm_hsusb_gadget_platform_data semc_fuji_gadget_pdata = {
	.is_phy_status_timer_on = 1,
};
#endif


/* dynamic composition */
static char *usb_functions_mtp_msc[] = {
	"mtp",
	"usb_mass_storage",
};

static char *usb_functions_mtp_msc_adb[] = {
	"mtp",
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_mtp_msc_adb_eng[] = {
	"mtp",
	"usb_mass_storage",
	"adb",
	"modem",
	"nmea",
	"diag",
#ifdef CONFIG_DIAG_SDIO_PIPE
	"diag_mdm",
#endif
};

static char *usb_functions_mtp[] = {
	"mtp",
};

static char *usb_functions_mtp_adb[] = {
	"mtp",
	"adb",
};

static char *usb_functions_mtp_adb_eng[] = {
	"mtp",
	"adb",
	"modem",
	"nmea",
	"diag",
#ifdef CONFIG_DIAG_SDIO_PIPE
	"diag_mdm",
#endif
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

static char *usb_functions_msc[] = {
	"usb_mass_storage",
};

#ifdef CONFIG_USB_ANDROID_ACCESSORY
static char *usb_functions_accessory[] = { "accessory" };
static char *usb_functions_accessory_adb[] = { "accessory", "adb" };
#endif

static char *usb_functions_all[] = {
	"rndis",
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
	"mtp",
	"usb_mass_storage",
	"adb",
	"modem",
	"nmea",
	"diag",
#ifdef CONFIG_DIAG_SDIO_PIPE
	"diag_mdm",
#endif
};

static struct android_usb_product usb_products[] = {
	{
		.product_id	= 0xA000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc),
		.functions	= usb_functions_mtp_msc,
		.msc_mode	= STORAGE_MODE_MSC,
	},
	{
		.product_id	= 0xB000 | CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc_adb),
		.functions	= usb_functions_mtp_msc_adb,
		.msc_mode	= STORAGE_MODE_MSC,
	},
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
		.msc_mode	= STORAGE_MODE_CDROM, /* for PC-C mode */
	},
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
	{
#ifdef CONFIG_DIAG_SDIO_PIPE
		.product_id	= 0xC146,
#else
		.product_id	= 0x7146,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_msc_adb_eng),
		.functions	= usb_functions_mtp_msc_adb_eng,
		.msc_mode	= STORAGE_MODE_MSC,
	},
	{
#ifdef CONFIG_DIAG_SDIO_PIPE
		.product_id	= 0xB146,
#else
		.product_id	= 0x5146,
#endif
		.num_functions	= ARRAY_SIZE(usb_functions_mtp_adb_eng),
		.functions	= usb_functions_mtp_adb_eng,
	},
	{
		.product_id	= 0xE000 |  CONFIG_USB_PRODUCT_SUFFIX,
		.num_functions	= ARRAY_SIZE(usb_functions_msc),
		.functions	= usb_functions_msc,
	},
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory),
		.functions	= usb_functions_accessory,
	},
	{
		.vendor_id	= USB_ACCESSORY_VENDOR_ID,
		.product_id	= USB_ACCESSORY_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_accessory_adb),
		.functions	= usb_functions_accessory_adb,
	},
#endif
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

	/* EUI-64 based identifier format */
	.eui64_id = {
		.ieee_company_id = {0x00, 0x0A, 0xD9},
		.vendor_specific_ext_field = {0x00, 0x00, 0x00, 0x00, 0x00},
	},
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
	.vendor_id	= 0x0FCE,
	.product_id	= 0x0000 | CONFIG_USB_PRODUCT_SUFFIX,
	.version	= 0x0100,
	.product_name		= "SEMC HSUSB Device",
	.manufacturer_name	= "SEMC",
	.num_products = ARRAY_SIZE(usb_products),

	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	/* actual serial_number is set by setup_serial_number() */
	.serial_number = "1234567890ABCDEF",
};

struct platform_device semc_fuji_usb_device = {
	.name	= "android_usb",
	.id		= -1,
	.dev		= {
		.platform_data = &android_usb_pdata,
	},
};

#if defined(CONFIG_USB_ANDROID_GG)
#define STARTUP_REASON_INDUS_LOG	(1<<29)
#define GG_PID				0xD14C

static char *usb_functions_gg[] = {
	"gg",
};

static struct android_usb_product usb_gg_products[] = {
	{
		.product_id	= GG_PID,
		.num_functions	= ARRAY_SIZE(usb_functions_gg),
		.functions	= usb_functions_gg,
	},
};

static void init_usb_gg(void)
{
	printk(KERN_INFO "init_usb_gg: Enable USB GG\n");
	android_usb_pdata.num_products = ARRAY_SIZE(usb_gg_products),
	android_usb_pdata.products = usb_gg_products,
	android_usb_pdata.product_id = GG_PID;
	android_usb_pdata.num_functions = ARRAY_SIZE(usb_functions_gg);
	android_usb_pdata.functions = usb_functions_gg;
	android_enable_usb_gg();
}
#endif

#define STARTUP_REASON_PWRKEY		(0x01)
#define STARTUP_REASON_WDOG		(0x10)
#define STARTUP_REASON_USB		(0x20)

static int __init startup_reason_setup(char *startup)
{
	unsigned long startup_reason = 0;
	int rval = 0;
	rval = strict_strtoul(startup, 0, &startup_reason);
	if (!rval) {
		printk(KERN_INFO "%s: 0x%lx\n", __func__, startup_reason);
		/* Set MSC only composition in charge only mode */
		if ((startup_reason & STARTUP_REASON_USB) &&
			!(startup_reason & STARTUP_REASON_PWRKEY) &&
			!(startup_reason & STARTUP_REASON_WDOG))
			android_usb_pdata.product_id = 0xE000 |
				CONFIG_USB_PRODUCT_SUFFIX;
#if defined(CONFIG_USB_ANDROID_GG)
		if (startup_reason & STARTUP_REASON_INDUS_LOG)
			init_usb_gg();
#endif
	}
	return 1;
}
__setup("startup=", startup_reason_setup);

#define USB_SERIAL_NUMBER_LEN 10
static int __init setup_serial_number(char *serial_number)
{
	int i;
	static char usb_serial_number[USB_SERIAL_NUMBER_LEN + 1];
	int serial_number_len;
	char *src = serial_number;

	/* create a MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	serial_number_len = strlen(serial_number);
	if (serial_number_len != USB_SERIAL_NUMBER_LEN)
		printk(KERN_ERR "Serial number length was %d, expected %d.",
			serial_number_len, USB_SERIAL_NUMBER_LEN);

	strncpy(usb_serial_number, serial_number, USB_SERIAL_NUMBER_LEN);
	usb_serial_number[USB_SERIAL_NUMBER_LEN] = '\0';
	android_usb_pdata.serial_number = usb_serial_number;
	mass_storage_pdata.serial_number = usb_serial_number;

	printk(KERN_INFO "USB serial number: %s\n",
			android_usb_pdata.serial_number);
	return 1;
}

__setup("androidboot.serialno=", setup_serial_number);

#ifdef CONFIG_USB_NCP373
#define GPIO_USB_OTG_EN			28
#define GPIO_USB_OTG_OVERCUR_INT	104
#define REGULATOR_NCP373_IN		"8901_mpp0"

struct ncp373_res_hdl {
	int en;
	struct regulator *in;
	int flg;
};
static struct ncp373_res_hdl ncp373_hdl;

static int ncp373_gpio_request(int gpio, char *name)
{
	int ret;

	ret = gpio_request(gpio, name);

	if (unlikely(ret < 0)) {
		pr_err("%s: failed to request gpio=%d name=%s\n", __func__,
								gpio, name);
		ret = 0;
	} else {
		pr_debug("%s: got gpio gpio=%d name=%s\n", __func__,
								gpio, name);
		ret = gpio;
	}

	return ret;
}

static struct regulator *ncp373_regulator_request(char *name)
{
	struct regulator *ret;

	ret = regulator_get(NULL, name);

	if (IS_ERR(ret)) {
		pr_err("%s: failed to get regulator name=\"%s\" ret=%d\n",
						__func__, name, (int)ret);
		ret = NULL;
	} else {
		pr_debug("%s: got regulator name=\"%s\"\n", __func__, name);
	}

	return ret;
}

static int ncp373_en_request(void)
{
	return ncp373_gpio_request(GPIO_USB_OTG_EN, "ncp373_en");
}

static struct regulator *ncp373_in_request(void)
{
	return ncp373_regulator_request(REGULATOR_NCP373_IN);
}

static int ncp373_flg_request(void)
{
	return ncp373_gpio_request(GPIO_USB_OTG_OVERCUR_INT, "ncp373_flg");
}

static int ncp373_probe(struct platform_device *pdev)
{
	/* It may not be got a resource here,
	 * due to the timeliness of the device initialization.
	*/
	if (likely(!ncp373_hdl.en))
		ncp373_hdl.en = ncp373_en_request();

	if (likely(!ncp373_hdl.in))
		ncp373_hdl.in = ncp373_in_request();

	if (likely(!ncp373_hdl.flg))
		ncp373_hdl.flg = ncp373_flg_request();

	return 0;
}

static int ncp373_en_set(int on)
{
	int ret = -EIO;

	if (unlikely(!ncp373_hdl.en))
		ncp373_hdl.en = ncp373_en_request();

	if (likely(ncp373_hdl.en)) {
		gpio_set_value(ncp373_hdl.en, !!on);
		ret = 0;
	}

	return ret;
}

static int ncp373_in_set(int on)
{
	int ret = -EIO;

	if (unlikely(!ncp373_hdl.in))
		ncp373_hdl.in = ncp373_in_request();

	if (likely(ncp373_hdl.in)) {
		if (on)
			ret = regulator_enable(ncp373_hdl.in);
		else
			ret = regulator_disable(ncp373_hdl.in);

		if (unlikely(ret < 0))
			pr_err("%s: failed to switch %s regulator %s ret=%d\n",
						__func__, on ? "on " : "off",
						REGULATOR_NCP373_IN, ret);
	}

	return ret;
}

static int ncp373_flg_get(void)
{
	int ret = -EIO;

	if (unlikely(!ncp373_hdl.flg))
		ncp373_hdl.flg = ncp373_flg_request();

	if (likely(ncp373_hdl.flg)) {
		ret = gpio_get_value(ncp373_hdl.flg);
		if (unlikely(ret < 0))
			pr_err("%s: failed to read GPIO=%d ret=%d\n",
						__func__, ncp373_hdl.flg, ret);
	}

	return ret;
}

static void ncp373_remove(void)
{
	if (likely(ncp373_hdl.en))
		gpio_free(ncp373_hdl.en);

	if (likely(ncp373_hdl.in))
		regulator_put(ncp373_hdl.in);

	if (likely(ncp373_hdl.flg))
		gpio_free(ncp373_hdl.flg);

	ncp373_hdl.en = 0;
	ncp373_hdl.in = NULL;
	ncp373_hdl.flg = 0;
}

static void ncp373_notify_flg_int(void)
{
	pr_info("%s: received over current notify\n", __func__);
	msm_otg_notify_vbus_drop();
}

static void ncp373_check_pin_state(void)
{
	int en = -1;
	int in = -1;
	int flg = -1;

	if (likely(ncp373_hdl.en))
		en = gpio_get_value(ncp373_hdl.en);

	if (likely(ncp373_hdl.in))
		in = regulator_is_enabled(ncp373_hdl.in);

	if (likely(ncp373_hdl.flg))
		flg = gpio_get_value(ncp373_hdl.flg);

	pr_debug("%s: EN=%d, IN=%d, FLG=%d\n", __func__, en, in, flg);
}

struct ncp373_platform_data ncp373_pdata = {
	.probe		= ncp373_probe,
	.remove		= ncp373_remove,
	.en_set		= ncp373_en_set,
	.in_set		= ncp373_in_set,
	.flg_get	= ncp373_flg_get,
	.notify_flg_int	= ncp373_notify_flg_int,
	.check_pin_state = ncp373_check_pin_state,
	.oc_delay_time	= 300000,
};

static struct resource ncp373_resources[] = {
	/* OTG_OVERCUR_INT */
	{
		.start	= MSM_GPIO_TO_INT(GPIO_USB_OTG_OVERCUR_INT),
		.end	= MSM_GPIO_TO_INT(GPIO_USB_OTG_OVERCUR_INT),
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device ncp373_device = {
	.name		= NCP373_DRIVER_NAME,
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ncp373_resources),
	.resource	= ncp373_resources,
	.dev		= {
		.platform_data = &ncp373_pdata,
	},
};
#endif
