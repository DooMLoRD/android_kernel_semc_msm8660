/* drivers/input/misc/asetm2034a.c
 *
 * Copyright (C) 2011 Sony Ericsson Mobile Communications AB.
 *
 * Author: Yukito Naganuma <Yukito.X.Naganuma@sonyericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/pm.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif /* CONFIG_HAS_EARLYSUSPEND */
#include <linux/wait.h>
#include <asm/atomic.h>

#include <linux/input/asetm2034a.h>

/* erase ROM */
#define ASETM2034A_CMD_ERASEROM		0xA0

/* sleep mode */
#define ASETM2034A_CMD_SLEEP_W_CAL	0xD1
#define ASETM2034A_CMD_SLEEP_WO_CAL	0xD2
#define ASETM2034A_CMD_STARTUP		0xD3

/* idle2 mode */
#define ASETM2034A_CMD_TO_IDLE2		0xF1
#define ASETM2034A_CMD_TO_NORM		0xF2

/* data request */
#define ASETM2034A_CMD_REQDATA		0xFA

/* fw version */
#define ASETM2034A_CMD_GETFWVER		0xFB

/* soft reset */
#define ASETM2034A_CMD_SOFTRESET	0xFC

/* flash FW */
#define ASETM2034A_CMD_FLASHFW		0xFD

/* ROM SUM */
#define ASETM2034A_CMD_ROMSUM		0xFE

/* message type */
#define ASETM2034A_MSG_TYPE_DATA	0x80
#define ASETM2034A_MSG_TYPE_FWVER	0x83
#define ASETM2034A_MSG_TYPE_INITDONE	0x8A
#define ASETM2034A_MSG_TYPE_FLASHMODE	0xC0
#define ASETM2034A_MSG_TYPE_SUCCESS	0xC1
#define ASETM2034A_MSG_TYPE_ROMSUM	0xC2
#define ASETM2034A_MSG_TYPE_ERROR	0xC8

/* FW flash area = 0x0400-0x2000 */
#define ASETM2034A_FW_AREA_SIZE		7168
#define ASETM2034A_FW_WRITE_UNIT	128
#define ASETM2034A_FW_WRITE_COUNT \
	(ASETM2034A_FW_AREA_SIZE / ASETM2034A_FW_WRITE_UNIT)

/* receive timeout */
#define ASETM2034A_RECEIVE_TIMEOUT	(HZ / 2)

/* Driver state bit */
#define ASETM2034A_ENTER_IDLE2		0x01

/* Drvier Firmware Base */
#define ASETM2034A_FIRMWARE_BASE	0x2503

struct asetm2034a_data {
	u8 byte[3];
};

struct asetm2034a_drvdata {
	struct i2c_client *client;
	struct input_dev *input_dev;
	u8 sw_status;
	struct mutex lock;
	struct mutex fw_lock;
	struct completion complete;
	u16 fw_version;
	u16 fw_romsum;
	u8 recv_msg_type;
	struct wake_lock wlock;
	u8 *fw_buf;
	int fw_buf_size;
	wait_queue_head_t irq_wq;
	unsigned long drv_status;
	atomic_t suspend_lock;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif /* CONFIG_HAS_EARLYSUSPEND */
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs_dir;
	struct dentry *debugfs_send_cmd;
#endif /* CONFIG_DEBUG_FS */
};

static int asetm2034a_block_write(struct i2c_client *client,
				void *buf, int size)
{
	int rc;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = size;
	msg.buf = buf;
	rc = i2c_transfer(client->adapter, &msg, 1);
	msleep(1);
	if (rc != 1) {
		dev_err(&client->dev,
			"%s: i2c_transfer failed rc=%d\n", __func__, rc);
		if (rc >= 0)
			rc = -EIO;
	} else {
		rc = 0;
	}

	return rc;
}

static int asetm2034a_write(struct i2c_client *client, int command)
{
	int rc;
	u8 buf = (u8)command;

	rc = asetm2034a_block_write(client, &buf, sizeof(buf));

	return rc;
}

#ifdef CONFIG_DEBUG_FS
static int asetm2034a_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;

	return 0;
}

