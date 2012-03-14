/* [kernel/arch/arm/mach-msm/simple_remote_msm7x30_pf.c]
 *
 * Copyright (C) [2010] Sony Ericsson Mobile Communications AB.
 *
 * Authors: Takashi Shiina <takashi.shiina@sonyericsson.com>
 *          Tadashi Kubo <tadashi.kubo@sonyericsson.com>
 *          Joachim Holst <joachim.holst@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */


#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <asm/atomic.h>
#include <linux/bitops.h>
#include <mach/gpio.h>
#include <mach/pmic.h>

#ifdef CONFIG_CRADLE_SUPPORT
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <asm/atomic.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/bitops.h>
#include <linux/switch.h>
#endif /* CONFIG_CRADLE_SUPPORT */

#include "proc_comm.h"
#include <linux/simple_remote.h>
#include <mach/simple_remote_msm7x30_pf.h>

#define HEADSET_BUTTON_ID       0x84
#define HEADSET_BUTTON_PRESS    0x00
#define HEADSET_BUTTON_RELEASE  0xFF

#define IS_BTN_PRESSED BIT(1)
#define DET_INTERRUPT_ENABLED BIT(2)

#ifdef CONFIG_CRADLE_SUPPORT
#define CRADLE_ADC_MAX 1630
#define CRADLE_ADC_MIN 900
#define NUM_DETECT_ITERATIONS 15 /* Detection will run max X * 200ms */
#endif /* CONFIG_CRADLE_SUPPORT */

struct params {
	unsigned int hr_value;
	unsigned int enum_value;
};

static const struct params period_time_vals[] = {
	{
		.hr_value = 1,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_1_CLK_CYCLES,
	},
	{
		.hr_value = 2,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_2_CLK_CYCLES,
	},
	{
		.hr_value = 3,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_3_CLK_CYCLES,
	},
	{
		.hr_value = 4,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_4_CLK_CYCLES,
	},
	{
		.hr_value = 5,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_5_CLK_CYCLES,
	},
	{
		.hr_value = 6,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_6_CLK_CYCLES,
	},
	{
		.hr_value = 7,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_7_CLK_CYCLES,
	},
	{
		.hr_value = 8,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_8_CLK_CYCLES,
	},
	{
		.hr_value = 9,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_9_CLK_CYCLES,
	},
	{
		.hr_value = 10,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_10_CLK_CYCLES,
	},
	{
		.hr_value = 11,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_11_CLK_CYCLES,
	},
	{
		.hr_value = 12,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_12_CLK_CYCLES,
	},
	{
		.hr_value = 13,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_13_CLK_CYCLES,
	},
	{
		.hr_value = 14,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_14_CLK_CYCLES,
	},
	{
		.hr_value = 15,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_15_CLK_CYCLES,
	},
	{
		.hr_value = 16,
		.enum_value = (unsigned int)PM_HSED_PERIOD_TIME_16_CLK_CYCLES,
	},
};

static const struct params hyst_time_vals[] = {
	{
		.hr_value = 1,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_1_CLK_CYCLES
	},
	{
		.hr_value = 2,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_2_CLK_CYCLES
	},
	{
		.hr_value = 3,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_3_CLK_CYCLES
	},
	{
		.hr_value = 4,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_4_CLK_CYCLES
	},
	{
		.hr_value = 5,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_5_CLK_CYCLES
	},
	{
		.hr_value = 6,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_6_CLK_CYCLES
	},
	{
		.hr_value = 7,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_7_CLK_CYCLES
	},
	{
		.hr_value = 8,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_8_CLK_CYCLES
	},
	{
		.hr_value = 9,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_9_CLK_CYCLES
	},
	{
		.hr_value = 10,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_10_CLK_CYCLES
	},
	{
		.hr_value = 11,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_11_CLK_CYCLES
	},
	{
		.hr_value = 12,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_12_CLK_CYCLES
	},
	{
		.hr_value = 13,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_13_CLK_CYCLES
	},
	{
		.hr_value = 14,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_14_CLK_CYCLES
	},
	{
		.hr_value = 15,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_15_CLK_CYCLES
	},
	{
		.hr_value = 16,
		.enum_value = (unsigned int)PM_HSED_HYST_TIME_16_CLK_CYCLES
	},
};

