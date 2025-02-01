/*
 * SPDX-FileCopyrightText: 2024 Pazzk <team@pazzk.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "qca/qca.h"

#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

#include "libmcu/ringbuf.h"

#define QCA_RXQ_MAXSIZE		2048
#define QCA_SPI_WRAPPER_LEN	10
#define QCA_ETH_MAXLEN		1500

#if !defined(MIN)
#define MIN(a, b)		(((a) > (b))? (b) : (a))
#endif

#if !defined(QCA_DEBUG)
#define QCA_DEBUG(...)
#endif
#if !defined(QCA_ERROR)
#define QCA_ERROR(...)
#endif

static struct {
	struct spi_device *spi;
	struct ringbuf *rxq;
	pthread_mutex_t transaction_lock;
	qca_handler_t cb;
	void *cb_ctx;
} m;

static int writeread(struct spi_device *iface, const void *tx, size_t txsize,
		void *rx, size_t rxsize)
{
	return spi_writeread(iface, tx, txsize, rx, rxsize);
}

static uint16_t get_spi_frame_len(const void *frame, size_t frame_size)
{
	if (frame_size < 6) {
		return 0;
	}

	const uint8_t *p = (const uint8_t *)frame;
	return (uint16_t)((uint16_t)p[5] << 8) | p[4];
}

static bool validate_frame(const void *frame, size_t frame_size)
{
	const uint8_t *p = (const uint8_t *)frame;

	if (frame_size < QCA_SPI_WRAPPER_LEN || frame_size > QCA_MAX_BUFSIZE) {
		return false;
	}

	if ((p[0] != 0xaa) || ((p[0] ^ p[1] ^ p[2] ^ p[3]) != 0x00)) {
		return false;
	}

	uint16_t len = get_spi_frame_len(frame, frame_size);

	if (len > QCA_ETH_MAXLEN ||
			frame_size < (size_t)(len + QCA_SPI_WRAPPER_LEN)) {
		return false;
	}

	if ((p[len + 8] != 0x55) || ((p[len + 8] ^ p[len + 9]) != 0x00)) {
		return false;
	}

	return true;
}

static size_t get_frame_size(const void *frame)
{
	uint16_t len = get_spi_frame_len(frame, 6);
	size_t frame_size = len + QCA_SPI_WRAPPER_LEN;

	if (!validate_frame(frame, frame_size)) {
		return 0;
	}

	return frame_size;
}

static uint8_t *encode_spi_frame(uint8_t *buf, size_t bufsize, size_t datasize)
{
	if (datasize > QCA_ETH_MAXLEN ||
			bufsize < datasize + QCA_SPI_WRAPPER_LEN + 2) {
		QCA_ERROR("invalid parameters %u %u", datasize, bufsize);
		return NULL;
	}

        /* The first two bytes are placeholders for the command */
	buf[2] = 0xAA;
	buf[3] = 0xAA;
	buf[4] = 0xAA;
	buf[5] = 0xAA;
	buf[6] = (uint8_t)datasize; /* packet length */
	buf[7] = (uint8_t)(datasize >> 8);
	buf[8] = 0; /* protocol version */
	buf[9] = 0;
	/* ethernet frame */
	buf[10 + datasize] = 0x55;
	buf[11 + datasize] = 0x55;

	return &buf[10];
}

static void encode_spi_request(uint8_t *cmd, qca_reg_t reg, bool read_req,
		bool register_addr_mode)
{
	cmd[0] = (uint8_t)(reg >> 8);
	cmd[1] = (uint8_t)reg;

	if (read_req) {
		cmd[0] |= 0x80;
	}
	if (register_addr_mode) {
		cmd[0] |= 0x40;
	}
}

