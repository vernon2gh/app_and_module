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
       M=$(PWD)

obj-m += mod.o
APP = $(COMMON)/dynamic_debug.c $(COMMON)/trace.c app.c

all:
	$(MAKE) modules
	$(CROSS_COMPILE)gcc --static -I$(COMMON) $(APP)

install:
	$(MAKE) INSTALL_MOD_PATH=$(ROOTFS) modules_install

clean:
	$(MAKE) clean
	rm -fr a.out
