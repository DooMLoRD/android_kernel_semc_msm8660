/* arch/arm/mach-msm/include/mach/mipi_dsi_samsung.h
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


#ifndef __ARCH_ARM_MACH_MSM_MIPI_DSI_SAMSUNG_H
#define __ARCH_ARM_MACH_MSM_MIPI_DSI_SAMSUNG_H

#include <linux/types.h>

#define S6D6AA0_DEVICE_NAME "mipi_samsung_s6d6aa0"

#ifdef CONFIG_FB_MSM_MIPI_S6D6AA0_PANEL_SONY_ACX439AKM
extern const struct panel_id sony_acx439akm_panel_id;
#endif /* CONFIG_FB_MSM_MIPI_S6D6AA0_PANEL_SONY_ACX439AKM */

#endif
