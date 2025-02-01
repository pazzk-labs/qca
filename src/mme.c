/*
 * SPDX-FileCopyrightText: 2024 Pazzk <team@pazzk.net>
 *
 * SPDX-License-Identifier: MIT
 */

#include "qca/mme.h"
#include <string.h>

#if !defined(ARRAY_SIZE)
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#endif

enum host_action_request {
	HOST_REQ_LOADER_READY	= 0x00,
	HOST_REQ_FW_READY	= 0x01,
	HOST_REQ_PIB_READY	= 0x02,
	HOST_REQ_FW_PIB_READY	= 0x03,
	HOST_REQ_SDRAM_CONFIG	= 0x04,
	HOST_REQ_FACTORY	= 0x05,
	HOST_REQ_PIB_READY_BG	= 0x06,
	HOST_REQ_REBOOTED	= 0x07,
};

enum module_operation {
	MOP_READ_RAM		= 0x00,
	MOP_READ_NVM		= 0x01,
	MOP_START_WRITE_SESSION	= 0x10,
	MOP_WRITE		= 0x11,
	MOP_COMMIT		= 0x12,
};

enum module_id {
	MID_INIT		= 0x1000,
	MID_UART		= 0x2000,
	MID_ENUM_ID_TABLE	= 0x3000,
	MID_POWER_MANAGEMENT	= 0x4000,
	MID_FORWARD_CONF	= 0x7000,
	MID_FIRMWARE		= 0x7001,
	MID_PIB			= 0x7002,
	MID_SOFTLOADER		= 0x7003,
	MID_PIB_MERGE		= 0x7005,
};

typedef size_t (*encoder_func_t)(struct qca_mme *qca,
		const void *msg, size_t msglen);

struct encoder {
	qca_mmtype_t type;
	encoder_func_t func;
};

static void set_oui(struct qca_mme *qca)
{
	qca->oui[0] = 0x00;
	qca->oui[1] = 0xB0;
	qca->oui[2] = 0x52;
}

static void set_header(struct qca_mme *qca)
{
	memset(qca, 0, sizeof(*qca));
	set_oui(qca);
}

static size_t encode_empty(struct qca_mme *qca, const void *msg, size_t msglen)
{
	(void)qca;
	(void)msg;
	(void)msglen;
	return 0;
}

static size_t encode_generic(struct qca_mme *qca,
		const void *msg, size_t msglen)
{
	if (msg && msglen) {
		memcpy(qca->body, msg, msglen);
	}
	return msglen;
}

static struct encoder encoders[] = {
	{ .type = QCA_MMTYPE_SW_VER,           .func = encode_generic },
	{ .type = QCA_MMTYPE_HST_ACTION,       .func = encode_generic },
	{ .type = QCA_MMTYPE_WRITE_EXC_APPLET, .func = encode_generic },
};

static size_t encode(struct qca_mme *qca, qca_mmtype_t type,
		const void *msg, size_t msglen)
{
	const size_t hlen = sizeof(*qca);

	set_header(qca);

	for (size_t i = 0; i < ARRAY_SIZE(encoders); i++) {
		if (encoders[i].type == type) {
			return encoders[i].func(qca, msg, msglen) + hlen;
		}
	}

	return encode_empty(qca, msg, msglen) + hlen;
}

static qca_mmtype_t decode(const struct qca_mme *qca,
		size_t len, uint16_t mmtype)
{
	(void)qca;
	(void)len;
	return mmtype;
}

size_t qca_encode_mme(struct qca_mme *qca, qca_mmtype_t type,
		const void *msg, size_t msglen)
{
	return encode(qca, type, msg, msglen);
}

qca_mmtype_t qca_decode_mme(const void *data, size_t datasize, uint16_t mmtype)
{
	return decode((const struct qca_mme *)data, datasize, mmtype);
}
#if 0
size_t qca_pack_pib_read(struct eth *buf, size_t bufsize, uint32_t offset)
{
	(void)bufsize;
	struct eth *eth = buf;
	struct hpgp_frame *mme = (struct hpgp_frame *)eth->payload;
	struct qca_mme *qca = (struct qca_mme *)mme->body;

	struct req {
		uint32_t reserved;
		uint8_t num_op_data;
		uint16_t mod_op;
		uint16_t mod_op_data_len;
		uint32_t reserved2;
		uint16_t module_id;
		uint16_t module_sub_id;
		uint16_t data_len;
		uint32_t offset;
	} __attribute__((packed)) *req = (struct req *)qca->body;

	set_header(eth, QCA_MMTYPE_MODULE);

	req->reserved = 0;
	req->num_op_data = 1;
	req->mod_op = MOP_READ_RAM;
	req->mod_op_data_len = 18;
	req->reserved2 = 0;
	req->module_id = MID_PIB;
	req->module_sub_id = 0;
	req->data_len = 1400;
	req->offset = offset;

	return 18;
}
#endif
