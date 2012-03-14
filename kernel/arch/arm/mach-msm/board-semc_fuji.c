/* arch/arm/mach-msm/board-semc_fuji.c
 *
 * Copyright (C) [2011 2012] Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/mfd/pmic8058.h>

#include <linux/input/pmic8058-keypad.h>
#include <linux/pmic8058-pwrkey.h>
#include <linux/pmic8058-vibrator.h>
#include <linux/leds.h>
#include <linux/pmic8058-othc.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <linux/bootmem.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <linux/leds-pmic8058.h>
#include <linux/pmic8058-xoadc.h>
#include <linux/msm_adc.h>
#include <linux/m_adcproc.h>
#include <linux/mfd/marimba.h>
#include <linux/msm-charger.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/dma-mapping.h>
#include <linux/leds-as3676.h>
#include <linux/lm356x.h>
#include <linux/apds9702.h>
#include <linux/bma250_ng.h>
#include <linux/clearpad.h>
#include <linux/serial_core.h>
#ifdef CONFIG_INPUT_AKM8975
#include <linux/akm8975.h>
#endif
#ifdef CONFIG_INPUT_AKM8972
#include <linux/akm8972.h>
#endif
#include <linux/i2c/bq27520_battery.h>
#include <linux/battery_chargalg.h>
#include <mach/usb_gadget_fserial.h>
#ifdef CONFIG_CHARGER_BQ24185
#include <linux/i2c/bq24185_charger.h>
#endif
#ifdef CONFIG_CHARGER_BQ24160
#include <linux/i2c/bq24160_charger.h>
#endif
#ifdef CONFIG_INPUT_LPS331AP
#include <linux/lps331ap.h>
#endif
#include <mach/semc_battery_data.h>
#include <mach/msm72k_otg.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/reboot.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/setup.h>

#include <mach/dma.h>
#include <mach/bcm_bt_lpm.h>
#include <mach/mpp.h>
#include <mach/board.h>
#include <mach/irqs.h>
#include <mach/msm_spi.h>
#include <mach/msm_serial_hs.h>
#include <mach/msm_serial_hs_lite.h>
#include <mach/msm_iomap.h>
#include <asm/mach/mmc.h>
#include <mach/msm_battery.h>
#include <mach/msm_xo.h>
#include <mach/msm_bus_board.h>
#include <mach/socinfo.h>
#include <mach/scm-io.h>
#include <linux/i2c/isl9519.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/machine.h>
#ifdef CONFIG_INPUT_ASETM2034A
#include <linux/input/asetm2034a.h>
#endif /* CONFIG_INPUT_ASETM2034A */
#include <mach/semc_charger_usb.h>
#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
#include <mach/semc_charger_cradle.h>
#endif
#include <mach/rpm.h>
#include <mach/rpm-regulator.h>
#include <mach/restart.h>

#include "devices.h"
#include "devices-msm8x60.h"
#include "cpuidle.h"
#include "pm.h"
#include "spm.h"
#include "rpm_log.h"
#include "timer.h"
#include "saw-regulator.h"
#include "gpiomux.h"
#include "gpiomux-8x60.h"
#include "mpm.h"
#include "gpiomux-semc_fuji.h"
#include "keypad-semc_fuji.h"
#include "vreg-semc_fuji.h"
#include "leds-semc_fuji.h"
#include "touch-semc_fuji.h"
#include "board-semc_fuji-usb.h"
#include "board-semc_fuji-video.h"
#include "board-semc_fuji-sdcc.h"
#include "board-semc_fuji-pmic.h"
#ifdef CONFIG_SEMC_ONESEG_TUNER_PM
#include "board-semc_fuji-oneseg.h"
#endif
#include <mach/simple_remote_msm8x60_pf.h>
#include "rpm_stats.h"

#if defined(CONFIG_SENSORS_MPU3050)
#include <linux/mpu.h>
#include "gyro-semc_fuji.h"
#endif

#ifdef CONFIG_NFC_PN544
#include <linux/pn544.h>
#include "nfc-fuji.h"
#endif

#ifdef CONFIG_MSM_GSBI7_UART
#include <linux/tty.h>
#endif

#ifdef CONFIG_SEMC_FELICA_SUPPORT
#include "board-semc_fuji-felica.h"
#endif

#define MSM_SHARED_RAM_PHYS 0x40000000
#define BMA250_GPIO			(41)
#define AKM897X_GPIO		(31)
#define BMA250_DEFAULT_RATE	50
#ifdef CONFIG_INPUT_AKM8975
#define GPIO_FUNC_NAME "akm8975_drdy"
#endif
#ifdef CONFIG_INPUT_AKM8972
#define GPIO_FUNC_NAME "akm8972_drdy"
#endif

#ifdef CONFIG_SENSORS_MPU3050
#define MPU3050_GPIO		(30)
#endif

/*
FM GPIO is GPIO 18 on PMIC 8058.
As the index starts from 0 in the PMIC driver, and hence 17
corresponds to GPIO 18 on PMIC 8058.
*/
#define FM_GPIO 17
#define BT_GPIO_EN			(23)
#define BT_GPIO_WAKE			(137)
#define BT_GPIO_HOST_WAKE		(63)

#ifdef CONFIG_NFC_PN544
#define PMIC_GPIO_NFC_EN		(17)
#define PMIC_GPIO_NFC_FWDL_EN		(27)
#define PMIC_GPIO_NFC_IRQ		(28)
#endif

/* Platform specific HW-ID GPIO mask */
static const u8 hw_id_gpios[] = {11, 15, 12, 86};

static struct msm_spm_platform_data msm_spm_data_v1[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x0F,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0xFFFFFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0xFFFFFFFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0x94,
		.retention_vlevel = 0x81,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x94,
		.collapse_mid_vlevel = 0x8C,

		.vctl_timeout_us = 50,
	},
};

static struct msm_spm_platform_data msm_spm_data[] __initdata = {
	[0] = {
		.reg_base_addr = MSM_SAW0_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},

	[1] = {
		.reg_base_addr = MSM_SAW1_BASE,

#ifdef CONFIG_MSM_AVS_HW
		.reg_init_values[MSM_SPM_REG_SAW_AVS_CTL] = 0x586020FF,
#endif
		.reg_init_values[MSM_SPM_REG_SAW_CFG] = 0x1C,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_CTL] = 0x68,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_SLP_TMR_DLY] = 0x0C0CFFFF,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_WAKE_TMR_DLY] = 0x78780FFF,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLK_EN] = 0x13,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_PRECLMP_EN] = 0x07,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_HSFS_POSTCLMP_EN] = 0x00,

		.reg_init_values[MSM_SPM_REG_SAW_SLP_CLMP_EN] = 0x01,
		.reg_init_values[MSM_SPM_REG_SAW_SLP_RST_EN] = 0x00,
		.reg_init_values[MSM_SPM_REG_SAW_SPM_MPM_CFG] = 0x00,

		.awake_vlevel = 0xA0,
		.retention_vlevel = 0x89,
		.collapse_vlevel = 0x20,
		.retention_mid_vlevel = 0x89,
		.collapse_mid_vlevel = 0x89,

		.vctl_timeout_us = 50,
	},
};

static struct msm_acpu_clock_platform_data msm8x60_acpu_clock_data = {
};

static struct regulator_consumer_supply saw_s0_supply =
	REGULATOR_SUPPLY("8901_s0", NULL);
static struct regulator_consumer_supply saw_s1_supply =
	REGULATOR_SUPPLY("8901_s1", NULL);

static struct regulator_init_data saw_s0_init_data = {
		.constraints = {
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1250000,
		},
		.num_consumer_supplies = 1,
		.consumer_supplies = &saw_s0_supply,
};

static struct regulator_init_data saw_s1_init_data = {
		.constraints = {
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
			.min_uV = 800000,
			.max_uV = 1250000,
		},
		.num_consumer_supplies = 1,
		.consumer_supplies = &saw_s1_supply,
};

static struct platform_device msm_device_saw_s0 = {
	.name          = "saw-regulator",
	.id            = SAW_VREG_ID_S0,
	.dev           = {
		.platform_data = &saw_s0_init_data,
	},
};

static struct platform_device msm_device_saw_s1 = {
	.name          = "saw-regulator",
	.id            = SAW_VREG_ID_S1,
	.dev           = {
		.platform_data = &saw_s1_init_data,
	},
};

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

#define QCE_SIZE		0x10000
#define QCE_0_BASE		0x18500000

#define QCE_HW_KEY_SUPPORT	0

#define QCE_SHARE_CE_RESOURCE	2
#define QCE_CE_SHARED		1

static struct resource qcrypto_resources[] = {
	[0] = {
		.start = QCE_0_BASE,
		.end = QCE_0_BASE + QCE_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.name = "crypto_channels",
		.start = DMOV_CE_IN_CHAN,
		.end = DMOV_CE_OUT_CHAN,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.name = "crypto_crci_in",
		.start = DMOV_CE_IN_CRCI,
		.end = DMOV_CE_IN_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.name = "crypto_crci_out",
		.start = DMOV_CE_OUT_CRCI,
		.end = DMOV_CE_OUT_CRCI,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.name = "crypto_crci_hash",
		.start = DMOV_CE_HASH_CRCI,
		.end = DMOV_CE_HASH_CRCI,
		.flags = IORESOURCE_DMA,
	},
};

static struct resource qcedev_resources[] = {
        [0] = {
                .start = QCE_0_BASE,
                .end = QCE_0_BASE + QCE_SIZE - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .name = "crypto_channels",
                .start = DMOV_CE_IN_CHAN,
                .end = DMOV_CE_OUT_CHAN,
                .flags = IORESOURCE_DMA,
        },
        [2] = {
                .name = "crypto_crci_in",
                .start = DMOV_CE_IN_CRCI,
                .end = DMOV_CE_IN_CRCI,
                .flags = IORESOURCE_DMA,
        },
        [3] = {
                .name = "crypto_crci_out",
                .start = DMOV_CE_OUT_CRCI,
                .end = DMOV_CE_OUT_CRCI,
                .flags = IORESOURCE_DMA,
        },
        [4] = {
                .name = "crypto_crci_hash",
                .start = DMOV_CE_HASH_CRCI,
                .end = DMOV_CE_HASH_CRCI,
                .flags = IORESOURCE_DMA,
        },
};

#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)

static struct msm_ce_hw_support qcrypto_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
};

static struct platform_device qcrypto_device = {
	.name		= "qcrypto",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcrypto_resources),
	.resource	= qcrypto_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcrypto_ce_hw_suppport,
	},
};
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)

static struct msm_ce_hw_support qcedev_ce_hw_suppport = {
	.ce_shared = QCE_CE_SHARED,
	.shared_ce_resource = QCE_SHARE_CE_RESOURCE,
	.hw_key_support = QCE_HW_KEY_SUPPORT,
};

static struct platform_device qcedev_device = {
	.name		= "qce",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(qcedev_resources),
	.resource	= qcedev_resources,
	.dev		= {
		.coherent_dma_mask = DMA_BIT_MASK(32),
		.platform_data = &qcedev_ce_hw_suppport,
	},
};
#endif

static struct msm_pm_platform_data msm_pm_data[MSM_PM_SLEEP_MODE_NR * 2] = {
	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 4000,
		.residency = 13000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 500,
		.residency = 6000,
	},

	[MSM_PM_MODE(0, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.supported = 1,
		.suspend_enabled = 1,
		.idle_enabled = 1,
		.latency = 2,
		.residency = 0,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 600,
		.residency = 7200,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE)] = {
		.supported = 1,
		.suspend_enabled = 0,
		.idle_enabled = 0,
		.latency = 500,
		.residency = 6000,
	},

	[MSM_PM_MODE(1, MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT)] = {
		.supported = 1,
		.suspend_enabled = 1,
		.idle_enabled = 1,
		.latency = 2,
		.residency = 0,
	},
};

static struct msm_cpuidle_state msm_cstates[] __initdata = {
	{0, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{0, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},

