/*
* drivers/misc/lps331ap_prs.c
*
* STMicroelectronics LPS331AP Pressure / Temperature Sensor module driver
*
* Copyright (C) 2010 STMicroelectronics- MSH - Motion Mems BU - Application Team
* Matteo Dameno (matteo.dameno@st.com)
* Carmine Iascone (carmine.iascone@st.com)
*
* Copyright (C) 2010 Sony Ericsson Mobile Communications AB.
* Author: Caroline Hamlin Olsson <caroline.hamlin.olsson@sonyericsson.com>
*
* Both authors are willing to be considered the contact and update points for
* the driver.
*
** Output data from the device are available from the assigned
* /dev/input/eventX device;
*
* LPS3311AP can be controlled by sysfs interface looking inside:
* /sys/bus/i2c/devices/<busnum>-<devaddr>/
*
* LPS331AP make available two i2C addresses selectable from platform_data
* by the LPS001WP_PRS_I2C_SAD_H or LPS001WP_PRS_I2C_SAD_L.
*
* Read pressures and temperatures output can be converted in units of
* measurement by dividing them respectively for SENSITIVITY_P and SENSITIVITY_T.
* Temperature values must then be added by the constant float TEMPERATURE_OFFSET
* expressed as Celsius degrees.
*
* Obtained values are then expessed as
* mbar (=0.1 kPa) and Celsius degrees.
*
* To use autozero feature you can write 0 zero or 1 to its corresponding sysfs
* file. This lets you to write current temperature and pressure into reference
* registers or to reset them.
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
*/
/******************************************************************************
 Revision 1.0.0 14/02/2011:
	first release
	moved to input/misc
 Revision 1.0.1 2011/Apr/04:
	xxx
******************************************************************************/

#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/lps331ap.h>
#include <linux/slab.h>
#include <linux/stat.h>

/*
#define	DEBUG
*/

#define	SAD0L				0x00
#define	SAD0H				0x01
#define	LPS331AP_PRS_I2C_SADROOT	0x2E
#define	LPS331AP_PRS_I2C_SAD_L		((LPS331AP_PRS_I2C_SADROOT<<1)|SAD0L)
#define	LPS331AP_PRS_I2C_SAD_H		((LPS331AP_PRS_I2C_SADROOT<<1)|SAD0H)

#define	PR_ABS_MAX	8388607		/* 24 bit 2'compl */
#define	PR_ABS_MIN	-8388608

#ifndef SHRT_MAX
#define SHRT_MAX	((s16)(0xffff>>1))
#define SHRT_MIN	((s16)(-SHRT_MAX - 1))
#endif

#define	TEMP_MAX	SHRT_MAX
#define TEMP_MIN	SHRT_MIN

#define	WHOAMI_LPS331AP_PRS	0xBB	/*	Expctd content for WAI	*/

/*	CONTROL REGISTERS	*/
#define	REF_P_XL	0x08		/*	pressure reference	*/
#define	REF_P_L		0x09		/*	pressure reference	*/
#define	REF_P_H		0x0A		/*	pressure reference	*/
#define	REF_T_L		0x0B		/*	temperature reference	*/
#define	REF_T_H		0x0C		/*	temperature reference	*/

#define	WHO_AM_I	0x0F		/*	WhoAmI register		*/
#define	TP_RESOL	0x10		/*	Pres Temp resolution set*/

#define	CTRL_REG1	0x20		/*	power / ODR control reg	*/
#define	CTRL_REG2	0x21		/*	boot reg		*/
#define	CTRL_REG3	0x22		/*	interrupt control reg	*/
#define	INT_CFG_REG	0x23		/*	interrupt config reg	*/
#define	INT_SRC_REG	0x24		/*	interrupt source reg	*/
#define	THS_P_L		0x25		/*	pressure threshold	*/
#define	THS_P_H		0x26		/*	pressure threshold	*/
#define	STATUS_REG	0X27		/*	status reg		*/

#define PRESS_OUT_XL	0x28		/*	press output (3 regs)	*/
#define TEMP_OUT_L	0x2B		/*	temper output (2 regs)	*/

/*	REGISTERS ALIASES	*/
#define	P_REF_INDATA_REG	        REF_P_XL
#define	T_REF_INDATA_REG	        REF_T_L
#define	P_THS_INDATA_REG	        THS_P_L
#define	P_OUTDATA_REG		        PRESS_OUT_XL
#define	T_OUTDATA_REG		        TEMP_OUT_L
#define	OUTDATA_REG		        PRESS_OUT_XL
/* end  REGISTERS ALIASES	*/