static ssize_t asetm2034a_debugfs_send_command(struct file *file,
		const char __user *buf, size_t count, loff_t *ppos)
{
	struct asetm2034a_drvdata *drvdata =
		(struct asetm2034a_drvdata *)file->private_data;
	char *string;
	unsigned long command;
	int rc;

	string = kzalloc((count + 1), GFP_KERNEL);
	if (!string)
		goto err_exit;

	if (copy_from_user(string, buf, count))
		goto err_free;

	rc = strict_strtoul(string, 16, &command);
	if (rc != 0)
		goto err_free;

	mutex_lock(&drvdata->lock);
	asetm2034a_write(drvdata->client, (int)command);
	mutex_unlock(&drvdata->lock);

err_free:
	kfree(string);
err_exit:
	return count;
}

const struct file_operations asetm2034a_debugfs_fops = {
	.open = asetm2034a_debugfs_open,
	.write = asetm2034a_debugfs_send_command,
};

static int asetm2034a_debugfs_init(struct asetm2034a_drvdata *drvdata)
{
	int rc = 0;

	drvdata->debugfs_dir = debugfs_create_dir("asetm2034a", NULL);
	if (!drvdata->debugfs_dir) {
		dev_err(&drvdata->client->dev,
			"%s: debugfs_create_dir failed\n", __func__);
		rc = -ENOENT;
		goto err_exit;
	}

	drvdata->debugfs_send_cmd = debugfs_create_file(
			"send_command", 0222, drvdata->debugfs_dir, drvdata,
			&asetm2034a_debugfs_fops);
	if (!drvdata->debugfs_send_cmd) {
		dev_err(&drvdata->client->dev,
			"%s: debugfs_create_file(send_command) failed\n",
			__func__);
		rc = -ENOENT;
		goto remove_dir;
	}

	return rc;

remove_dir:
	debugfs_remove(drvdata->debugfs_dir);
	drvdata->debugfs_dir = NULL;
err_exit:
	return rc;
}
#endif /* CONFIG_DEBUG_FS */

static int asetm2034a_read(struct i2c_client *client,
			struct asetm2034a_data *data)
{
	int rc = 0;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = ARRAY_SIZE(data->byte);
	msg.buf = data->byte;
	rc = i2c_transfer(client->adapter, &msg, 1);
	msleep(1);
	if (rc != 1) {
		dev_err(&client->dev,
			"%s: i2c_transfer failed rc=%d\n", __func__, rc);
		if (rc >= 0)
			rc = -EIO;
	} else {
		rc = 0;
	}

	return rc;
}