	{0, 2, "C2", "POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE},

	{1, 0, "C0", "WFI",
		MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT},

	{1, 1, "C1", "STANDALONE_POWER_COLLAPSE",
		MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE},
};



#define LM356X_HW_RESET_GPIO 138
int lm356x_request_gpio_pins(void)
{
	int result;

	result = gpio_request(LM356X_HW_RESET_GPIO, "LM356X hw reset");
	if (result)
		return result;

	gpio_set_value(LM356X_HW_RESET_GPIO, 1);

	udelay(20);
	return result;
}

int lm356x_release_gpio_pins(void)
{
	gpio_set_value(LM356X_HW_RESET_GPIO, 0);
	gpio_free(LM356X_HW_RESET_GPIO);

	return 0;
}

static struct lm356x_platform_data lm3560_platform_data = {
	.hw_enable		= lm356x_request_gpio_pins,
	.hw_disable		= lm356x_release_gpio_pins,
	.led_nums		= 2,
	.strobe_trigger		= LM356X_STROBE_TRIGGER_EDGE,
	.privacy_terminate	= LM356X_PRIVACY_MODE_TURN_BACK,
	.privacy_led_nums	= 1,
	.privacy_blink_period	= 0, /* No bliking */
	.current_limit		= 2300000, /* uA */
	.flash_sync		= LM356X_SYNC_OFF,
	.strobe_polarity	= LM356X_STROBE_POLARITY_HIGH,
	.ledintc_pin_setting	= LM356X_LEDINTC_NTC_THERMISTOR_INPUT,
	.tx1_polarity		= LM356X_TX1_POLARITY_HIGH,
	.tx2_polarity		= LM356X_TX2_POLARITY_HIGH,
	.hw_torch_mode		= LM356X_HW_TORCH_MODE_DISABLE,
};

static int hw_id_get_mask(void)
{
	int hwid;
	unsigned int i;
	int rc;
	for (hwid = i = 0; i < ARRAY_SIZE(hw_id_gpios); i++) {
		rc = gpio_request(hw_id_gpios[i], "HWID_GPIO");
		if (rc) {
			pr_err("%s: gpio_requeset failed, "
					"rc=%d\n", __func__, rc);
			return rc;
		}
		hwid |= gpio_get_value(hw_id_gpios[i]) << i;
		gpio_free(hw_id_gpios[i]);
	}
	return hwid;
}

static ssize_t hw_id_show(struct class *class,
			  struct class_attribute *attr,
			  char *buf)
{
	int hwid;

	hwid = hw_id_get_mask();
	if (hwid < 0)
		return -EINVAL;

	pr_info("Board Fuji HW ID: 0x%02x\n", hwid);
	return snprintf(buf, sizeof(buf), "0x%02x\n", hwid);
}

static CLASS_ATTR(hwid, 0400, hw_id_show, NULL);
static struct class hwid_class = {.name	= "hwid",};
static void __init hw_id_class_init(void)
{
	int error;
	error = class_register(&hwid_class);
	if (error) {
		pr_err("%s: class_register failed\n", __func__);
		return;
	}
	error = class_create_file(&hwid_class, &class_attr_hwid);
	if (error) {
		pr_err("%s: class_create_file failed\n", __func__);
		class_unregister(&hwid_class);
	}
}

#define PLUG_DET_ENA_PIN 17
#define PLUG_DET_READ_PIN 61
#define BUTTON_DET_IRQ PM8058_SW_1_IRQ(PM8058_IRQ_BASE)

#define PM8058_OTHC_CNTR_BASE1	0x134

int simple_remote_pf_initialize_gpio(struct simple_remote_platform_data *data)
{
	int err = 0;
	int i;
	int hwid;

	if (!data ||
	    -1 == data->headset_detect_enable_pin ||
	    -1 == data->headset_detect_read_pin) {
		pr_err("%s: Invalid inparameter (GPIO Pins)."
		       " Aborting!\n", __func__);
		return -EIO;
	}

	hwid = hw_id_get_mask();
	if (hwid < 0) {
		pr_err("%s: not able to get hw_id\n", __func__);
		return -EINVAL;
	}

	if (!hwid) {
		err = gpio_request(data->headset_detect_enable_pin,
				   "Simple_remote_plug_detect_enable");
		if (err) {
			pr_err("%s: %d Request hs_detect_enable pin",
			       __func__, err);
			goto out;
		}
		gpio_set_value(data->headset_detect_enable_pin, 1);
	}

	err = gpio_request(data->headset_detect_read_pin,
			   "Simple_remote_plug_detect_read");
	if (err) {
		pr_err("%s: %d Request hs-detect_read pin",
		       __func__, err);
		goto out_hs_det_enable;
	}

	err = gpio_direction_input(data->headset_detect_read_pin);
	if (err) {
		pr_err("%s: %d Set hs-detect pin as input\n",
		       __func__, err);
		goto out_hs_det_read;
	}

	for (i = 0; i < data->num_regs; i++) {
		data->regs[i].reg = regulator_get(NULL, data->regs[i].name);
		if (IS_ERR(data->regs[i].reg)) {
			pr_err("%s - Failed to find regulator %s\n",
			       __func__, data->regs[i].name);
			err = PTR_ERR(data->regs[i].reg);
			goto out_hs_det_read;
		}

		err = regulator_set_voltage(data->regs[i].reg,
					    data->regs[i].vol_min,
					    data->regs[i].vol_max);
		if (err) {
			pr_err("%s - Unable to set the voltage for regulator "
			       "%s\n", __func__, data->regs[i].name);
			goto out_hs_det_read;
		}
	}

	return err;

out_hs_det_read:
	gpio_free(data->headset_detect_read_pin);

out_hs_det_enable:
	gpio_free(data->headset_detect_enable_pin);
out:
	return err;
}

void simple_remote_pf_deinitialize_gpio(
	struct simple_remote_platform_data *data)
{
	gpio_free(data->headset_detect_read_pin);
	gpio_free(data->headset_detect_enable_pin);
}

static struct simple_remote_platform_regulators regs[] =  {
	{
		.name = "8058_ncp",
		.vol_min = 1800000,
		.vol_max = 1800000,
	},
	{
		.name = "8058_s3",
		.vol_min = 1800000,
		.vol_max = 1800000,
	},

};

struct simple_remote_platform_data simple_remote_pf_data = {
	.headset_detect_enable_pin = PLUG_DET_ENA_PIN,
	.headset_detect_read_pin = PLUG_DET_READ_PIN,
	.button_detect_irq = BUTTON_DET_IRQ,
	.adc_channel = CHANNEL_ADC_HDSET,
	.othc_base = PM8058_OTHC_CNTR_BASE1,
	.initialize = &simple_remote_pf_initialize_gpio,
	.deinitialize = &simple_remote_pf_deinitialize_gpio,
	.regs = regs,
	.num_regs = ARRAY_SIZE(regs),
#ifdef CONFIG_SIMPLE_REMOTE_INVERT_PLUG_DETECTION_STATE
	.invert_plug_det = 1,
#else
	.invert_plug_det = 0,
#endif
};

extern struct wifi_platform_data fuji_wifi_control;

static struct platform_device fuji_wifi = {
	.name   = "bcm4330_wlan",
	.id	    = -1,
	.dev    = {
		.platform_data = &fuji_wifi_control,
	},
};

#ifdef CONFIG_FUJI_GPIO_KEYPAD
struct platform_device gpio_key_device = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_key_data,
	},
};
#endif /* CONFIG_FUJI_GPIO_KEYPAD */

#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
#define CHG_CRADLE_DET_GPIO 126

static int semc_chg_cradle_gpio_configure(int enable)
{
	int rc = 0;

	if (!!enable) {
		rc = gpio_request(CHG_CRADLE_DET_GPIO, "chg_cradle_det");
		if (rc)
			pr_err("%s: gpio_requeset failed, "
					"rc=%d\n", __func__, rc);
	} else {
		gpio_free(CHG_CRADLE_DET_GPIO);
	}
	return rc;
}

static char *semc_chg_cradle_supplied_to[] = {
	SEMC_CHARGER_AC_NAME,
};

struct semc_chg_cradle_platform_data semc_chg_cradle_platform_data = {
	.cradle_detect_gpio = CHG_CRADLE_DET_GPIO,
	.supplied_to = semc_chg_cradle_supplied_to,
	.num_supplicants = ARRAY_SIZE(semc_chg_cradle_supplied_to),
	.supply_current_limit_from_cradle = 2500,
	.gpio_configure = semc_chg_cradle_gpio_configure,
};

static struct platform_device semc_chg_cradle = {
	.name   = SEMC_CHG_CRADLE_NAME,
	.id	= -1,
	.dev    = {
		.platform_data = &semc_chg_cradle_platform_data,
	},
};
#endif

#ifdef CONFIG_SEMC_CHARGER_USB_ARCH
static char *semc_chg_usb_supplied_to[] = {
	BATTERY_CHARGALG_NAME,
	BQ27520_NAME,
};
#endif

static char *semc_bdata_supplied_to[] = {
	BQ27520_NAME,
	BATTERY_CHARGALG_NAME,
};

static struct semc_battery_platform_data semc_battery_platform_data = {
	.supplied_to = semc_bdata_supplied_to,
	.num_supplicants = ARRAY_SIZE(semc_bdata_supplied_to),
	.get_current_average = bq27520_get_current_average,
};

static struct platform_device bdata_driver = {
	.name = SEMC_BDATA_NAME,
	.id = -1,
	.dev = {
		.platform_data = &semc_battery_platform_data,
	},
};

#ifdef CONFIG_CHARGER_BQ24185
#define BQ24185_GPIO_IRQ	125

static char *bq24185_supplied_to[] = {
	BATTERY_CHARGALG_NAME,
	SEMC_BDATA_NAME,
};

static int bq24185_gpio_configure(int enable)
{
	int rc = 0;

	if (!!enable) {
		rc = gpio_request(BQ24185_GPIO_IRQ, "bq24185");
		if (rc)
			pr_err("%s: gpio_requeset failed, "
					"rc=%d\n", __func__, rc);
	} else {
		gpio_free(BQ24185_GPIO_IRQ);
	}
	return rc;
}

struct bq24185_platform_data bq24185_platform_data = {
	.name = BQ24185_NAME,
	.supplied_to = bq24185_supplied_to,
	.num_supplicants = ARRAY_SIZE(bq24185_supplied_to),
	.support_boot_charging = 1,
	.rsens = BQ24185_RSENS_REF,
	/* Maximum battery regulation voltage = 4200mV */
	.mbrv = BQ24185_MBRV_MV_4200,
	/* Maximum charger current sense voltage = 105.4mV */
	.mccsv = BQ24185_MCCSV_MV_13p6 | BQ24185_MCCSV_MV_54p4 |
		BQ24185_MCCSV_MV_37p4,
	.notify_vbus_drop = msm_otg_notify_vbus_drop,
	.gpio_configure = bq24185_gpio_configure,
	.vindpm_usb_compliant = VINDPM_4550MV,
	.vindpm_non_compliant = VINDPM_4390MV,
};
#endif

#ifdef CONFIG_CHARGER_BQ24160
#define BQ24160_GPIO_IRQ	125

static char *bq24160_supplied_to[] = {
	BATTERY_CHARGALG_NAME,
	SEMC_BDATA_NAME,
};

static int bq24160_gpio_configure(int enable)
{
	int rc = 0;

	if (!!enable) {
		rc = gpio_request(BQ24160_GPIO_IRQ, "bq24160");
		if (rc)
			pr_err("%s: gpio_requeset failed, "
					"rc=%d\n", __func__, rc);
	} else {
		gpio_free(BQ24160_GPIO_IRQ);
	}
	return rc;
}

struct bq24160_platform_data bq24160_platform_data = {
	.name = BQ24160_NAME,
	.supplied_to = bq24160_supplied_to,
	.num_supplicants = ARRAY_SIZE(bq24160_supplied_to),
	.support_boot_charging = 1,
	.notify_vbus_drop = msm_otg_notify_vbus_drop,
	.gpio_configure = bq24160_gpio_configure,
};
#endif

#define GPIO_BQ27520_SOC_INT 123
#define LIPO_BAT_MAX_VOLTAGE 4200
#define LIPO_BAT_MIN_VOLTAGE 3000
#define FULLY_CHARGED_AND_RECHARGE_CAP 95

static char *bq27520_supplied_to[] = {
	BATTERY_CHARGALG_NAME,
};

static struct bq27520_block_table bq27520_block_table[BQ27520_BTBL_MAX] = {
	{0x61, 0x00}, {0x3E, 0x24}, {0x3F, 0x00}, {0x42, 0x00},
	{0x43, 0x46}, {0x44, 0x00}, {0x45, 0x19}, {0x46, 0x00},
	{0x47, 0x64}, {0x48, 0x28}, {0x4B, 0xFF}, {0x4C, 0x5F},
	{0x60, 0xF4}
};

static int bq27520_gpio_configure(int enable)
{
	int rc = 0;

	if (!!enable) {
		rc = gpio_request(GPIO_BQ27520_SOC_INT, "bq27520");
		if (rc)
			pr_err("%s: gpio_requeset failed, "
					"rc=%d\n", __func__, rc);
	} else {
		gpio_free(GPIO_BQ27520_SOC_INT);
	}
	return rc;
}

struct bq27520_platform_data bq27520_platform_data = {
	.name = BQ27520_NAME,
	.supplied_to = bq27520_supplied_to,
	.num_supplicants = ARRAY_SIZE(bq27520_supplied_to),
	.lipo_bat_max_volt = LIPO_BAT_MAX_VOLTAGE,
	.lipo_bat_min_volt = LIPO_BAT_MIN_VOLTAGE,
	.battery_dev_name = SEMC_BDATA_NAME,
	.gpio_configure = bq27520_gpio_configure,
	.polling_lower_capacity = FULLY_CHARGED_AND_RECHARGE_CAP,
	.polling_upper_capacity = 100,
	.udatap = bq27520_block_table,
#ifdef CONFIG_BATTERY_CHARGALG
	.disable_algorithm = battery_chargalg_disable,
#endif
#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
	.get_ac_online_status = semc_charger_get_ac_online_status,
#endif
};

static char *battery_chargalg_supplied_to[] = {
	SEMC_BDATA_NAME,
};