#define	LPS331AP_PRS_ENABLE_MASK        0x80	/*  ctrl_reg1 */
#define	LPS331AP_PRS_DIFF_MASK          0x08    /*  ctrl_reg1 */
#define LPS331AP_PRS_AUTOZ_MASK         0x02    /*  ctrl_reg2 */

#define	LPS331AP_PRS_PM_NORMAL		0x80	/* Power Normal Mode*/
#define	LPS331AP_PRS_PM_OFF		0x00	/* Power Down */

#define	LPS331AP_PRS_DIFF_ON		0x08	/* En Difference circuitry */
#define	LPS331AP_PRS_DIFF_OFF		0x00	/* Dis Difference circuitry */

#define	LPS331AP_PRS_AUTOZ_ON		0x02	/* En AutoZero Function */
#define	LPS331AP_PRS_AUTOZ_OFF		0x00	/* Dis Difference Function */

#define	FUZZ			        0
#define	FLAT			        0

#define	I2C_AUTO_INCREMENT              0x80

#define NO_OF_OUTPUT_REGS               5       /* Number of output registers
						for collecting pressure
						and temp data           */
/* RESUME STATE INDICES */
enum resume_state_ind {
	LPS331AP_RES_REF_P_XL,
	LPS331AP_RES_REF_P_L,
	LPS331AP_RES_REF_P_H,
	LPS331AP_RES_REF_T_L,
	LPS331AP_RES_REF_T_H,
	LPS331AP_RES_TP_RESOL,
	LPS331AP_RES_CTRL_REG1,
	LPS331AP_RES_CTRL_REG2,
	LPS331AP_RES_CTRL_REG3,
	LPS331AP_RES_INT_CFG_REG,
	LPS331AP_RES_THS_P_L,
	LPS331AP_RES_THS_P_H,
	RESUME_ENTRIES,
};

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const struct {
	unsigned int cutoff_ms;
	unsigned int mask;
} lps331ap_prs_odr_table[] = {
	{40,	LPS331AP_PRS_ODR_25_25 },
	{80,	LPS331AP_PRS_ODR_12_12 },
	{143,	LPS331AP_PRS_ODR_7_7 },
	{1000,	LPS331AP_PRS_ODR_1_1 },
};

struct lps331ap_prs_data {
	struct i2c_client *client;
	struct lps331ap_prs_platform_data *pdata;

	struct mutex lock;
	struct delayed_work input_work;

	struct input_dev *input_dev;

	bool enabled;
	u8 resume_state[RESUME_ENTRIES];
	unsigned int poll_interval;

#ifdef DEBUG
	u8 reg_addr;
#endif
};

struct outputdata {
	s32 press;
	s16 temperature;
};

