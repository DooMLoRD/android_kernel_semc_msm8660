/* arch/arm/mach-msm/gpiomux-fuji_csfb_cdb.c
 *
 * Copyright (C) [2011] Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include "gpiomux.h"
#include "gpiomux-fuji_pm8058.h"

static struct gpiomux_setting unused_gpio = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting gpio_2ma_no_pull_low = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting gpio_2ma_no_pull_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_2ma_pull_up_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_2ma_pull_down_low = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting gpio_2ma_pull_down_in = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting uart = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting uart1dm_active = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting uart1dm_suspended = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sensor_i2c = {
	.func = GPIOMUX_FUNC_2,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting generic_i2c = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cam_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ps_hold = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hdmi_ddc = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting fm_i2s = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting audio_pcm = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting uart9dm_active = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA ,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gsbi9 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc1_dat_0_3_cmd_actv_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_10MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc1_dat_4_7_cmd_actv_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_10MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc1_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc2_dat_0_3_cmd_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_10MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc2_dat_4_7_cmd_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_10MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc2_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc2_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc5_dat_0_3_cmd_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_10MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc5_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc5_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ap2mdm_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_vfr_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting mdm2ap_vfr_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_sync_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_sync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ap2mdm_pmic_reset_n_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting ap2mdm_kpdpwr_n_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting mdm2ap_vddmin_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_vddmin_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};


static struct msm_gpiomux_config semc_fuji_all_cfgs[] __initdata = {
	{ /* ETM_TRACECLK_A */
		.gpio = 0,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ETM_TRACECTL_A */
		.gpio = 1,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* EX_CHARGER_DISABLE */
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* ETM_TRACEDATA_A1 */
		.gpio = 3,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ETM_TRACEDATA_A2 */
		.gpio = 4,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ETM_TRACEDATA_A3 */
		.gpio = 5,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ETM_TRACEDATA_A4 */
		.gpio = 6,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ETM_TRACEDATA_A5 */
		.gpio = 7,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* FLASH_TRG */
		.gpio = 8,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_down_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* ETM_TRACEDATA_A7 */
		.gpio = 9,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* ETM_TRACEDATA_A8 */
		.gpio = 10,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* HW_ID0 */
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* HW_ID2 */
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* MHL_EN */
		.gpio = 13,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MHL_RESET_N */
		.gpio = 14,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* HW_ID1 */
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* LOUDSPKR_AMP_R_EN */
		.gpio = 16,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* HEADSET_DET_OUT */
		.gpio = 17,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* LCD_PWR_EN */
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 19,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* LOUDSPKR_AMP_L_EN */
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* GYRO_FSYNC */
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 22,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* BT_RST_N */
		.gpio = 23,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 24,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* CHAT_CAM_RST_N */
		.gpio = 25,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* SW_SERVICE */
		.gpio = 26,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* DTV_RESET_N */
		.gpio = 27,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* USB_OTG_EN */
		.gpio = 28,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* MDM2AP_VFR */
		.gpio = 29,
		.settings = {
			[GPIOMUX_ACTIVE] = &mdm2ap_vfr_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdm2ap_vfr_suspend_cfg,
		}
	},
	{ /* GYRO_INT */
		.gpio = 30,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* COMPASS_IRQ */
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* CAMIF_MCLK */
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE] = &cam_mclk,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 33,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* PROX_INT */
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* Peripheral2nd_I2C_SDA */
		.gpio = 35,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* Peripheral2nd_I2C_SCL */
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* UIM1_M_DATA */
		.gpio = 37,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* UIM1_M_CLK */
		.gpio = 38,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* PWR_ATT_EN */
		.gpio = 39,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MDM2AP_WAKEUP */
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	{ /* ACCEL_IRQ */
		.gpio = 41,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* SUBTP_INT */
		.gpio = 42,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* TP_I2C_DATA */
		.gpio = 43,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* TP_I2C_CLK */
		.gpio = 44,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* FELICA_UART_TX/NFC_EN */
		.gpio = 45,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* FELICA_UART_RX/NFC_IRQ */
		.gpio = 46,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* CAMIF_I2C_DATA */
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* CAMIF_I2C_CLK */
		.gpio = 48,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* IrDA_TX/UICC_DP */
		.gpio = 49,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* IrDA_RX/UICC_DN */
		.gpio = 50,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* DTV_I2C_DATA */
		.gpio = 51,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* DTV_I2C_SCL */
		.gpio = 52,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* UARTDM_BT_TX */
		.gpio = 53,
		.settings = {
			[GPIOMUX_ACTIVE] = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ /* UARTDM_BT_RX */
		.gpio = 54,
		.settings = {
			[GPIOMUX_ACTIVE] = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ /* UARTDM_BT_CTS_N */
		.gpio = 55,
		.settings = {
			[GPIOMUX_ACTIVE] = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ /* UARTDM_BT_RFR_N */
		.gpio = 56,
		.settings = {
			[GPIOMUX_ACTIVE] = &uart1dm_active,
			[GPIOMUX_SUSPENDED] = &uart1dm_suspended,
		},
	},
	{ /* NC */
		.gpio = 57,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MHL_INT */
		.gpio = 58,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* AUD_I2C_DATA */
		.gpio = 59,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* AUD_I2C_CLK */
		.gpio = 60,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* HEADSET_DET_N */
		.gpio = 61,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* TUNER_PWR_EN */
		.gpio = 62,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* BT_WAKES_MSM */
		.gpio = 63,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_down_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* PERIPHERAL_I2C_DATA */
		.gpio = 64,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* PERIPHERAL_I2C_CLK */
		.gpio = 65,
		.settings = {
			[GPIOMUX_ACTIVE] = &generic_i2c,
			[GPIOMUX_SUSPENDED] = &generic_i2c,
		},
	},
	{ /* AP2MDM_UART_TX_DATA */
		.gpio = 66,
		.settings = {
			[GPIOMUX_ACTIVE]    = &uart9dm_active,
			[GPIOMUX_SUSPENDED] = &gsbi9,
		},
	},
	{ /* AP2MDM_UART_RX_DATA */
		.gpio = 67,
		.settings = {
			[GPIOMUX_ACTIVE]    = &uart9dm_active,
			[GPIOMUX_SUSPENDED] = &gsbi9,
		},
	},
	{ /* AP2MDM_UART_CTS_N */
		.gpio = 68,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* AP2MDM_UART_RFR_N */
		.gpio = 69,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MLCD_RESET_N */
		.gpio = 70,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 71,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 72,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 73,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* BOOT_FROM_ROM */
		.gpio = 74,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* WDOC_DISABLE */
		.gpio = 75,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* BOOT_CONFIG_6 */
		.gpio = 76,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* NC */
		.gpio = 77,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* BOOT_CONFIG_5 */
		.gpio = 78,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* BOOT_CONFIG_4 */
		.gpio = 79,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* BOOT_CONFIG_3 */
		.gpio = 80,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* BOOT_CONFIG_0 */
		.gpio = 81,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* BOOT_CONFIG_2 */
		.gpio = 82,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 83,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* BOOT_CONFIG_1 */
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* NC */
		.gpio = 85,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* HW_ID3 */
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* PM8058_APC_SEC_IRQ_N */
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* PM8058_APC_USR_IRQ_N */
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* PM8058_MDM_IRQ_N */
		.gpio = 89,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* PM8901_APC_SEC_IRQ_N */
		.gpio = 90,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* PM8901_APC_USR_IRQ_N */
		.gpio = 91,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	{ /* PS_HOLD */
		.gpio = 92,
		.settings = {
			[GPIOMUX_ACTIVE] = &ps_hold,
			[GPIOMUX_SUSPENDED] = &ps_hold,
		},
	},
	{ /* AP2MDM_ERRFATAL */
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	{ /* SIM_CARD_DETECT */
		.gpio = 94,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_in,
		},
	},
	/* SDCC5 cmd */
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc5_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc5_suspend_config,
		},
	},
	/* SDCC5 data[3]*/
	{
		.gpio = 96,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc5_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc5_suspend_config,
		},
	},
	/* SDCC5 clk */
	{
		.gpio = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc5_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc5_suspend_config,
		},
	},
	/* SDCC5 data[2]*/
	{
		.gpio = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc5_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc5_suspend_config,
		},
	},
	/* SDCC5 data[1]*/
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc5_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc5_suspend_config,
		},
	},
	/* SDCC5 data[0]*/
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc5_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc5_suspend_config,
		},
	},
	{ /* FM_I2S_WS */
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE] = &fm_i2s,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* FM_I2S_SCK */
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE] = &fm_i2s,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* IrDA_PWRDWN */
		.gpio = 103,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* OTG_OVERCUR_INT  */
		.gpio = 104,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* NC */
		.gpio = 105,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* CAM_RST_N */
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* FM_I2S_SD */
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE] = &fm_i2s,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* CDC_TX_MCLK */
		.gpio = 108,
		.settings = {
			[GPIOMUX_ACTIVE] = &cdc_mclk,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_low,
		},
	},
	{ /* CDC_RX_MCLK1 */
		.gpio = 109,
		.settings = {
			[GPIOMUX_ACTIVE] = &cdc_mclk,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_low,
		},
	},
	{ /* NC */
		.gpio = 110,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* AUDIO_PCM_DOUT */
		.gpio = 111,
		.settings = {
			[GPIOMUX_ACTIVE] = &audio_pcm,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* AUDIO_PCM_DIN */
		.gpio = 112,
		.settings = {
			[GPIOMUX_ACTIVE] = &audio_pcm,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* AUDIO_PCM_SYNC */
		.gpio = 113,
		.settings = {
			[GPIOMUX_ACTIVE] = &audio_pcm,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* AUDIO_PCM_CLK */
		.gpio = 114,
		.settings = {
			[GPIOMUX_ACTIVE] = &audio_pcm,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* SENSOR_I2C_CLK */
		.gpio = 115,
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_i2c,
			[GPIOMUX_SUSPENDED] = &sensor_i2c,
		},
	},
	{ /* SENSOR_I2C_DATA */
		.gpio = 116,
		.settings = {
			[GPIOMUX_ACTIVE] = &sensor_i2c,
			[GPIOMUX_SUSPENDED] = &sensor_i2c,
		},
	},
	{ /* UART_RX */
		.gpio = 117,
		.settings = {
			[GPIOMUX_ACTIVE] = &uart,
			[GPIOMUX_SUSPENDED] = &uart,
		},
	},
	{ /* UART_TX */
		.gpio = 118,
		.settings = {
			[GPIOMUX_ACTIVE] = &uart,
			[GPIOMUX_SUSPENDED] = &uart,
		},
	},
	{ /* NC */
		.gpio = 119,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 120,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 121,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 122,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* FG_SOC_INT */
		.gpio = 123,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* NC */
		.gpio = 124,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* EX_CHARGER_INT_N */
		.gpio = 125,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* NC */
		.gpio = 126,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* TP_INT */
		.gpio = 127,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_pull_up_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_up_in,
		},
	},
	{ /* WL_HOST_WAKEUP */
		.gpio = 128,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_in,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_pull_down_in,
		},
	},
	{ /* SDIO_DETECTION */
		.gpio = 129,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdm2ap_sync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdm2ap_sync_suspend_cfg,
		},
	},
	{ /* WL_RST_N */
		.gpio = 130,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* AP2MDM_PMIC_RESET_N */
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_pmic_reset_n_cfg,
		}
	},
	{ /* AP2MDM_KPDPWR_N */
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
	{ /* MDM2AP_ERRFATAL */
		.gpio = 133,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	{ /* MDM2AP_STATUS */
		.gpio = 134,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	{ /* AP2MDM_WAKEUP */
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	{ /* AP2MDM_STATUS */
		.gpio = 136,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_status_cfg,
		}
	},
	{ /* MSM_WAKES_BT */
		.gpio = 137,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* FLASH_DR_RST */
		.gpio = 138,
		.settings = {
			[GPIOMUX_ACTIVE] = &gpio_2ma_no_pull_low,
			[GPIOMUX_SUSPENDED] = &gpio_2ma_no_pull_low,
		},
	},
	{ /* NC */
		.gpio = 139,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* MDM2AP_VDDMIN */
		.gpio = 140,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdm2ap_vddmin_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdm2ap_vddmin_suspend_cfg,
		},
	},
	{ /* NC */
		.gpio = 141,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 142,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	/* SDCC2 data[0] */
	{
		.gpio = 143,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[1] */
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[2] */
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[3] */
	{
		.gpio = 146,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[4] */
	{
		.gpio = 147,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[5] */
	{
		.gpio = 148,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[6] */
	{
		.gpio = 149,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 data[7] */
	{
		.gpio = 150,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	/* SDCC2 CMD */
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},

	/* SDCC2 CLK */
	{
		.gpio = 152,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_config,
		},
	},
	{ /* NC */
		.gpio = 153,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 154,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 155,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 156,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* NC */
		.gpio = 157,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* HPH_MUTE_EN */
		.gpio = 158,
		.settings = { [GPIOMUX_SUSPENDED] = &unused_gpio, },
	},
	{ /* SDC1_DATA0 */
		.gpio = 159,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_0_3_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA1 */
		.gpio = 160,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_0_3_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA2 */
		.gpio = 161,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_0_3_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA3 */
		.gpio = 162,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_0_3_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA4 */
		.gpio = 163,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_4_7_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA5 */
		.gpio = 164,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_4_7_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA6 */
		.gpio = 165,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_4_7_cmd_actv_cfg,
		},
	},
	{ /* SDC1_DATA7 */
		.gpio = 166,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_4_7_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_4_7_cmd_actv_cfg,
		},
	},
	{ /* SDC1_CLK */
		.gpio = 167,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_clk_actv_cfg,
		},
	},
	{ /* SDC1_CMD */
		.gpio = 168,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc1_dat_0_3_cmd_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc1_dat_0_3_cmd_actv_cfg,
		},
	},
	{ /* HDMI_CEC */
		.gpio = 169,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi,
			[GPIOMUX_SUSPENDED] = &hdmi,
		},
	},
	{ /* HDMI_DDC_CLOCK */
		.gpio = 170,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi_ddc,
			[GPIOMUX_SUSPENDED] = &hdmi,
		},
	},
	{ /* HDMI_DDC_DATA */
		.gpio = 171,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi_ddc,
			[GPIOMUX_SUSPENDED] = &hdmi,
		},
	},
	{ /* HDMI_HOT_PLUG_DETECT */
		.gpio = 172,
		.settings = {
			[GPIOMUX_ACTIVE] = &hdmi,
			[GPIOMUX_SUSPENDED] = &hdmi,
		},
	},
};

struct msm_gpiomux_configs
semc_fuji_gpiomux_cfgs[] __initdata = {
	{semc_fuji_all_cfgs, ARRAY_SIZE(semc_fuji_all_cfgs)},
	{NULL, 0},
};

static int pm8058_unused_gpio[0];
struct pmic8058_unused_gpio pmic8058_unused_gpios = {
	.unused_gpio = pm8058_unused_gpio,
	.unused_gpio_num = ARRAY_SIZE(pm8058_unused_gpio),
};