static const struct params period_freq_vals[] = {
	{
		.hr_value = 4,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_256,
	},
	{
		.hr_value = 8,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_128,
	},
	{
		.hr_value = 16,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_64,
	},
	{
		.hr_value = 32,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_32,
	},
	{
		.hr_value = 64,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_16,
	},
	{
		.hr_value = 128,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_8,
	},
	{
		.hr_value = 256,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_4,
	},
	{
		.hr_value = 512,
		.enum_value = (unsigned int)PM_HSED_PERIOD_PRE_DIV_2,
	},
};

static const struct params hyst_freq_vals[] = {
	{
		.hr_value = 8,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_128,
	},
	{
		.hr_value = 16,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_64,
	},
	{
		.hr_value = 32,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_32,
	},
	{
		.hr_value = 64,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_16,
	},
	{
		.hr_value = 128,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_8,
	},
	{
		.hr_value = 256,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_4,
	},
	{
		.hr_value = 512,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_2,
	},
	{
		.hr_value = 1024,
		.enum_value = (unsigned int)PM_HSED_HYST_PRE_DIV_1,
	},
};

#ifdef CONFIG_CRADLE_SUPPORT
struct cradle_data {
	struct mutex cradle_detect_mutex;

	/* Since Work func will sleep, this should be placed on it's own WQ */
	struct delayed_work cradle_work;
	struct workqueue_struct *cradle_queue;

	int detect_iterations;
	int allow_suspend; /* Will be set to -EAGAIN if suspend not allowed */
	int dock_state;
	int last_dock_state;

	atomic_t docked;
	atomic_t adc_done;

	struct switch_dev cradle_dev;

	struct timer_list cradle_stop_timer;

	/* When we use cradle support, we need to intercept some interrupts
	 * For this purpose, we need to store away the original IRQ handler
	 * so we can call it when required. */
	irq_handler_t cradle_plug_det_cb_func;
	void *real_plug_det_data;
};
#endif /* CONFIG_CRADLE_SUPPORT */

struct local_data {
	struct simple_remote_platform_data *jack_pf;
	long unsigned int simple_remote_pf_flags;
	spinlock_t simple_remote_pf_lock;
	irq_handler_t simple_remote_btn_det_cb_func;
	void *simple_remote_vad_data;

	unsigned int trigger_level;
	enum hsed_period_pre_div period_pre_div;
	enum hsed_period_time period_time;
	enum hsed_hyst_pre_div hyst_pre_div;
	enum hsed_hyst_time hyst_time;

	struct device *dev;

#ifdef CONFIG_CRADLE_SUPPORT
	struct cradle_data cradle;
#endif /* CONFIG_CRADLE_SUPPORT */
};

static struct local_data *loc_dat;

#ifdef CONFIG_CRADLE_SUPPORT
static int simple_remote_pf_get_current_plug_status(u8 *status);
static irqreturn_t cradle_irq_btn_det_irq_interceptor(int irq, void *dev_id);
static int simple_remote_pf_register_hssd_button_interrupt(irq_handler_t func,
							   void *data);
static void simple_remote_pf_unregister_hssd_button_interrupt(void *data);
static int simple_remote_pf_enable_mic_bias(unsigned int enable);

static int get_cradle_adc(uint *adc_value)
{


	int err = msm_proc_comm(PCOM_OEM_GET_CRADLE_ADC_VALUE, adc_value, 0);
	/*
	 * Read twice, since we cannot trust the first read,
	 * seems to get it wrong
	 */
	err = msm_proc_comm(PCOM_OEM_GET_CRADLE_ADC_VALUE, adc_value, 0);

	dev_dbg(loc_dat->dev, "%s - Cradle ADC value = %u\n",
		__func__, *adc_value);

	return err;
}


