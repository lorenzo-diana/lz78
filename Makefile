#CCFLAGS = -O2 -Wall -Werror
CCFLAGS = -O0 -g
LINK_FLAGS = -lm
BITIO_DIR = ./bitio_lib

all: lz78
clean:
	rm *o lz78
	cd $(BITIO_DIR) && make $@

$(BITIO_DIR):
	if [ ! -d $(BITIO_DIR) ]; then \
		mkdir -p $(BITIO_DIR); \
		git clone https://github.com/lorenzo-diana/bitiolib $(BITIO_DIR); \
	fi
bitio.o: $(BITIO_DIR)
	cd $(BITIO_DIR) && make
lz78_compressor.o: $(BITIO_DIR)/bitio.h lz78.h lz78_compressor.c
	cc $(CCFLAGS) -c lz78_compressor.c $(LINK_FLAGS)
lz78_decompressor.o: $(BITIO_DIR)/bitio.h lz78.h lz78_decompressor.c
	cc $(CCFLAGS) -c lz78_decompressor.c $(LINK_FLAGS)

lz78: lz78.c bitio.o lz78_compressor.o lz78_decompressor.o
	cc $(CCFLAGS) -o lz78 lz78.c $(BITIO_DIR)/bitio.o $(LINK_FLAGS)
