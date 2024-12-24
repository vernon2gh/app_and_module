ifeq ($(KERNEL_SRC),)
	KERNEL_SRC = /usr/src/linux-headers-`uname -r`
endif

ifeq ($(KERNEL_BUILD),)
	KERNEL_BUILD = $(KERNEL_SRC)
endif

ifeq ($(ROOTFS),)
	ROOTFS = /
endif

ifeq ($(ARCH),)
	ARCH = x86
endif

ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

COMMON = ../common

MAKE = make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
       -C $(KERNEL_SRC) O=$(KERNEL_BUILD)               \
       M=$(PWD) EXTRA_CFLAGS=-I$(PWD)/include ## Specify a custom header file path

obj-m += mod.o

all:
	$(MAKE) modules

install:
	$(MAKE) INSTALL_MOD_PATH=$(ROOTFS) modules_install

clean:
	$(MAKE) clean