/* The work function is responsible for detecting if the cradle is attached
 * or not. If it's attached, it will wait until the generic layer is done
 * with plug and accessory detection and then initialize HSED to handle new
 * plug detection. Whend HSED is used for plug detection, we need to intercept
 * the HSED interrupt in order to trigger the generic layers plug detection.
 * That way, the generic layer will perform it's work and correctly detect
 * the accessory that has been inserted into the cradle. */
static void cradle_detect_work(struct work_struct *work)
{
	uint adc_val;
	int i;
	u8 getgpiovalue;

	get_cradle_adc(&adc_val);

	simple_remote_pf_get_current_plug_status(&getgpiovalue);

	mutex_lock(&loc_dat->cradle.cradle_detect_mutex);
	/* Detecting if cradle is connected. */
	if (CRADLE_ADC_MAX >= adc_val && CRADLE_ADC_MIN <= adc_val
			&& !getgpiovalue) {
		dev_dbg(loc_dat->dev, "Cradle detected.\n");
		loc_dat->cradle.dock_state = 1;
		loc_dat->cradle.last_dock_state = 1;
		switch_set_state(&loc_dat->cradle.cradle_dev,
				 loc_dat->cradle.dock_state);
	} else {
		dev_dbg(loc_dat->dev,"Cradle not detected\n");
		/* If no cradle is detected here, restart the detection scheme
		 * a bit later just to try again. */
		if (NUM_DETECT_ITERATIONS > loc_dat->cradle.detect_iterations) {
			queue_delayed_work(loc_dat->cradle.cradle_queue,
					   &loc_dat->cradle.cradle_work,
					   msecs_to_jiffies(200));
			loc_dat->cradle.detect_iterations++;
			loc_dat->cradle.dock_state = 0;
			switch_set_state(&loc_dat->cradle.cradle_dev,
					 loc_dat->cradle.dock_state);
			goto out;
		}
	}

	if (loc_dat->cradle.dock_state) {
		/* Wait for ADC Read from generic layer to end */
		while(!atomic_read(&loc_dat->cradle.adc_done))
			msleep(300);

		/* Enable HSED interrupt here */
		simple_remote_pf_enable_mic_bias(1);

		i= simple_remote_pf_register_hssd_button_interrupt(
			cradle_irq_btn_det_irq_interceptor,
			NULL);

		if (i)
			dev_err(loc_dat->dev,
				"%s - Failed to enable HSED interrupt\n",
				__func__);
		else
			atomic_set(&loc_dat->cradle.docked, 1);

	} else if (!loc_dat->cradle.dock_state &&
		 loc_dat->cradle.last_dock_state) {
		simple_remote_pf_enable_mic_bias(0);
		simple_remote_pf_unregister_hssd_button_interrupt(NULL);
		atomic_set(&loc_dat->cradle.docked, 0);
		loc_dat->cradle.last_dock_state = 0;
	}

	loc_dat->cradle.allow_suspend = 0;

out:
	mutex_unlock(&loc_dat->cradle.cradle_detect_mutex);

}


/* If we get a plug_detect IRQ, we need to start the workqueue that will handle
 * detection of the docking cradle. We also need to activate the generic layer's
 * plug detect functionality, so we intercept the interrupt, start the local
 * workqueue and transfer the interrupt date up to the logic layer. */
