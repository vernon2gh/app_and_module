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

APP = $(COMMON)/dynamic_debug.c $(COMMON)/trace.c app.c

all:
	$(CROSS_COMPILE)gcc --static -I$(COMMON) $(APP)

clean:
	rm -fr a.out