static struct battery_chargalg_platform_data battery_chargalg_platform_data = {
	.name = BATTERY_CHARGALG_NAME,
	.supplied_to = battery_chargalg_supplied_to,
	.num_supplicants = ARRAY_SIZE(battery_chargalg_supplied_to),
	.ext_eoc_recharge_enable = 1,
	.temp_hysteresis_design = 3,
	.ddata = &device_data,
	.batt_volt_psy_name = BQ27520_NAME,
	.batt_curr_psy_name = BQ27520_NAME,
#ifdef CONFIG_CHARGER_BQ24185
	.turn_on_charger = bq24185_turn_on_charger,
	.turn_off_charger = bq24185_turn_off_charger,
	.set_charger_voltage = bq24185_set_charger_voltage,
	.set_charger_current = bq24185_set_charger_current,
	.set_input_current_limit = bq24185_set_input_current_limit,
	.set_charging_status = bq24185_set_ext_charging_status,
	.get_supply_current_limit = NULL,
	.get_restrict_ctl = NULL,
	.get_restricted_setting = NULL,
	.setup_exchanged_power_supply = NULL,
	.charge_set_current_1 = 350,
	.charge_set_current_2 = 650,
	.charge_set_current_3 = 850,
	.overvoltage_max_design = 4225,
#endif
#ifdef CONFIG_CHARGER_BQ24160
	.turn_on_charger = bq24160_turn_on_charger,
	.turn_off_charger = bq24160_turn_off_charger,
	.set_charger_voltage = bq24160_set_charger_voltage,
	.set_charger_current = bq24160_set_charger_current,
	.set_input_current_limit = bq24160_set_input_current_limit,
	.set_charging_status = bq24160_set_ext_charging_status,
	.get_supply_current_limit = NULL,
	.get_restrict_ctl = bq24160_is_restricted_by_charger_revision,
	.get_restricted_setting = bq24160_get_restricted_setting,
	.setup_exchanged_power_supply = bq24160_setup_exchanged_power_supply,
	.charge_set_current_1 = 350,
	.charge_set_current_2 = 575,
	.charge_set_current_3 = 725,
	.overvoltage_max_design = 4245,
#endif
#ifdef CONFIG_SEMC_CHARGER_USB_ARCH
	.get_supply_current_limit = semc_charger_usb_current_ma,
#endif
#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
	.get_ac_online_status = semc_charger_get_ac_online_status,
	.set_input_current_limit_dual = bq24160_set_input_current_limit_dual,
	.set_input_voltage_dpm_usb = bq24160_set_input_voltage_dpm_usb,
	.set_input_voltage_dpm_cradle = bq24160_set_input_voltage_dpm_in,
	.get_supply_current_limit_cradle = semc_charger_cradle_current_ma,
#endif
	.allow_dynamic_charge_current_ctrl = 1,
	.average_current_min_limit = -1,
	.average_current_max_limit = 250,
};

static struct platform_device battery_chargalg_platform_device = {
	.name = BATTERY_CHARGALG_NAME,
	.id = -1,
	.dev = {
		.platform_data = &battery_chargalg_platform_data,
	},
};

#if defined(CONFIG_MSM_VPE) || defined(CONFIG_MSM_VPE_STANDALONE)
static struct resource msm_vpe_resources[] = {
	{
		.start	= 0x05300000,
		.end	= 0x05300000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_VPE,
		.end	= INT_VPE,
		.flags	= IORESOURCE_IRQ,
	},
};
#endif

#ifdef CONFIG_MSM_VPE
static struct platform_device msm_vpe_device = {
	.name = "msm_vpe",
	.id   = 0,
	.num_resources = ARRAY_SIZE(msm_vpe_resources),
	.resource = msm_vpe_resources,
};
#endif
#ifdef CONFIG_MSM_VPE_STANDALONE
static struct platform_device msm_vpe_standalone_device = {
	.name		= "msm_vpe_standalone",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(msm_vpe_resources),
	.resource	= msm_vpe_resources,
};
#endif

#ifdef CONFIG_MSM_CAMERA
#define VFE_CAMIF_TIMER1_GPIO 29
#define VFE_CAMIF_TIMER2_GPIO 30
#define VFE_CAMIF_TIMER3_GPIO_INT 31

int msm_cam_gpio_tbl[] = {
	25, /* CHAT_CAM_RST_N */
	32, /* CAMIF_MCLK */
	47, /* CAMIF_I2C_DATA */
	48, /* CAMIF_I2C_CLK */
	106, /* CAM_RST_N */
};

enum msm_cam_stat {
	MSM_CAM_OFF,
	MSM_CAM_ON,
};

static int config_gpio_table(enum msm_cam_stat stat)
{
	int rc = 0;
	int i = 0;
	int j = 0;

	if (stat == MSM_CAM_ON) {
		for (i = 0; i < ARRAY_SIZE(msm_cam_gpio_tbl); i++) {
			rc = gpio_request(msm_cam_gpio_tbl[i], "CAM_GPIO");
			if (unlikely(rc < 0)) {
				pr_err("%s not able to get gpio\n", __func__);
				for (j = i - 1; j >= 0; j--)
					gpio_free(msm_cam_gpio_tbl[j]);
				break;
			}
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(msm_cam_gpio_tbl); i++)
			gpio_free(msm_cam_gpio_tbl[i]);
	}
	return rc;
}

static int config_camera_on_gpios(void)
{
	int rc = 0;

	rc = config_gpio_table(MSM_CAM_ON);
	if (rc < 0) {
		printk(KERN_ERR "%s: CAMSENSOR gpio table request"
		"failed\n", __func__);
	}

	return rc;
}

static void config_camera_off_gpios(void)
{
	config_gpio_table(MSM_CAM_OFF);
}
#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors cam_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_preview_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 484614900,
		.ib  = 2100000000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 149299200,
		.ib  = 2640000000U,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 223948800,
		.ib  = 335923200,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 656132400,
		.ib  = 2100000000,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 215654400,
		.ib  = 2640000000U,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 323481600,
		.ib  = 485222400,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 660234240,
		.ib  = 1056374784,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 79902720,
		.ib  = 127844352,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 1042495488,
		.ib  = 1667992781,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_zsl_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 566231040,
		.ib  = 905969664,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 706199040,
		.ib  = 1129918464,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 320864256,
		.ib  = 513382810,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 320864256,
		.ib  = 513382810,
	},
};

static struct msm_bus_vectors cam_stereo_video_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 212336640,
		.ib  = 339738624,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 25090560,
		.ib  = 40144896,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 239708160,
		.ib  = 383533056,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 79902720,
		.ib  = 127844352,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 0,
		.ib  = 0,
	},
};

static struct msm_bus_vectors cam_stereo_snapshot_vectors[] = {
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 0,
		.ib  = 0,
	},
	{
		.src = MSM_BUS_MASTER_VFE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 300902400,
		.ib  = 481443840,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 230307840,
		.ib  = 368492544,
	},
	{
		.src = MSM_BUS_MASTER_VPE,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 245113344,
		.ib  = 392181351,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab  = 106536960,
		.ib  = 170459136,
	},
	{
		.src = MSM_BUS_MASTER_JPEG_ENC,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab  = 106536960,
		.ib  = 170459136,
	},
};

static struct msm_bus_paths cam_bus_client_config[] = {
	{
		ARRAY_SIZE(cam_init_vectors),
		cam_init_vectors,
	},
	{
		ARRAY_SIZE(cam_preview_vectors),
		cam_preview_vectors,
	},
	{
		ARRAY_SIZE(cam_video_vectors),
		cam_video_vectors,
	},
	{
		ARRAY_SIZE(cam_snapshot_vectors),
		cam_snapshot_vectors,
	},
	{
		ARRAY_SIZE(cam_zsl_vectors),
		cam_zsl_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_video_vectors),
		cam_stereo_video_vectors,
	},
	{
		ARRAY_SIZE(cam_stereo_snapshot_vectors),
		cam_stereo_snapshot_vectors,
	},
};

static struct msm_bus_scale_pdata cam_bus_client_pdata = {
		cam_bus_client_config,
		ARRAY_SIZE(cam_bus_client_config),
		.name = "msm_camera",
};
#endif


struct msm_camera_device_platform_data msm_camera_device_data = {
	.camera_gpio_on		= config_camera_on_gpios,
	.camera_gpio_off	= config_camera_off_gpios,
	.ioext.csiphy		= 0x04800000,
	.ioext.csisz		= 0x00000400,
	.ioext.csiirq		= CSI_0_IRQ,
	.ioclk.mclk_clk_rate	= 8000000,
	.ioclk.vfe_clk_rate	= 266667000,
#ifdef CONFIG_MSM_BUS_SCALING
	.cam_bus_scale_table = &cam_bus_client_pdata,
#endif
};

struct msm_camera_device_platform_data msm_sub_camera_device_data = {
	.camera_gpio_on		= config_camera_on_gpios,
	.camera_gpio_off	= config_camera_off_gpios,
	.ioext.csiphy		= 0x04900000,
	.ioext.csisz		= 0x00000400,
	.ioext.csiirq		= CSI_1_IRQ,
	.ioclk.mclk_clk_rate	= 8000000,
	.ioclk.vfe_clk_rate	= 228570000,
#ifdef CONFIG_MSM_BUS_SCALING
	.cam_bus_scale_table = &cam_bus_client_pdata,
#endif
};

struct resource msm_camera_resources[] = {
	{
		.start	= 0x04500000,
		.end	= 0x04500000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= VFE_IRQ,
		.end	= VFE_IRQ,
		.flags	= IORESOURCE_IRQ,
	},
};

#if defined(CONFIG_SEMC_CAMERA_MODULE) || defined(CONFIG_SEMC_SUB_CAMERA_MODULE)
static struct msm_camera_sensor_flash_data flash_none = {
	.flash_type	= MSM_CAMERA_FLASH_NONE,
	.flash_src	= NULL,
 };
#endif

#ifdef CONFIG_SEMC_CAMERA_MODULE
static struct msm_camera_sensor_info msm_camera_sensor_semc_camera_data = {
	.sensor_name		= "semc_camera",
	.sensor_reset		= 106,
	.sub_sensor_reset	= 25,
	.sensor_pwd		= 106,
	.vcm_pwd		= 106,
	.vcm_enable		= 106,
	.mclk			= 32,
	.flash_type		= MSM_CAMERA_FLASH_NONE,
	.pdata			= &msm_camera_device_data,
	.resource		= msm_camera_resources,
	.num_resources		= ARRAY_SIZE(msm_camera_resources),
	.flash_data		= &flash_none,
	.csi_if			= 1, /* mipi interface direct */
	.csi_params = {
		.data_format	= CSI_10BIT,
		.lane_cnt	= 4,
		.lane_assign	= 0xe4,
		.settle_cnt	= 25,
		.dpcm_scheme	= 0,
	},
	.vcam_io = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_lvs0",
	},
	.vcam_sd = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_l1",
	},
	.vcam_sa = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_l15",
	},
	.vcam_af = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_l9",
	},
};

static struct platform_device msm_camera_sensor_semc_camera = {
	.name = "msm_camera_semc_camera",
	.dev = {
		.platform_data = &msm_camera_sensor_semc_camera_data,
	},
};
#endif

#ifdef CONFIG_SEMC_SUB_CAMERA_MODULE
static struct msm_camera_sensor_info msm_camera_sensor_semc_sub_camera_data = {
	.sensor_name		= "semc_sub_camera",
	.sensor_reset		= 106,
	.sub_sensor_reset	= 25,
	.sensor_pwd		= 106,
	.vcm_pwd		= 106,
	.vcm_enable		= 106,
	.mclk			= 32,
	.flash_type		= MSM_CAMERA_FLASH_NONE,
	.pdata			= &msm_sub_camera_device_data,
	.resource		= msm_camera_resources,
	.num_resources		= ARRAY_SIZE(msm_camera_resources),
	.flash_data		= &flash_none,
	.csi_if			= 1, /* mipi interface direct */
	.csi_params = {
		.data_format	= CSI_10BIT,
		.lane_cnt	= 1,
		.lane_assign	= 0xe4,
		.settle_cnt	= 34,
		.dpcm_scheme	= 0,
	},
	.vcam_io = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_lvs0",
	},
	.vcam_sd = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_l1",
	},
	.vcam_sa = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_l15",
	},
	.vcam_af = {
		.type		= MSM_CAMERA_SENSOR_PWR_VREG,
		.resource.name	= "8058_l9",
	},
};

static struct platform_device msm_camera_sensor_semc_sub_camera = {
	.name = "msm_camera_semc_sub_camera",
	.dev  = {
		.platform_data = &msm_camera_sensor_semc_sub_camera_data,
	},
};
#endif

static struct i2c_board_info msm_camera_boardinfo[] __initdata = {
#ifdef CONFIG_SEMC_CAMERA_MODULE
	{
		I2C_BOARD_INFO("semc_camera", 0x1A),
	},
#endif
#ifdef CONFIG_SEMC_SUB_CAMERA_MODULE
	{
		I2C_BOARD_INFO("semc_sub_camera", 0x48),
	},
#endif
};
#endif

