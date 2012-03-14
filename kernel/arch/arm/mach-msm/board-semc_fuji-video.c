/* arch/arm/mach-msm/board-semc_fuji-video.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
#include <linux/platform_device.h>
#include <mach/board.h>
#include <linux/gpio.h>
#include <linux/mfd/pmic8058.h>
#include <linux/regulator/pmic8058-regulator.h>
#include <linux/regulator/pmic8901-regulator.h>
#include <mach/msm_bus_board.h>
#ifdef CONFIG_FB_MSM_MIPI_DSI_RENESAS_R63306
#include <mach/mipi_dsi_renesas.h>
#endif /* CONFIG_FB_MSM_MIPI_DSI_RENESAS_R63306 */
#ifdef CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0
#include <mach/mipi_dsi_samsung.h>
#endif /* CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0 */
#include "devices.h"
#include "board-semc_fuji-video.h"

#if defined(CONFIG_FB_MSM_HDMI_MSM_PANEL) || defined(CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL)
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_bvs_power(int on);
static int mipi_dsi_power(int on);
static int lcd_reset(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_bvs_power,
};

struct platform_device semc_fuji_hdmi_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL || CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL */

struct resource semc_fuji_fb_resources[] = {
	{
		.flags  = IORESOURCE_DMA,
	}
};

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = NULL,
};

struct platform_device semc_fuji_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(semc_fuji_fb_resources),
	.resource          = semc_fuji_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

#if defined(CONFIG_FB_MSM_HDMI_MSM_PANEL) || defined(CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL)
#define _GET_REGULATOR(var, name) do {				\
	var = regulator_get(NULL, name);			\
	if (IS_ERR(var)) {					\
		pr_err("'%s' regulator not found, rc=%ld\n",	\
			name, IS_ERR(var));			\
		var = NULL;					\
		return -ENODEV;					\
	}							\
} while (0)

static int writeback_offset(void)
{
	return MSM_FB_WRITEBACK_OFFSET;
}

static int hdmi_enable_5v(int on)
{
	static struct regulator *reg_8901_hdmi_mvs;	/* HDMI_5V */
	static struct regulator *reg_8901_mpp0;		/* External 5V */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8901_hdmi_mvs)
		_GET_REGULATOR(reg_8901_hdmi_mvs, "8901_hdmi_mvs");
	if (!reg_8901_mpp0)
		_GET_REGULATOR(reg_8901_mpp0, "8901_mpp0");

	if (on) {
		rc = regulator_enable(reg_8901_mpp0);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"reg_8901_mpp0", rc);
			return rc;
		}
		rc = regulator_enable(reg_8901_hdmi_mvs);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8901_hdmi_mvs", rc);
			return rc;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8901_hdmi_mvs);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8901_hdmi_mvs", rc);
		rc = regulator_disable(reg_8901_mpp0);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"reg_8901_mpp0", rc);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8058_l16;		/* VDD_HDMI */
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!reg_8058_l16)
		_GET_REGULATOR(reg_8058_l16, "8058_l16");

	if (on) {
		rc = regulator_set_voltage(reg_8058_l16, 1800000, 1800000);
		if (!rc)
			rc = regulator_enable(reg_8058_l16);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"8058_l16", rc);
			return rc;
		}
		rc = gpio_request(170, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 170, rc);
			goto error1;
		}
		rc = gpio_request(171, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 171, rc);
			goto error2;
		}
		rc = gpio_request(172, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 172, rc);
			goto error3;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_free(170);
		gpio_free(171);
		gpio_free(172);
		rc = regulator_disable(reg_8058_l16);
		if (rc)
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"8058_l16", rc);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;

error3:
	gpio_free(171);
error2:
	gpio_free(170);
error1:
	regulator_disable(reg_8058_l16);
	return rc;
}

