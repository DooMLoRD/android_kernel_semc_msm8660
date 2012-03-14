/* drivers/video/msm/mipi_dsi_video_panel.h
 *
 * Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


#ifndef MIPI_DSI_VIDEO_PANEL_H
#define MIPI_DSI_VIDEO_PANEL_H

#include <linux/types.h>

struct dsi_video_controller {
	struct msm_panel_info *(*get_panel_info) (void);
	struct dsi_cmd_desc *display_init_cmds;
	struct dsi_cmd_desc *display_on_cmds;
	struct dsi_cmd_desc *display_off_cmds;
	struct dsi_cmd_desc *read_id_cmds;
	int display_init_cmds_size;
	int display_on_cmds_size;
	int display_off_cmds_size;
};

struct panel_id {
	const char			*name;
	struct dsi_video_controller	*pctrl;
	const u32			width;	/* in mm */
	const u32			height;	/* in mm */
	const char			*id;
	const int			id_num;
};

#endif /* MIPI_DSI_VIDEO_PANEL_H */