static irqreturn_t cradle_irq_plug_det_irq_interceptor(int irq, void *dev_id)
{
	irqreturn_t err = IRQ_HANDLED;

	atomic_set(&loc_dat->cradle.adc_done, 0);

	mutex_lock(&loc_dat->cradle.cradle_detect_mutex);

	loc_dat->cradle.detect_iterations = 0;
	loc_dat->cradle.allow_suspend = -EAGAIN;

	if (delayed_work_pending(&loc_dat->cradle.cradle_work))
		cancel_delayed_work(&loc_dat->cradle.cradle_work);

	queue_delayed_work(loc_dat->cradle.cradle_queue,
			   &loc_dat->cradle.cradle_work,
			   msecs_to_jiffies(500));

	if (loc_dat->cradle.cradle_plug_det_cb_func)
		err = loc_dat->cradle.cradle_plug_det_cb_func(irq, dev_id);

	mutex_unlock(&loc_dat->cradle.cradle_detect_mutex);

	return err;
}


/* This function is used to re-route a button IRQ to a plug_det IRQ in the
 * generic layer. This is required by cradle support in order to handle plug
 * detection in the cradle */
static irqreturn_t cradle_irq_btn_det_irq_interceptor(int irq, void *dev_id)
{
	/* We need to re-route button detect IRQ's to plug detect IRQ's if
	 * we have a cradle connected. We do this here. */
	irqreturn_t err = IRQ_HANDLED;

	mutex_lock(&loc_dat->cradle.cradle_detect_mutex);

	if (loc_dat->cradle.cradle_plug_det_cb_func)
		err = loc_dat->cradle.cradle_plug_det_cb_func(
			irq, loc_dat->cradle.real_plug_det_data);

	mutex_unlock(&loc_dat->cradle.cradle_detect_mutex);

	return err;
}


static ssize_t cradle_print_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "Cradle : %s\n",
		       switch_get_state(&loc_dat->cradle.cradle_dev) ?
		       "Attached" : "Detached");
}


void cradle_detect_tmr_func(unsigned long func_data)
{
	atomic_set(&loc_dat->cradle.adc_done, 1);
}


static int init_cradle_data(struct cradle_data *dat)
{
	int ret = 0;

	mutex_init(&dat->cradle_detect_mutex);
	dat->cradle_queue = create_singlethread_workqueue("cradle_work");
	INIT_DELAYED_WORK(&dat->cradle_work, cradle_detect_work);

	dat->cradle_stop_timer.function = cradle_detect_tmr_func;
	init_timer(&dat->cradle_stop_timer);

	dat->cradle_dev.name = "dock";
	dat->cradle_dev.print_name = cradle_print_name;

	ret = switch_dev_register(&dat->cradle_dev);
	if (ret)
		dev_err(loc_dat->dev, "switch_dev_register for cradle device"
			" failed.\n");

	switch_set_state(&loc_dat->cradle.cradle_dev,
			 loc_dat->cradle.dock_state);

	return ret;
}

/* Here, we destroy and delete all thing required by cradle support.
 * It's better to create a function for this than to add everything
 * within ifdefs in the destroy function., */
static int destroy_cradle_data(struct cradle_data *dat)
{
	destroy_workqueue(dat->cradle_queue);
	switch_dev_unregister(&dat->cradle_dev);

	return 0;
}

int cradle_suspend(struct platform_device *pdev, pm_message_t state)
{
	return loc_dat->cradle.allow_suspend;
}
#endif /* CONFIG_CRADLE_SUPPORT */

static int get_param_value(const struct params *parm, int parm_size,
			   u8 enum_val, unsigned int value)
{
	int i;

	for (i = 0; i < parm_size; i++) {
		if (enum_val) {
			if (parm[i].hr_value >= value)
				return parm[i].enum_value;
		} else {
			if (value == parm[i].enum_value)
				return parm[i].hr_value;
		}
	}

	return -EINVAL;
}


static int simple_remote_pf_enable_button_isr(unsigned int enable)
{
	return msm_proc_comm(PCOM_OEM_ENABLE_HSED_ISR, &enable, 0);
}


