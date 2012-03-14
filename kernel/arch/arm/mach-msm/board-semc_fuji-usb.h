/*
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
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

#ifndef _ARCH_ARM_MACH_MSM_BOARD_SEMC_FUJI_USB_H
#define _ARXH_ARM_MACH_MSM_BOARD_SEMC_FUJI_USB_H

#include <linux/platform_device.h>
#include <mach/msm_hsusb.h>

extern struct platform_device semc_fuji_usb_device;
extern struct platform_device semc_fuji_mass_storage_device;
extern struct platform_device semc_fuji_rndis_device;
extern struct msm_usb_host_platform_data semc_fuji_usb_host_pdata;
extern struct msm_otg_platform_data semc_fuji_otg_pdata;
extern struct msm_hsusb_gadget_platform_data semc_fuji_gadget_pdata;
#ifdef CONFIG_USB_NCP373
extern struct platform_device ncp373_device;
#endif

#endif