static irqreturn_t asetm2034a_worker(int irq, void *data)
{
	int rc;
	struct asetm2034a_drvdata *drvdata =
		(struct asetm2034a_drvdata *)data;
	struct asetm2034a_platform_data *pdata =
		drvdata->client->dev.platform_data;
	struct asetm2034a_data tmp_data;
	int i;
	struct asetm2034a_key *keys = pdata->keymap->keys;
	u8 prev_sw_status, cur_sw_status;
	int event_reported = 0;
	int gesture_reported = 0;

	wait_event(drvdata->irq_wq,
		!atomic_cmpxchg(&drvdata->suspend_lock, 0, 1));

	mutex_lock(&drvdata->lock);

	rc = asetm2034a_read(drvdata->client, &tmp_data);
	if (rc) {
		dev_err(&drvdata->client->dev,
			"%s: receive_sensor_data failed rc=%d\n",
			__func__, rc);
		goto exit;
	}

	dev_dbg(&drvdata->client->dev,
		"%s: [0x%02x, 0x%02x, 0x%02x]\n", __func__,
		tmp_data.byte[0], tmp_data.byte[1], tmp_data.byte[2]);

	switch (tmp_data.byte[0]) {
	case ASETM2034A_MSG_TYPE_DATA:
		for (i = 0 ; i < pdata->keymap->num_keys ; i++) {
			prev_sw_status = drvdata->sw_status & keys[i].sw_bit;
			cur_sw_status = tmp_data.byte[1] & keys[i].sw_bit;

			if (keys[i].sw_bit == ASETM2034A_IDLE2) {
				if (drvdata->fw_version
					>= ASETM2034A_FIRMWARE_BASE) {
					if (prev_sw_status != cur_sw_status)
						gesture_reported = 1;
				} else {
					if ((cur_sw_status == ASETM2034A_IDLE2)
						&& (test_bit(
							ASETM2034A_ENTER_IDLE2,
							&drvdata->drv_status)))
						gesture_reported = 1;
					tmp_data.byte[1] &= ~ASETM2034A_IDLE2;
				}
				if (gesture_reported) {
					input_report_key(drvdata->input_dev,
						keys[i].keycode, 1);
					input_report_key(drvdata->input_dev,
						keys[i].keycode, 0);
					event_reported = 1;
				}
			} else {
				if (prev_sw_status != cur_sw_status) {
					input_report_key(drvdata->input_dev,
						keys[i].keycode,
						(cur_sw_status ? 1 : 0));
					event_reported = 1;
				}
			}
		}
		if (event_reported)
			input_sync(drvdata->input_dev);
		drvdata->sw_status = tmp_data.byte[1];
		break;

	case ASETM2034A_MSG_TYPE_FWVER:
		drvdata->fw_version =
			 ((u16)tmp_data.byte[1] << 8) | (u16)tmp_data.byte[2];
		dev_info(&drvdata->client->dev,
			"ASETM2034A FW Ver: 0x%04x\n", drvdata->fw_version);
		break;

	case ASETM2034A_MSG_TYPE_INITDONE:
		dev_info(&drvdata->client->dev, "Initailization done\n");
		asetm2034a_write(drvdata->client, ASETM2034A_CMD_GETFWVER);
		asetm2034a_write(drvdata->client, ASETM2034A_CMD_ROMSUM);
		asetm2034a_write(drvdata->client, ASETM2034A_CMD_REQDATA);
		break;

	case ASETM2034A_MSG_TYPE_ROMSUM:
		drvdata->fw_romsum =
			((u16)tmp_data.byte[1] << 8) | (u16)tmp_data.byte[2];
		dev_info(&drvdata->client->dev,
			"ASETM2034A ROM SUM=0x%04x\n", drvdata->fw_romsum);
		break;

	case ASETM2034A_MSG_TYPE_FLASHMODE:
		dev_info(&drvdata->client->dev,
			"ASETM2034A Enter firmware flash mode\n");
		break;

	case ASETM2034A_MSG_TYPE_SUCCESS:
	case ASETM2034A_MSG_TYPE_ERROR:
		break;

	default:
		dev_err(&drvdata->client->dev,
			"%s: Unknown msg type (0x%02x)\n", __func__,
			tmp_data.byte[0]);
		break;
	}

	if (drvdata->recv_msg_type == tmp_data.byte[0])
		complete(&drvdata->complete);

exit:
	mutex_unlock(&drvdata->lock);
	atomic_set(&drvdata->suspend_lock, 0);

	return IRQ_HANDLED;
}

static ssize_t asetm2034a_fw_info_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct asetm2034a_drvdata *drvdata = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&drvdata->lock);
	ret = snprintf(buf, PAGE_SIZE,
			"FW_VER=0x%04x\n"
			"ROM_SUM=0x%04x\n",
			drvdata->fw_version, drvdata->fw_romsum);
	mutex_unlock(&drvdata->lock);

	return ret;
}

static int asetm2034a_wait(struct asetm2034a_drvdata *drvdata)
{
	if (!wait_for_completion_timeout(&drvdata->complete,
				ASETM2034A_RECEIVE_TIMEOUT))
		return -EIO;

	return 0;
}

static ssize_t asetm2034a_fw_data_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct asetm2034a_drvdata *drvdata = dev_get_drvdata(dev);
	ssize_t ret;

	mutex_lock(&drvdata->fw_lock);

	if ((drvdata->fw_buf_size + count) > ASETM2034A_FW_AREA_SIZE) {
		ret = -EFBIG;
		goto exit;
	}

	if (drvdata->fw_buf == NULL) {
		drvdata->fw_buf = kzalloc(ASETM2034A_FW_AREA_SIZE, GFP_KERNEL);
		if (drvdata->fw_buf == NULL) {
			ret = -ENOMEM;
			goto exit;
		}
	}

	memcpy((drvdata->fw_buf + drvdata->fw_buf_size), buf, count);

	drvdata->fw_buf_size += count;
	ret = count;

	dev_info(&drvdata->client->dev, "%s: firmware data %d Byte written\n",
					__func__, drvdata->fw_buf_size);

exit:
	mutex_unlock(&drvdata->fw_lock);

	return ret;
}