#ifdef CONFIG_MSM_GEMINI
static struct resource msm_gemini_resources[] = {
	{
		.start	= 0x04600000,
		.end	= 0x04600000 + SZ_1M - 1,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= INT_JPEG,
		.end	= INT_JPEG,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device msm_gemini_device = {
	.name		= "msm_gemini",
	.resource	= msm_gemini_resources,
	.num_resources	= ARRAY_SIZE(msm_gemini_resources),
};
#endif

#ifdef CONFIG_I2C_QUP
#define GSBI7_SDA 59
#define GSBI7_SCL 60

static uint32_t gsbi7_gpio_table[] = {
	GPIO_CFG(GSBI7_SDA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GSBI7_SCL, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static uint32_t gsbi7_i2c_table[] = {
	GPIO_CFG(GSBI7_SDA, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
	GPIO_CFG(GSBI7_SCL, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
}

static void gsbi7_qup_i2c_gpio_config(int adap_id, int config_type)
{
	if (config_type == 0) {
		gpio_tlmm_config(gsbi7_gpio_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi7_gpio_table[1], GPIO_CFG_ENABLE);
	} else {
		gpio_tlmm_config(gsbi7_i2c_table[0], GPIO_CFG_ENABLE);
		gpio_tlmm_config(gsbi7_i2c_table[1], GPIO_CFG_ENABLE);
	}
}

static struct msm_i2c_platform_data msm_gsbi3_qup_i2c_pdata = {
	.clk_freq = 400000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi4_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

#ifdef CONFIG_MSM_GSBI5_I2C
static struct msm_i2c_platform_data msm_gsbi5_qup_i2c_pdata = {
	.clk_freq = 355000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.use_gsbi_shared_mode = 1,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

static struct msm_i2c_platform_data msm_gsbi7_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.use_gsbi_shared_mode = 1,
	.pri_clk = 60,
	.pri_dat = 59,
	.msm_i2c_config_gpio = gsbi7_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi8_qup_i2c_pdata = {
	.clk_freq = 355000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi9_qup_i2c_pdata = {
	.clk_freq = 100000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi12_qup_i2c_pdata = {
	.clk_freq = 355000,
	.src_clk_rate = 24000000,
	.clk = "gsbi_qup_clk",
	.pclk = "gsbi_pclk",
	.use_gsbi_shared_mode = 1,
	.msm_i2c_config_gpio = gsbi_qup_i2c_gpio_config,
};
#endif

#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
static struct msm_spi_platform_data msm_gsbi1_qup_spi_pdata = {
	.max_clock_speed = 24000000,
};

static struct msm_spi_platform_data msm_gsbi10_qup_spi_pdata = {
	.max_clock_speed = 24000000,
};
#endif

#ifdef CONFIG_I2C_SSBI
/* PMIC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi1_pdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
};

/* PMIC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi2_pdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
};

/* CODEC/TSSC SSBI */
static struct msm_i2c_ssbi_platform_data msm_ssbi3_pdata = {
	.controller_type = MSM_SBI_CTRL_SSBI,
};
#endif

/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE + \
				MSM_FB_WRITEBACK_SIZE + \
				MSM_FB_DSUB_PMEM_ADDER, 4096)
#define MSM_PMEM_SF_SIZE 0x4000000 /* 64 Mbytes */

#define MSM_PMEM_KERNEL_EBI1_SIZE  0x600000
#define MSM_PMEM_ADSP_SIZE         0x1000000
#define MSM_PMEM_CAMERA_SIZE       0x5000000
#define MSM_PMEM_AUDIO_SIZE        0x239000
#define MSM_PMEM_SWIQI_SIZE        0x2000000

#define MSM_SMI_BASE          0x38000000
/* Kernel SMI PMEM Region for video core, used for Firmware */
/* and encoder,decoder scratch buffers */
/* Kernel SMI PMEM Region Should always precede the user space */
/* SMI PMEM Region, as the video core will use offset address */
/* from the Firmware base */
#define PMEM_KERNEL_SMI_BASE  (MSM_SMI_BASE)
#define PMEM_KERNEL_SMI_SIZE  0x600000
/* User space SMI PMEM Region for video core*/
/* used for encoder, decoder input & output buffers  */
#define MSM_PMEM_SMIPOOL_BASE (PMEM_KERNEL_SMI_BASE + PMEM_KERNEL_SMI_SIZE)
#define MSM_PMEM_SMIPOOL_SIZE 0x3A00000

static unsigned fb_size = MSM_FB_SIZE;
static int __init fb_size_setup(char *p)
{
	fb_size = memparse(p, NULL);
	return 0;
}
early_param("fb_size", fb_size_setup);

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static unsigned pmem_kernel_ebi1_size = MSM_PMEM_KERNEL_EBI1_SIZE;
static int __init pmem_kernel_ebi1_size_setup(char *p)
{
	pmem_kernel_ebi1_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_kernel_ebi1_size", pmem_kernel_ebi1_size_setup);
#endif

#ifdef CONFIG_ANDROID_PMEM
static unsigned pmem_sf_size = MSM_PMEM_SF_SIZE;
static int __init pmem_sf_size_setup(char *p)
{
	pmem_sf_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_sf_size", pmem_sf_size_setup);

static unsigned pmem_adsp_size = MSM_PMEM_ADSP_SIZE;

static int __init pmem_adsp_size_setup(char *p)
{
	pmem_adsp_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_adsp_size", pmem_adsp_size_setup);

static unsigned pmem_camera_size = MSM_PMEM_CAMERA_SIZE;

static int __init pmem_camera_size_setup(char *p)
{
	pmem_camera_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_camera_size", pmem_camera_size_setup);

static unsigned pmem_audio_size = MSM_PMEM_AUDIO_SIZE;

static int __init pmem_audio_size_setup(char *p)
{
	pmem_audio_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_audio_size", pmem_audio_size_setup);

#ifdef CONFIG_SEMC_SWIQI
static unsigned pmem_swiqi_size = MSM_PMEM_SWIQI_SIZE;

static int __init pmem_swiqi_size_setup(char *p)
{
	pmem_swiqi_size = memparse(p, NULL);
	return 0;
}
early_param("pmem_swiqi_size", pmem_swiqi_size_setup);
#endif
#endif

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
static struct android_pmem_platform_data android_pmem_kernel_ebi1_pdata = {
	.name = PMEM_KERNEL_EBI1_DATA_NAME,
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_kernel_ebi1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &android_pmem_kernel_ebi1_pdata },
};
#endif

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
static struct android_pmem_platform_data android_pmem_kernel_smi_pdata = {
	.name = PMEM_KERNEL_SMI_DATA_NAME,
	/* defaults to bitmap don't edit */
	.cached = 0,
};

static struct platform_device android_pmem_kernel_smi_device = {
	.name = "android_pmem",
	.id = 6,
	.dev = { .platform_data = &android_pmem_kernel_smi_pdata },
};
#endif

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data android_pmem_pdata = {
	.name = "pmem",
	.allocator_type = PMEM_ALLOCATORTYPE_ALLORNOTHING,
	.cached = 1,
	.map_on_demand = 1,
};

static struct platform_device android_pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = {.platform_data = &android_pmem_pdata},
};

static struct android_pmem_platform_data android_pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
	.map_on_demand = 1,
};

static struct platform_device android_pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &android_pmem_adsp_pdata },
};

static struct android_pmem_platform_data android_pmem_camera_pdata = {
	.name = "pmem_camera",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.map_on_demand = 1,
};

static struct platform_device android_pmem_camera_device = {
	.name = "android_pmem",
	.id = 3,
	.dev = { .platform_data = &android_pmem_camera_pdata },
};

static struct android_pmem_platform_data android_pmem_audio_pdata = {
	.name = "pmem_audio",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 0,
};

static struct platform_device android_pmem_audio_device = {
	.name = "android_pmem",
	.id = 4,
	.dev = { .platform_data = &android_pmem_audio_pdata },
};

#ifdef CONFIG_SEMC_SWIQI
static struct android_pmem_platform_data android_pmem_swiqi_pdata = {
	.name = "pmem_swiqi",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.map_on_demand = 1,
};
static struct platform_device android_pmem_swiqi_device = {
	.name = "android_pmem",
	.id = 5,
	.dev = { .platform_data = &android_pmem_swiqi_pdata },
};
#endif

#define PMEM_BUS_WIDTH(_bw) \
	{ \
		.vectors = &(struct msm_bus_vectors){ \
			.src = MSM_BUS_MASTER_AMPSS_M0, \
			.dst = MSM_BUS_SLAVE_SMI, \
			.ib = (_bw), \
			.ab = 0, \
		}, \
	.num_paths = 1, \
	}
static struct msm_bus_paths pmem_smi_table[] = {
	[0] = PMEM_BUS_WIDTH(0), /* Off */
	[1] = PMEM_BUS_WIDTH(1), /* On */
};

static struct msm_bus_scale_pdata smi_client_pdata = {
	.usecase = pmem_smi_table,
	.num_usecases = ARRAY_SIZE(pmem_smi_table),
	.active_only = 1,
	.name = "pmem_smi",
};

void pmem_request_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 1);
}

void pmem_release_smi_region(void *data)
{
	int bus_id = (int) data;

	msm_bus_scale_client_update_request(bus_id, 0);
}

void *pmem_setup_smi_region(void)
{
	return (void *)msm_bus_scale_register_client(&smi_client_pdata);
}
static struct android_pmem_platform_data android_pmem_smipool_pdata = {
	.name = "pmem_smipool",
	.allocator_type = PMEM_ALLOCATORTYPE_BITMAP,
	.cached = 1,
	.request_region = pmem_request_smi_region,
	.release_region = pmem_release_smi_region,
	.setup_region = pmem_setup_smi_region,
	.map_on_demand = 1,
};
static struct platform_device android_pmem_smipool_device = {
	.name = "android_pmem",
	.id = 7,
	.dev = { .platform_data = &android_pmem_smipool_pdata },
};

#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define MSM_RAM_CONSOLE_START (0x80000000 - MSM_RAM_CONSOLE_SIZE)
#define MSM_RAM_CONSOLE_SIZE  (128 * SZ_1K)

static struct resource ram_console_resources[] = {
	[0] = {
		.start  = MSM_RAM_CONSOLE_START,
		.end    = MSM_RAM_CONSOLE_START + MSM_RAM_CONSOLE_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device ram_console_device = {
	.name           = "ram_console",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(ram_console_resources),
	.resource       = ram_console_resources,
};
#endif

#ifdef CONFIG_RAMDUMP_CRASH_LOGS
#define MSM_RAMDUMP_INFO_START	0x7FF00000
#define MSM_RAMDUMP_INFO_SIZE  (4 * SZ_1K)
#define MSM_AMSS_LOG_START	(MSM_RAMDUMP_INFO_START + MSM_RAMDUMP_INFO_SIZE)
#define MSM_AMSS_LOG_SIZE  (16 * SZ_1K)

static struct resource ramdumplog_resources[] = {
	{
		.name   = "ramdumpinfo",
		.start  = MSM_RAMDUMP_INFO_START,
		.end    = MSM_RAMDUMP_INFO_START + MSM_RAMDUMP_INFO_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	{
		.name   = "amsslog",
		.start  = MSM_AMSS_LOG_START,
		.end    = MSM_AMSS_LOG_START + MSM_AMSS_LOG_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device ramdumplog_device = {
	.name           = "ramdumplog",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(ramdumplog_resources),
	.resource       = ramdumplog_resources,
};
#endif

static void __init msm8x60_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem(size);
	semc_fuji_fb_resources[0].start = __pa(addr);
	semc_fuji_fb_resources[0].end =
	  semc_fuji_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
		size, addr, __pa(addr));

#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
	size = pmem_kernel_ebi1_size;
	if (size) {
		addr = alloc_bootmem_aligned(size, 0x100000);
		android_pmem_kernel_ebi1_pdata.start = __pa(addr);
		android_pmem_kernel_ebi1_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for kernel"
			" ebi1 pmem arena\n", size, addr, __pa(addr));
	}
#endif

#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
	size = PMEM_KERNEL_SMI_SIZE;
	if (size) {
		android_pmem_kernel_smi_pdata.start = PMEM_KERNEL_SMI_BASE;
		android_pmem_kernel_smi_pdata.size = size;
		pr_info("allocating %lu bytes at %lx physical for kernel"
			" smi pmem arena\n", size,
			(unsigned long) PMEM_KERNEL_SMI_BASE);
	}
#endif

#ifdef CONFIG_ANDROID_PMEM
	size = pmem_adsp_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_adsp_pdata.start = __pa(addr);
		android_pmem_adsp_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for adsp "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = MSM_PMEM_SMIPOOL_SIZE;
	if (size) {
		android_pmem_smipool_pdata.start = MSM_PMEM_SMIPOOL_BASE;
		android_pmem_smipool_pdata.size = size;
		pr_info("allocating %lu bytes at %lx physical for user"
			" smi  pmem arena\n", size,
			(unsigned long) MSM_PMEM_SMIPOOL_BASE);
	}

	size = pmem_camera_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_camera_pdata.start = __pa(addr);
		android_pmem_camera_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for camera "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = MSM_PMEM_AUDIO_SIZE;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_audio_pdata.start = __pa(addr);
		android_pmem_audio_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for audio "
			"pmem arena\n", size, addr, __pa(addr));
	}

	size = pmem_sf_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_pdata.start = __pa(addr);
		android_pmem_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for sf "
			"pmem arena\n", size, addr, __pa(addr));
	}

#ifdef CONFIG_SEMC_SWIQI
	size = pmem_swiqi_size;
	if (size) {
		addr = alloc_bootmem(size);
		android_pmem_swiqi_pdata.start = __pa(addr);
		android_pmem_swiqi_pdata.size = size;
		pr_info("allocating %lu bytes at %p (%lx physical) for swiqi "
			"pmem arena\n", size, addr, __pa(addr));
	}
#endif
#endif
}

static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup = 0,
	.exit_lpm_cb = bcm_bt_lpm_exit_lpm_locked,
};

#ifdef CONFIG_MSM_GSBI7_UART
static int felica_uart_pre_startup(struct uart_port *uport)
{
	struct tty_struct *tty = uport->state->port.tty;

	mutex_lock(&tty->termios_mutex);
	tty->termios->c_ispeed = 460800;
	tty->termios->c_ospeed = 460800;
	tty->termios->c_cflag |= B460800;
	tty->termios->c_cflag &= ~0xD;
	tty->termios->c_cflag |= (CLOCAL | CREAD);
	tty->termios->c_cflag &= ~PARENB;
	tty->termios->c_cflag &= ~CSTOPB;
	tty->termios->c_cflag &= ~CSIZE;
	tty->termios->c_cflag &= ~PARODD;
	tty->termios->c_cflag |= CS8;
	tty->termios->c_lflag &= ~(ICANON | IEXTEN | ISIG | ECHO);
	tty->termios->c_oflag &= ~OPOST;
	tty->termios->c_iflag &= ~(ICRNL | INPCK | ISTRIP |
						IXON | BRKINT);
	tty->termios->c_cc[VMIN] = 0;
	tty->termios->c_cc[VTIME] = 0;
	tty->ldisc->ops->set_termios(tty, NULL);
	mutex_unlock(&tty->termios_mutex);

	return 0;
}

static int configure_uart_dm7_gpios(int on)
{
	int ret = 0, i;
	int uart_gpios[] = {57, 58};

	for (i = 0; i < ARRAY_SIZE(uart_gpios); i++) {
		if (on) {
			ret = msm_gpiomux_get(uart_gpios[i]);
			if (unlikely(ret))
				break;
		} else {
			ret = msm_gpiomux_put(uart_gpios[i]);
			if (unlikely(ret))
				return ret;
		}
	}
	if (ret)
		for (; i >= 0; i--)
			msm_gpiomux_put(uart_gpios[i]);
	return ret;
}

static struct msm_serial_hs_platform_data msm_uart_dm7_pdata = {
	.gpio_config = configure_uart_dm7_gpios,
	.inject_rx_on_wakeup = 0,
	.pre_startup	= felica_uart_pre_startup,
};
#endif

static struct platform_device msm_bt_power_device = {
	.name = "bt_power",
};
static int bluetooth_power(int on)
{
	int ret = 0, i;
	int bt_power_gpios[] = {53, 54, 55, 56};
	/*	already configured, return */
	if (on == gpio_get_value(BT_GPIO_EN))
		return 0;

	if (on) {
		for (i = 0; i < ARRAY_SIZE(bt_power_gpios); i++) {
			ret = msm_gpiomux_get(bt_power_gpios[i]);
			if (unlikely(ret))
				goto error;
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(bt_power_gpios); i++)
			msm_gpiomux_put(bt_power_gpios[i]);
	}

	gpio_set_value(BT_GPIO_EN, on);
	return 0;

error:
	for (; i >= 0; i--)
		msm_gpiomux_put(bt_power_gpios[i]);
	return ret;
}

static void __init bt_power_init(void)
{
	int ret = 0, i;
	int bt_gpios[] = {23, 137, 63};
	for (i = 0; i < ARRAY_SIZE(bt_gpios); i++) {
		ret = msm_gpiomux_get(bt_gpios[i]);
		if (unlikely(ret))
			goto error;
	}
	gpio_set_value(BT_GPIO_EN, 0);
	msm_bt_power_device.dev.platform_data = &bluetooth_power;
	return;
error:
	for (; i >= 0; i--)
		msm_gpiomux_put(bt_gpios[i]);
}

static struct bcm_bt_lpm_platform_data bcm_bt_lpm_pdata = {
	.gpio_wake = BT_GPIO_WAKE,
	.gpio_host_wake = BT_GPIO_HOST_WAKE,
	.request_clock_off_locked = msm_hs_request_clock_off_locked,
	.request_clock_on_locked = msm_hs_request_clock_on_locked,
};

struct platform_device bcm_bt_lpm_device = {
	.name = "bcm_bt_lpm",
	.id = 0,
	.dev = {
		.platform_data = &bcm_bt_lpm_pdata,
	},
};

#ifdef CONFIG_MSM_GSBI5_UART
static struct msm_serial_hslite_platform_data msm_uart_gsbi5_pdata = {
	.config_gpio	= 1,
	.uart_tx_gpio	= 49,
	.uart_rx_gpio	= 50,
	.type		= PORT_IRDA,
};
#endif

#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)

static struct msm_rpm_log_platform_data msm_rpm_log_pdata = {
	.phys_addr_base = 0x00106000,
	.reg_offsets = {
		[MSM_RPM_LOG_PAGE_INDICES] = 0x00000C80,
		[MSM_RPM_LOG_PAGE_BUFFER]  = 0x00000CA0,
	},
	.phys_size = SZ_8K,
	.log_len = 4096,		  /* log's buffer length in bytes */
	.log_len_mask = (4096 >> 2) - 1,  /* length mask in units of u32 */
};

static struct platform_device msm_rpm_log_device = {
	.name	= "msm_rpm_log",
	.id	= -1,
	.dev	= {
		.platform_data = &msm_rpm_log_pdata,
	},
};
#endif

static struct platform_device *early_regulators[] __initdata = {
	&msm_device_saw_s0,
	&msm_device_saw_s1,
#ifdef CONFIG_PMIC8058
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S0],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S1],
#endif
};

static struct platform_device *early_devices[] __initdata = {
#ifdef CONFIG_MSM_BUS_SCALING
	&msm_bus_apps_fabric,
	&msm_bus_sys_fabric,
	&msm_bus_mm_fabric,
	&msm_bus_sys_fpb,
	&msm_bus_cpss_fpb,
#endif
	&msm_device_dmov_adm0,
	&msm_device_dmov_adm1,
};


#ifdef CONFIG_THERMAL_TSENS
static struct platform_device msm_tsens_device = {
	.name   = "tsens-tm",
	.id = -1,
};
#endif

static struct platform_device *fuji_devices[] __initdata = {
	&msm_device_smd,
	&msm_device_uart_dm12,
#ifdef CONFIG_MSM_GSBI5_UART
	&msm_device_uart_gsbi5,
#endif
#ifdef CONFIG_I2C_QUP
	&msm_gsbi3_qup_i2c_device,
	&msm_gsbi4_qup_i2c_device,
#ifdef CONFIG_MSM_GSBI5_I2C
	&msm_gsbi5_qup_i2c_device,
#endif
	&msm_gsbi7_qup_i2c_device,
	&msm_gsbi8_qup_i2c_device,
	&msm_gsbi9_qup_i2c_device,
	&msm_gsbi12_qup_i2c_device,
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	&msm_gsbi1_qup_spi_device,
#endif
	&msm_device_uart_dm1,
#ifdef CONFIG_MSM_GSBI7_UART
	&msm_device_uart_dm7,
#endif
#ifdef CONFIG_SEMC_FELICA_SUPPORT
	&semc_felica_device,
#endif
	&msm_bt_power_device,
	&bcm_bt_lpm_device,
#ifdef CONFIG_I2C_SSBI
	&msm_device_ssbi1,
	&msm_device_ssbi2,
	&msm_device_ssbi3,
#endif
#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_HCD)
	&msm_device_otg,
#endif
#ifdef CONFIG_USB_GADGET_MSM_72K
	&msm_device_gadget_peripheral,
#endif
#ifdef CONFIG_USB_ANDROID
	&semc_fuji_mass_storage_device,
	&semc_fuji_rndis_device,
#ifdef CONFIG_USB_ANDROID_DIAG
	&usb_diag_device,
#endif
#ifdef CONFIG_USB_F_SERIAL
	&usb_gadget_fserial_device,
#endif
	&semc_fuji_usb_device,
#endif
#ifdef CONFIG_KERNEL_PMEM_EBI_REGION
	&android_pmem_kernel_ebi1_device,
#endif
#ifdef CONFIG_KERNEL_PMEM_SMI_REGION
	&android_pmem_kernel_smi_device,
#endif
#ifdef CONFIG_ANDROID_PMEM
	&android_pmem_device,
	&android_pmem_adsp_device,
	&android_pmem_camera_device,
	&android_pmem_audio_device,
	&android_pmem_smipool_device,
#ifdef CONFIG_SEMC_SWIQI
	&android_pmem_swiqi_device,
#endif
#endif
#ifdef CONFIG_MSM_ROTATOR
	&msm_rotator_device,
#endif
	&semc_fuji_fb_device,
	&msm_kgsl_3d0,
#ifdef CONFIG_MSM_KGSL_2D
	&msm_kgsl_2d0,
	&msm_kgsl_2d1,
#endif
#if defined(CONFIG_FB_MSM_HDMI_MSM_PANEL) || defined(CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL)
	&semc_fuji_hdmi_device,
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL || CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL */
#ifdef CONFIG_SEMC_CAMERA_MODULE
	&msm_camera_sensor_semc_camera,
#endif
#ifdef CONFIG_SEMC_SUB_CAMERA_MODULE
	&msm_camera_sensor_semc_sub_camera,
#endif
#ifdef CONFIG_MSM_GEMINI
	&msm_gemini_device,
#endif
#ifdef CONFIG_MSM_VPE
	&msm_vpe_device,
#endif
#ifdef CONFIG_MSM_VPE_STANDALONE
	&msm_vpe_standalone_device,
#endif

#if defined(CONFIG_MSM_RPM_LOG) || defined(CONFIG_MSM_RPM_LOG_MODULE)
	&msm_rpm_log_device,
#endif
	&msm_device_vidc,
#ifdef CONFIG_SENSORS_MSM_ADC
	&semc_fuji_adc_device,
#endif
#ifdef CONFIG_PMIC8058
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L0],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L1],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L2],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L3],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L4],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L5],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L6],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L7],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L8],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L9],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L10],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L12],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L13],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L14],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L15],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L16],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L17],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L18],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L19],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L20],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L21],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L22],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_L25],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S2],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S3],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_S4],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_LVS0],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_LVS1],
	&rpm_vreg_device[RPM_VREG_ID_PM8058_NCP],