static int lps331ap_prs_i2c_read(struct lps331ap_prs_data *prs,
				  u8 *buf, int len)
{
	int err;
	struct i2c_msg msgs[] = {
		{
		 .addr = prs->client->addr,
		 .flags = prs->client->flags & I2C_M_TEN,
		 .len = 1,
		 .buf = buf,
		 }, {
		 .addr = prs->client->addr,
		 .flags = (prs->client->flags & I2C_M_TEN) | I2C_M_RD,
		 .len = len,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(prs->client->adapter, msgs, 2);
	if (err != 2) {
		dev_err(&prs->client->dev, "read transfer error\n");
		err = (err < 0) ? err : -EIO;
	} else {
		err = 0;
	}
	return err;
}

static int lps331ap_prs_i2c_write(struct lps331ap_prs_data *prs,
				   u8 *buf, int len)
{
	int err;
	struct i2c_msg msgs[] = {
		{
		 .addr = prs->client->addr,
		 .flags = prs->client->flags & I2C_M_TEN,
		 .len = len,
		 .buf = buf,
		 },
	};

	err = i2c_transfer(prs->client->adapter, msgs, 1);
	if (err != 1) {
		dev_err(&prs->client->dev, "write transfer error\n");
		err = (err < 0) ? err : -EIO;
	} else {
		err = 0;
	}
	return err;
}

static int lps331ap_prs_register_write(struct lps331ap_prs_data *prs, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int err;

	/* Sets configuration register at reg_address
	 *  NOTE: this is a straight overwrite  */
	buf[0] = reg_address;
	buf[1] = new_value;
	err = lps331ap_prs_i2c_write(prs, buf, 2);
	return err;
}

static int lps331ap_prs_register_read(struct lps331ap_prs_data *prs, u8 *buf,
		u8 reg_address)
{
	int err;
	buf[0] = reg_address;
	err = lps331ap_prs_i2c_read(prs, buf, 1);

	return err;
}

static int lps331ap_prs_register_update(struct lps331ap_prs_data *prs, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err;
	u8 init_val;
	u8 updated_val;
	err = lps331ap_prs_register_read(prs, buf, reg_address);
	if (err != 0) {
		init_val = buf[0];
		updated_val = (mask & new_bit_values) | (~mask & init_val);
		err = lps331ap_prs_register_write(prs, buf, reg_address,
				updated_val);
	}
	return err;
}

static int lps331ap_prs_hw_init(struct lps331ap_prs_data *prs)
{
	int err;
	u8 buf[6];

	dev_dbg(&prs->client->dev, "%s: hw init start\n",
		LPS331AP_PRS_DEV_NAME);

	buf[0] = WHO_AM_I;
	err = lps331ap_prs_i2c_read(prs, buf, MIN(sizeof(buf), 1));
	if (err != 0) {
		dev_err(&prs->client->dev, "Error reading WHO_AM_I: is device "
						"available/working?\n");
		goto err_resume_state;
	}

	buf[0] = I2C_AUTO_INCREMENT | P_REF_INDATA_REG;
	buf[1] = prs->resume_state[LPS331AP_RES_REF_P_XL];
	buf[2] = prs->resume_state[LPS331AP_RES_REF_P_L];
	buf[3] = prs->resume_state[LPS331AP_RES_REF_P_H];
	buf[4] = prs->resume_state[LPS331AP_RES_REF_T_L];
	buf[5] = prs->resume_state[LPS331AP_RES_REF_T_H];
	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
	if (err != 0)
		goto err_resume_state;

	buf[0] = TP_RESOL;
	buf[1] = prs->resume_state[LPS331AP_RES_TP_RESOL];
	err = lps331ap_prs_i2c_write(prs, buf, MIN(sizeof(buf), 2));
	if (err != 0)
		goto err_resume_state;

	buf[0] = I2C_AUTO_INCREMENT | P_THS_INDATA_REG;
	buf[1] = prs->resume_state[LPS331AP_RES_THS_P_L];
	buf[2] = prs->resume_state[LPS331AP_RES_THS_P_H];
	err = lps331ap_prs_i2c_write(prs, buf, MIN(sizeof(buf), 3));
	if (err != 0)
		goto err_resume_state;

	buf[0] = I2C_AUTO_INCREMENT | CTRL_REG1;
	buf[1] = prs->resume_state[LPS331AP_RES_CTRL_REG1];
	buf[2] = prs->resume_state[LPS331AP_RES_CTRL_REG2];
	buf[3] = prs->resume_state[LPS331AP_RES_CTRL_REG3];
	err = lps331ap_prs_i2c_write(prs, buf, MIN(sizeof(buf), 4));
	if (err != 0)
		goto err_resume_state;

	buf[0] = INT_CFG_REG;
	buf[1] = prs->resume_state[LPS331AP_RES_INT_CFG_REG];
	err = lps331ap_prs_i2c_write(prs, buf, MIN(sizeof(buf), 2));
	if (err != 0)
		goto err_resume_state;

	dev_dbg(&prs->client->dev, "%s: hw init start\n",
		LPS331AP_PRS_DEV_NAME);
	return 0;

err_resume_state:
	dev_err(&prs->client->dev, "hw init error 0x%x,0x%x: %d\n", buf[0],
			buf[1], err);
	return err;
}

static int lps331ap_prs_update_odr(struct lps331ap_prs_data *prs)
{
	int err;
	int i;
	int poll_period_ms = prs->poll_interval;
	u8 buf[2];
	u8 init_val, updated_val;
	u8 curr_val, new_val;
	u8 mask = LPS331AP_PRS_ODR_MASK;

	/* Following, looks for the longest possible odr interval scrolling the
	 * odr_table vector from the end (longest period) backward (shortest
	 * period), to support the poll_interval requested by the system.
	 * It must be the longest period shorter then the set poll period.*/
	for (i = ARRAY_SIZE(lps331ap_prs_odr_table) - 1; i >= 0; i--) {
		if (lps331ap_prs_odr_table[i].cutoff_ms <= poll_period_ms)
			break;
	}

	new_val = lps331ap_prs_odr_table[i].mask;

	buf[0] = CTRL_REG1;
	err = lps331ap_prs_i2c_read(prs, buf, MIN(sizeof(buf), 1));
	if (err != 0)
		goto error;
	/* work on all but ENABLE bits */
	init_val = buf[0];
	prs->resume_state[LPS331AP_RES_CTRL_REG1] = init_val;

	curr_val = (LPS331AP_PRS_ENABLE_MASK & LPS331AP_PRS_PM_OFF)
			| ((~LPS331AP_PRS_ENABLE_MASK) & init_val);
	buf[0] = CTRL_REG1;
	buf[1] = curr_val;
	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
	if (err != 0)
		goto error;

	updated_val = (mask & new_val) | (~mask & curr_val);

	buf[0] = CTRL_REG1;
	buf[1] = updated_val;
	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
	if (err != 0)
		goto error;

	curr_val = ((LPS331AP_PRS_ENABLE_MASK &
			LPS331AP_PRS_PM_NORMAL) |
			((~LPS331AP_PRS_ENABLE_MASK) & updated_val));
	buf[0] = CTRL_REG1;
	buf[1] = curr_val;
	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
	if (err != 0)
		goto error;

	prs->resume_state[LPS331AP_RES_CTRL_REG1] = curr_val;

	return err;

error:
	dev_err(&prs->client->dev, "update odr failed 0x%x,0x%x: %d\n",
			buf[0], buf[1], err);

	return err;
}

static void lps331ap_prs_device_power_off(struct lps331ap_prs_data *prs)
{
	int err;
	u8 buf[2] = { CTRL_REG1, LPS331AP_PRS_PM_OFF };

	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
	if (err != 0)
		dev_err(&prs->client->dev, "soft power off failed: %d\n", err);

	if (prs->pdata->power_off)
		prs->pdata->power_off();
	prs->enabled = false;
}

static int lps331ap_prs_device_power_on(struct lps331ap_prs_data *prs)
{
	int err;

	if (prs->pdata->power_on) {
		err = prs->pdata->power_on();
		if (err) {
			dev_err(&prs->client->dev,
					"power_on failed: %d\n", err);
			goto err_exit;
		}
	}

	err = lps331ap_prs_hw_init(prs);
	if (err)
		goto err_exit;
	err = lps331ap_prs_update_odr(prs);
	if (err) {
		dev_err(&prs->client->dev, "ODR set failed: %d\n", err);
		goto err_exit;
	}
	prs->enabled = true;
	return 0;

err_exit:
	lps331ap_prs_device_power_off(prs);
	return err;
}

static int lps331ap_prs_set_press_reference(struct lps331ap_prs_data *prs,
				s32 new_reference)
{
	int err;

	u8 bit_valuesxl, bit_valuesl, bit_valuesh;
	u8 buf[4];

	bit_valuesxl = new_reference & 0x0000FF;
	bit_valuesl = (new_reference & 0x00FF00) >> 8;
	bit_valuesh = (new_reference & 0xFF0000) >> 16;

	buf[0] = I2C_AUTO_INCREMENT | P_REF_INDATA_REG;
	buf[1] = bit_valuesxl;
	buf[2] = bit_valuesl;
	buf[3] = bit_valuesh;

	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
	if (err != 0)
		return err;

	prs->resume_state[LPS331AP_RES_REF_P_XL] = bit_valuesxl;
	prs->resume_state[LPS331AP_RES_REF_P_L] = bit_valuesl;
	prs->resume_state[LPS331AP_RES_REF_P_H] = bit_valuesh;

	return err;
}

static int lps331ap_prs_get_press_reference(struct lps331ap_prs_data *prs,
		s32 *buf32)
{
	int err;

	u8 bit_valuesxl, bit_valuesl, bit_valuesh;
	u8 buf[3];
	u16 temp;

	buf[0] =  I2C_AUTO_INCREMENT | P_REF_INDATA_REG;
	err = lps331ap_prs_i2c_read(prs, buf, sizeof(buf));
	if (err != 0)
		return err;
	bit_valuesxl = buf[0];
	bit_valuesl = buf[1];
	bit_valuesh = buf[2];

	temp = (bit_valuesh << 8) | bit_valuesl;
	*buf32 = (((s32) temp) << 8) | bit_valuesxl;
	dev_dbg(&prs->client->dev, "val: %+d\n", *buf32);

	return err;
}

static int lps331ap_prs_set_temperature_reference(struct lps331ap_prs_data *prs,
				s16 new_reference)
{
	int err;
	u8 bit_valuesl, bit_valuesh;
	u8 buf[3];

	bit_valuesl = new_reference & 0x00FF;
	bit_valuesh = new_reference >> 8;

	buf[0] = I2C_AUTO_INCREMENT | T_REF_INDATA_REG;
	buf[1] = bit_valuesl;
	buf[2] = bit_valuesh;
	err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));

	if (err != 0)
		return err;

	prs->resume_state[LPS331AP_RES_REF_T_L] = bit_valuesl;
	prs->resume_state[LPS331AP_RES_REF_T_H] = bit_valuesh;
	return err;
}

