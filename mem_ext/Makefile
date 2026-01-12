ifeq ($(CROSS_COMPILE),)
	CROSS_COMPILE =
endif

APP = app.c

all:
	$(CROSS_COMPILE)gcc --static $(APP)

	## wget https://www.cs.virginia.edu/stream/FTP/Code/stream.c
	$(CROSS_COMPILE)gcc -O3 -march=native -fopenmp -DSTREAM_ARRAY_SIZE=80000000 -DNTIMES=100 stream.c -o stream
clean:
	rm -fr a.out stream