#endif
#ifdef CONFIG_PMIC8901
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L1],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L2],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L4],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_L5],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_S3],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_S4],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS0],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS1],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS2],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_LVS3],
	&rpm_vreg_device[RPM_VREG_ID_PM8901_MVS0],
#endif

#if defined(CONFIG_CRYPTO_DEV_QCRYPTO) || \
		defined(CONFIG_CRYPTO_DEV_QCRYPTO_MODULE)
	&qcrypto_device,
#endif

#if defined(CONFIG_CRYPTO_DEV_QCEDEV) || \
		defined(CONFIG_CRYPTO_DEV_QCEDEV_MODULE)
	&qcedev_device,
#endif

#if defined(CONFIG_TSIF) || defined(CONFIG_TSIF_MODULE)
#ifdef CONFIG_MSM_USE_TSIF1
	&msm_device_tsif[1],
#else
	&msm_device_tsif[0],
#endif /* CONFIG_MSM_USE_TSIF1 */
#endif /* CONFIG_TSIF */

#ifdef CONFIG_HW_RANDOM_MSM
	&msm_device_rng,
#endif
#ifdef CONFIG_ANDROID_RAM_CONSOLE
	&ram_console_device,
#endif
#ifdef CONFIG_RAMDUMP_CRASH_LOGS
	&ramdumplog_device,
#endif
	&bdata_driver,
	&battery_chargalg_platform_device,
	&fuji_wifi,
#ifdef CONFIG_THERMAL_TSENS
	&msm_tsens_device,
#endif
#ifdef CONFIG_SEMC_CHARGER_CRADLE_ARCH
	&semc_chg_cradle,
#endif
#ifdef CONFIG_FUJI_GPIO_KEYPAD
	&gpio_key_device,
#endif /* CONFIG_FUJI_GPIO_KEYPAD */
#ifdef CONFIG_SEMC_ONESEG_TUNER_PM
	&oneseg_tunerpm_device,
#endif
#ifdef CONFIG_USB_NCP373
	&ncp373_device,
#endif
};

#define EXT_CHG_VALID_MPP 10
#define EXT_CHG_VALID_MPP_2 11

#define PM_GPIO_CDC_RST_N 20
#define GPIO_CDC_RST_N PM8058_GPIO_PM_TO_SYS(PM_GPIO_CDC_RST_N)

static struct regulator *vreg_timpani_1;
static struct regulator *vreg_timpani_2;

static unsigned int msm_timpani_setup_power(void)
{
	int rc;

	vreg_timpani_1 = regulator_get(NULL, "8058_l0");
	if (IS_ERR(vreg_timpani_1)) {
		pr_err("%s: Unable to get 8058_l0\n", __func__);
		return -ENODEV;
	}

	vreg_timpani_2 = regulator_get(NULL, "8058_s3");
	if (IS_ERR(vreg_timpani_2)) {
		pr_err("%s: Unable to get 8058_s3\n", __func__);
		regulator_put(vreg_timpani_1);
		return -ENODEV;
	}

	rc = regulator_set_voltage(vreg_timpani_1, 1200000, 1200000);
	if (rc) {
		pr_err("%s: unable to set L0 voltage to 1.2V\n", __func__);
		goto fail;
	}

	rc = regulator_set_voltage(vreg_timpani_2, 1800000, 1800000);
	if (rc) {
		pr_err("%s: unable to set S3 voltage to 1.8V\n", __func__);
		goto fail;
	}

	rc = regulator_enable(vreg_timpani_1);
	if (rc) {
		pr_err("%s: Enable regulator 8058_l0 failed\n", __func__);
		goto fail;
	}

	/* The settings for LDO0 should be set such that
	*  it doesn't require to reset the timpani. */
	rc = regulator_set_optimum_mode(vreg_timpani_1, 5000);
	if (rc < 0) {
		pr_err("Timpani regulator optimum mode setting failed\n");
		goto fail;
	}

	rc = regulator_enable(vreg_timpani_2);
	if (rc) {
		pr_err("%s: Enable regulator 8058_s3 failed\n", __func__);
		regulator_disable(vreg_timpani_1);
		goto fail;
	}

	rc = gpio_request(GPIO_CDC_RST_N, "CDC_RST_N");
	if (rc) {
		pr_err("%s: GPIO Request %d failed\n", __func__,
			GPIO_CDC_RST_N);
		regulator_disable(vreg_timpani_1);
		regulator_disable(vreg_timpani_2);
		goto fail;
	} else {
		gpio_direction_output(GPIO_CDC_RST_N, 1);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_CDC_RST_N, 0);
		usleep_range(1000, 1050);
		gpio_direction_output(GPIO_CDC_RST_N, 1);
		gpio_free(GPIO_CDC_RST_N);
	}
	return rc;

