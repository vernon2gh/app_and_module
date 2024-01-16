ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

COMMON = ../common

APP = $(COMMON)/dynamic_debug.c $(COMMON)/trace.c app.c

all:
	$(CROSS_COMPILE)gcc --static -I$(COMMON) $(APP)

clean:
	rm -fr a.out
