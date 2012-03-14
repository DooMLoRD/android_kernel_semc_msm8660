/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef _ARCH_ARM_MACH_MSM_BOARD_SEMC_FUJI_VIDEO_H
#define _ARXH_ARM_MACH_MSM_BOARD_SEMC_FUJI_VIDEO_H

#include <linux/platform_device.h>
#include <mach/board.h>

void __init semc_fuji_add_lcd_device(void);
void __init semc_fuji_fb_add_devices(void);
extern struct platform_device semc_fuji_hdmi_device;
extern struct resource semc_fuji_fb_resources[];
extern struct platform_device semc_fuji_fb_device;

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE (1280 * 736 * 4 * 3) /* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE (1280 * 736 * 4 * 2) /* 4 bpp x 2 pages */
#endif

#if defined(CONFIG_FB_MSM_HDMI_MSM_PANEL) \
	|| defined(CONFIG_FB_MSM_HDMI_MSM_FUJI_PANEL)
#define MSM_FB_EXT_BUF_SIZE  (1920 * 1080 * 2 * 1) /* 2 bpp x 1 page */
#elif defined(CONFIG_FB_MSM_TVOUT)
#define MSM_FB_EXT_BUF_SIZE  (720 * 576 * 2 * 2) /* 2 bpp x 2 pages */
#else
#define MSM_FB_EXT_BUF_SIZE    0
#endif

#ifdef CONFIG_FB_MSM_LCDC_DSUB
/* VGA = 1440 x 900 x 4(bpp) x 2(pages)
   prim = 1024 x 600 x 4(bpp) x 2(pages)
   This is the difference. */
#define MSM_FB_DSUB_PMEM_ADDER (0x9E3400-0x4B0000)
#else
#define MSM_FB_DSUB_PMEM_ADDER (0)
#endif
#ifdef CONFIG_FB_MSM_OVERLAY_WRITEBACK
/* width x height x bpp x 2 frame buffer */
#define MSM_FB_WRITEBACK_SIZE (736 * 1280 * 4 * 2)
#define MSM_FB_WRITEBACK_OFFSET  \
		(MSM_FB_PRIM_BUF_SIZE + MSM_FB_EXT_BUF_SIZE)
#else
#define MSM_FB_WRITEBACK_SIZE 0
#define MSM_FB_WRITEBACK_OFFSET 0
#endif

#endif