static int lps331ap_prs_get_temperature_reference(struct lps331ap_prs_data *prs,
		s16 *buf16)
{
	int err;

	u8 bit_valuesl, bit_valuesh;
	u8 buf[2] = {0, 0};
	u16 temp;

	buf[0] = I2C_AUTO_INCREMENT | T_REF_INDATA_REG;
	err = lps331ap_prs_i2c_read(prs, buf, sizeof(buf));
	if (err != 0)
		return err;

	bit_valuesl = buf[0];
	bit_valuesh = buf[1];

	temp = bit_valuesh << 8;
	*buf16 = temp | bit_valuesl;

	return err;
}

static int lps331ap_prs_autozero_manage(struct lps331ap_prs_data *prs,
					bool control)
{
	int err;
	u8 buf[6];
	u8 const mask = LPS331AP_PRS_AUTOZ_MASK;
	u8 bit_values = LPS331AP_PRS_AUTOZ_OFF;
	u8 init_val;

	if (control) {
		bit_values = LPS331AP_PRS_AUTOZ_ON;

		buf[0] = CTRL_REG2;
		err = lps331ap_prs_i2c_read(prs, buf, MIN(sizeof(buf), 1));
		if (err != 0)
			goto error;

		init_val = buf[0];
		prs->resume_state[LPS331AP_RES_CTRL_REG2] = init_val;

		err = lps331ap_prs_register_update(prs, buf, CTRL_REG2,
					mask, bit_values);
		if (err != 0)
			goto error;
	} else {
		memset(buf, 0, sizeof(buf));
		buf[0] = I2C_AUTO_INCREMENT | P_REF_INDATA_REG;
		err = lps331ap_prs_i2c_write(prs, buf, sizeof(buf));
		if (err != 0)
			goto error;
		prs->resume_state[LPS331AP_RES_REF_P_XL] = 0;
		prs->resume_state[LPS331AP_RES_REF_P_L] = 0;
		prs->resume_state[LPS331AP_RES_REF_P_H] = 0;
		prs->resume_state[LPS331AP_RES_REF_T_L] = 0;
		prs->resume_state[LPS331AP_RES_REF_T_H] = 0;
	}
error:
	return err;
}

