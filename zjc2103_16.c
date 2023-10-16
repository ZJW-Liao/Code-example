/***************************************************************************//**
 *   @file   ZJC2103_16.c
 *   @brief  Source file for the ZJC2103_16 driver
********************************************************************************

*******************************************************************************/
#include <stdlib.h>
#include "ZJC2103_16.h"
#include "no_os_util.h"
#include "no_os_error.h"
#include "no_os_print_log.h"
#include "no_os_delay.h"
#include "no_os_alloc.h"

const char *ZJC2103_16_device_name[] = {
	"ZJC2103_16",
	"ZJC2104_16",
	"ZJC2103_14",
	"ZJC2102_16",
};

static void _ZJC2103_16_config_put(struct ZJC2103_16_dev *dev,
			       struct ZJC2103_16_config *config)
{
	dev->configs[1] = dev->configs[0];
	if (config)
		dev->configs[0] = *config;
	else
		dev->configs[0] = dev->configs[1];
}

static struct ZJC2103_16_config *_ZJC2103_16_config_get(struct ZJC2103_16_dev *dev)
{
	// dev->configs[1] is the config currently in use. If the current
	// SPI transaction is numbered N, this config was written at N-2
	// and applied at the EOC of N-1.
	return &dev->configs[1];
}

static int32_t _ZJC2103_16_rac(struct ZJC2103_16_dev *dev,
			   struct ZJC2103_16_config *config_in, struct ZJC2103_16_config *config_out,
			   uint16_t *data)
{
	int32_t ret;
	uint16_t cfg = 0;
	uint8_t buf[4] = {0, 0, 0, 0};
	uint16_t sz;
	struct ZJC2103_16_config *c;

	if (!dev)
		return -EINVAL;

	c = _ZJC2103_16_config_get(dev);

	if (config_in) {
		cfg |= no_os_field_prep(ZJC2103_16_CFG_CFG_MSK, 1);
		cfg |= no_os_field_prep(ZJC2103_16_CFG_INCC_MSK, config_in->incc);
		cfg |= no_os_field_prep(ZJC2103_16_CFG_INX_MSK, config_in->inx);
		cfg |= no_os_field_prep(ZJC2103_16_CFG_BW_MSK, config_in->bw);
		cfg |= no_os_field_prep(ZJC2103_16_CFG_REF_MSK, config_in->ref);
		cfg |= no_os_field_prep(ZJC2103_16_CFG_SEQ_MSK, config_in->seq);
		cfg |= no_os_field_prep(ZJC2103_16_CFG_RB_MSK, !config_in->rb);
		cfg <<= 2;
		buf[0] = cfg >> 8;
		buf[1] = cfg;
	}
	sz = c->rb && config_out ? 4 : 2;
	ret = no_os_spi_write_and_read(dev->spi_desc, buf, sz);
	if (ret < 0)
		return ret;

	_ZJC2103_16_config_put(dev, config_in);

	if (data) {
		// by default, data width is 16-bits
		*data = ((uint16_t)buf[0] << 8) | buf[1];

		// handle 14-bit data width:
		if (dev->id == ID_ZJC2103_14) {
			// Bipolar samples are in two's complement, scale down to 14-bit
			// by keeping the sign bit using division instead of implementation
			// specific right shift. This works for unipolar samples too.
			*data = (int16_t)*data / 4;
		}
	}

	if (c->rb && config_out) {
		if (dev->id == ID_ZJC2103_14)
			cfg = (((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3]) >> 2;
		else
			cfg = ((uint16_t)buf[2] << 8) | buf[3];
		cfg >>= 2;
		config_out->incc = no_os_field_get(ZJC2103_16_CFG_INCC_MSK, cfg);
		config_out->inx = no_os_field_get(ZJC2103_16_CFG_INX_MSK, cfg);
		config_out->bw = no_os_field_get(ZJC2103_16_CFG_BW_MSK, cfg);
		config_out->ref = no_os_field_get(ZJC2103_16_CFG_REF_MSK, cfg);
		config_out->seq = no_os_field_get(ZJC2103_16_CFG_SEQ_MSK, cfg);
		config_out->rb = !(bool)no_os_field_get(ZJC2103_16_CFG_RB_MSK, cfg);
	}

	return 0;
}

/***************************************************************************//**
 * @brief Initialize the ZJC2103_16 driver and create a descriptor.
 *
 * @param dev - Device descriptor to create.
 * @param init_param - Initialization parameters.
 *
 * @return Returns negative error code or 0 in case of success.
 *         Example: -EINVAL - Bad input parameters.
 *                  -ENOMEM - Failed to allocate memory.
 *                  0 - No errors encountered.
*******************************************************************************/
int32_t ZJC2103_16_init(struct ZJC2103_16_dev **dev,
		    struct ZJC2103_16_init_param *init_param)
{
	struct ZJC2103_16_dev *d;
	int32_t ret;

	if (init_param->id > ID_ZJC2102_16)
		return -EINVAL;

	d = (struct ZJC2103_16_dev *)no_os_calloc(1, sizeof(*d));
	if (!d)
		return -ENOMEM;

	d->id = init_param->id;
	d->name = ZJC2103_16_device_name[d->id];

	ret = no_os_spi_init(&d->spi_desc, &init_param->spi_init);
	if (ret < 0)
		goto error_spi;

	ret = ZJC2103_16_write_config(d, &init_param->config);
	if (ret < 0)
		goto error_init;

	// perform one extra dummy conversion (2 are needed after power-up)
	ret = _ZJC2103_16_rac(d, NULL, NULL, NULL);
	if (ret < 0)
		goto error_init;

	*dev = d;

	return 0;
error_init:
	no_os_spi_remove(d->spi_desc);
error_spi:
	no_os_free(d);
	pr_err("%s initialization failed with status %d\n", d->name,
	       ret);

	return ret;
}

/***************************************************************************//**
 * @brief Write the device's CFG register.
 *
 * @param dev - Device descriptor.
 * @param config - Configuration to write.
 *
 * @return Returns negative error code or 0 in case of success.
 *         Example: -EINVAL - Bad input parameters.
 *                  0 - No errors encountered.
*******************************************************************************/
int32_t ZJC2103_16_write_config(struct ZJC2103_16_dev *dev,
			    struct ZJC2103_16_config *config)
{
	return _ZJC2103_16_rac(dev, config, NULL, NULL);
}

/***************************************************************************//**
 * @brief Read the device's CFG register.
 *
 * If the readback is enabled when making this call, one SPI transaction
 * is enough to retrieve the CFG register.
 *
 * If the readback is disabled when making this call, it is temporarily
 * enabled, then disabled back. 3 SPI transactions are needed to retrieve
 * the CFG register.
 *
 * @param dev - Device descriptor.
 * @param config - pointer to location where the read configuration gets stored.
 *
 * @return Returns negative error code or 0 in case of success.
 *         Example: -EINVAL - Bad input parameters.
 *                  0 - No errors encountered.
*******************************************************************************/
int32_t ZJC2103_16_read_config(struct ZJC2103_16_dev *dev, struct ZJC2103_16_config *config)
{
	int32_t ret = 0;
	struct ZJC2103_16_config *c = _ZJC2103_16_config_get(dev);
	if (c->rb == true)
		return _ZJC2103_16_rac(dev, NULL, config, NULL);

	struct ZJC2103_16_config c_in = *c;
	c_in.rb = true;
	ret = _ZJC2103_16_rac(dev, &c_in, NULL, NULL);
	if (ret < 0)
		return ret;

	c_in.rb = false;
	ret = _ZJC2103_16_rac(dev, &c_in, NULL, NULL);
	if (ret < 0)
		return ret;

	return _ZJC2103_16_rac(dev, NULL, config, NULL);
}

/***************************************************************************//**
 * @brief Read ADC samples.
 *
 * This function uses the RAC mode to perform the SPI transactions.
 *
 * @param dev - Device descriptor.
 * @param data - pointer to a large enough buffer where the data gets stored.
 * @param nb_samples - Number of samples to read.
 *
 * @return Returns negative error code or 0 in case of success.
 *         Example: -EINVAL - Bad input parameters.
 *                  0 - No errors encountered.
*******************************************************************************/
int32_t ZJC2103_16_read(struct ZJC2103_16_dev *dev, uint16_t *data, uint32_t nb_samples)
{
	int32_t ret;
	uint32_t i = 0;

	if (!data)
		return -EINVAL;
	if (!nb_samples)
		return 0;

	do {
		ret = _ZJC2103_16_rac(dev, NULL, NULL, &data[i]);
		if (ret < 0)
			return ret;
		i++;
	} while (i < nb_samples);

	return 0;
}

/***************************************************************************//**
 * @brief Remove the driver's descriptor by freeing the associated resources.
 *
 * @param dev - Device descriptor.
 *
 * @return Returns negative error code or 0 in case of success.
 *         Example: -EINVAL - Bad input parameters.
 *                  0 - No errors encountered.
*******************************************************************************/
int32_t ZJC2103_16_remove(struct ZJC2103_16_dev *dev)
{
	if (!dev)
		return -EINVAL;

	no_os_spi_remove(dev->spi_desc);
	no_os_free(dev);

	return 0;
}