static int simple_remote_pf_read_hsd_adc(unsigned int *adc_value)
{
	int err = msm_proc_comm(PCOM_OEM_GET_HEADSET_ADC_VALUE, adc_value, 0);
	/*
	 * Read twice, since we cannot trust the first read,
	 * seems to get it wrong
	 */
	err = msm_proc_comm(PCOM_OEM_GET_HEADSET_ADC_VALUE, adc_value, 0);

#ifdef CONFIG_CRADLE_SUPPORT
	/* This timer will kick in once the generic layer stops  requesting
	 * ADC values from headset. This will then trigger the work function
	 *  to continue and enable HSED interrupt that will be used for plug
	 * detect if a cradle is inserted */
	mod_timer(&loc_dat->cradle.cradle_stop_timer,
		  jiffies + msecs_to_jiffies(300));
#endif /* CONFIG_CRADLE_SUPPORT */

	return err;
}


static int simple_remote_pf_enable_mic_bias(unsigned int enable)
{
	int err;

#ifdef CONFIG_CRADLE_SUPPORT
	if (atomic_read(&loc_dat->cradle.docked) && !enable) {
		dev_dbg(loc_dat->dev,
			 "%s - Cradle docked. Not disabling MIC Bias\n",
		       __func__);
		return 0;
	}
#endif /* CONFIG_CRADLE_SUPPORT */

	err = pmic_hsed_enable(loc_dat->jack_pf->controller,
		enable ? PM_HSED_ENABLE_ALWAYS : PM_HSED_ENABLE_OFF);
	if (err)
		dev_err(loc_dat->dev, "Unable to toggle HSED\n");
	return err;
}


static int simple_remote_pf_set_period_freq(unsigned int value)
{
	int err = -EINVAL;
	int ret_val = get_param_value(period_freq_vals,
				ARRAY_SIZE(period_freq_vals), 1, value);

	if (ret_val < 0)
		return ret_val;

	err = pmic_hsed_set_period(loc_dat->jack_pf->controller,
				   (enum hsed_period_pre_div)ret_val,
				   loc_dat->period_time);

	if (!err)
		loc_dat->period_pre_div = (enum hsed_period_pre_div)ret_val;
	else
		dev_err(loc_dat->dev, "%s - Failed to set PMIC value (%u)\n",
			__func__, ret_val);
	return err;
}


static int simple_remote_pf_set_period_time(unsigned int value)
{
	int err = -EINVAL;
	int ret_val = get_param_value(period_time_vals,
				ARRAY_SIZE(period_time_vals), 1, value);

	if (ret_val < 0)
		return ret_val;

	err = pmic_hsed_set_period(loc_dat->jack_pf->controller,
				   loc_dat->period_pre_div,
				   (enum hsed_period_time)ret_val);

	if (!err)
		loc_dat->period_time = (enum hsed_period_time)ret_val;

	return err;
}


static int simple_remote_pf_set_hysteresis_freq(unsigned int value)
{
	int err = -EINVAL;
	int ret_val = get_param_value(hyst_freq_vals, ARRAY_SIZE(hyst_freq_vals),
				1, value);

	if (ret_val < 0)
		return ret_val;

	err = pmic_hsed_set_hysteresis(loc_dat->jack_pf->controller,
				       (enum hsed_hyst_pre_div)ret_val,
				       loc_dat->hyst_time);

	if (!err)
		loc_dat->hyst_pre_div = (enum hsed_hyst_pre_div)ret_val;

	return err;
}


static int simple_remote_pf_set_hysteresis_time(unsigned int value)
{
	int err = -EINVAL;
	int ret_val = get_param_value(hyst_time_vals, ARRAY_SIZE(hyst_time_vals),
				     1, value);

	if (ret_val < 0)
		return ret_val;

	err = pmic_hsed_set_hysteresis(loc_dat->jack_pf->controller,
				       loc_dat->hyst_pre_div,
				       (enum hsed_hyst_time)ret_val);

	if (!err)
		loc_dat->hyst_time = (enum hsed_hyst_time)ret_val;

	return err;
}