static int lps331ap_prs_get_presstemp_data(struct lps331ap_prs_data *prs,
						struct outputdata *out)
{
	int err;
	/* Data bytes from hardware	PRESS_OUT_XL,PRESS_OUT_L,PRESS_OUT_H, */
	/*				TEMP_OUT_L, TEMP_OUT_H */
	u8 prs_data[NO_OF_OUTPUT_REGS];

	prs_data[0] = I2C_AUTO_INCREMENT | OUTDATA_REG;
	err = lps331ap_prs_i2c_read(prs, prs_data, sizeof(prs_data));
	if (err != 0)
		return err;

	out->press = (prs_data[2] << 16) | (prs_data[1] <<  8) | prs_data[0];
	out->temperature = ((prs_data[4]) << 8) | (prs_data[3]);

	return err;
}

static void lps331ap_prs_report_values(struct lps331ap_prs_data *prs,
					struct outputdata *out)
{
	input_report_abs(prs->input_dev, ABS_PRESSURE, out->press);

	input_report_abs(prs->input_dev, ABS_MISC, out->temperature);
	input_sync(prs->input_dev);
}

static ssize_t attr_get_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	mutex_lock(&prs->lock);
	val = prs->poll_interval;
	mutex_unlock(&prs->lock);
	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t attr_set_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	unsigned long interval_ms;

	if (strict_strtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (interval_ms < prs->pdata->min_interval) {
		dev_err(&prs->client->dev, "minimum poll interval violated\n");
		return -EINVAL;
	}
	mutex_lock(&prs->lock);
	prs->poll_interval = interval_ms;
	if (prs->enabled)
		lps331ap_prs_update_odr(prs);
	mutex_unlock(&prs->lock);
	return size;
}