static int hdmi_bvs_power(int on)
{
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(169, "HDMI_CEC_VAR");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_CEC_VAR", 169, rc);
			goto error;
		}
		pr_info("%s(on): success\n", __func__);
	} else {
		gpio_free(169);
		pr_info("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
error:
	return rc;
}

#undef _GET_REGULATOR

#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL || CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL */

#ifdef CONFIG_FB_MSM_MIPI_DSI_RENESAS_R63306
#define LCD_PWR_EN 18
#define MLCD_RESET_N 70
#ifdef CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0
/*
 * semc_fuji_is_dric_s6d6aa0() determine if display DrIC is S6D6AA0 by
 * checking value of MSM GPIO 119.
 * If it's 1, DrIC is S6D6AA0, otherwise R63306.
 * NOTE:
 * If DrIC is S6D6AA0, return 1.
 * If DrIc is NOT S6D6AA0, return 0.
 * If NO DrIC is detected with any error, return -ENODEV.
 */
#define LCD_VENDOR_DET 119
static int semc_fuji_is_dric_s6d6aa0(void)
{
	int rc = 0;
	static int val = -ENODEV;
	struct regulator *vreg_lcd_vddio;

	if (val != -ENODEV)
		return val;

	vreg_lcd_vddio = regulator_get(NULL, "8901_lvs1");
	if (IS_ERR(vreg_lcd_vddio)) {
		pr_err("%s: Unable to get 8901_lvs1\n", __func__);
		return -ENODEV;
	}

	rc = regulator_enable(vreg_lcd_vddio);
	if (rc) {
		pr_err("%s: Enable regulator 8901_lvs1 failed\n",
		       __func__);
		goto release_regulator;
	}

	msleep(50);

	/* LCD_VENDOR_DET */
	rc = gpio_request(LCD_VENDOR_DET, "lcd vendor detect");
	if (rc) {
		pr_err("%s: GPIO %d: request failed. rc=%d\n",
		       __func__, LCD_VENDOR_DET, rc);
		goto release_regulator;
	}
	rc = gpio_direction_input(LCD_VENDOR_DET);
	if (rc) {
		pr_err("%s: GPIO %d: direction in failed. rc=%d\n",
		       __func__, LCD_VENDOR_DET, rc);
		goto release_gpio;
	}

	val = gpio_get_value(LCD_VENDOR_DET);
	pr_info("%s: GPIO:%d\n", __func__, val);

	rc = regulator_disable(vreg_lcd_vddio);
	if (rc) {
		pr_err("%s: Enable regulator 8901_lvs1 failed\n",
		       __func__);
	}
	msleep(50);

release_gpio:
	gpio_free(LCD_VENDOR_DET);
release_regulator:
	regulator_put(vreg_lcd_vddio);

	return val;
}

static int s6d6aa0_vreg_power(int on)
{
	static struct regulator *vreg_lcd_vddio;
	int rc = 0;

	if (!vreg_lcd_vddio) {
		vreg_lcd_vddio = regulator_get(NULL, "8901_lvs1");
		if (IS_ERR(vreg_lcd_vddio)) {
			pr_err("%s: Unable to get 8901_lvs1\n", __func__);
			vreg_lcd_vddio = NULL;
			return -ENODEV;
		}
	}

	if (on) {
		rc = regulator_enable(vreg_lcd_vddio);
		if (rc) {
			pr_err("%s: Enable regulator 8901_lvs1 failed\n",
				__func__);
			regulator_put(vreg_lcd_vddio);
			return rc;
		}
		gpio_set_value(LCD_PWR_EN, 1);

		lcd_reset(1);
	} else {
		lcd_reset(0);

		gpio_set_value(LCD_PWR_EN, 0);
		rc = regulator_disable(vreg_lcd_vddio);
		if (rc)
			pr_warning("%s: '%s' regulator disable failed, rc=%d\n",
				__func__, "8901_lvs1", rc);
	}

	return 0;
}

static int s6d6aa0_lcd_power(int on)
{
	static int curr_power;
	int rc;

	if (curr_power == on)
		return 0;

	if (on) {
		rc = s6d6aa0_vreg_power(on);
		msleep(50);
		if (!rc)
			rc = mipi_dsi_power(on);
	} else {
		rc = mipi_dsi_power(on);
		msleep(2);
		if (!rc)
			rc = s6d6aa0_vreg_power(on);
	}

	if (!rc)
		curr_power = on;

	return rc;
}

#endif /* CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0 */

static int lcd_gpio_setup(int request);

static int r63306_vreg_power(int on)
{
	static struct regulator *vreg_lcd_vci;
	static struct regulator *vreg_lcd_vddio;
	int rc = 0;

	if (!vreg_lcd_vci) {
		vreg_lcd_vci = regulator_get(NULL, "8901_l2");
		if (IS_ERR(vreg_lcd_vci)) {
			pr_err("%s: Unable to get 8901_l2\n", __func__);
			vreg_lcd_vci = NULL;
			return -ENODEV;
		}
	}

	if (!vreg_lcd_vddio) {
		vreg_lcd_vddio = regulator_get(NULL, "8901_lvs1");
		if (IS_ERR(vreg_lcd_vddio)) {
			pr_err("%s: Unable to get 8901_lvs1\n", __func__);
			vreg_lcd_vddio = NULL;
			return -ENODEV;
		}
	}

	if (on) {
		rc = regulator_set_voltage(vreg_lcd_vci, 2850000, 2850000);
		if (rc) {
			pr_err("%s:%d unable to set L2 voltage to 2.8V\n",
				__func__, rc);
			regulator_put(vreg_lcd_vci);
			regulator_put(vreg_lcd_vddio);
			return rc;
		}

		rc = regulator_enable(vreg_lcd_vci);
		if (rc) {
			pr_err("%s: Enable regulator 8901_l2 failed\n",
				__func__);
			regulator_put(vreg_lcd_vci);
			regulator_put(vreg_lcd_vddio);
			return rc;
		}

		rc = regulator_enable(vreg_lcd_vddio);
		if (rc) {
			pr_err("%s: Enable regulator 8901_lvs1 failed\n",
				__func__);
			regulator_put(vreg_lcd_vci);
			regulator_put(vreg_lcd_vddio);
			return rc;
		}

		lcd_gpio_setup(1);
		gpio_set_value(MLCD_RESET_N, 0);
		msleep(10);
		gpio_set_value(MLCD_RESET_N, 1);
		msleep(10);
	} else {
		gpio_set_value(MLCD_RESET_N, 1);
		gpio_set_value(MLCD_RESET_N, 0);
		lcd_gpio_setup(0);
		msleep(10);
		rc = regulator_disable(vreg_lcd_vci);
		if (rc)
			pr_warning("%s: '%s' regulator disable failed, rc=%d\n",
				__func__, "8901_l2", rc);
		rc = regulator_disable(vreg_lcd_vddio);
		if (rc)
			pr_warning("%s: '%s' regulator disable failed, rc=%d\n",
				__func__, "8901_lvs1", rc);
	}

	return 0;
}

static int r63306_lcd_power(int on)
{
	static int curr_power;
	int rc;

	if (curr_power == on)
		return 0;

	if (on) {
		rc = r63306_vreg_power(on);
		msleep(2);
		if (!rc)
			rc = mipi_dsi_power(on);
	} else {
		rc = mipi_dsi_power(on);
		msleep(2);
		if (!rc)
			rc = r63306_vreg_power(on);
	}

	if (!rc)
		curr_power = on;

	return rc;
}

static int lcd_gpio_setup(int request)
{
	int rc = 0;

	if (!!request) {
		/* LCD_PWR_EN */
		rc = gpio_request(LCD_PWR_EN, "lcd power gpio");
		if (rc) {
			pr_err("%s: GPIO %d: request failed. rc=%d\n",
			       __func__, LCD_PWR_EN, rc);
			return rc;
		}
		rc = gpio_direction_output(LCD_PWR_EN, 0);
		if (rc) {
			pr_err("%s: GPIO %d: direction out failed. rc=%d\n",
			       __func__, LCD_PWR_EN, rc);
			goto out_pwr;
		}
		/* MLCD_RESET_N */
		rc = gpio_request(MLCD_RESET_N, "lcd reset");
		if (rc) {
			pr_err("%s: GPIO %d: request failed. rc=%d\n",
			       __func__, MLCD_RESET_N, rc);
			goto out_pwr;
		}
		rc = gpio_direction_output(MLCD_RESET_N, 0);
		if (rc) {
			pr_err("%s: GPIO %d: direction out failed. rc=%d\n",
			       __func__, MLCD_RESET_N, rc);
			goto out_reset;
		}
	} else {
		gpio_free(LCD_PWR_EN);
		gpio_free(MLCD_RESET_N);
	}

	return rc;
out_reset:
	gpio_free(MLCD_RESET_N);
out_pwr:
	gpio_free(LCD_PWR_EN);
	return rc;
}

static int lcd_power(int on)
{
	if (on)
		gpio_set_value(LCD_PWR_EN, 1);
	else
		gpio_set_value(LCD_PWR_EN, 0);
	msleep(50);

	return 0;
}

static int lcd_reset(int on)
{
	if (on) {
		gpio_set_value(MLCD_RESET_N, 0);
		msleep(10);
		gpio_set_value(MLCD_RESET_N, 1);
	} else {
		gpio_set_value(MLCD_RESET_N, 1);
		msleep(10);
		gpio_set_value(MLCD_RESET_N, 0);
	}
	msleep(10);

	return 0;
}

static const struct panel_id *panel_ids_r63306[] = {
#ifdef CONFIG_FB_MSM_MIPI_R63306_PANEL_TMD_MDV20
	&tmd_video_wxga_mdv20_panel_id_00,
	&tmd_video_wxga_mdv20_panel_id_01,
	&tmd_video_wxga_mdv20_panel_id_02,
	&tmd_video_wxga_mdv20_panel_id,
#endif /* CONFIG_FB_MSM_MIPI_R63306_PANEL_TMD_MDV20 */
#ifdef CONFIG_FB_MSM_MIPI_R63306_PANEL_TMD_MDV22
	&tmd_video_wxga_mdv22_panel_id_00,
	&tmd_video_wxga_mdv22_panel_id_01,
	&tmd_video_wxga_mdv22_panel_id_02,
	&tmd_video_wxga_mdv22_panel_id,
#endif /* CONFIG_FB_MSM_MIPI_R63306_PANEL_TMD_MDV22 */
#ifdef CONFIG_FB_MSM_MIPI_R63306_PANEL_TMD_MDW30
	&tmd_video_wxga_mdw30_panel_id,
#endif /* CONFIG_FB_MSM_MIPI_R63306_PANEL_TMD_MDW30 */
#ifdef CONFIG_FB_MSM_MIPI_R63306_PANEL_SHARP_LS043K3SX01
	&sharp_video_wxga_ls043k3sx01_panel_id,
#endif /* CONFIG_FB_MSM_MIPI_R63306_PANEL_SHARP_LS043K3SX01 */
	NULL,
};

static struct lcd_panel_platform_data lcd_data_r63306 = {
	.panels = panel_ids_r63306,
	.lcd_power = lcd_power,
	.lcd_reset = lcd_reset,
};

static struct platform_device semc_fuji_lcd_device_r63306 = {
	.name = R63306_DEVICE_NAME,
	.id = 0,
	.dev.platform_data = &lcd_data_r63306,
};

#ifdef CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0
static const struct panel_id *panel_ids_s6d6aa0[] = {
#ifdef CONFIG_FB_MSM_MIPI_S6D6AA0_PANEL_SONY_ACX439AKM
	&sony_acx439akm_panel_id,
#endif /* CONFIG_FB_MSM_MIPI_S6D6AA0_PANEL_SONY_ACX439AKM */
	NULL,
};

static struct lcd_panel_platform_data lcd_data_s6d6aa0 = {
	.panels = panel_ids_s6d6aa0,
};

static struct platform_device semc_fuji_lcd_device_s6d6aa0 = {
	.name = S6D6AA0_DEVICE_NAME,
	.id = 0,
	.dev.platform_data = &lcd_data_s6d6aa0,
};

#endif /* CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0 */

void __init semc_fuji_add_lcd_device(void)
{
	int rc;
	struct platform_device *panel_dev = &semc_fuji_lcd_device_r63306;

#ifdef CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0
	if (semc_fuji_is_dric_s6d6aa0() == 1)
		panel_dev = &semc_fuji_lcd_device_s6d6aa0;
#endif
	rc = platform_device_register(panel_dev);
	if (rc)
		dev_err(&panel_dev->dev,
			"%s: platform_device_register() failed = %d\n",
			__func__, rc);

}

#endif /* CONFIG_FB_MSM_MIPI_DSI_RENESAS_R63306 */

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors mdp_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_sd_smi_vectors[] = {
	/* Default case static display/UI/2d/3d if FB SMI */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 407808000,
		.ib = 1019520000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_sd_ebi_vectors[] = {
	/* Default case static display/UI/2d/3d if FB SMI */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 407808000,
		.ib = 509760000 * 2,
	},
};
static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 248832000,
		.ib = 311040000 * 2,
	},
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 248832000,
		.ib = 311040000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 304128000,
		.ib = 380160000 * 2,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 304128000,
		.ib = 380160000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 850000000,
		.ib = 2100000000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 850000000,
		.ib = 2100000000,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_sd_smi_vectors),
		mdp_sd_smi_vectors,
	},
	{
		ARRAY_SIZE(mdp_sd_ebi_vectors),
		mdp_sd_ebi_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};