fail:
	regulator_put(vreg_timpani_1);
	regulator_put(vreg_timpani_2);
	return rc;
}

static void msm_timpani_shutdown_power(void)
{
	int rc;

	rc = regulator_disable(vreg_timpani_1);
	if (rc)
		pr_err("%s: Disable regulator 8058_l0 failed\n", __func__);

	regulator_put(vreg_timpani_1);

	rc = regulator_disable(vreg_timpani_2);
	if (rc)
		pr_err("%s: Disable regulator 8058_s3 failed\n", __func__);

	regulator_put(vreg_timpani_2);
}

/* Power analog function of codec */
static struct regulator *vreg_timpani_cdc_apwr;
static int msm_timpani_codec_power(int vreg_on)
{
	int rc = 0;

	if (!vreg_timpani_cdc_apwr) {

		vreg_timpani_cdc_apwr = regulator_get(NULL, "8058_s4");

		if (IS_ERR(vreg_timpani_cdc_apwr)) {
			pr_err("%s: vreg_get failed (%ld)\n",
			__func__, PTR_ERR(vreg_timpani_cdc_apwr));
			rc = PTR_ERR(vreg_timpani_cdc_apwr);
			return rc;
		}
	}

	if (vreg_on) {

		rc = regulator_set_voltage(vreg_timpani_cdc_apwr,
				2200000, 2200000);
		if (rc) {
			pr_err("%s: unable to set 8058_s4 voltage to 2.2 V\n",
					__func__);
			goto vreg_fail;
		}

		rc = regulator_enable(vreg_timpani_cdc_apwr);
		if (rc) {
			pr_err("%s: vreg_enable failed %d\n", __func__, rc);
			goto vreg_fail;
		}
	} else {
		rc = regulator_disable(vreg_timpani_cdc_apwr);
		if (rc) {
			pr_err("%s: vreg_disable failed %d\n",
			__func__, rc);
			goto vreg_fail;
		}
	}

	return 0;

vreg_fail:
	regulator_put(vreg_timpani_cdc_apwr);
	vreg_timpani_cdc_apwr = NULL;
	return rc;
}

static struct marimba_codec_platform_data timpani_codec_pdata = {
	.marimba_codec_power =  msm_timpani_codec_power,
};

#define TIMPANI_SLAVE_ID_CDC_ADDR		0X77
#define TIMPANI_SLAVE_ID_QMEMBIST_ADDR		0X66

static struct marimba_platform_data timpani_pdata = {
	.slave_id[MARIMBA_SLAVE_ID_CDC]	= TIMPANI_SLAVE_ID_CDC_ADDR,
	.slave_id[MARIMBA_SLAVE_ID_QMEMBIST] = TIMPANI_SLAVE_ID_QMEMBIST_ADDR,
	.marimba_setup = msm_timpani_setup_power,
	.marimba_shutdown = msm_timpani_shutdown_power,
	.codec = &timpani_codec_pdata,
};

#define TIMPANI_I2C_SLAVE_ADDR	0xD

static struct i2c_board_info msm_i2c_gsbi7_timpani_info[] = {
	{
		I2C_BOARD_INFO("timpani", TIMPANI_I2C_SLAVE_ADDR),
		.platform_data = &timpani_pdata,
	},
};

#define VREG8_VDDIO_VOLTAGE 1800000
#define VREG10_VDDIO_VOLTAGE 2850000
#define SHARED_VREG_ON 1
#define SHARED_VREG_OFF 0

static void shared_vreg_switch(int enable)
{
	int rc;
	static struct regulator *vreg_l8;
	static struct regulator *vreg_l10;

	if (!vreg_l10) {
		vreg_l10 = regulator_get(NULL, "8058_l10");
		if (IS_ERR(vreg_l10)) {
			pr_err("%s: unable to get 8058_l10\n", __func__);
			return;
		}
	}
	if (!vreg_l8) {
		vreg_l8 = regulator_get(NULL, "8058_l8");
		if (IS_ERR(vreg_l8)) {
			pr_err("%s: unable to get 8058_l8\n", __func__);
			return;
		}
	}

	if (!!enable) {
		rc = regulator_set_voltage(vreg_l10, VREG10_VDDIO_VOLTAGE,
					VREG10_VDDIO_VOLTAGE);
		if (rc) {
			pr_err("%s: regulator_set_voltage failed\n", __func__);
			goto exit_free_reg_10;
		}
		rc = regulator_enable(vreg_l10);
		if (rc) {
			pr_err("%s: vreg_enable failed\n", __func__);
			goto exit_free_reg_10;
		}
		mdelay(10); /* Device Spec */
		rc = regulator_set_voltage(vreg_l8, VREG8_VDDIO_VOLTAGE,
					VREG8_VDDIO_VOLTAGE);
		if (rc) {
			pr_err("%s: regulator_set_voltage failed\n", __func__);
			goto exit_free_reg_8;
		}
		rc = regulator_enable(vreg_l8);
		if (rc) {
			pr_err("%s: vreg_enable failed\n", __func__);
			goto exit_free_reg_8;
		}
	} else {
		if (regulator_is_enabled(vreg_l8)) {
			rc = regulator_disable(vreg_l8);
			if (rc) {
				pr_err("%s: vreg_disable failed\n", __func__);
				regulator_put(vreg_l8);
			}
		}
		mdelay(1); /* Device Spec */
		if (regulator_is_enabled(vreg_l10)) {
			rc = regulator_disable(vreg_l10);
			if (rc) {
				pr_err("%s: vreg_disable failed\n", __func__);
				goto exit_free_reg_10;
			}
		}
	}
	return;

exit_free_reg_8:
	regulator_put(vreg_l8);
exit_free_reg_10:
	regulator_put(vreg_l10);
	return;
}

static void __init shared_vreg_init(void)
{
	shared_vreg_switch(SHARED_VREG_ON);
}

static int shared_vreg_exit
	(struct notifier_block *this, unsigned long code, void *_cmd)
{
	shared_vreg_switch(SHARED_VREG_OFF);
	return NOTIFY_DONE;
}

static struct notifier_block vreg_poweroff_notifier = {
	.notifier_call = shared_vreg_exit,
};

static int bma250_gpio_setup(struct device *dev)
{
	return 0;
}

static void bma250_gpio_teardown(struct device *dev)
{
	return;
}

static void bma250_hw_config(int enable)
{
	return;
}

static struct registers bma250_reg_setup = {
	.range                = BMA250_RANGE_2G,
	.bw_sel               = BMA250_BW_250HZ,
};

static struct bma250_platform_data bma250_platform_data = {
	.setup                = bma250_gpio_setup,
	.teardown             = bma250_gpio_teardown,
	.hw_config            = bma250_hw_config,
	.reg                  = &bma250_reg_setup,
	.bypass_state         = mpu3050_bypassmode,
	.read_axis_data       = bma250_read_axis_from_mpu3050,
	.check_sleep_status   = check_bma250_sleep_state,
	.vote_sleep_status    = vote_bma250_sleep_state,
	.rate                 = BMA250_DEFAULT_RATE,
};

#define APDS9702_DOUT_GPIO   34

static int apds9702_gpio_setup(int request)
{
	if (request) {
		return gpio_request(APDS9702_DOUT_GPIO, "apds9702_dout");
	} else {
		gpio_free(APDS9702_DOUT_GPIO);
		return 0;
	}
}

static void apds9702_hw_config(int enable)
{
	return;
}

static struct apds9702_platform_data apds9702_pdata = {
	.gpio_dout      = APDS9702_DOUT_GPIO,
	.is_irq_wakeup  = 1,
	.hw_config      = apds9702_hw_config,
	.gpio_setup     = apds9702_gpio_setup,
	.ctl_reg = {
		.trg   = 1,
		.pwr   = 1,
		.burst = 7,
		.frq   = 3,
		.dur   = 2,
		.th    = 15,
		.rfilt = 0,
	},
	.phys_dev_path = "/sys/devices/i2c-3/3-0054"
};

static int akm897x_gpio_setup(void)
{
	int rc;

	rc = gpio_request(AKM897X_GPIO, GPIO_FUNC_NAME);
	if (rc)
		pr_err("%s: gpio_request failed rc=%d\n", __func__, rc);
	return rc;
}

static void akm897x_gpio_shutdown(void)
{
	gpio_free(AKM897X_GPIO);
}

static void akm897x_hw_config(int enable)
{
	return;
}

#ifdef CONFIG_INPUT_AKM8975
static struct akm8975_platform_data akm8975_platform_data = {
	.setup = akm897x_gpio_setup,
	.shutdown = akm897x_gpio_shutdown,
	.hw_config = akm897x_hw_config,
};
#endif
#ifdef CONFIG_INPUT_AKM8972
static struct akm8972_platform_data akm8972_platform_data = {
	.setup = akm897x_gpio_setup,
	.shutdown = akm897x_gpio_shutdown,
	.hw_config = akm897x_hw_config,
};
#endif

#ifdef CONFIG_SENSORS_MPU3050
int mpu3050_gpio_setup(struct device *dev, int enable)
{

	int rc = 0;

	if (enable) {
		rc = gpio_request(MPU3050_GPIO, "MPUIRQ");
		if (rc)
			pr_err("%s: gpio_request failed. rc=%d\n",
					__func__, rc);
	} else {
		gpio_free(MPU3050_GPIO);
	}

	return rc;
}

void mpu3050_hw_config(int enable)
{
	return;
}
#endif

static struct regulator *vreg_touch_vdd;
static struct regulator *vreg_touch_vddio;

static int clearpad_vreg_low_power_mode(int enable)
{
	int rc = 0;

	if (IS_ERR(vreg_touch_vdd)) {
		pr_err("%s: vreg_touch_vdd is not initialized\n", __func__);
		return -ENODEV;
	}

	if (enable)
		rc = regulator_set_optimum_mode(vreg_touch_vdd, 1000);
	else
		rc = regulator_set_optimum_mode(vreg_touch_vdd, 15000);

	if (rc < 0) {
		pr_err("%s: vdd: set mode (%s) failed, rc=%d\n",
			__func__, (enable ? "LPM" : "HPM"), rc);
		return rc;
	} else {
		pr_debug("%s: vdd: set mode (%s) ok, new mode=%d\n",
				__func__, (enable ? "LPM" : "HPM"), rc);
		return 0;
	}
}

static int clearpad_vreg_configure(int enable)
{
	int rc = 0;

	vreg_touch_vdd = regulator_get(NULL, "8901_l1");
	if (IS_ERR(vreg_touch_vdd)) {
		pr_err("%s: get vdd failed\n", __func__);
		return -ENODEV;
	}
	vreg_touch_vddio = regulator_get(NULL, "8901_lvs2");

	if (enable) {
		rc = regulator_set_voltage(vreg_touch_vdd, 3050000, 3050000);
		if (rc) {
			pr_err("%s: set voltage failed, rc=%d\n", __func__, rc);
			goto clearpad_vreg_configure_err;
		}
		rc = regulator_enable(vreg_touch_vdd);
		if (rc) {
			pr_err("%s: enable vdd failed, rc=%d\n", __func__, rc);
			goto clearpad_vreg_configure_err;
		}
		if (!IS_ERR(vreg_touch_vddio)) {
			rc = regulator_enable(vreg_touch_vddio);
			if (rc) {
				pr_err("%s: enable vddio failed, rc=%d\n",
								__func__, rc);
				goto clearpad_vreg_configure_err;
			}
		}
		rc = clearpad_vreg_low_power_mode(0);
		if (rc) {
			pr_err("%s: set vdd mode failed, rc=%d\n",
				__func__, rc);
			goto clearpad_vreg_configure_err;
		}
	} else {
		rc = regulator_disable(vreg_touch_vdd);
		if (rc)
			pr_err("%s: disable vdd failed, rc=%d\n",
								__func__, rc);
		if (!IS_ERR(vreg_touch_vddio)) {
			rc = regulator_disable(vreg_touch_vddio);
			if (rc)
				pr_err("%s: disable vddio failed, rc=%d\n",
								__func__, rc);
		}
	}
	return rc;
clearpad_vreg_configure_err:
	regulator_put(vreg_touch_vdd);
	regulator_put(vreg_touch_vddio);
	return rc;
}

#define SYNAPTICS_TOUCH_GPIO_IRQ	(127)
static int clearpad_gpio_configure(int enable)
{
	int rc = 0;

	if (enable) {
		rc = gpio_request(SYNAPTICS_TOUCH_GPIO_IRQ, CLEARPAD_NAME);
		if (rc)
			pr_err("%s: gpio_requeset failed, "
					"rc=%d\n", __func__, rc);
	} else {
		gpio_free(SYNAPTICS_TOUCH_GPIO_IRQ);
	}
	return rc;
}

static struct clearpad_platform_data clearpad_platform_data = {
	.irq = MSM_GPIO_TO_INT(SYNAPTICS_TOUCH_GPIO_IRQ),
	.funcarea = clearpad_funcarea_array,
	.vreg_configure = clearpad_vreg_configure,
	.vreg_suspend = clearpad_vreg_low_power_mode,
	.gpio_configure = clearpad_gpio_configure,
};