static ssize_t attr_get_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	int val;

	mutex_lock(&prs->lock);
	val = prs->enabled;
	mutex_unlock(&prs->lock);
	return snprintf(buf, sizeof(buf), "%d\n", val);
}


static ssize_t attr_get_press_ref(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	s32 val = 0;

	err = lps331ap_prs_get_press_reference(prs, &val);
	if (err < 0)
		return err;

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t attr_set_press_ref(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int err;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	long val = 0;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (val < PR_ABS_MIN || val > PR_ABS_MAX)
		return -EINVAL;

	mutex_lock(&prs->lock);
	err = lps331ap_prs_set_press_reference(prs, val);
	mutex_unlock(&prs->lock);
	if (err < 0)
		return err;
	return size;
}

static ssize_t attr_get_temperature_ref(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	int err;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	s16 val = 0;

	err = lps331ap_prs_get_temperature_reference(prs, &val);
	if (err < 0)
		return err;

	return snprintf(buf, PAGE_SIZE, "%d\n", val);
}

static ssize_t attr_set_temperature_ref(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	int err;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	long val = 0;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (val < TEMP_MIN || val > TEMP_MAX)
		return -EINVAL;

	mutex_lock(&prs->lock);
	err = lps331ap_prs_set_temperature_reference(prs, val);
	mutex_unlock(&prs->lock);
	if (err < 0)
		return err;
	return size;
}

static ssize_t attr_set_autozero(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t size)
{
	int err;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	mutex_lock(&prs->lock);
	err = lps331ap_prs_autozero_manage(prs, !!val);
	mutex_unlock(&prs->lock);
	if (err < 0)
		return err;
	return size;
}

#ifdef DEBUG
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&prs->lock);
	x[0] = prs->reg_addr;
	mutex_unlock(&prs->lock);
	x[1] = val;
	rc = lps331ap_prs_i2c_write(prs, x, sizeof(x));
	if (rc != 0) {
		dev_err(&prs->client->dev, "attr_reg_set failed: %d\n", rc);
		return rc;
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&prs->lock);
	data = prs->reg_addr;
	mutex_unlock(&prs->lock);
	rc = lps331ap_prs_i2c_read(prs, &data, 1);
	if (rc != 0)
		dev_err(&prs->client->dev, "attr_reg_get failed: %d\n", rc);
	ret = snprintf(buf, PAGE_SIZE, "0x%02x\n", data);
	return rc;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct lps331ap_prs_data *prs = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&prs->lock);
	prs->reg_addr = val;
	mutex_unlock(&prs->lock);
	return size;
}
#endif

static struct device_attribute attributes[] = {
	__ATTR(poll_period_ms, S_IRUGO | S_IWUSR | S_IWGRP,
		attr_get_polling_rate,
		attr_set_polling_rate),
	__ATTR(enable_device, S_IRUGO, attr_get_enable, NULL),
	__ATTR(pressure_reference_level, S_IRUGO | S_IWUSR | S_IWGRP,
		attr_get_press_ref,
		attr_set_press_ref),
	__ATTR(temperature_reference_level, S_IRUGO | S_IWUSR | S_IWGRP,
		attr_get_temperature_ref,
		attr_set_temperature_ref),
	__ATTR(enable_autozero, S_IWUSR | S_IWGRP, NULL, attr_set_autozero),
#ifdef DEBUG
	__ATTR(reg_value, S_IRUGO | S_IWUSR | S_IWGRP, attr_reg_get,
		attr_reg_set),
	__ATTR(reg_addr, S_IWUSR | S_IWGRP, NULL, attr_addr_set),
#endif
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(attributes); i++) {
		ret = device_create_file(dev, attributes + i);
		if (ret < 0)
			goto error;
	}
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return ret;
}

static void remove_sysfs_interfaces(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
}

static void lps331ap_prs_input_work_func(struct work_struct *work)
{
	struct lps331ap_prs_data *prs = container_of(
			(struct delayed_work *)work,
			struct lps331ap_prs_data,
			input_work);

	struct outputdata output = {0, 0};
	int err;

	err = lps331ap_prs_get_presstemp_data(prs, &output);
	if (err < 0)
		dev_err(&prs->client->dev, "get_pressure_data failed\n");
	else
		lps331ap_prs_report_values(prs, &output);

	schedule_delayed_work(&prs->input_work,
				msecs_to_jiffies(prs->poll_interval));
}

