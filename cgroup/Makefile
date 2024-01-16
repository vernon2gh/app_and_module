ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

APP = app.c

all:
	$(CROSS_COMPILE)gcc --static $(APP)

clean:
	rm -fr a.out
