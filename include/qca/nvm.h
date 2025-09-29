/*
 * SPDX-FileCopyrightText: 2024 Pazzk <team@pazzk.net>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef QCA_NVM_H
#define QCA_NVM_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
	QCA_NVM_IMAGE_GENERIC		= 0x0000,
	QCA_NVM_IMAGE_FIRMWARE		= 0x0004,
	QCA_NVM_IMAGE_CUSTOM		= 0x0006,
	QCA_NVM_IMAGE_MEMCTL		= 0x0007, /* the configuration applet */
	QCA_NVM_IMAGE_ADVPWRMGMT	= 0x0008,
	QCA_NVM_IMAGE_NVM_SOFTLOADER	= 0x000B,
	QCA_NVM_IMAGE_MANIFEST		= 0x000E,
	QCA_NVM_IMAGE_PIB		= 0x000F,
} qca_nvm_image_t;

typedef struct {
	uint16_t MajorVersion;
	uint16_t MinorVersion;
	uint32_t AppletExecuteMask;
	uint32_t ImageNvmAddress; /* The module’s offset in the NVM chain.
			This is a relative offset that needs to be added to
			the chain’s address to obtain the absolute address of
			the module data in NVM. */
	uint32_t ImageMemoryAddress; /* The address in memory where to load
			the module data (typically used for loading programs
			into memory). */
	uint32_t ImageLength;
	uint32_t ImageChecksum;
	uint32_t AppletEntryPtr; /* If the payload for this header is an applet,
			then this field is the address to start executing the
			applet from. If the payload associated to the header
			only holds pure (non-executable) data, this field must
			be set to 0xffffffff. */
	uint32_t NextNvmHeaderPtr;
	uint32_t PreviousNvmHeaderPtr;
	uint32_t EntryType; /* qca_nvm_image_t */
	uint16_t ModuleId;
	uint16_t ModuleSubId;
	uint16_t AppletEntryVersion;
	uint16_t Reserved0;
	uint32_t Reserved1;
	uint32_t Reserved2;
	uint32_t Reserved3;
	uint32_t Reserved4;
	uint32_t Reserved5;
	uint32_t Reserved6;
	uint32_t Reserved7;
	uint32_t Reserved8;
	uint32_t Reserved9;
	uint32_t Reserved10;
	uint32_t Reserved11;
	uint32_t HeaderChecksum;
} __attribute__((packed)) qca_nvm_header_t;

typedef struct {
	uint16_t version;
	uint16_t reserved1;
	uint16_t length;
	uint16_t reserved2;
	uint32_t checksum;
	uint8_t mac[6];
	uint8_t dak[16];
	uint16_t reserved3;
	uint8_t mfg[64];
	uint8_t nmk[16];
	uint8_t usr[64];
	uint8_t net[64];
	uint8_t cco_selection;
	uint8_t cexist_mode_select;
	uint8_t pl_freq_select;
	uint8_t reserved4;
	uint8_t preferred_nid[7];
	uint8_t auto_fw_upgradeable;
	uint8_t mdu_configuration;
	uint8_t mdu_role;
	uint8_t reserved5[10];
	uint8_t static_network_configuration[128];
	uint8_t interface_configuration[64];
} __attribute__((packed)) qca_nvm_pib_t;

/**
 * @brief Function pointer type for handling NVM header callbacks.
 *
 * This callback is invoked whenever an NVM header is discovered.
 *
 * @param[in] header Pointer to the NVM header structure.
 * @param[in] ctx    Context pointer that can be used to pass additional
 *                   information.
 *
 * @return true to continue the iteration, false to stop the iteration.
 */
typedef bool (*qca_nvm_header_callback_t)(const qca_nvm_header_t *header,
		void *ctx);

/**
 * @brief Function pointer type for reading from NVM (Non-Volatile Memory).
 *
 * This type defines a function pointer for reading data from NVM.
 *
 * @param[out] buf Pointer to the buffer where the read data will be stored.
 * @param[in]  bufsize Size of the buffer.
 * @param[in]  ctx Context pointer that can be used to pass additional
 *             information.
 *
 * @return The number of bytes read. Returns 0 in case of an error or if the
 *         end is reached.
 */
typedef size_t (*qca_nvm_reader_t)(void *buf, size_t bufsize, void *ctx);

/**
 * @brief Iterates over the NVM (Non-Volatile Memory) data.
 *
 * This function iterates over the NVM data using the provided reader function
 * and calls the specified callback function whenever an NVM header is
 * discovered.
 *
 * @param[in] reader The function pointer for reading data from NVM.
 * @param[in] nvm_size The size of the NVM data.
 * @param[in] cb The function pointer for handling NVM header callbacks.
 * @param[in] ctx The context pointer that can be used to pass additional
 *            information.
 *
 * @return 0 on success, or an error code on failure.
 */
int qca_nvm_iterate(qca_nvm_reader_t reader, size_t nvm_size,
		qca_nvm_header_callback_t cb, void *ctx);

/**
 * @brief Calculates the checksum of the given data.
 *
 * This function calculates the checksum of the provided data array.
 *
 * @param[in] data Pointer to the data array.
 * @param[in] len Length of the data array.
 * @param[in] checksum Initial checksum value.
 *
 * @return The calculated checksum.
 */
uint32_t qca_calc_chksum(const void *data, size_t len, uint32_t checksum);

/**
 * @brief Calculate the offset for a specific NVM image type.
 *
 * This function determines the offset within the NVM data for a given image
 * type. It uses the provided reader instance and ensures the offset is within
 * the bounds of the NVM size. The calculated offset is stored in the location
 * pointed to by the @p offset parameter.
 *
 * @param[in]  type     The type of the NVM image to locate.
 * @param[in]  reader   The reader instance used to access the NVM data.
 * @param[in]  nvm_size The total size of the NVM data.
 * @param[out] offset   Pointer to store the calculated offset for the specified
 *                      image type.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int qca_nvm_offset(qca_nvm_image_t type,
		qca_nvm_reader_t reader, size_t nvm_size, uint32_t *offset);

#if defined(__cplusplus)
}
#endif

#endif /* QCA_NVM_H */