int lps331ap_prs_input_open(struct input_dev *input)
{
	struct lps331ap_prs_data *prs = input_get_drvdata(input);
	int err;

	mutex_lock(&prs->lock);
	err = lps331ap_prs_device_power_on(prs);
	if (!err)
		schedule_delayed_work(&prs->input_work,
			msecs_to_jiffies(prs->poll_interval));
	mutex_unlock(&prs->lock);
	return err;
}

void lps331ap_prs_input_close(struct input_dev *dev)
{
	struct lps331ap_prs_data *prs = input_get_drvdata(dev);

	mutex_lock(&prs->lock);
	cancel_delayed_work_sync(&prs->input_work);
	if (prs->enabled)
		lps331ap_prs_device_power_off(prs);
	mutex_unlock(&prs->lock);
}

static int lps331ap_prs_input_init(struct lps331ap_prs_data *prs)
{
	int err;

	INIT_DELAYED_WORK(&prs->input_work, lps331ap_prs_input_work_func);
	prs->input_dev = input_allocate_device();
	if (!prs->input_dev) {
		err = -ENOMEM;
		dev_err(&prs->client->dev, "input device allocate failed\n");
		goto err_input_alloc_failed;
	}

	prs->input_dev->open = lps331ap_prs_input_open;
	prs->input_dev->close = lps331ap_prs_input_close;
	prs->input_dev->name = LPS331AP_PRS_DEV_NAME;
	prs->input_dev->id.bustype = BUS_I2C;

	input_set_drvdata(prs->input_dev, prs);

	set_bit(EV_ABS, prs->input_dev->evbit);

	input_set_abs_params(prs->input_dev, ABS_PRESSURE,
			PR_ABS_MIN, PR_ABS_MAX, FUZZ, FLAT);

	input_set_abs_params(prs->input_dev, ABS_MISC,
			TEMP_MIN, TEMP_MAX, FUZZ, FLAT);

	err = input_register_device(prs->input_dev);
	if (err) {
		dev_err(&prs->client->dev,
			"unable to register input polled device %s\n",
			prs->input_dev->name);
		goto err_input_cleanup;
	}

	return 0;

err_input_cleanup:
	input_free_device(prs->input_dev);
err_input_alloc_failed:
	return err;
}

static int lps331ap_prs_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct lps331ap_prs_data *prs;
	int err;
	int tempvalue;

	dev_info(&client->dev, "%s: probe start.\n", LPS331AP_PRS_DEV_NAME);

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODATA;
		goto err_exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable\n");
		err = -ENODEV;
		goto err_exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter,
					I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "client not smb-i2c capable:2\n");
		err = -EIO;
		goto err_exit_check_functionality_failed;
	}

	prs = kzalloc(sizeof(struct lps331ap_prs_data), GFP_KERNEL);
	if (prs == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"failed to allocate memory for module data: "
			"%d\n", err);
		goto err_exit_alloc_data_failed;
	}

	mutex_init(&prs->lock);

	i2c_set_clientdata(client, prs);
	prs->client = client;
	prs->pdata = client->dev.platform_data;
	if (!prs->pdata) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for pdata: %d\n",
				err);
		goto err_pdata;
	}

	err = i2c_smbus_read_byte(client);
	if (err < 0) {
		dev_err(&client->dev, "i2c_smbus_read_byte error!!\n");
		goto err_io;
	} else {
		dev_info(&client->dev, "%s: Device detected!\n",
				LPS331AP_PRS_DEV_NAME);
	}

	/* read chip id */
	tempvalue = i2c_smbus_read_word_data(client, WHO_AM_I);
	if ((tempvalue & 0x00FF) == WHOAMI_LPS331AP_PRS) {
		dev_info(&client->dev, "%s: I2C driver registered!\n",
				LPS331AP_PRS_DEV_NAME);
	} else {
		prs->client = NULL;
		err = -ENODEV;
		dev_err(&client->dev, "I2C driver not registered."
				" Device unknown: %d\n", err);
		goto err_whoami;
	}

	if (prs->pdata->poll_interval < prs->pdata->min_interval)
		prs->poll_interval = prs->pdata->poll_interval;
	else
		prs->poll_interval = prs->pdata->min_interval;

	if (prs->pdata->init) {
		err = prs->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err_exit_pointer;
		}
	}

	/* init registers which need values different from zero */
	prs->resume_state[LPS331AP_RES_CTRL_REG1] =
		(LPS331AP_PRS_ENABLE_MASK | LPS331AP_PRS_PM_NORMAL) |
			(LPS331AP_PRS_ODR_MASK & LPS331AP_PRS_ODR_1_1);

	err = lps331ap_prs_device_power_on(prs);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err_exit_pointer;
	}

	lps331ap_prs_device_power_off(prs);

	err = lps331ap_prs_input_init(prs);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_input;
	}

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev, "sysfs register failed\n");
		goto err_input_cleanup;
	}

	err = sysfs_create_link(&prs->input_dev->dev.kobj,
			  &client->dev.kobj,
			  "device");
	if (err < 0) {
		dev_err(&client->dev, "create link failed\n");
		goto err_sysfs_cleanup;
	}

	dev_info(&client->dev, "%s: probed\n", LPS331AP_PRS_DEV_NAME);

	return 0;

