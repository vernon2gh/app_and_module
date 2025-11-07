ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

APP = app.c

all:
	$(CROSS_COMPILE)gcc -O2 --static $(APP)
	$(CROSS_COMPILE)gcc -O2 --static -o tlb_demo tlb_demo.c

clean:
	rm -fr a.out tlb_demo