static int read_register(struct spi_device *iface,
		qca_reg_t reg, uint16_t *value)
{
	uint8_t result[2];
	uint8_t cmd[2];
	encode_spi_request(cmd, reg, true, true);
	int err = writeread(iface, cmd, sizeof(cmd), result, sizeof(result));
	*value = (uint16_t)(((uint16_t)result[0] << 8) | result[1]);
	return err;
}

static int write_register(struct spi_device *iface,
		qca_reg_t reg, uint16_t value)
{
	uint8_t cmd[4];
	encode_spi_request(cmd, reg, false, true);
	cmd[3] = (uint8_t)(value & 0xff);
	cmd[2] = (uint8_t)(value >> 8);
	return writeread(iface, cmd, sizeof(cmd), 0, 0);
}

static int read_buffer_len(struct spi_device *iface)
{
	uint16_t len = 0;
	if (read_register(iface, QCA_REG_RDBUF_AVAILABLE, &len) == 0) {
		return len;
	}
	return 0;
}

static int fetch_buffer(struct spi_device *iface, uint16_t nr_to_write)
{
	uint8_t cmd[4];
	encode_spi_request(cmd, QCA_REG_BUFSIZE, false, true);
	cmd[2] = (uint8_t)(nr_to_write >> 8);
	cmd[3] = (uint8_t)(nr_to_write & 0xff);
	return writeread(iface, cmd, sizeof(cmd), 0, 0);
}

static int read_buffer(struct spi_device *iface, void *buf, size_t expected_len)
{
	uint8_t cmd[2];
	encode_spi_request(cmd, QCA_REG_BUFFER, true, false);
	return writeread(iface, cmd, sizeof(cmd), buf, expected_len);
}

static int write_buffer(struct spi_device *iface, void *data, size_t datasize)
{
	encode_spi_request((uint8_t *)data, QCA_REG_BUFFER, false, false);
	return writeread(iface, data, datasize + 2, 0, 0);
}

static int write_to_qca(void *data, size_t datasize)
{
	if (!data || datasize == 0 || datasize > QCA_MAX_BUFSIZE) {
		QCA_ERROR("invalid data %p %u", data, datasize);
		return -EINVAL;
	}

	int err;
	uint16_t wrbuf = 0;

	if ((err = read_register(m.spi, QCA_REG_WRBUF_AVAILABLE, &wrbuf)) ||
			wrbuf < datasize) {
		QCA_ERROR("failed to write %u bytes: %d, %u",
				datasize, err, wrbuf);
		return -EIO;
	}

	if ((err = fetch_buffer(m.spi, (uint16_t)datasize)) == 0) {
		err = write_buffer(m.spi, data, datasize);
	}

	return err;
}

int qca_read_reg(qca_reg_t reg, uint16_t *value)
{
	int err;

	pthread_mutex_lock(&m.transaction_lock);
	err = read_register(m.spi, reg, value);
	pthread_mutex_unlock(&m.transaction_lock);

	return err;
}

int qca_write_reg(qca_reg_t reg, uint16_t value)
{
	int err;

	pthread_mutex_lock(&m.transaction_lock);
	err = write_register(m.spi, reg, value);
	pthread_mutex_unlock(&m.transaction_lock);

	return err;
}

int qca_clear_interrupt(void)
{
	return qca_write_reg(QCA_REG_INT_SRC, 0xffff);
}

int qca_read(void *buf, size_t bufsize)
{
	pthread_mutex_lock(&m.transaction_lock);

	int err = -EIO;
	uint16_t len = (uint16_t)read_buffer_len(m.spi);

	if (len == 0) {
		err = 0;
		goto out;
	}

	len = MIN(len, (uint16_t)(bufsize-2));

	if (fetch_buffer(m.spi, len) == 0) {
		if (read_buffer(m.spi, buf, len) == 0) {
			err = (int)len;
		}
	}

out:
	pthread_mutex_unlock(&m.transaction_lock);
	return err;
}

