#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include "qca/nvm.h"
#include <stdio.h>
#include <stdlib.h>

static size_t nvm_reader(void *buf, size_t bufsize, void *ctx) {
	FILE *file = (FILE *)ctx;
	int bytes_read = fread(buf, 1, bufsize, file);
	return (bytes_read > 0)? (size_t)bytes_read : 0;
}

static void on_nvm_header(const qca_nvm_header_t *header, void *ctx) {
	printf("Mask %4x, Addr %4x %6x, ", header->AppletExecuteMask,
			header->ImageNvmAddress, header->ImageMemoryAddress);
	printf("Image size: %7u, chksum %8x, ",
			header->ImageLength, header->ImageChecksum);
	printf("Ptr entry %8x, next %8x, prev %8x, ", header->AppletEntryPtr,
			header->NextNvmHeaderPtr, header->PreviousNvmHeaderPtr);
	printf("Type %2x, id %x, subid %x, ", header->EntryType,
			header->ModuleId, header->ModuleSubId);
	printf("Entry Ver %x, hdr chksum %x", header->AppletEntryVersion, header->HeaderChecksum);
	printf("\n");
}

TEST_GROUP(NVM) {
	FILE *file;
	size_t filesize;

	void setup(void) {
		const char *filename = "tests/assets/MAC-QCA7000-QCA7005-GP-v3.3.0.0010-00-X-ED.nvm";
		file = fopen(filename, "rb");
		fseek(file, 0, SEEK_END);
		filesize = (size_t)ftell(file);
		fseek(file, 0, SEEK_SET);
	}
	void teardown(void) {
		fclose(file);

		mock().checkExpectations();
		mock().clear();
	}
};

TEST(NVM, t) {
	qca_nvm_iterate(nvm_reader, filesize, on_nvm_header, file);
}
