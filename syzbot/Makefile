ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

all:
	$(CROSS_COMPILE)gcc --static -o repro repro.c
	./repro
	## ./syz-execprog -enable=all -repeat=0 -procs=6 ./repro.syz

clean:
	rm -fr repro syzkaller*
