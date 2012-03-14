/* drivers/video/msm/mipi_samsung_s6d6aa0.c
 *
 * Copyright (C) [2011] Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2; as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <mach/mipi_dsi_samsung.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_dsi_video_panel.h"

struct mipi_s6d6aa0_data {
	struct lcd_panel_platform_data *pdata;
	struct dsi_buf tx_buf;
	struct dsi_buf rx_buf;
	struct msm_fb_panel_data panel_data;
	const struct panel_id *panel;
};

static int mipi_s6d6aa0_disp_on(struct msm_fb_data_type *mfd)
{
	struct mipi_s6d6aa0_data *dsi_data;

	dsi_data = platform_get_drvdata(mfd->panel_pdev);
	if (!dsi_data || !dsi_data->pdata)
		return -ENODEV;

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_buf_init(&dsi_data->tx_buf);
	mipi_dsi_cmds_tx(mfd, &dsi_data->tx_buf,
			 dsi_data->panel->pctrl->display_on_cmds,
			 dsi_data->panel->pctrl->display_on_cmds_size);
	mutex_unlock(&mfd->dma->ov_mutex);

	return 0;
}

static int mipi_s6d6aa0_disp_off(struct msm_fb_data_type *mfd)
{
	struct mipi_s6d6aa0_data *dsi_data;

	dsi_data = platform_get_drvdata(mfd->panel_pdev);

	if (!dsi_data || !dsi_data->pdata)
		return -ENODEV;

	mipi_dsi_op_mode_config(DSI_CMD_MODE);

	mutex_lock(&mfd->dma->ov_mutex);
	mipi_dsi_buf_init(&dsi_data->tx_buf);
	mipi_dsi_cmds_tx(mfd, &dsi_data->tx_buf,
			dsi_data->panel->pctrl->display_off_cmds,
			dsi_data->panel->pctrl->display_off_cmds_size);
	mutex_unlock(&mfd->dma->ov_mutex);

	return 0;
}
static int mipi_s6d6aa0_lcd_on(struct platform_device *pdev)
{
	int ret;
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	ret = mipi_s6d6aa0_disp_on(mfd);
	if (ret)
		dev_err(&pdev->dev, "%s: Display on failed\n", __func__);

	return ret;
}

static int mipi_s6d6aa0_lcd_off(struct platform_device *pdev)
{
	int ret;
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	ret = mipi_s6d6aa0_disp_off(mfd);
	if (ret)
		dev_err(&pdev->dev, "%s: Display off failed\n", __func__);

	return ret;
}

static void mipi_s6d6aa0_set_panel(struct mipi_s6d6aa0_data *dsi_data)
{
	dsi_data->panel = dsi_data->pdata->panels[0];
	MSM_FB_INFO("panel: %s\n", dsi_data->panel->name);
	dsi_data->panel_data.panel_info =
		*dsi_data->panel->pctrl->get_panel_info();
}

static int __devexit mipi_s6d6aa0_lcd_remove(struct platform_device *pdev)
{
	struct mipi_s6d6aa0_data *dsi_data;

	dsi_data = platform_get_drvdata(pdev);
	if (!dsi_data)
		return -ENODEV;

	platform_set_drvdata(pdev, NULL);
	mipi_dsi_buf_release(&dsi_data->tx_buf);
	mipi_dsi_buf_release(&dsi_data->rx_buf);
	kfree(dsi_data);
	return 0;
}

static int __devinit mipi_s6d6aa0_lcd_probe(struct platform_device *pdev)
{
	int ret;
	struct lcd_panel_platform_data *platform_data;
	struct mipi_s6d6aa0_data *dsi_data;

	platform_data = pdev->dev.platform_data;
	if (platform_data == NULL) {
		ret = -EINVAL;
		goto error;
	}

	dsi_data = kzalloc(sizeof(struct mipi_s6d6aa0_data), GFP_KERNEL);
	if (dsi_data == NULL) {
		ret = -ENOMEM;
		goto error;
	}

	dsi_data->pdata = platform_data;
	dsi_data->panel_data.on = mipi_s6d6aa0_lcd_on;
	dsi_data->panel_data.off = mipi_s6d6aa0_lcd_off;

#ifdef CONFIG_FB_MSM_MIPI_PANEL_DETECT
	if (msm_fb_detect_client(dsi_data->pdata->pdata->name)) {
		kfree(dsi_data);
		ret = -ENODEV;
		goto error;
	}
#endif

	ret = mipi_dsi_buf_alloc(&dsi_data->tx_buf, DSI_BUF_SIZE);
	if (ret <= 0) {
		dev_err(&pdev->dev, "mipi_dsi_buf_alloc(tx) failed!\n");
		kfree(dsi_data);
		goto error;
	}

	ret = mipi_dsi_buf_alloc(&dsi_data->rx_buf, DSI_BUF_SIZE);
	if (ret <= 0) {
		dev_err(&pdev->dev, "mipi_dsi_buf_alloc(rx) failed!\n");
		mipi_dsi_buf_release(&dsi_data->tx_buf);
		kfree(dsi_data);
		goto error;
	}

	platform_set_drvdata(pdev, dsi_data);

	mipi_s6d6aa0_set_panel(dsi_data);

	ret = platform_device_add_data(pdev, &dsi_data->panel_data,
		sizeof(dsi_data->panel_data));
	if (ret) {
		dev_err(&pdev->dev,
			"platform_device_add_data failed!\n");
		mipi_dsi_buf_release(&dsi_data->tx_buf);
		mipi_dsi_buf_release(&dsi_data->rx_buf);
		kfree(dsi_data);
		goto error;
	}

	msm_fb_add_device(pdev);

	return 0;
error:
	return ret;
}

static struct platform_driver this_driver = {
	.probe  = mipi_s6d6aa0_lcd_probe,
	.remove = mipi_s6d6aa0_lcd_remove,
	.driver = {
		.name   = S6D6AA0_DEVICE_NAME,
	},
};

static int __init mipi_s6d6aa0_lcd_init(void)
{
	return platform_driver_register(&this_driver);
}

static void __exit mipi_s6d6aa0_lcd_exit(void)
{
	platform_driver_unregister(&this_driver);
}

module_init(mipi_s6d6aa0_lcd_init);
module_exit(mipi_s6d6aa0_lcd_exit);

