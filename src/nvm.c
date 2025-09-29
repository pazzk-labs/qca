/*
 * SPDX-FileCopyrightText: 2024 Pazzk <team@pazzk.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "qca/nvm.h"
#include <errno.h>
#include <string.h>
#include "libmcu/ringbuf.h"

#if !defined(QCA_NVM_BUFSIZE)
#define QCA_NVM_BUFSIZE		(sizeof(qca_nvm_header_t) * 2)
#endif

#if !defined(MIN)
#define MIN(a, b)		(((a) > (b))? (b) : (a))
#endif

#if !defined(QCA_DEBUG)
#define QCA_DEBUG(...)
#endif

struct header_iterator_ctx {
	qca_nvm_image_t type;
	struct {
		uint32_t header;
		uint32_t module;
	} offset;
};

static uint32_t calc_chksum(const void *data, size_t len, uint32_t checksum)
{
	const uint8_t *mem = (const uint8_t *)data;
	uint32_t tmp;

	while (len >= sizeof(checksum)) {
		memcpy(&tmp, mem, sizeof(tmp));
		checksum ^= tmp;
		mem += sizeof(checksum);
		len -= sizeof(checksum);
	}

	return ~checksum;
}

static int iterate_nvm_header(qca_nvm_reader_t reader, size_t nvm_size,
		qca_nvm_header_callback_t cb, void *ctx)
{
	struct ringbuf *q;
	size_t index = 0;
	size_t next = 0;


	if (!reader) {
		return -EINVAL;
	}
	if (!(q = ringbuf_create(QCA_NVM_BUFSIZE * 2))) {
		return -ENOMEM;
	}

	if (nvm_size == 0) {
		nvm_size = (size_t)-1; /* up to EOF */
	}

	while (index < nvm_size && next != (size_t)-1) {
		uint8_t buf[sizeof(qca_nvm_header_t)];
		const size_t len = reader(buf, sizeof(buf), ctx);

		if (len > 0) {
			ringbuf_write(q, buf, len);
		}

		if (ringbuf_length(q) < sizeof(qca_nvm_header_t)) {
			if (len <= 0) { /* EOF */
				QCA_DEBUG("EOF");
				break;
			}
			QCA_DEBUG("not enough data %u", ringbuf_length(q));
			continue;
		}

		if (index == next) {
			ringbuf_read(q, 0, buf, sizeof(qca_nvm_header_t));
			const qca_nvm_header_t *header =
				(const qca_nvm_header_t *)buf;
			next = header->NextNvmHeaderPtr;
			index += sizeof(*header);

			if (cb) {
				if (!((*cb)(header, ctx))) {
					break;
				}
			}
		} else {
			const size_t diff = next - index;
			const size_t offset = MIN(diff, ringbuf_length(q));
			ringbuf_consume(q, offset);
			index += offset;
		}
	}

	ringbuf_destroy(q);

	return 0;
}

static bool on_nvm_header_for_offset(const qca_nvm_header_t *header, void *ctx)
{
	struct header_iterator_ctx *p = (struct header_iterator_ctx *)ctx;

	if (header->EntryType == p->type) {
		p->offset.header = header->ImageNvmAddress -
			(uint32_t)sizeof(*header);
		p->offset.module = header->ImageNvmAddress;
		return false;
	}

	return true;
}

uint32_t qca_calc_chksum(const void *data, size_t len, uint32_t checksum)
{
	return calc_chksum(data, len, checksum);
}

int qca_nvm_iterate(qca_nvm_reader_t reader, size_t nvm_size,
		qca_nvm_header_callback_t cb, void *ctx)
{
	return iterate_nvm_header(reader, nvm_size, cb, ctx);
}

int qca_nvm_offset(qca_nvm_image_t type,
		qca_nvm_reader_t reader, size_t nvm_size, uint32_t *offset)
{
	struct header_iterator_ctx ctx = {
		.type = type,
	};

	iterate_nvm_header(reader, nvm_size, on_nvm_header_for_offset, &ctx);

	if (offset) {
		*offset = ctx.offset.header;
	}

	return ctx.offset.header? 0 : -ENOENT;
}
