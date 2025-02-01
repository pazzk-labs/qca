/*
 * SPDX-FileCopyrightText: 2024 Pazzk <team@pazzk.net>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef QCA_H
#define QCA_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "libmcu/spi.h"
#include <stdbool.h>
#include "mme.h"

#define QCA_MAX_BUFSIZE		1532U /* 1500 eth frame + 30 spi frame + 2 cmd */
#define QCA_MIN_PACKET_LEN	60U /* ethernet frame minimum length */
#define QCA_SIGNATURE		0xAA55

enum {
	QCA_REG_BUFFER		= 0x0000,
	QCA_REG_BUFSIZE		= 0x0100,
	QCA_REG_WRBUF_AVAILABLE	= 0x0200,
	QCA_REG_RDBUF_AVAILABLE	= 0x0300,
	QCA_REG_SPI_CONFIG	= 0x0400,
	QCA_REG_SPI_STATUS	= 0x0500,
	QCA_REG_INT_SRC		= 0x0C00,
	QCA_REG_INT_ENABLE	= 0x0D00,
	QCA_REG_RDBUF_WATERMARK	= 0x1200,
	QCA_REG_WRBUF_WATERMARK	= 0x1300,
	QCA_REG_SIGNATURE	= 0x1A00,
	QCA_REG_ACT_CTR		= 0x1B00,
};
typedef uint16_t qca_reg_t;

typedef void (*qca_handler_t)(const void *frame, size_t frame_size, void *ctx);

/**
 * @brief Initializes the QCA device.
 *
 * This function initializes the QCA device with the specified SPI interface and
 * sets up the handler for processing received data.
 *
 * @param[in] spi_iface Pointer to the SPI device interface.
 * @param[in] handler Callback function to handle received data.
 * @param[in] handler_ctx Context to be passed to the handler callback.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_init(struct spi_device *spi_iface,
		qca_handler_t handler, void *handler_ctx);

/**
 * @brief Deinitializes the QCA device.
 *
 * This function deinitializes the QCA device and releases any resources that
 * were allocated during initialization.
 */
void qca_deinit(void);

/**
 * @brief Resets the QCA device.
 *
 * This function performs a soft reset operation on the QCA device, bringing it
 * back to its initial state.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_reset(void);

/**
 * @brief Reads a register from the QCA device.
 *
 * This function reads the value of the specified register from the QCA device.
 *
 * @param[in] reg The register to read.
 * @param[out] value Pointer to a variable where the read value will be stored.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_read_reg(qca_reg_t reg, uint16_t *value);

/**
 * @brief Writes a value to a register on the QCA device.
 *
 * This function writes the specified value to the specified register on the QCA
 * device.
 *
 * @param[in] reg The register to write to.
 * @param[in] value The value to write to the register.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_write_reg(qca_reg_t reg, uint16_t value);

/**
 * @brief Clears any pending interrupts on the QCA device.
 *
 * This function clears any interrupts that have occurred on the QCA device.
 * It ensures that the interrupt status is reset and no pending interrupts
 * are left uncleared.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_clear_interrupt(void);

/**
 * @brief Reads data from the QCA device.
 *
 * This function reads data from the QCA device into the provided buffer.
 *
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in] bufsize Size of the buffer, in bytes.
 *
 * @return The number of bytes read on success, or a negative error code on
 *         failure.
 */
int qca_read(void *buf, size_t bufsize);

/**
 * @brief Processes SPI decapsulation and delivers the Ethernet frame to the
 * callback.
 *
 * This function takes the input stream from SPI, decapsulates it, and then
 * delivers the resulting Ethernet frame to the specified callback function.
 *
 * @param[in] instream Pointer to the input stream data.
 * @param[in] instream_len Length of the input stream data.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_input(const void *instream, size_t instream_len);

/**
 * @brief Writes encoded data to the QCA device.
 *
 * This function encodes the given data and writes it to the QCA device over the
 * SPI interface.
 *
 * @param[in] data Pointer to the data to be encoded and written.
 * @param[in] datasize Size of the data to be encoded and written, in bytes.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_write_encoding(const void *data, size_t datasize);

#if defined(__cplusplus)
}
#endif

#endif /* QCA_H */