static ssize_t asetm2034a_fw_update_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct asetm2034a_drvdata *drvdata = dev_get_drvdata(dev);
	ssize_t ret = 0;
	int i;

	wake_lock(&drvdata->wlock);

	mutex_lock(&drvdata->fw_lock);

	if ((drvdata->fw_buf == NULL) ||
	    (drvdata->fw_buf_size != ASETM2034A_FW_AREA_SIZE)) {
		dev_err(&drvdata->client->dev,
			"ASETM2034A Firmware data not prepared\n");
		ret = -EINVAL;
		goto exit;
	}

	/* enter firmware flash mode */
	mutex_lock(&drvdata->lock);
	drvdata->recv_msg_type = ASETM2034A_MSG_TYPE_FLASHMODE;
	mutex_unlock(&drvdata->lock);
	ret = asetm2034a_write(drvdata->client, ASETM2034A_CMD_FLASHFW);
	if (ret)
		goto free_and_reset;
	ret = asetm2034a_wait(drvdata);
	if (ret)
		goto free_and_reset;

	/* erase ROM */
	mutex_lock(&drvdata->lock);
	drvdata->recv_msg_type = ASETM2034A_MSG_TYPE_SUCCESS;
	mutex_unlock(&drvdata->lock);
	ret = asetm2034a_write(drvdata->client, ASETM2034A_CMD_ERASEROM);
	if (ret)
		goto free_and_reset;
	ret = asetm2034a_wait(drvdata);
	if (ret)
		goto free_and_reset;

	/* write data */
	for (i = 0 ; i < ASETM2034A_FW_WRITE_COUNT ; i++) {
		mutex_lock(&drvdata->lock);
		drvdata->recv_msg_type = ASETM2034A_MSG_TYPE_SUCCESS;
		mutex_unlock(&drvdata->lock);
		ret = asetm2034a_block_write(drvdata->client,
			(drvdata->fw_buf + (i * ASETM2034A_FW_WRITE_UNIT)),
			ASETM2034A_FW_WRITE_UNIT);
		if (ret)
			goto free_and_reset;
		ret = asetm2034a_wait(drvdata);
		if (ret)
			goto free_and_reset;
	}

free_and_reset:
	kfree(drvdata->fw_buf);
	drvdata->fw_buf = NULL;
	drvdata->fw_buf_size = 0;
	drvdata->recv_msg_type = 0;

exit:
	mutex_unlock(&drvdata->fw_lock);

	if (ret < 0) {
		dev_err(&drvdata->client->dev,
			"ASETM2034A Firmware update failed ret=%d\n", ret);
	} else {
		dev_info(&drvdata->client->dev,
			"ASETM2034A Firmware update done\n");
		ret = count;
	}

	wake_unlock(&drvdata->wlock);

	return ret;
}

static ssize_t asetm2034a_hw_reset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct asetm2034a_drvdata *drvdata = dev_get_drvdata(dev);
	struct asetm2034a_platform_data *pdata =
			drvdata->client->dev.platform_data;

	mutex_lock(&drvdata->lock);
	pdata->hw_reset();
	mutex_unlock(&drvdata->lock);

	return count;
}

static struct device_attribute asetm2034a_attrs[] = {
	__ATTR(fw_info, 0444, asetm2034a_fw_info_show, NULL),
	__ATTR(fw_data, 0200, NULL, asetm2034a_fw_data_store),
	__ATTR(fw_update, 0200, NULL, asetm2034a_fw_update_store),
	__ATTR(hw_reset, 0200, NULL, asetm2034a_hw_reset_store),
};

static int asetm2034a_sysfs_create(struct device *dev)
{
	int i;
	int ret;

	for (i = 0 ; i < ARRAY_SIZE(asetm2034a_attrs); i++) {
		ret = device_create_file(dev, &asetm2034a_attrs[i]);
		if (ret < 0)
			goto revert;
	}

	return 0;

revert:
	for (; i >= 0 ; i--)
		device_remove_file(dev, &asetm2034a_attrs[i]);

	return ret;
}

#ifdef CONFIG_SUSPEND
static int asetm2034a_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct asetm2034a_drvdata *drvdata = i2c_get_clientdata(client);

	if (atomic_cmpxchg(&drvdata->suspend_lock, 0, 1))
		return -EAGAIN;

	if (device_may_wakeup(dev))
		enable_irq_wake(drvdata->client->irq);

	return 0;
}