int qca_write_encoding(const void *data, size_t datasize)
{
	uint8_t buf[QCA_MAX_BUFSIZE];
	uint8_t *p = encode_spi_frame(buf, sizeof(buf), datasize);

	if (!p) {
		return -EINVAL;
	}

	memcpy(p, data, datasize);

	pthread_mutex_lock(&m.transaction_lock);
	int err = write_to_qca(buf, get_frame_size(&buf[2]/*preserved cmd*/));
	pthread_mutex_unlock(&m.transaction_lock);

	return err;
}

int qca_input(const void *instream, size_t instream_len)
{
#define PREFIX_LEN	12 /* hw-generated frame length + SOF + PL + Ver */
#define POSTFIX_LEN	2 /* 0x5555 */
	uint8_t *buf = (uint8_t *)malloc(QCA_MAX_BUFSIZE);

	if (buf == NULL) {
		QCA_ERROR("failed to allocate buffer");
		return -ENOMEM;
	}

	ringbuf_write(m.rxq, instream, instream_len);

	while (ringbuf_length(m.rxq) > PREFIX_LEN + POSTFIX_LEN) {
		uint8_t p[PREFIX_LEN];
		ringbuf_peek(m.rxq, 0, p, sizeof(p));

		const uint32_t frame_len = (uint32_t)
			((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
		const uint8_t magic = p[4] ^ p[5] ^ p[6] ^ p[7];
		const uint16_t ver = (uint16_t)((p[10] << 8) | p[11]);
		if (frame_len > QCA_MAX_BUFSIZE ||
				p[4] != 0xaa || magic != 0 || ver != 0) {
			ringbuf_consume(m.rxq, 1); /* drop one byte */
			continue;
		}

		const size_t packet_len = ((size_t)p[9] << 8) | p[8];
		const size_t received_len = ringbuf_length(m.rxq);
		if (packet_len > received_len - PREFIX_LEN - POSTFIX_LEN) {
			return -EAGAIN;
		}

		ringbuf_read(m.rxq, PREFIX_LEN, buf, packet_len);
		ringbuf_consume(m.rxq, PREFIX_LEN + packet_len + POSTFIX_LEN);

		if (m.cb) {
			(*m.cb)(buf, packet_len, m.cb_ctx);
		}
	}

	free(buf);

	return 0;
}

int qca_reset(void)
{
	return qca_write_reg(QCA_REG_SPI_CONFIG, 0x40);
}

int qca_init(struct spi_device *spi_iface,
		qca_handler_t handler, void *handler_ctx)
{
	m.cb = handler;
	m.cb_ctx = handler_ctx;
	m.spi = spi_iface;
	m.rxq = ringbuf_create(QCA_RXQ_MAXSIZE);

	pthread_mutex_init(&m.transaction_lock, NULL);

	int err = 0;
#if !defined(UNIT_TEST)
	uint16_t signature;
	err |= qca_read_reg(QCA_REG_SIGNATURE, &signature);

	if (signature != QCA_SIGNATURE || err) {
		err |= qca_read_reg(QCA_REG_SIGNATURE, &signature);
		if (signature != QCA_SIGNATURE || err) {
			QCA_ERROR("QCA700x not found(%d): %x", err, signature);
			return -ENODEV;
		}
	}

	err |= qca_write_reg(QCA_REG_ACT_CTR, 2);
	err |= qca_write_reg(QCA_REG_INT_ENABLE,
			0x41/*cpu_on and packet_available*/);

        /* Clear any interrupts that occurred before system initialization to
         * avoid missing them. */
	uint16_t intsrc;
	err |= qca_read_reg(QCA_REG_INT_SRC, &intsrc);
	err |= qca_write_reg(QCA_REG_INT_SRC, intsrc);
#endif

	if (err) {
		QCA_ERROR("QCA700x init failed(%d)", err);
	}

	return err;
}

void qca_deinit(void)
{
	pthread_mutex_destroy(&m.transaction_lock);
	ringbuf_destroy(m.rxq);
}