static int simple_remote_pf_set_trig_level(unsigned int value)
{
	int ret = -EINVAL;

	if (value >= 200 && value <= 1700) {
		loc_dat->trigger_level = value;
		ret = pmic_hsed_set_current_threshold(
			loc_dat->jack_pf->controller,
			PM_HSED_SC_SWITCH_TYPE,
			loc_dat->trigger_level);
	} else {
		dev_warn(loc_dat->dev, "Trig level out of range\n");
	}

	return ret;
}


static int simple_remote_pf_get_period_freq(unsigned int *value)
{
	int val = get_param_value(period_freq_vals,
				  ARRAY_SIZE(period_freq_vals), 0,
				  (unsigned int)loc_dat->period_pre_div);

	if (0 > val)
		return val;

	*value = (unsigned int)val;

	return 0;
}


static int simple_remote_pf_get_period_time(unsigned int *value)
{
	int val = get_param_value(period_time_vals,
				  ARRAY_SIZE(period_time_vals), 0,
				  (unsigned int)loc_dat->period_time);

	if (0 > val)
		return val;

	*value = (unsigned int)val;

	return 0;
}


static int simple_remote_pf_get_hysteresis_freq(unsigned int *value)
{
	int val = get_param_value(hyst_freq_vals, ARRAY_SIZE(hyst_freq_vals),
				  0, (unsigned int)loc_dat->hyst_pre_div);

	if (0 > val)
		return val;

	*value = (unsigned int)val;

	return 0;
}


static int simple_remote_pf_get_hysteresis_time(unsigned int *value)
{
	int val = get_param_value(hyst_time_vals, ARRAY_SIZE(hyst_time_vals),
				  0, (unsigned int)loc_dat->hyst_time);

	if (0 > val)
		return val;

	*value = (unsigned int)val;

	return 0;
}


static int simple_remote_pf_get_trig_level(unsigned int *value)
{
	*value = loc_dat->trigger_level;
	return 0;
}


static int simple_remote_pf_get_current_plug_status(u8 *status)
{
	*status = gpio_get_value(loc_dat->jack_pf->headset_detect_read_pin);
	if (loc_dat->jack_pf->invert_plug_det)
		*status = !(*status);

	return 0;
}


static int simple_remote_pf_register_plug_detect_interrupt(irq_handler_t func,
							   void *data)
{
	int err = -EALREADY;
	int irq = 0;
	irq_handler_t regfunc = func;

#ifdef CONFIG_CRADLE_SUPPORT
	/* Swap IRQ handlers so that we intercept the plug detect IRQ
	 * in order to start the cradle detect functionality. Also
	 * store the original function so that we can forward the
	 * IRQ to this handler. */
	regfunc = cradle_irq_plug_det_irq_interceptor;
	loc_dat->cradle.cradle_plug_det_cb_func = func;
	loc_dat->cradle.real_plug_det_data = data;
#endif

	if (test_bit(DET_INTERRUPT_ENABLED, &loc_dat->simple_remote_pf_flags))
		return err;

	irq = gpio_to_irq(loc_dat->jack_pf->headset_detect_read_pin);
	if (0 <= irq) {
		err = request_threaded_irq(irq, NULL, regfunc,
					   IRQF_TRIGGER_FALLING |
					   IRQF_TRIGGER_RISING,
					   "simple_remote_plug_detect",
					   data);

		if (err) {
			dev_crit(loc_dat->dev, "Failed to subscribe to plug "
				 "detect interrupt\n");
			return err;
			}
	} else {
		dev_crit(loc_dat->dev, "Failed to register interrupt for GPIO "
			 "(%d). GPIO Does not exist\n",
			 loc_dat->jack_pf->headset_detect_read_pin);
		return irq;
	}

	/*
	 * Setting interrupt enabled here, will in worst case generate
	 * a "unmatched irq_wake" print in the kernel log when shutting
	 * down the system, but at least some detection will work.
	 */
	set_bit(DET_INTERRUPT_ENABLED, &loc_dat->simple_remote_pf_flags);

	err = enable_irq_wake(irq);
	if (err)
		dev_crit(loc_dat->dev,
			 "Failed to enable wakeup on interrupt\n");

	return err;
}