#ifdef CONFIG_NFC_PN544
int pn544_chip_config(enum pn544_state state, void *not_used)
{
	switch (state) {
	case PN544_STATE_OFF:
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_FWDL_EN - 1), 0);
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1), 0);
		usleep(50000);
		break;
	case PN544_STATE_ON:
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_FWDL_EN - 1), 0);
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1), 1);
		usleep(10000);
		break;
	case PN544_STATE_FWDL:
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_FWDL_EN - 1), 1);
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1), 0);
		usleep(10000);
		gpio_set_value_cansleep(
			PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1), 1);
		break;
	default:
		pr_err("%s: undefined state %d\n", __func__, state);
		return -EINVAL;
	}
	return 0;
}

int pn544_gpio_request(void)
{
	int ret;

	ret = gpio_request(
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_IRQ - 1), "pn544_irq");
	if (ret)
		goto err_irq;
	ret = gpio_request(
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1), "pn544_ven");
	if (ret)
		goto err_ven;
	ret = gpio_request(
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_FWDL_EN - 1), "pn544_fw");
	if (ret)
		goto err_fw;
	return 0;
err_fw:
	gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1));
err_ven:
	gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_IRQ - 1));
err_irq:
	pr_err("%s: gpio request err %d\n", __func__, ret);
	return ret;
}

void pn544_gpio_release(void)
{
	gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_EN - 1));
	gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_IRQ - 1));
	gpio_free(PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_NFC_FWDL_EN - 1));
}
#endif

#ifdef CONFIG_INPUT_LPS331AP
static struct lps331ap_prs_platform_data lps331ap_platform_data = {
	.init                 = NULL,
	.exit                 = NULL,
	.power_on             = NULL,
	.power_off            = NULL,
	.poll_interval        = 200,
	.min_interval         = 40,
};
#endif

static struct i2c_board_info msm_i2c_gsbi3_clearpad_info[] = {
	{
		I2C_BOARD_INFO(CLEARPADI2C_NAME, 0x2C),
		.platform_data = &clearpad_platform_data,
	},
};

#ifdef CONFIG_I2C
static struct i2c_board_info fuji_gsbi8_peripherals_info[] __initdata = {
	{
		I2C_BOARD_INFO("as3676", 0x80 >> 1),
		.platform_data = &as3676_platform_data
	},
        {
                I2C_BOARD_INFO("lm3560", 0xA6 >> 1),
                .platform_data = &lm3560_platform_data,
        },
        {
                I2C_BOARD_INFO(APDS9702_NAME, 0xA8 >> 1),
                .platform_data = &apds9702_pdata,
                .type = APDS9702_NAME,
        },
#ifdef CONFIG_CHARGER_BQ24185
        {
                I2C_BOARD_INFO(BQ24185_NAME, 0xD6 >> 1),
                .irq = MSM_GPIO_TO_INT(BQ24185_GPIO_IRQ),
                .platform_data = &bq24185_platform_data,
                .type = BQ24185_NAME,
        },
#endif
#ifdef CONFIG_CHARGER_BQ24160
        {
                I2C_BOARD_INFO(BQ24160_NAME, 0xD6 >> 1),
                .irq = MSM_GPIO_TO_INT(BQ24160_GPIO_IRQ),
                .platform_data = &bq24160_platform_data,
                .type = BQ24160_NAME,
        },
#endif
        {
                I2C_BOARD_INFO(BQ27520_NAME, 0xAA >> 1),
                .irq = MSM_GPIO_TO_INT(GPIO_BQ27520_SOC_INT),
                .platform_data = &bq27520_platform_data,
                .type = BQ27520_NAME,
        },
#ifdef CONFIG_NFC_PN544
	{
		I2C_BOARD_INFO(PN544_DEVICE_NAME, 0x50 >> 1),
		.irq = PM8058_GPIO_IRQ(PM8058_IRQ_BASE, PMIC_GPIO_NFC_IRQ - 1),
		.platform_data = &pn544_pdata,
	},
#endif
};

static struct i2c_board_info fuji_gsbi12_peripherals_info[] __initdata = {
	{
		I2C_BOARD_INFO("bma250", 0x30 >> 1),
		.irq = MSM_GPIO_TO_INT(BMA250_GPIO),
		.platform_data = &bma250_platform_data,
	},
	{
#ifdef CONFIG_INPUT_AKM8975
		I2C_BOARD_INFO(AKM8975_I2C_NAME, 0x18 >> 1),
		.irq = MSM_GPIO_TO_INT(AKM897X_GPIO),
		.platform_data = &akm8975_platform_data,
#endif
#ifdef CONFIG_INPUT_AKM8972
		I2C_BOARD_INFO(AKM8972_I2C_NAME, 0x18 >> 1),
		.irq = MSM_GPIO_TO_INT(AKM897X_GPIO),
		.platform_data = &akm8972_platform_data,
#endif
	},
#ifdef CONFIG_SENSORS_MPU3050
	{
		I2C_BOARD_INFO(MPU_NAME, 0xD0 >> 1),
		.irq = MSM_GPIO_TO_INT(MPU3050_GPIO),
		.platform_data = &mpu_data,
	},
#endif
#ifdef CONFIG_INPUT_LPS331AP
	{
		I2C_BOARD_INFO(LPS331AP_PRS_DEV_NAME, 0xB8 >> 1),
		.platform_data = &lps331ap_platform_data,
	},
#endif
};

#ifdef CONFIG_INPUT_ASETM2034A
#define ASETM2034A_GPIO_IRQ		42
#define PMIC_GPIO_ASETM2034A_RST	18

static int asetm2034a_gpio_setup(void)
{
	int rc;

	rc = gpio_request(ASETM2034A_GPIO_IRQ, "asetm2034a_irq");
	if (rc)
		pr_err("%s: gpio_request failed rc=%d\n", __func__, rc);

	return rc;
}

static void asetm2034a_gpio_shutdown(void)
{
	gpio_free(ASETM2034A_GPIO_IRQ);
}

static int asetm2034a_hw_reset(void)
{
	int rc;
	struct pm8058_gpio pmic_gpio_18 = {
		.direction = PM_GPIO_DIR_OUT,
		.pull = PM_GPIO_PULL_DN,
		.vin_sel = PM_GPIO_VIN_L5,
		.out_strength = PM_GPIO_STRENGTH_MED,
		.function = PM_GPIO_FUNC_NORMAL,
	};

	pmic_gpio_18.output_value = 0;
	rc = pm8058_gpio_config(PMIC_GPIO_ASETM2034A_RST - 1, &pmic_gpio_18);
	if (rc) {
		pr_err("%s: Config PMIC_GPIO_ASETM2034A_RST failed rc=%d\n",
			__func__, rc);
		goto exit;
	}

	udelay(20);

	pmic_gpio_18.output_value = 1;
	rc = pm8058_gpio_config(PMIC_GPIO_ASETM2034A_RST - 1, &pmic_gpio_18);
	if (rc)
		pr_err("%s: Config PMIC_GPIO_ASETM2034A_RST failed rc=%d\n",
			__func__, rc);

exit:
	return rc;
}

static struct asetm2034a_platform_data asetm2034a_pdata = {
	.gpio_setup = asetm2034a_gpio_setup,
	.gpio_shutdown = asetm2034a_gpio_shutdown,
	.hw_reset = asetm2034a_hw_reset,
	.keymap = &asetm2034a_keymap,
};

static struct i2c_board_info asetm2034a_boardinfo[] __initdata = {
	{
		I2C_BOARD_INFO("asetm2034a", 0x54 >> 1),
		.platform_data = &asetm2034a_pdata,
		.irq = MSM_GPIO_TO_INT(ASETM2034A_GPIO_IRQ),
		.type = ASETM2034A_NAME,
	},
};
#endif /* CONFIG_INPUT_ASETM2034A */

struct i2c_registry {
	int                    bus;
	struct i2c_board_info *info;
	int                    len;
};

static struct i2c_registry msm8x60_i2c_devices[] __initdata = {
	{
		MSM_GSBI3_QUP_I2C_BUS_ID,
		msm_i2c_gsbi3_clearpad_info,
		ARRAY_SIZE(msm_i2c_gsbi3_clearpad_info),
	},
#ifdef CONFIG_INPUT_ASETM2034A
	{
		MSM_GSBI3_QUP_I2C_BUS_ID,
		asetm2034a_boardinfo,
		ARRAY_SIZE(asetm2034a_boardinfo),
	},
#endif /* CONFIG_INPUT_ASETM2034A */
#ifdef CONFIG_MSM_CAMERA
	{
		MSM_GSBI4_QUP_I2C_BUS_ID,
		msm_camera_boardinfo,
		ARRAY_SIZE(msm_camera_boardinfo),
	},
#endif
	{
		MSM_GSBI7_QUP_I2C_BUS_ID,
		msm_i2c_gsbi7_timpani_info,
		ARRAY_SIZE(msm_i2c_gsbi7_timpani_info),
	},
	{
		MSM_GSBI8_QUP_I2C_BUS_ID,
		fuji_gsbi8_peripherals_info,
		ARRAY_SIZE(fuji_gsbi8_peripherals_info)
	},
	{
		MSM_GSBI12_QUP_I2C_BUS_ID,
		fuji_gsbi12_peripherals_info,
		ARRAY_SIZE(fuji_gsbi12_peripherals_info)
	}
};
#endif /* CONFIG_I2C */

static void register_i2c_devices(void)
{
	int i;

	semc_fuji_pmic_register();

	/* Run the array and install devices */
	for (i = 0; i < ARRAY_SIZE(msm8x60_i2c_devices); ++i)
		i2c_register_board_info(msm8x60_i2c_devices[i].bus,
					msm8x60_i2c_devices[i].info,
					msm8x60_i2c_devices[i].len);
}

#define TCSR_ADM_1_A_CRCI_MUX_SEL (MSM_TCSR_BASE + 0x078)
#define ADM1_CRCI_GSBI7_RX_SEL (1<<13)
#define ADM1_CRCI_GSBI7_TX_SEL (1<<12)
#define ADM1_CRCI_GSBI7_MASK (ADM1_CRCI_GSBI7_RX_SEL | \
				ADM1_CRCI_GSBI7_TX_SEL)
static void __init msm8x60_init_uart7dm(void)
{
	uint32_t tmp;

	/* Read the Current Register Value from TCSR_ADM_1 */
	tmp = secure_readl(TCSR_ADM_1_A_CRCI_MUX_SEL);
	/* Clear the bits for GSBI7 */
	tmp &= ~(ADM1_CRCI_GSBI7_MASK);
	/* Set GSBI7 for UART TX and RX */
	tmp |= (ADM1_CRCI_GSBI7_RX_SEL | ADM1_CRCI_GSBI7_TX_SEL);
	/* Write the Data */
	secure_writel(tmp, TCSR_ADM_1_A_CRCI_MUX_SEL);
}

static void __init msm8x60_init_uart12dm(void)
{
#ifndef CONFIG_USB_PEHCI_HCD
	/* 0x1D000000 now belongs to EBI2:CS3 i.e. USB ISP Controller */
	void *fpga_mem = ioremap_nocache(0x1D000000, SZ_4K);
	/* Advanced mode */
	writew(0xFFFF, fpga_mem + 0x15C);
	/* FPGA_UART_SEL */
	writew(0, fpga_mem + 0x172);
	/* FPGA_GPIO_CONFIG_117 */
	writew(1, fpga_mem + 0xEA);
	/* FPGA_GPIO_CONFIG_118 */
	writew(1, fpga_mem + 0xEC);
	dsb();
	iounmap(fpga_mem);
#endif
}