static int asetm2034a_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct asetm2034a_drvdata *drvdata = i2c_get_clientdata(client);

	if (device_may_wakeup(dev))
		disable_irq_wake(drvdata->client->irq);

	atomic_set(&drvdata->suspend_lock, 0);
	wake_up(&drvdata->irq_wq);

	return 0;
}
#else /* CONFIG_SUSPEND */
#define asetm2034a_suspend	NULL
#define asetm2034a_resume	NULL
#endif /* CONFIG_SUSPEND */

static const struct dev_pm_ops asetm2034a_pm = {
	.suspend = asetm2034a_suspend,
	.resume = asetm2034a_resume,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void asetm2034a_early_suspend(struct early_suspend *handler)
{
	int rc;
	struct asetm2034a_drvdata *drvdata =
		container_of(handler, struct asetm2034a_drvdata, early_suspend);

	mutex_lock(&drvdata->lock);
	dev_info(&drvdata->client->dev, "%s\n", __func__);

	/* Enter idle2 mode */
	rc = asetm2034a_write(drvdata->client, ASETM2034A_CMD_TO_IDLE2);
	if (rc) {
		dev_err(&drvdata->client->dev,
			"Enter idle2 mode failed rc=%d\n", rc);
	} else {
		if (drvdata->fw_version >= ASETM2034A_FIRMWARE_BASE)
			drvdata->sw_status |= ASETM2034A_IDLE2;
		else
			set_bit(ASETM2034A_ENTER_IDLE2, &drvdata->drv_status);
	}

	mutex_unlock(&drvdata->lock);
}

static void asetm2034a_late_resume(struct early_suspend *handler)
{
	int rc;
	struct asetm2034a_drvdata *drvdata =
		container_of(handler, struct asetm2034a_drvdata, early_suspend);

	mutex_lock(&drvdata->lock);
	dev_info(&drvdata->client->dev, "%s\n", __func__);
	/* Enter normal mode */
	rc = asetm2034a_write(drvdata->client, ASETM2034A_CMD_TO_NORM);
	if (rc) {
		dev_err(&drvdata->client->dev,
			"Enter normal mode failed rc=%d\n", rc);
	} else {
		if (drvdata->fw_version >= ASETM2034A_FIRMWARE_BASE)
			drvdata->sw_status &= ~ASETM2034A_IDLE2;
		else
			clear_bit(ASETM2034A_ENTER_IDLE2, &drvdata->drv_status);
	}

	/* Data Request */
	rc = asetm2034a_write(drvdata->client, ASETM2034A_CMD_REQDATA);
	if (rc)
		dev_err(&drvdata->client->dev,
			"Data request failed rc=%d\n", rc);

	mutex_unlock(&drvdata->lock);
}
#endif /* CONFIG_HAS_EARLYSUSPEND */


static int asetm2034a_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int rc;
	struct asetm2034a_drvdata *drvdata = NULL;
	struct asetm2034a_platform_data *pdata = client->dev.platform_data;
	int i;

	if (pdata == NULL) {
		dev_err(&client->dev, "%s: no platform_data\n", __func__);
		rc = -EINVAL;
		goto err_exit;
	}

	drvdata = kzalloc(sizeof(struct asetm2034a_drvdata), GFP_KERNEL);
	if (!drvdata) {
		dev_err(&client->dev, "%s: kzalloc failed\n", __func__);
		rc = -ENOMEM;
		goto err_exit;
	}
	drvdata->client = client;

	mutex_init(&drvdata->lock);
	mutex_init(&drvdata->fw_lock);
	init_completion(&drvdata->complete);
	wake_lock_init(&drvdata->wlock, WAKE_LOCK_SUSPEND, ASETM2034A_NAME);

	drvdata->input_dev = input_allocate_device();
	if (!drvdata->input_dev) {
		dev_err(&client->dev,
			"%s: input_allocate_device failed\n", __func__);
		rc = -ENOMEM;
		goto free_drvdata;
	}
	input_set_drvdata(drvdata->input_dev, drvdata);

	for (i = 0 ; i < pdata->keymap->num_keys ; i++)
		input_set_capability(drvdata->input_dev, EV_KEY,
				pdata->keymap->keys[i].keycode);

	drvdata->input_dev->name	= "asetm2034a";
	drvdata->input_dev->phys	= "asetm2034a/input0";

	rc = input_register_device(drvdata->input_dev);
	if (rc) {
		input_free_device(drvdata->input_dev);
		dev_err(&client->dev,
			"%s: input_register_device failed rc=%d\n",
			__func__, rc);
		goto free_input_dev;
	}

	i2c_set_clientdata(client, drvdata);

	rc = pdata->gpio_setup();
	if (rc) {
		dev_err(&client->dev,
			"%s: cannot setup GPIO rc=%d\n", __func__, rc);
		goto clear_i2c_clientdata;
	}

	rc = request_threaded_irq(client->irq,
			NULL, asetm2034a_worker,
			(IRQF_TRIGGER_FALLING | IRQF_DISABLED),
			client->name, drvdata);
	if (rc) {
		dev_err(&client->dev,
			"%s: request_threaded_irq failed rc=%d\n",
			__func__, rc);
		goto free_gpio;
	}

	drvdata->recv_msg_type = ASETM2034A_MSG_TYPE_INITDONE;

	rc = pdata->hw_reset();
	if (rc)
		goto free_irq;

	rc = asetm2034a_wait(drvdata);
	if (rc)
		goto free_irq;

#ifdef CONFIG_DEBUG_FS
	asetm2034a_debugfs_init(drvdata);
#endif /* CONFIG_DEBUG_FS */

	rc = asetm2034a_sysfs_create(&drvdata->input_dev->dev);
	if (rc < 0)
		dev_err(&client->dev,
			"%s: sysfs create failed rc=%d\n", __func__, rc);

#ifdef CONFIG_HAS_EARLYSUSPEND
	drvdata->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	drvdata->early_suspend.suspend = asetm2034a_early_suspend;
	drvdata->early_suspend.resume = asetm2034a_late_resume;
	register_early_suspend(&drvdata->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	init_waitqueue_head(&drvdata->irq_wq);
	device_init_wakeup(&drvdata->client->dev, 1);

	dev_info(&client->dev, "%s: done\n", __func__);

	return 0;

free_irq:
	free_irq(client->irq, drvdata);
free_gpio:
	pdata->gpio_shutdown();
clear_i2c_clientdata:
	i2c_set_clientdata(client, NULL);
	input_unregister_device(drvdata->input_dev);
free_input_dev:
	input_set_drvdata(drvdata->input_dev, NULL);
free_drvdata:
	wake_lock_destroy(&drvdata->wlock);
	kfree(drvdata);
err_exit:
	dev_err(&client->dev, "%s: error return\n", __func__);

	return rc;
}

static int __devexit asetm2034a_remove(struct i2c_client *client)
{
	struct asetm2034a_drvdata *drvdata = i2c_get_clientdata(client);
	struct asetm2034a_platform_data *pdata = client->dev.platform_data;

	free_irq(client->irq, NULL);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&drvdata->early_suspend);
#endif /* CONFIG_HAS_EARLYSUSPEND */

	device_init_wakeup(&drvdata->client->dev, 0);

	pdata->gpio_shutdown();
	i2c_set_clientdata(client, NULL);
	input_unregister_device(drvdata->input_dev);
	input_set_drvdata(drvdata->input_dev, NULL);
	wake_lock_destroy(&drvdata->wlock);
	kfree(drvdata);

	return 0;
}

static const struct i2c_device_id asetm2034a_i2c_id[] = {
	{ASETM2034A_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, asetm2034a_i2c_id);

static struct i2c_driver asetm2034a_driver = {
	.driver = {
		.name  = ASETM2034A_NAME,
		.owner = THIS_MODULE,
		.pm    = &asetm2034a_pm,
	},
	.probe         = asetm2034a_probe,
	.remove        = __devexit_p(asetm2034a_remove),
	.id_table      = asetm2034a_i2c_id,
};

static int __init asetm2034a_init(void)
{
	int rc;

	rc = i2c_add_driver(&asetm2034a_driver);
	if (rc)
		pr_err("%s: i2c_add_driver failed rc=%d\n", __func__, rc);

	return rc;
}

static void __exit asetm2034a_exit(void)
{
	i2c_del_driver(&asetm2034a_driver);
}

module_init(asetm2034a_init);
module_exit(asetm2034a_exit);

MODULE_AUTHOR("Yukito Naganuma");
MODULE_DESCRIPTION("Alps ASETM2034A");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("ASETM2034A");

