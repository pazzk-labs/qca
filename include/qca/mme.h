/*
 * SPDX-FileCopyrightText: 2024 Pazzk <team@pazzk.net>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef QCA_MME_H
#define QCA_MME_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

enum qca_mmtype {
	QCA_MMTYPE_SW_VER		= 0x0000U,
	QCA_MMTYPE_WR_MEM		= 0x0001U,
	QCA_MMTYPE_RD_MEM		= 0x0002U,
	QCA_MMTYPE_ST_MAC		= 0x0003U,
	QCA_MMTYPE_GET_NVM		= 0x0004U,
	QCA_MMTYPE_RS_DEV		= 0x0007U,
	QCA_MMTYPE_WR_MOD		= 0x0008U,
	QCA_MMTYPE_RD_MOD		= 0x0009U,
	QCA_MMTYPE_MOD_NVM		= 0x000AU,
	QCA_MMTYPE_WD_RPT		= 0x000BU,
	QCA_MMTYPE_LINK_STATS		= 0x000CU,
	QCA_MMTYPE_NW_INFO		= 0x000EU,
	QCA_MMTYPE_CP_RPT		= 0x0010U,
	QCA_MMTYPE_SET_KEY		= 0x0014U,
	QCA_MMTYPE_MFG_STR		= 0x0015U,
	QCA_MMTYPE_RD_CBLOCK		= 0x0016U,
	QCA_MMTYPE_SET_SDRAM		= 0x0017U,
	QCA_MMTYPE_HST_ACTION		= 0x0018U,
	QCA_MMTYPE_OP_ATTR		= 0x001AU,
	QCA_MMTYPE_ETH_SET		= 0x001BU,
	QCA_MMTYPE_TONE_MAP		= 0x001CU,
	QCA_MMTYPE_NW_STAT		= 0x001DU,
	QCA_MMTYPE_SLAVE_MEM		= 0x001EU,
	QCA_MMTYPE_FAC_DEFAULT		= 0x001FU,
	QCA_MMTYPE_MULTICAST_INFO	= 0x0021U,
	QCA_MMTYPE_CLASSIIFCATION	= 0x0022U,
	QCA_MMTYPE_RX_TONE_MAP		= 0x0024U,
	QCA_MMTYPE_SET_LED		= 0x0025U,
	QCA_MMTYPE_WRITE_EXC_APPLET	= 0x0026U,
	QCA_MMTYPE_MDIO_CMD		= 0x0027U,
	QCA_MMTYPE_SLAVE_REG		= 0x0028U,
	QCA_MMTYPE_BW_LIMIT		= 0x0029U,
	QCA_MMTYPE_SNID			= 0x002AU,
	QCA_MMTYPE_NN_MITIGATE		= 0x002BU,
	QCA_MMTYPE_MODULE		= 0x002CU,
	QCA_MMTYPE_DIAG_NETWORK_PROBE	= 0x002DU,
	QCA_MMTYPE_PL_LINK_STATUS	= 0x002EU,
	QCA_MMTYPE_GPIO_STATE		= 0x002FU,
	QCA_MMTYPE_CONN_ADD		= 0x0030U,
	QCA_MMTYPE_CONN_MOD		= 0x0031U,
	QCA_MMTYPE_CONN_REL		= 0x0032U,
	QCA_MMTYPE_CONN_INFO		= 0x0033U,
	QCA_MMTYPE_MULTIPORT_LNK_STA	= 0x0034U,
	QCA_MMTYPE_EM_ID_TABLE		= 0x0037U,
	QCA_MMTYPE_STANDBY		= 0x0038U,
	QCA_MMTYPE_SLEEP_SCHED		= 0x0039U,
	QCA_MMTYPE_SLEEP_SCHED_NOTI	= 0x003AU,
	QCA_MMTYPE_MCU_DIAG		= 0x003CU,
	QCA_MMTYPE_GET_PROERTY		= 0x003EU,
	QCA_MMTYPE_SET_PROERTY		= 0x003FU,
	QCA_MMTYPE_ATTEN		= 0x0053U,
	QCA_MMTYPE_UNKNOWN		= 0x07FFU,
};

typedef uint16_t qca_mmtype_t;

struct qca_mme {
	uint8_t oui[3]; /*< Qualcomm OUI: 0x00, 0xB0, 0x52 */
	uint8_t body[];
} __attribute__((packed));

struct qca_mme_sw_ver {
	uint32_t cookie;
} __attribute__((packed));

struct qca_mme_sw_ver_cnf {
	uint8_t status;
	uint8_t device_class;
	uint8_t version_len;
	uint8_t version[253];
	uint8_t reserved;
	uint32_t chip_id;
	uint32_t chip_rev;
	uint32_t chip_seq;
	uint32_t chip_package;
	uint32_t chip_options;
} __attribute__((packed));

struct qca_mme_host_action {
	uint8_t request;
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t session_id;
	uint16_t outstanding_retries;
	uint16_t retry_interval_in_10ms;
} __attribute__((packed));

struct qca_mme_host_action_rsp {
	uint8_t status;
	uint8_t version_major;
	uint8_t version_minor;
	uint8_t request;
	uint8_t session_id;
	uint16_t outstanding_retries;
} __attribute__((packed));

struct qca_mme_write_execute {
	uint32_t session_id_client;
	uint32_t session_id_server;
	uint32_t flags;
	uint64_t memory_type;
	uint32_t total_len; /* the length of all parts in this session.
			must be a multiple of 4 */
	uint32_t current_len; /* the length of this part */
	uint32_t current_offset;
	uint32_t start_addr;
	uint32_t checksum;
	uint64_t reserved;
	uint8_t data[];
} __attribute__((packed));

struct qca_mme_write_execute_rsp {
	uint32_t status;
	uint32_t session_id_client;
	uint32_t session_id_server;
	uint32_t flags;
	uint64_t memory_type;
	uint32_t total_len;
	uint32_t current_len;
	uint32_t current_offset;
	uint32_t start_addr;
	uint32_t checksum;
	uint64_t reserved;
	uint32_t target_addr_abs;
	uint32_t start_addr_abs;
} __attribute__((packed));

struct qca_mme_ver_cnf {
	uint8_t status; /*< 0x00 on success */
	uint8_t device_class;
	uint8_t verlen;
	uint8_t verstr[253];
	uint8_t reserved;
	uint32_t ic_id;
	uint32_t ic_rev;
	uint32_t chip_serial;
	uint32_t chip_package;
	uint32_t chip_option;
} __attribute__((packed));

struct qca_mme_mo_cnf {
	uint16_t status;
	uint16_t err_recovery_code;
	uint32_t reserved;
	uint8_t num_op_data;
	uint8_t data[];
} __attribute__((packed));

uint16_t qca_get_mmcode(qca_mmtype_t type);
size_t qca_encode_mme(struct qca_mme *qca, qca_mmtype_t type,
		const void *msg, size_t msglen);
qca_mmtype_t qca_decode_mme(const void *data, size_t datasize, uint16_t mmtype);

#if defined(__cplusplus)
}
#endif

#endif /* QCA_MME_H */