static void simple_remote_pf_unregister_plug_detect_interrupt(void *data)
{
	int irq;

	if (!test_bit(DET_INTERRUPT_ENABLED, &loc_dat->simple_remote_pf_flags))
		return;

	irq = gpio_to_irq(loc_dat->jack_pf->headset_detect_read_pin);
	if (0 <= irq) {
		disable_irq_wake(irq);
		free_irq(irq, data);
		clear_bit(DET_INTERRUPT_ENABLED,
			  &loc_dat->simple_remote_pf_flags);
	} else {
		dev_crit(loc_dat->dev, "Failed to disable plug detect interrupt"
			 ". GPIO (%d) does not exist\n",
			 loc_dat->jack_pf->headset_detect_read_pin);
	}
}

/* If we have a cradle connected, this function should never be called by
 * the generic layer. We won't have to do anything here. Register this interrupt
 * later on in the cradle workqueue */
static int simple_remote_pf_register_hssd_button_interrupt(irq_handler_t func,
						    void *data)
{
	int err;

	spin_lock(&loc_dat->simple_remote_pf_lock);
	loc_dat->simple_remote_btn_det_cb_func = func;
	loc_dat->simple_remote_vad_data = data;
	err = simple_remote_pf_enable_button_isr(1);
	spin_unlock(&loc_dat->simple_remote_pf_lock);

	return err;
}


static void simple_remote_pf_unregister_hssd_button_interrupt(void *data)
{
#ifdef CONFIG_CRADLE_SUPPORT
	if (atomic_read(&loc_dat->cradle.docked)) {
		return;
	}
#endif /* CONFIG_CRADLE_SUPPORT */

	spin_lock(&loc_dat->simple_remote_pf_lock);
	loc_dat->simple_remote_btn_det_cb_func = NULL;
	loc_dat->simple_remote_vad_data = NULL;
	simple_remote_pf_enable_button_isr(0);
	spin_unlock(&loc_dat->simple_remote_pf_lock);
}


void simple_remote_pf_button_handler(uint32_t key, uint32_t event)
{
	dev_vdbg(loc_dat->dev, "%s - Called\n", __func__);
	dev_vdbg(loc_dat->dev, "%s - key = 0x%X, event = 0x%X\n", __func__,
		 key, event);

	if (loc_dat->simple_remote_btn_det_cb_func == NULL)
		return;

	if (HEADSET_BUTTON_ID != key)
		return;


	switch (event) {
	case HEADSET_BUTTON_PRESS:
		if (!test_bit(IS_BTN_PRESSED,
			      &loc_dat->simple_remote_pf_flags)) {
			dev_dbg(loc_dat->dev, "%s - HEADSET_BUTTON_PRESS\n",
				__func__);
			set_bit(IS_BTN_PRESSED,
				&loc_dat->simple_remote_pf_flags);
			loc_dat->simple_remote_btn_det_cb_func(
				1, loc_dat->simple_remote_vad_data);
			}
			break;
	case HEADSET_BUTTON_RELEASE:
		dev_dbg(loc_dat->dev, "%s - HEADSET_BUTTON_RELEASE\n",
			__func__);
		clear_bit(IS_BTN_PRESSED, &loc_dat->simple_remote_pf_flags);
		loc_dat->simple_remote_btn_det_cb_func(
			1, loc_dat->simple_remote_vad_data);
		break;
	}
}


static struct simple_remote_pf_interface interface = {
	.read_hs_adc = simple_remote_pf_read_hsd_adc,
	.enable_mic_bias = simple_remote_pf_enable_mic_bias,
	.get_current_plug_status = simple_remote_pf_get_current_plug_status,

	.set_period_freq = simple_remote_pf_set_period_freq,
	.set_period_time = simple_remote_pf_set_period_time,
	.set_hysteresis_freq = simple_remote_pf_set_hysteresis_freq,
	.set_hysteresis_time = simple_remote_pf_set_hysteresis_time,
	.set_trig_level = simple_remote_pf_set_trig_level,

