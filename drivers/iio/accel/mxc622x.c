/*
 * mxc622x.c - IIO driver for Memsic MXC622X dual axis acceleration sensor
 *
 * Copyright 2016 Lawrence Yu <lyu@micile.com>
 *
 * This file is subject to the terms and conditions of version 2 of
 * the GNU General Public License.  See the file COPYING in the main
 * directory of this archive for more details.
 *
 * I2C address is the x.  i.e. mxc6225 has I2C address of 0x15
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>

#define MXC622X_DRV_NAME "mxc622x"


struct mxc622x_data {
	struct i2c_client *client;
};

enum mxc622x_chan {
	AXIS_X,
	AXIS_Y,
};

static int mxc622x_chip_init(struct mxc622x_data *data)
{
	/* Try to read data from the chip chip */
	int ret = i2c_smbus_read_byte_data(data->client, 0x00);

	if (ret < 0)
		return ret;
	return 0;
}

static int mxc622x_read_raw(struct iio_dev *indio_dev,
		struct iio_chan_spec const *chan, int *val, int *val2,
		long mask)
{
	struct mxc622x_data *data = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
 		ret = i2c_smbus_read_byte_data(data->client, chan->scan_index);
 		if (ret < 0)
 			return ret;
 		*val = (s8)ret;
		return IIO_VAL_INT;
	default:
		return -EINVAL;
	}
}

static const struct iio_info mxc622x_info = {
	.read_raw		= mxc622x_read_raw,
	.driver_module		= THIS_MODULE,
};

#define MXC622X_ACC_CHANNEL(_axis, _bits) {				\
	.type = IIO_ACCEL,						\
	.modified = 1,							\
	.channel2 = IIO_MOD_##_axis,					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),			\
	.scan_index = AXIS_##_axis,					\
	.scan_type = {							\
		.sign = 's',						\
		.realbits = _bits,					\
		.storagebits = 16,					\
		.shift = 16 - _bits,					\
	},								\
}

static const struct iio_chan_spec mxc622x_channels[] = {
	MXC622X_ACC_CHANNEL(X, 8),
	MXC622X_ACC_CHANNEL(Y, 8),
};


static int mxc622x_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct mxc622x_data *data;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);
	i2c_set_clientdata(client, indio_dev);
	data->client = client;

	ret = mxc622x_chip_init(data);
	if (ret < 0) {
		printk("MXC622X chip not detected\n");
		return ret;
	}

	indio_dev->dev.parent = &client->dev;
	indio_dev->channels = mxc622x_channels;
	indio_dev->num_channels = 2;
	indio_dev->name = id->name;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &mxc622x_info;

	ret = iio_device_register(indio_dev);
	if (ret < 0) {
		dev_err(&client->dev, "unable to register iio device\n");
		return ret;
	}
	
	printk("MXC622X registered\n");
	
	return 0;
}

static int mxc622x_remove(struct i2c_client *client)
{
	struct iio_dev *indio_dev = i2c_get_clientdata(client);

	iio_device_unregister(indio_dev);

	return 0;
}

#define MXC622X_PM_OPS NULL

static struct i2c_device_id mxc622x_ids[] = {
	{ "mxc622x", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, mxc622x_ids);

static struct i2c_driver mxc622x_driver = {
	.driver = {
		.name	= "mxc622x",
		.pm	= MXC622X_PM_OPS,
	},
	.probe		= mxc622x_probe,
	.remove		= mxc622x_remove,
	.id_table	= mxc622x_ids,
};

module_i2c_driver(mxc622x_driver);

MODULE_AUTHOR("Lawrence Yu <lyu@micile.com>");
MODULE_DESCRIPTION("Bosch MXC622X/BMA250 biaxial acceleration sensor");
MODULE_LICENSE("GPL");