static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

#endif
#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};
static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 850000000,
		.ib = 2100000000,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 850000000,
		.ib = 2100000000,
	},
};
static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
};
#endif

/*
 * MIPI_DSI only use 8058_LDO0 which need always on
 * therefore it need to be put at low power mode if
 * it was not used instead of turn it off.
 */
static int mipi_dsi_power(int on)
{
	static int curr_power;
	static struct regulator *ldo0;
	int rc;

	if (curr_power == on)
		return 0;

	if (ldo0 == NULL) {	/* init */
		ldo0 = regulator_get(NULL, "8058_l0");
		if (IS_ERR(ldo0)) {
			pr_err("%s: LDO0 failed\n", __func__);
			rc = PTR_ERR(ldo0);
			return rc;
		}

		rc = regulator_set_voltage(ldo0, 1200000, 1200000);
		if (rc) {
			pr_err("%s: Unable to set voltage level for LDO0\n",
			       __func__);
			goto out_put;
		}

		rc = regulator_enable(ldo0);
		if (rc) {
			pr_err("%s: Unable to enable LDO0\n", __func__);
			goto out_put;
		}

		/* set ldo0 to HPM */
		rc = regulator_set_optimum_mode(ldo0, 100000);
		if (rc < 0) {
			pr_err("%s: Unable to set HPM of LDO0:", __func__);
			goto out_disable;
		} else {
			return 0;
		}
	}

	if (on) {
		/* set ldo0 to HPM */
		rc = regulator_set_optimum_mode(ldo0, 100000);
		if (rc < 0)
			pr_err("%s: Unable to set HPM of LDO0:", __func__);
	} else {
		/* set ldo0 to LPM */
		rc = regulator_set_optimum_mode(ldo0, 1000);
		if (rc < 0)
			pr_err("%s: Unable to set LPM of LDO0:", __func__);
	}

	curr_power = on;

	return 0;
out_disable:
	regulator_disable(ldo0);
out_put:
	regulator_put(ldo0);
	ldo0 = NULL;
	return rc;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.dsi_power_save   = r63306_lcd_power,
};

