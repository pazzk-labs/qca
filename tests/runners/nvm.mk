COMPONENT_NAME = NVM

SRC_FILES = \
	../src/nvm.c \
	../external/libmcu/modules/common/src/ringbuf.c \

TEST_SRC_FILES = \
	src/nvm_test.cpp \
	stubs/logging.c \
	src/test_all.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	../include \
	../external/libmcu/modules/logging/include \
	../external/libmcu/modules/common/include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -DUNIT_TEST \
		    -include ../external/libmcu/modules/logging/include/libmcu/logging.h \
		    -DQCA_DEBUG=debug \

include runners/MakefileRunner
