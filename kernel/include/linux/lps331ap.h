/*
* drivers/misc/lps001wp.h
*
* STMicroelectronics LPS331AP Pressure / Temperature Sensor module driver
*
* Copyright (C) 2010 STMicroelectronics- MSH - Motion Mems BU - Application Team
* Matteo Dameno (matteo.dameno@st.com)
* Carmine Iascone (carmine.iascone@st.com)
*
* Both authors are willing to be considered the contact and update points for
* the driver.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
* Revision 1.0.1 2011/Apr/04:
*/

#ifndef	__LPS331AP_H__
#define	__LPS331AP_H__

#define	LPS331AP_PRS_DEV_NAME		"lps331ap_prs_sysfs"

/* Barometer and Termometer output data rate ODR */
#define	LPS331AP_PRS_ODR_MASK	0x70	/* Mask to access odr bits only	*/
#define	LPS331AP_PRS_ODR_ONESH	0x00	/* one shot both		*/
#define	LPS331AP_PRS_ODR_1_1	0x10	/*  1  Hz baro,  1  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_7_1	0x20	/*  7  Hz baro,  1  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_12_1	0x30	/* 12.5Hz baro,  1  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_25_1	0x40	/* 25  Hz baro,  1  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_7_7	0x50	/*  7  Hz baro,  7  Hz term ODR	*/
#define	LPS331AP_PRS_ODR_12_12	0x60	/* 12.5Hz baro, 12.5Hz term ODR	*/
#define	LPS331AP_PRS_ODR_25_25	0x70	/* 25  Hz baro, 25  Hz term ODR	*/

/*	Output conversion factors		*/
#define	SENSITIVITY_T		480	/* =	480 LSB/degrC	*/
#define	SENSITIVITY_P		4096	/* =	LSB/mbar	*/
#define	SENSITIVITY_P_SHIFT	12	/* =	4096 LSB/mbar	*/
#define	TEMPERATURE_OFFSET	42.5f	/* =	42.5 degrC	*/

struct lps331ap_prs_platform_data {
	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);

	int poll_interval;
	int min_interval;
};

#endif  /* __LPS331AP_H__ */
