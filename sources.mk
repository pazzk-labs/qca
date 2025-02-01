ifneq ($(QCA_ROOT),)
	qca-basedir := $(QCA_ROOT)/
endif

QCA_SRCS := \
$(qca-basedir)src/qca.c \
$(qca-basedir)src/mme.c \
$(qca-basedir)src/nvm.c \

QCA_INCS := $(qca-basedir)include
