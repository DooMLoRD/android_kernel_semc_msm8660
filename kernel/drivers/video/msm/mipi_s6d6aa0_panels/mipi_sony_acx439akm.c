/* drivers/video/msm/mipi_s6d6aa0_panels/mipi_sony_acx439akm.c
 *
 * Copyright (C) [2011] Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2; as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_dsi_video_panel.h"

/* Display ON Sequence */
static char exit_sleep[] = {
	0x11
};
static char display_scan_ctrl[] = {
	0x36, 0x40
};
static char display_on[] = {
	0x29
};

/* Display OFF Sequence */
static char display_off[] = {
	0x28
};
static char enter_sleep[] = {
	0x10
};

static struct dsi_cmd_desc sony_display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 140,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0,
		sizeof(display_scan_ctrl), display_scan_ctrl},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc sony_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150,
		sizeof(enter_sleep), enter_sleep}
};

/* to be fixed */
static const struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db[] = {
	/* 720*1280, RGB888, 4 Lane 60 fps video mode */
	{
		{0x13, 0x0b, 0x05, 0x50},	/* regulator */
		/* timing   */
		{0xA7, 0x89, 0x16, 0x00, 0x18, 0x90, 0x19,
		 0x8b, 0x18, 0x03, 0x04},
		{0x7f, 0x00, 0x00, 0x00},	/* phy ctrl */
		{0xdd, 0x02, 0x86, 0x00},	/* strength */
		/* pll control */
		{0x40, 0x5C, 0xb1, 0xda, 0x00, 0x2f, 0x48, 0x63,
		0x31, 0x0f, 0x03,
		0x05, 0x14, 0x03, 0x00, 0x00, 0x54, 0x06, 0x10, 0x04, 0x00 },
	},
};

static struct msm_panel_info acx439akm_pinfo;

static struct msm_panel_info *get_panel_info(void)
{
	/* should fix porch, pulse widht and so on */
	acx439akm_pinfo.xres = 720;
	acx439akm_pinfo.yres = 1280;
	acx439akm_pinfo.type = MIPI_VIDEO_PANEL;
	acx439akm_pinfo.pdest = DISPLAY_1;
	acx439akm_pinfo.wait_cycle = 0;
	acx439akm_pinfo.bpp = 24;
	acx439akm_pinfo.lcdc.h_back_porch = 10;
	acx439akm_pinfo.lcdc.h_front_porch = 21;
	acx439akm_pinfo.lcdc.h_pulse_width = 3;
	acx439akm_pinfo.lcdc.v_back_porch = 2;
	acx439akm_pinfo.lcdc.v_front_porch = 2;
	acx439akm_pinfo.lcdc.v_pulse_width = 2;
	acx439akm_pinfo.lcdc.border_clr = 0;	/* blk */
	acx439akm_pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	acx439akm_pinfo.lcdc.hsync_skew = 0;
	acx439akm_pinfo.bl_max = 15;
	acx439akm_pinfo.bl_min = 1;
	acx439akm_pinfo.fb_num = 2;
	acx439akm_pinfo.clk_rate = 350000000;
	acx439akm_pinfo.lcd.refx100 = 6000;

	acx439akm_pinfo.mipi.mode = DSI_VIDEO_MODE;
	acx439akm_pinfo.mipi.pulse_mode_hsa_he = TRUE;
	acx439akm_pinfo.mipi.hfp_power_stop = FALSE;
	acx439akm_pinfo.mipi.hbp_power_stop = FALSE;
	acx439akm_pinfo.mipi.hsa_power_stop = FALSE;
	acx439akm_pinfo.mipi.eof_bllp_power_stop = TRUE;
	acx439akm_pinfo.mipi.bllp_power_stop = TRUE;
	acx439akm_pinfo.mipi.traffic_mode = DSI_NON_BURST_SYNCH_EVENT;
	acx439akm_pinfo.mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	acx439akm_pinfo.mipi.vc = 0;
	acx439akm_pinfo.mipi.dlane_swap = 0x01;
	acx439akm_pinfo.mipi.rgb_swap = DSI_RGB_SWAP_BGR;
	acx439akm_pinfo.mipi.r_sel = 0;
	acx439akm_pinfo.mipi.g_sel = 0;
	acx439akm_pinfo.mipi.b_sel = 0;
	acx439akm_pinfo.mipi.data_lane0 = TRUE;
	acx439akm_pinfo.mipi.data_lane1 = TRUE;
	acx439akm_pinfo.mipi.data_lane2 = TRUE;
	acx439akm_pinfo.mipi.data_lane3 = TRUE;
	acx439akm_pinfo.mipi.tx_eot_append = TRUE;
	acx439akm_pinfo.mipi.t_clk_post = 34;
	acx439akm_pinfo.mipi.t_clk_pre = 60;
	acx439akm_pinfo.mipi.stream = 0; /* dma_p */
	acx439akm_pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
	acx439akm_pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	acx439akm_pinfo.mipi.frame_rate  = 60;
	acx439akm_pinfo.mipi.dsi_phy_db =
		(struct mipi_dsi_phy_ctrl *)dsi_video_mode_phy_db;

	return &acx439akm_pinfo;
}

static struct dsi_video_controller dsi_video_controller_panel = {
	.get_panel_info = get_panel_info,
	.display_on_cmds = sony_display_on_cmds,
	.display_off_cmds = sony_display_off_cmds,
	.display_on_cmds_size = ARRAY_SIZE(sony_display_on_cmds),
	.display_off_cmds_size = ARRAY_SIZE(sony_display_off_cmds),
};

const struct panel_id sony_acx439akm_panel_id = {
	.name = "mipi_video_sony_wxga_acx439akm",
	.pctrl = &dsi_video_controller_panel,
};