err_sysfs_cleanup:
	remove_sysfs_interfaces(&client->dev);
err_input_cleanup:
	input_unregister_device(prs->input_dev);
err_input:
err_exit_pointer:
	if (prs->pdata->exit)
		prs->pdata->exit();
err_whoami:
err_io:
err_pdata:
	kfree(prs);
err_exit_alloc_data_failed:
err_exit_check_functionality_failed:
	dev_err(&client->dev, "%s Driver Init failed\n", LPS331AP_PRS_DEV_NAME);
	return err;
}

static int __devexit lps331ap_prs_remove(struct i2c_client *client)
{
	struct lps331ap_prs_data *prs = i2c_get_clientdata(client);

	input_unregister_device(prs->input_dev);
	lps331ap_prs_device_power_off(prs);
	remove_sysfs_interfaces(&client->dev);
	sysfs_remove_link(&prs->input_dev->dev.kobj, "device");

	if (prs->pdata->exit)
		prs->pdata->exit();
	kfree(prs);

	return 0;
}

#ifdef CONFIG_PM
static int lps331ap_prs_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lps331ap_prs_data *prs;
	prs = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "Resuming LPS331AP\n");
	mutex_lock(&prs->lock);
	if (prs->input_dev->users) {
		if (!lps331ap_prs_device_power_on(prs))
			schedule_delayed_work(&prs->input_work,
				msecs_to_jiffies(prs->poll_interval));
	}
	mutex_unlock(&prs->lock);
	return 0;
}

static int lps331ap_prs_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct lps331ap_prs_data *prs;
	prs = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "Suspending LPS331AP\n");

	if (!mutex_trylock(&prs->lock))
		return -EAGAIN;

	cancel_delayed_work_sync(&prs->input_work);
	if (prs->enabled)
		lps331ap_prs_device_power_off(prs);
	mutex_unlock(&prs->lock);

	return 0;
}

static const struct dev_pm_ops lps331ap_pm = {
	.suspend  = lps331ap_prs_suspend,
	.resume   = lps331ap_prs_resume,
};

#else

#define lps001ap_prs_resume NULL
#define lps001ap_prs_suspend NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id lps331ap_prs_id[]
		= { { LPS331AP_PRS_DEV_NAME, 0}, { },};

MODULE_DEVICE_TABLE(i2c, lps331ap_prs_id);

static struct i2c_driver lps331ap_prs_driver = {
	.driver = {
			.name = LPS331AP_PRS_DEV_NAME,
			.owner = THIS_MODULE,
#ifdef CONFIG_PM
			.pm =  &lps331ap_pm,
#endif
	},
	.probe = lps331ap_prs_probe,
	.remove = __devexit_p(lps331ap_prs_remove),
	.id_table = lps331ap_prs_id,
};

static int __init lps331ap_prs_init(void)
{
	pr_debug("%s barometer driver: init\n", LPS331AP_PRS_DEV_NAME);
	return i2c_add_driver(&lps331ap_prs_driver);
}

static void __exit lps331ap_prs_exit(void)
{
	pr_debug("%s barometer driver: exit\n", LPS331AP_PRS_DEV_NAME);
	i2c_del_driver(&lps331ap_prs_driver);
	return;
}

module_init(lps331ap_prs_init);
module_exit(lps331ap_prs_exit);

MODULE_DESCRIPTION("STMicrolelectronics lps331ap pressure sensor sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Carmine Iascone, STMicroelectronics");
MODULE_LICENSE("GPL");
