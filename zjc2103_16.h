/***************************************************************************//**
 *   @file   zjc2103_16.h
 *   @brief  Header file for the zjc2103_16 driver
********************************************************************************

*******************************************************************************/
#ifndef ZJC2103_16_H_
#define ZJC2103_16_H_

#include <stdint.h>
#include <stdbool.h>
#include "no_os_spi.h"

#define ZJC2103_16_CFG_CFG_MSK      NO_OS_BIT(13)
#define ZJC2103_16_CFG_INCC_MSK     NO_OS_GENMASK(12,10)
#define ZJC2103_16_CFG_INX_MSK      NO_OS_GENMASK(9,7)
#define ZJC2103_16_CFG_BW_MSK       NO_OS_BIT(6)
#define ZJC2103_16_CFG_REF_MSK      NO_OS_GENMASK(5,3)
#define ZJC2103_16_CFG_SEQ_MSK      NO_OS_GENMASK(2,1)
#define ZJC2103_16_CFG_RB_MSK       NO_OS_BIT(0)

/**
 * @enum ZJC2103_16_device_id
 * @brief Device ID definitions
 */
enum ZJC2103_16_device_id {
	/** 16-Bit, 8-Channel, 250 kSPS PulSAR ADC */
	ID_ZJC2103_16,
	/** 16-Bit, 4-Channel, 250 kSPS PulSAR ADC */
	ID_ZJC2104_16,
	/** 14-Bit, 8-Channel, 300 kSPS PulSAR ADC */
	ID_ZJC2103_14,
	/** 16-Bit, 8-Channel, 500 kSPS PulSAR ADC */
	ID_ZJC2102_16,
};

/**
 * @enum ZJC2103_16_incc
 * @brief Input channel configuration
 */
enum ZJC2103_16_incc {
	/** Bipolar differential pairs; INx− referenced to VREF/2 ± 0.1 V. */
	ZJC2103_16_BIPOLAR_DIFFERENTIAL_PAIRS = 0x1,
	/** Bipolar; INx referenced to COM = V REF /2 ± 0.1 V. */
	ZJC2103_16_BIPOLAR_COM,
	/** Temperature sensor. */
	ZJC2103_16_TEMPERATURE_SENSOR,
	/** Unipolar differential pairs; INx− referenced to GND ± 0.1 V. */
	ZJC2103_16_UNIPOLAR_DIFFERENTIAL_PAIRS = 0x5,
	/** Unipolar, INx referenced to COM = GND ± 0.1 V. */
	ZJC2103_16_UNIPOLAR_COM,
	/** Unipolar, INx referenced to GND. */
	ZJC2103_16_UNIPOLAR_GND
};

/**
 * @enum ZJC2103_16_bw
 * @brief Low-pass filter bandwidth selection
 */
enum ZJC2103_16_bw {
	/** 1⁄4 of BW, uses an additional series resistor to further bandwidth limit the noise. Maximum throughput must be reduced to 1⁄4. */
	ZJC2103_16_BW_QUARTER,
	/** Full bandwidth. */
	ZJC2103_16_BW_FULL
};

/**
 * @enum ZJC2103_16_ref
 * @brief Reference/buffer selection
 */
enum ZJC2103_16_ref {
	/** Internal reference and temperature sensor enabled. REF = 2.5 V buffered output. */
	ZJC2103_16_REF_INTERNAL_2p5V,
	/** Internal reference and temperature sensor enabled. REF = 4.096 V buffered output. */
	ZJC2103_16_REF_INTERNAL_4p096V,
	/** Use external reference. Temperature sensor enabled. Internal buffer disabled. */
	ZJC2103_16_REF_EXTERNAL_TEMP,
	/** Use external reference. Internal buffer and temperature sensor enabled. */
	ZJC2103_16_REF_EXTERNAL_TEMP_IBUF,
	/** Use external reference. Internal reference, internal buffer, and temperature sensor disabled. */
	ZJC2103_16_REF_EXTERNAL = 0x6,
	/** Use external reference. Internal buffer enabled. Internal reference and temperature sensor disabled. */
	ZJC2103_16_REF_IBUF
};

/**
 * @enum ZJC2103_16_seq
 * @brief Channel sequencer configuration
 */
enum ZJC2103_16_seq {
	/** Disable sequencer. */
	ZJC2103_16_SEQ_DISABLE,
	/** Update configuration during sequence. */
	ZJC2103_16_SEQ_UPDATE_CFG,
	/** Scan IN0 to INX, then temperature. */
	ZJC2103_16_SEQ_SCAN_ALL_THEN_TEMP,
	/** Scan IN0 to INX. */
	ZJC2103_16_SEQ_SCAN_ALL
};

/**
 * @struct ZJC2103_16_config
 * @brief ZJC2103_16 configuration
 */
struct ZJC2103_16_config {
	/** Input channel configuration */
	enum ZJC2103_16_incc incc;
	/** INX channel selection (sequencer iterates from IN0 to INX) */
	uint8_t inx;
	/** Low-pass filter bandwidth selection */
	enum ZJC2103_16_bw bw;
	/**  Reference/buffer selection */
	enum ZJC2103_16_ref ref;
	/** Channel sequencer configuration */
	enum ZJC2103_16_seq seq;
	/** Read back the CFG register */
	bool rb;
};

struct ZJC2103_16_init_param {
	/** Device ID */
	enum ZJC2103_16_device_id id;
	/** ADC specific parameters */
	struct ZJC2103_16_config config;
	/** SPI initialization parameters */
	struct no_os_spi_init_param spi_init;
};

struct ZJC2103_16_dev {
	/** Device name string */
	const char *name;
	/** Device ID */
	enum ZJC2103_16_device_id id;
	/** ZJC2103_16 configs (configs[1] - in use, configs[0] - will be in use during next transaction) */
	struct ZJC2103_16_config configs[2];
	/** SPI descriptor*/
	struct no_os_spi_desc *spi_desc;
};

int32_t ZJC2103_16_init(struct ZJC2103_16_dev **dev,
		    struct ZJC2103_16_init_param *init_param);
int32_t ZJC2103_16_write_config(struct ZJC2103_16_dev *dev,
			    struct ZJC2103_16_config *config);
int32_t ZJC2103_16_read_config(struct ZJC2103_16_dev *dev,
			   struct ZJC2103_16_config *config);
int32_t ZJC2103_16_read(struct ZJC2103_16_dev *dev, uint16_t *data,
		    uint32_t nb_samples);
int32_t ZJC2103_16_remove(struct ZJC2103_16_dev *dev);

#endif