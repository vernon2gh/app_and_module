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

MAKE = make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) \
       -C $(KERNEL_SRC) O=$(KERNEL_BUILD)               \
       M=$(PWD)

ifneq ($(wildcard mod.c),)
	obj-m += mod.o
endif

ifneq ($(wildcard app.c),)
	COMMON = ../common
	APP ?= $(COMMON)/dynamic_debug.c $(COMMON)/trace.c app.c
	CFLAGS ?= --static -O2 -pthread -I$(COMMON)
	LIBS ?=
endif

all:
ifneq ($(wildcard mod.c),)
	$(MAKE) modules
endif
ifneq ($(wildcard app.c),)
	$(CROSS_COMPILE)gcc $(CFLAGS) $(LIBS) $(APP)
endif

install:
ifneq ($(wildcard mod.c),)
	$(MAKE) INSTALL_MOD_PATH=$(ROOTFS) modules_install
endif

clean:
ifneq ($(wildcard mod.c),)
	$(MAKE) clean
endif
	rm -fr a.out