#ifdef CONFIG_FB_MSM_TVOUT
static struct regulator *reg_8058_l13;

static int atv_dac_power(int on)
{
	int rc = 0;
	#define _GET_REGULATOR(var, name) do {				\
		var = regulator_get(NULL, name);			\
		if (IS_ERR(var)) {					\
			pr_info("'%s' regulator not found, rc=%ld\n",	\
				name, IS_ERR(var));			\
			var = NULL;					\
			return -ENODEV;					\
		}							\
	} while (0)

	if (!reg_8058_l13)
		_GET_REGULATOR(reg_8058_l13, "8058_l13");
	#undef _GET_REGULATOR

	if (on) {
		rc = regulator_set_voltage(reg_8058_l13, 2050000, 2050000);
		if (rc) {
			pr_info("%s: '%s' regulator set voltage failed,\
				rc=%d\n", __func__, "8058_l13", rc);
			return rc;
		}

		rc = regulator_enable(reg_8058_l13);
		if (rc) {
			pr_err("%s: '%s' regulator enable failed,\
				rc=%d\n", __func__, "8058_l13", rc);
			return rc;
		}
	} else {
		rc = regulator_force_disable(reg_8058_l13);
		if (rc)
			pr_warning("%s: '%s' regulator disable failed, rc=%d\n",
				__func__, "8058_l13", rc);
	}
	return rc;

}
#endif

