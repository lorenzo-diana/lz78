/*	Contains the main() function that allow to get the parameter nedded to *	perform compression/decompression of a file using bitio library. * *	Usage example: *	To compress use: 	./lz78 -c -s 1024 -i in_file.txt -o out_c.txt *	To decompress use:	./lz78 -d -i out_c.txt -o out_file.txt * *	Accepted parameters: *	-c						compress command *	-d						decompress command *	-i	<filename>			input file name parameter *	-o	<filename>			output file name parameter *	-v						verbose command *	-s	<dictionary size>	dictionary size parameter */#include "lz78.h"#include "lz78_compressor.c"#include "lz78_decompressor.c"#include "bitio.h"#include <unistd.h>#include <stdio.h>#include <stdlib.h>#include <fcntl.h>#include <sys/types.h>#include <sys/stat.h>#include <time.h>#include <utime.h>#include <errno.h>/*	Print info and instructions on program usage on standard output. */voidprint_help(){	printf("Usege: lz78 -[c/d] -i <filename> -o <filename> [OPTION]...\nCompress or decompress a file.\n\nAccepted arguments:\n-c\t\t\tCompress a file\n-d\t\t\tDecompress a file\n-i <filename>\t\tSpecify input file\n-o <filename>\t\tSpecify output file\n-v\t\t\tVerbose, print aditional information\n-s <dictionary size>\tSpecify dictionary size, compression only. This must be a positive value\n");}/*	Get parameters, check their correctness and pass it to the functions to *	compress/decompress a file. */intmain (int argc, char* argv[]){	extern char *optarg;	// contain eventual parameter	const char* option = "cdi:o:s:v";	// option for parameter	int operation;	// cd -> is 'c' for compress command, 'd' for decompress command	char cd=0, * input_filename = NULL, * output_filename = NULL;	struct stat original_file, compressed_file;	// create the structure that will contain the parameter	struct param * par=malloc(sizeof(struct param));	if (!par) {		perror("Error: malloc() fail!\n");		return -1;	}	par->verbose='f';		// false by default	par->albero_size=1024;	// defaul size	if (argc < 4) {	// we need at least 3 parameters		print_help();		return -1;	}	while((operation=getopt(argc,argv,option)) != -1) {	// get parameters		switch(operation) {			case 'c':								// compression						cd='c';						break ;			case 'd':								// decompression						cd='d';						break ;			case 'i':								// input file name						input_filename=optarg;						break ;			case 'o':								// output file name						output_filename=optarg;						break ;			case 'v': 								// verbose flag set						par->verbose='t';						break ;			case 's':								// dictionary size						if (optarg[0] == '-') {	// if is a negative value							perror("Use positive value for dictionary size!\n");							return -1;						}						par->albero_size=strtoull(optarg, NULL, 10);						// if size is less than DIM_ALFABETO						if (par->albero_size < DIM_ALFABETO +1)							par->albero_size=1024;	// use default size						break ;			default:	// unknown parameter						printf("Error: unknow parameter!\n");						return -1;		}	}	// this three parameter are required	if (!cd || !input_filename || !output_filename) {		printf("Yuo must provide compress/decompress command and input and output filename!\n");		print_help();		exit(-1);	}	if (par->verbose=='t')	// print additional info		printf("Input file:\t%s\nOutput file:\t%s\n", input_filename, output_filename);	if (cd=='c') {	// if compression command is set		// set the size of the table to 4 times the size of the tree		par->hash_table_size=4*(par->albero_size);		// if this value is too high use the same value of the tree		if (par->hash_table_size < par->albero_size)			par->hash_table_size=par->albero_size;		if (par->verbose=='t')			printf("Tree size:\t%lu nodes\nHash table size: %lu elements\n\n", par->albero_size, par->hash_table_size);		// open input file		par->text_file=open(input_filename, O_RDONLY);		// open output file		par->bitio_file=bit_open(output_filename, 'w');		// write size of the tree to output file		if (bit_write(par->bitio_file, par->albero_size, sizeof(par->albero_size)*8) != sizeof(par->albero_size)*8) {			printf("Write error!\n");			exit(-1);		}		// write time and owner info from compressed file		if (write_file_info(par) == -1) {	// if error occur, exit			perror("Error writing file info!\n");			exit(-1);		}		printf("Compression of %s started.\n", input_filename);		if (compress(par) == -1) {	// compress file, if error occur exit			perror("Error in malloc()\n");			exit(-1);		}		// if compressed file is larger than original file, delete compressed file		if (stat(input_filename, &original_file) || stat(output_filename, &compressed_file))			printf("Impossible to check the size of the compressed file! Check it manually.\n");		else			if (original_file.st_size <= compressed_file.st_size) {				printf("\nCompressed file is larger than original file!\nCompressed file deleted.\n");				unlink(output_filename);				exit(1);			}	}	else {	// else decompress command is set		// open output file		par->text_file=open(output_filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);		// open input file		par->bitio_file=bit_open(input_filename, 'r');		// read size of the tree form compressed file		par->albero_size=bit_read(par->bitio_file, sizeof(par->albero_size)*8, &operation);		if (operation < 0) {			printf("Read error!\n");			exit(-1);		}		// set the size of the table to 4 times the size of the tree		par->hash_table_size=4*(par->albero_size);		// if this value is too high use the same value of the tree		if (par->hash_table_size < par->albero_size)			par->hash_table_size=par->albero_size;		if (par->verbose=='t')			printf("Tree size:\t%lu nodes\nHash table size: %lu elements\n\n", par->albero_size, par->hash_table_size);		// read time and owner info from compressed file		if (read_file_info(par) < 0) {	// if error occur			printf("Error reading file info!\n");	// print a message			exit(-1);	// and exit		}		printf("Decompression of %s started.\n", input_filename);		if (decompress(par) < 0) {	// decompress file, if error occur			perror("Error during decompression!\n");	// print a  message			exit(-1);	// and exit		}	}	printf("Done!\n");	return 0;	// if no errors occur}