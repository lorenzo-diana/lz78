/*	Data structure used to pass parameters to functions that need it in
 *	lz78_compressor.c and lz78_decompressor.c
 */
#ifndef _LZ78_H_57385679_
#define _LZ78_H_57385679_

#include <stdint.h>
#define DIM_ALFABETO 256		// 1 byte

struct param {	// struct that contain all the parameter needed by the program
	uint64_t albero_size;		// > DIM_ALFABETO
	uint64_t hash_table_size;	// >= albero_size, used only in compressor
	int text_file;				// file descriptor of standard file
	struct bitio * bitio_file;	// file descriptor of bitio file
	char verbose;				// 1 -> print more info
};
#endif	/* ! _LZ78_H_57385679_ */