#ifdef CONFIG_FB_MSM_MIPI_DSI
int mdp_core_clk_rate_table[] = {
	85330000,
	85330000,
	160000000,
	200000000,
};
#else
int mdp_core_clk_rate_table[] = {
	59080000,
	85330000,
	128000000,
	200000000,
};
#endif

static struct msm_panel_common_pdata mdp_pdata = {
	.mdp_core_clk_rate = 59080000,
	.mdp_core_clk_table = mdp_core_clk_rate_table,
	.num_mdp_clk = ARRAY_SIZE(mdp_core_clk_rate_table),
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_41,
	.writeback_offset = writeback_offset,
};

#ifdef CONFIG_FB_MSM_TVOUT

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors atv_bus_init_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 0,
		.ib = 0,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};
static struct msm_bus_vectors atv_bus_def_vectors[] = {
	/* For now, 0th array entry is reserved.
	 * Please leave 0 as is and don't use it
	 */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_SMI,
		.ab = 236390400,
		.ib = 265939200,
	},
	/* Master and slaves can be from different fabrics */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 236390400,
		.ib = 265939200,
	},
};
static struct msm_bus_paths atv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(atv_bus_init_vectors),
		atv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(atv_bus_def_vectors),
		atv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata atv_bus_scale_pdata = {
	atv_bus_scale_usecases,
	ARRAY_SIZE(atv_bus_scale_usecases),
	.name = "atv",
};
#endif

static struct tvenc_platform_data atv_pdata = {
	.poll		 = 0,
	.pm_vid_en	 = atv_dac_power,
#ifdef CONFIG_MSM_BUS_SCALING
	.bus_scale_table = &atv_bus_scale_pdata,
#endif
};
#endif

void __init semc_fuji_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
#ifdef CONFIG_FB_MSM_MIPI_DSI_SAMSUNG_S6D6AA0
	if (semc_fuji_is_dric_s6d6aa0())
		mipi_dsi_pdata.dsi_power_save = s6d6aa0_lcd_power;
#endif
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
#ifdef CONFIG_FB_MSM_TVOUT
	msm_fb_register_device("tvenc", &atv_pdata);
	msm_fb_register_device("tvout_device", NULL);
#endif
}
