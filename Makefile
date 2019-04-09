#CCFLAGS = -O2 -Wall -Werror
CCFLAGS = -O0 -g
LINK_FLAGS = -lm

all: lz78
clean:
	rm *o lz78

bitio.o: bitio.h bitio.c
	cc $(CCFLAGS) -c bitio.c
lz78_compressor.o: bitio.h lz78.h lz78_compressor.c
	cc $(CCFLAGS) -c lz78_compressor.c $(LINK_FLAGS)
lz78_decompressor.o: bitio.h lz78.h lz78_decompressor.c
	cc $(CCFLAGS) -c lz78_decompressor.c $(LINK_FLAGS)

lz78: lz78.c bitio.o lz78_compressor.o lz78_decompressor.o
	cc $(CCFLAGS) -o lz78 lz78.c bitio.o $(LINK_FLAGS)