static void __init msm8x60_init_buses(void)
{
#if defined(CONFIG_USB_F_SERIAL_SDIO) || defined(CONFIG_USB_F_SERIAL_SMD)
	struct usb_gadget_fserial_platform_data *fserial_pdata = NULL;
#endif
#ifdef CONFIG_I2C_QUP
	void *gsbi_mem = ioremap_nocache(0x19C00000, 4);
	void *gsbi5_mem = ioremap_nocache(0x16400000, 4);
	void *gsbi7_mem = ioremap_nocache(0x16600000, 4);
	/* Setting protocol code to 0x60 for dual UART/I2C in GSBI12 */
	writel_relaxed(0x6 << 4, gsbi_mem);
	/* Ensure protocol code is written before proceeding further */
	dsb();
	iounmap(gsbi_mem);
	/* Setting protocol code to 0x60 for dual UART/I2C in GSBI5 */
	writel(0x6 << 4, gsbi5_mem);
	iounmap(gsbi5_mem);
	/* Setting protocol code to 0x60 for dual UART/I2C in GSBI7 */
	writel(0x6 << 4, gsbi7_mem);
	iounmap(gsbi7_mem);

	msm_gsbi3_qup_i2c_device.dev.platform_data = &msm_gsbi3_qup_i2c_pdata;
	msm_gsbi4_qup_i2c_device.dev.platform_data = &msm_gsbi4_qup_i2c_pdata;
#ifdef CONFIG_MSM_GSBI5_I2C
	msm_gsbi5_qup_i2c_device.dev.platform_data = &msm_gsbi5_qup_i2c_pdata;
#endif
	msm_gsbi7_qup_i2c_device.dev.platform_data = &msm_gsbi7_qup_i2c_pdata;
	msm_gsbi8_qup_i2c_device.dev.platform_data = &msm_gsbi8_qup_i2c_pdata;
	msm_gsbi12_qup_i2c_device.dev.platform_data = &msm_gsbi12_qup_i2c_pdata;

#ifdef CONFIG_MSM_GSBI9_UART
	if (machine_is_msm8x60_charm_surf() ||
		machine_is_msm8x60_charm_ffa())
		msm_gsbi9_qup_i2c_pdata.use_gsbi_shared_mode = 1;
#endif
	msm_gsbi9_qup_i2c_device.dev.platform_data = &msm_gsbi9_qup_i2c_pdata;
	msm_gsbi12_qup_i2c_device.dev.platform_data = &msm_gsbi12_qup_i2c_pdata;
#endif
#if defined(CONFIG_SPI_QUP) || defined(CONFIG_SPI_QUP_MODULE)
	msm_gsbi1_qup_spi_device.dev.platform_data = &msm_gsbi1_qup_spi_pdata;
#endif
#ifdef CONFIG_I2C_SSBI
	msm_device_ssbi1.dev.platform_data = &msm_ssbi1_pdata;
	msm_device_ssbi2.dev.platform_data = &msm_ssbi2_pdata;
	msm_device_ssbi3.dev.platform_data = &msm_ssbi3_pdata;
#endif
	msm_gsbi10_qup_spi_device.dev.platform_data =
					&msm_gsbi10_qup_spi_pdata;
#if defined(CONFIG_USB_GADGET_MSM_72K) || defined(CONFIG_USB_EHCI_HCD)
	msm_device_otg.dev.platform_data = &semc_fuji_otg_pdata;
#endif

#ifdef CONFIG_USB_GADGET_MSM_72K
	msm_device_gadget_peripheral.dev.platform_data = &semc_fuji_gadget_pdata;
#endif
#if defined(CONFIG_USB_F_SERIAL_SDIO) || defined(CONFIG_USB_F_SERIAL_SMD)
	fserial_pdata = usb_gadget_fserial_device.dev.platform_data;
	/* for charm: Port1: SDIO - DUN,  Port2: TTY - NMEA
	* for svlte-2: Port1: SDIO - DUN1,  Port2: SDIO - DUN2
	*/
		fserial_pdata->transport[0] =
			USB_GADGET_FSERIAL_TRANSPORT_SDIO;
		fserial_pdata->transport[1] =
			USB_GADGET_FSERIAL_TRANSPORT_SMD;
#endif
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#ifdef CONFIG_MSM_GSBI5_UART
	msm_device_uart_gsbi5.dev.platform_data = &msm_uart_gsbi5_pdata;
#endif
#ifdef CONFIG_MSM_GSBI7_UART
	msm_device_uart_dm7.dev.platform_data = &msm_uart_dm7_pdata;
#endif
#ifdef CONFIG_MSM_GSBI9_UART
	if (machine_is_msm8x60_charm_surf() ||
		machine_is_msm8x60_charm_ffa()) {
		msm_device_uart_gsbi9 = msm_add_gsbi9_uart();
		if (IS_ERR(msm_device_uart_gsbi9))
			pr_err("%s(): Failed to create uart gsbi9 device\n",
				__func__);
	}
#endif

#ifdef CONFIG_MSM_BUS_SCALING

	/* RPM calls are only enabled on V2 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) == 2) {
		msm_bus_apps_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fabric_pdata.rpm_enabled = 1;
		msm_bus_mm_fabric_pdata.rpm_enabled = 1;
		msm_bus_sys_fpb_pdata.rpm_enabled = 1;
		msm_bus_cpss_fpb_pdata.rpm_enabled = 1;
	}

	msm_bus_apps_fabric.dev.platform_data = &msm_bus_apps_fabric_pdata;
	msm_bus_sys_fabric.dev.platform_data = &msm_bus_sys_fabric_pdata;
	msm_bus_mm_fabric.dev.platform_data = &msm_bus_mm_fabric_pdata;
	msm_bus_sys_fpb.dev.platform_data = &msm_bus_sys_fpb_pdata;
	msm_bus_cpss_fpb.dev.platform_data = &msm_bus_cpss_fpb_pdata;
#endif
}

static void __init msm8x60_map_io(void)
{
	msm_shared_ram_phys = MSM_SHARED_RAM_PHYS;
	msm_map_msm8x60_io();
	msm8x60_allocate_memory_regions();
}

/*
 * Most segments of the EBI2 bus are disabled by default.
 */
static void __init msm8x60_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void *ebi2_cfg_ptr;

	ebi2_cfg_ptr = ioremap_nocache(0x1a100000, sizeof(uint32_t));
	if (ebi2_cfg_ptr != 0) {
		ebi2_cfg = readl(ebi2_cfg_ptr);

		if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
			machine_is_msm8x60_fluid())
				ebi2_cfg |= (1 << 4) | (1 << 5); /* CS2, CS3 */
		else if (machine_is_msm8x60_sim())
			ebi2_cfg |= (1 << 4); /* CS2 */
		else if (machine_is_msm8x60_rumi3())
			ebi2_cfg |= (1 << 5); /* CS3 */

		writel(ebi2_cfg, ebi2_cfg_ptr);
		iounmap(ebi2_cfg_ptr);
	}

	if (machine_is_msm8x60_surf() || machine_is_msm8x60_ffa() ||
	    machine_is_msm8x60_fluid()) {
		ebi2_cfg_ptr = ioremap_nocache(0x1a110000, SZ_4K);
		if (ebi2_cfg_ptr != 0) {
			/* EBI2_XMEM_CFG:PWRSAVE_MODE off */
			writel(0UL, ebi2_cfg_ptr);

			/* CS2: Delay 9 cycles (140ns@64MHz) between SMSC
			 * LAN9221 Ethernet controller reads and writes.
			 * The lowest 4 bits are the read delay, the next
			 * 4 are the write delay. */
			writel(0x031F1C99, ebi2_cfg_ptr + 0x10);
#ifdef CONFIG_USB_PEHCI_HCD
			/*
			 * RECOVERY=5, HOLD_WR=1
			 * INIT_LATENCY_WR=1, INIT_LATENCY_RD=1
			 * WAIT_WR=1, WAIT_RD=2
			 */
			writel(0x51010112, ebi2_cfg_ptr + 0x14);
			/*
			 * HOLD_RD=1
			 * ADV_OE_RECOVERY=0, ADDR_HOLD_ENA=1
			 */
			writel(0x01000020, ebi2_cfg_ptr + 0x34);
#else
			/* EBI2 CS3 muxed address/data,
			* two cyc addr enable */
			writel(0xA3030020, ebi2_cfg_ptr + 0x34);

#endif
			iounmap(ebi2_cfg_ptr);
		}
	}
}

static void __init msm8x60_init_tlmm(void)
{
	msm_gpio_install_direct_irq(PM8058_GPIO_INT, 1, 0);

	msm_gpio_install_direct_irq(PM8901_GPIO_INT, 2, 0);

}

#ifdef CONFIG_MSM_RPM
static struct msm_rpm_platform_data msm_rpm_data = {
	.reg_base_addrs = {
		[MSM_RPM_PAGE_STATUS] = MSM_RPM_BASE,
		[MSM_RPM_PAGE_CTRL] = MSM_RPM_BASE + 0x400,
		[MSM_RPM_PAGE_REQ] = MSM_RPM_BASE + 0x600,
		[MSM_RPM_PAGE_ACK] = MSM_RPM_BASE + 0xa00,
	},

	.irq_ack = RPM_SCSS_CPU0_GP_HIGH_IRQ,
	.irq_err = RPM_SCSS_CPU0_GP_LOW_IRQ,
	.irq_vmpm = RPM_SCSS_CPU0_GP_MEDIUM_IRQ,
	.msm_apps_ipc_rpm_reg = MSM_GCC_BASE + 0x008,
	.msm_apps_ipc_rpm_val = 4,
};
#endif

#define USB_HUB_EN_GPIO	138

void msm_fusion_setup_pinctrl(void)
{
    struct msm_xo_voter *a1;

    if (socinfo_get_platform_subtype() == 0x3) {
	/*
	 * Vote for the A1 clock to be in pin control mode before
	 * the external images are loaded.
	 */
	a1 = msm_xo_get(MSM_XO_TCXO_A1, "mdm");
	BUG_ON(!a1);
	msm_xo_mode_vote(a1, MSM_XO_MODE_PIN_CTRL);
    }
}

#define PMIC_GPIO_VOLT_DET_CTRL	40

static int enable_voltage_detection
	(struct notifier_block *this, unsigned long code, void *_cmd)
{
	gpio_set_value_cansleep(
		PM8058_GPIO_PM_TO_SYS(PMIC_GPIO_VOLT_DET_CTRL - 1), 0);

	return NOTIFY_DONE;
}

static struct notifier_block voltage_detection_notifier = {
	.notifier_call = enable_voltage_detection,
};

static void __init msm8x60_init(void)
{

	pmic_reset_irq = PM8058_RESOUT_IRQ(PM8058_IRQ_BASE);

	/*
	 * Initialize RPM first as other drivers and devices may need
	 * it for their initialization.
	 */
#ifdef CONFIG_MSM_RPM
	BUG_ON(msm_rpm_init(&msm_rpm_data));
#endif
	if (msm_xo_init())
		pr_err("Failed to initialize XO votes\n");

	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);
	msm8x60_check_2d_hardware();

	/*
	 * Initialize SPM before acpuclock as the latter calls into SPM
	 * driver to set ACPU voltages.
	 */
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
		msm_spm_init(msm_spm_data, ARRAY_SIZE(msm_spm_data));
	else
		msm_spm_init(msm_spm_data_v1, ARRAY_SIZE(msm_spm_data_v1));

	/*
	 * Disable regulator info printing so that regulator registration
	 * messages do not enter the kmsg log.
	 */
	//regulator_suppress_info_printing();

	/* Initialize regulators needed for clock_init. */
	platform_add_devices(early_regulators, ARRAY_SIZE(early_regulators));

	msm_clock_init(msm_clocks_8x60, msm_num_clocks_8x60);

	/* Buses need to be initialized before early-device registration
	 * to get the platform data for fabrics.
	 */
	msm8x60_init_buses();
	platform_add_devices(early_devices, ARRAY_SIZE(early_devices));
	msm_acpu_clock_init(&msm8x60_acpu_clock_data);

	msm8x60_init_ebi2();
	msm8x60_init_tlmm();
	msm8x60_init_gpiomux(semc_fuji_gpiomux_cfgs);
	hw_id_class_init();
	msm8x60_init_uart7dm();
	msm8x60_init_uart12dm();
	semc_fuji_init_mmc();
#ifdef CONFIG_SEMC_CHARGER_USB_ARCH
	semc_chg_usb_set_supplicants(semc_chg_usb_supplied_to,
				  ARRAY_SIZE(semc_chg_usb_supplied_to));
#endif
	if (SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1)
		platform_add_devices(msm_footswitch_devices,
				     msm_num_footswitch_devices);
	platform_add_devices(fuji_devices,
			     ARRAY_SIZE(fuji_devices));
#ifdef CONFIG_FB_MSM_MIPI_DSI_RENESAS_R63306
	semc_fuji_add_lcd_device();
#endif
#ifdef CONFIG_USB_EHCI_MSM_72K
	msm_add_host(0, &semc_fuji_usb_host_pdata);
#endif

	semc_fuji_fb_add_devices();
	register_i2c_devices();

	msm_pm_set_platform_data(msm_pm_data, ARRAY_SIZE(msm_pm_data));
	msm_cpuidle_set_states(msm_cstates, ARRAY_SIZE(msm_cstates),
				msm_pm_data);

#ifdef CONFIG_MSM8X60_AUDIO
	msm_snddev_init();
#endif

	semc_fuji_multi_sdio_init();

	if (machine_is_msm8x60_charm_surf() ||
	    machine_is_msm8x60_charm_ffa())
		msm_fusion_setup_pinctrl();

	bt_power_init();
	shared_vreg_init();

	register_reboot_notifier(&voltage_detection_notifier);
	register_reboot_notifier(&vreg_poweroff_notifier);
}

static void __init semc_fuji_fixup(struct machine_desc *desc, struct tag *tags,
				 char **cmdline, struct meminfo *mi)
{
#define MSM_BANK0_BASE			PHYS_OFFSET
#define MSM_BANK0_SIZE			0x02C00000

#define MSM_BANK1_BASE			0x48000000
#define MSM_BANK1_SIZE			0x18000000

#define MSM_BANK2_BASE			0x60000000
#define MSM_BANK2_SIZE			0x1FF00000

	mi->nr_banks = 3;
	mi->bank[0].start = MSM_BANK0_BASE;
	mi->bank[0].node = PHYS_TO_NID(MSM_BANK0_BASE);
	mi->bank[0].size = MSM_BANK0_SIZE;

	mi->bank[1].start = MSM_BANK1_BASE;
	mi->bank[1].node = PHYS_TO_NID(mi->bank[1].start);
	mi->bank[1].size = MSM_BANK1_SIZE;

	mi->bank[2].start = MSM_BANK2_BASE;
	mi->bank[2].size = MSM_BANK2_SIZE;
	mi->bank[2].node = PHYS_TO_NID(mi->bank[2].start);
}

MACHINE_START(SEMC_FUJI, "fuji")
#ifdef CONFIG_MSM_DEBUG_UART
	.phys_io  = MSM_DEBUG_UART_PHYS,
	.io_pg_offst = ((MSM_DEBUG_UART_BASE) >> 18) & 0xfffc,
#endif
	.fixup = semc_fuji_fixup,
	.map_io = msm8x60_map_io,
	.init_irq = msm8x60_init_irq,
	.init_machine = msm8x60_init,
	.timer = &msm_timer,
MACHINE_END
