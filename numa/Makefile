ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

COMMON = ../common

APP = $(COMMON)/dynamic_debug.c $(COMMON)/trace.c app.c

all:
	$(CROSS_COMPILE)gcc -I$(COMMON) $(APP) -lnuma -pthread

clean:
	rm -fr a.out