	.get_period_freq = simple_remote_pf_get_period_freq,
	.get_period_time = simple_remote_pf_get_period_time,
	.get_hysteresis_freq = simple_remote_pf_get_hysteresis_freq,
	.get_hysteresis_time = simple_remote_pf_get_hysteresis_time,
	.get_trig_level = simple_remote_pf_get_trig_level,

	.register_plug_detect_interrupt =
	simple_remote_pf_register_plug_detect_interrupt,

	.unregister_plug_detect_interrupt =
	simple_remote_pf_unregister_plug_detect_interrupt,

	.register_hssd_button_interrupt =
	simple_remote_pf_register_hssd_button_interrupt,

	.unregister_hssd_button_interrupt =
	simple_remote_pf_unregister_hssd_button_interrupt,
};


static struct platform_device simple_remote_device = {
	.name = SIMPLE_REMOTE_NAME,
	.dev = {
		.platform_data = &interface,
	},
};


static int simple_remote_pf_probe(struct platform_device *pdev)
{
	int ret;
	struct platform_device *n_pdev = &simple_remote_device;

	loc_dat = kzalloc(sizeof(*loc_dat), GFP_KERNEL);
	if (!loc_dat)
		return -ENOMEM;

	loc_dat->jack_pf = pdev->dev.platform_data;
	loc_dat->dev = &pdev->dev;

	ret = loc_dat->jack_pf->initialize(loc_dat->jack_pf);
	if (ret)
		goto out;

	spin_lock_init(&loc_dat->simple_remote_pf_lock);

	(void)simple_remote_pf_enable_mic_bias(0);
	(void)simple_remote_pf_enable_button_isr(0);

	ret = platform_add_devices(&n_pdev, 1);
	if (ret)
		goto out;

#ifdef CONFIG_CRADLE_SUPPORT
	if (init_cradle_data(&loc_dat->cradle))
		dev_err(loc_dat->dev, "Failed to initialize cradle support."
			"cradle will not work\n");
	else
#endif /* CONFIG_CRADLE_SUPPORT */
		dev_info(loc_dat->dev, "Successfully registered\n");

	return ret;

out:
	kfree(loc_dat);
	dev_err(&pdev->dev, "Failed to register driver\n");
	return ret;
}


static int simple_remote_pf_remove(struct platform_device *pdev)
{
	struct platform_device *n_pdev = &simple_remote_device;

	(void)simple_remote_pf_enable_mic_bias(0);
	(void)simple_remote_pf_enable_button_isr(0);

	platform_device_unregister(n_pdev);

	loc_dat->jack_pf->deinitialize(loc_dat->jack_pf);

#ifdef CONFIG_CRADLE_SUPPORT
	destroy_cradle_data(&loc_dat->cradle);
#endif /* CONFIG_CRADLE_SUPPORT */

	kfree(loc_dat);
	return 0;
}


static struct platform_driver simple_remote_pf = {
	.probe		= simple_remote_pf_probe,
	.remove		= simple_remote_pf_remove,
	.driver		= {
		.name		= SIMPLE_REMOTE_PF_NAME,
		.owner		= THIS_MODULE,
	},
#ifdef CONFIG_CRADLE_SUPPORT
	.suspend = cradle_suspend,
#endif /* CONFIG_CRADLE_SUPPORT */
};


static int __init simple_remote_pf_init(void)
{
	return platform_driver_register(&simple_remote_pf);
}


static void __exit simple_remote_pf_exit(void)
{
	platform_driver_unregister(&simple_remote_pf);
}

module_init(simple_remote_pf_init);
module_exit(simple_remote_pf_exit);

MODULE_AUTHOR("Takashi Shiina, Tadashi Kubo");
MODULE_DESCRIPTION("3.5mm audio jack platform driver");
MODULE_LICENSE("GPL");
