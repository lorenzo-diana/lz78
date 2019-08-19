/*	Provide functions and data structure to decompress a file and extract from
 *	it time and owner info.
 */

#ifdef _WIN32
#include "bitio_lib\bitio.h"
#else
#include "bitio_lib/bitio.h"
#endif
#include "lz78.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

struct nodo {	// element of the tree
	char value;				// value of this node, a character
	uint64_t depth;			// depth of this node
	struct nodo * padre;	// father of this node
};

/*	Read time and owner info from compressed file and save it on decompressed
 *	file. This function must be call after size of the tree was read from file.
 *
 *	@param	par, contain reference to info nedded to interact with input and
 *			output file.
 *	@return	0 un success, -1 if it can't set time info on output file,
 *			-2 if it can't set owner info to output file.
 */
int
read_file_info(struct param * par) {
	int stat=0;
	struct timespec times[2];
	// read last access time, second and nano second
	times[0].tv_sec=bit_read(par->bitio_file, sizeof(times[0].tv_sec)*8, &stat);
	times[0].tv_nsec=bit_read(par->bitio_file, sizeof(times[0].tv_nsec)*8, &stat);
	// read last modification time, second and nano second
	times[1].tv_sec=bit_read(par->bitio_file, sizeof(times[1].tv_sec)*8, &stat);
	times[1].tv_nsec=bit_read(par->bitio_file, sizeof(times[1].tv_nsec)*8, &stat);
	if (futimens(par->text_file, times))	// set time info
		return -1;
	
	// write user and group ID
	uid_t owner_id=bit_read(par->bitio_file, sizeof(uid_t)*8, &stat); // user ID
	gid_t group_id=bit_read(par->bitio_file, sizeof(gid_t)*8, &stat); // group ID
	if (fchown(par->text_file, owner_id, group_id))	// set owner info
		return -2;
	
	if (par->verbose=='t') {	// if verbose info
		printf("Decompressed file info:\n\tLast access time: %s", asctime(localtime(&times[0].tv_sec)));
		printf("\tLast modification time: %s", asctime(localtime(&times[1].tv_sec)));
		printf("\tUser ID: %d\n\tGroup ID: %d\n\n", owner_id, group_id);
	}
	
	return 0;
}

/*	Decompress input file and write the result in output file, then close them.
 *	This function must be call after read_file_info().
 *	
 *	@param	par, contain reference to info nedded to interact with input and
 *			output file.
 *
 *	@return	0 un success, -1 if malloc fail, -2 if write() can't correctly
 *			write to output file.
 */
int
decompress(struct param * par) {
	int stat=0, i;
	//denote how many bit is needed to identify a symbol,depend on alphabet size
	unsigned int bit_per_symbol=ceil(log2((double)par->albero_size)), contatore=0;
	// curr_node 		-> node currently highlighted
	// in_cod	 		-> input symbol ID
	// byte_da_emettere	-> number of byte to emitt
	// curr_node 		-> node currently highlighted
	uint64_t in_cod, temp_cod, byte_da_emettere, curr_node=0, next_free= DIM_ALFABETO + 1;
	char *emissione, ricorsiva=0; // emissione will contain the string to emitt
	struct nodo *n;	// last node of the string to emitt
	// the tree
	struct nodo *albero=malloc(par->albero_size * sizeof(struct nodo));
	if (!albero)	// if malloc() fail
		return -1;

	// initializes the tree
	albero[0].value=0;		// set first node value
	albero[0].depth=0;		// dept is zero because this is the root of the tree
	albero[0].padre=NULL;	// no father exist
	// set up first nodes that rappresent the element of the alphabet
	for(i=0; i< DIM_ALFABETO ; i++) {
		albero[i+1].value=i;
		albero[i+1].depth=1;
		albero[i+1].padre=albero;
	}

	in_cod=bit_read(par->bitio_file, bit_per_symbol, &stat); //read first symbol
	while (stat==bit_per_symbol) {	// go on while we can read a symbol ID
		if (in_cod>next_free) {	// this should never happen!
			printf("ERROR: Unexpected data in compressed file!");
			exit(1);
		}
		
		// in_cod always different from albero_size
		if (in_cod==next_free) {	// if original file have a recursive string
			temp_cod=in_cod; 		// temp_cod is used here *
			in_cod=curr_node;		// we decode again the last symbol
			ricorsiva=1;   // and mark that there si a recursive string to emitt
		}
		byte_da_emettere=albero[in_cod].depth; // how many byte we have to emitt
		if ( !(emissione=malloc(byte_da_emettere)) )	// if malloc() fail
			return -1;
		n=albero+in_cod;	// get last node of the string to emitt
		for (i=byte_da_emettere-1; i>=0; i--, n=n->padre)
			emissione[i]=n->value;	// build the enteire string to emitt
		if (byte_da_emettere != write(par->text_file, emissione, byte_da_emettere) )
			return -2;	// if write can't complete correctly
		contatore+=byte_da_emettere;	// count how many byte we emitt

		// if currently highlighted node is not the root, we add new son to it
		// with the first byte of the string just emitted as value
		if (curr_node != 0) {
			albero[next_free].padre=albero+curr_node;
			// increas deph of the son
			albero[next_free].depth=albero[curr_node].depth+1;
			albero[next_free].value=emissione[0];	// set the value of the son
			next_free++;	// update next ID available
			if (next_free==par->albero_size) {	// if the tree is full
				next_free= DIM_ALFABETO + 1;	// reset the tree
				in_cod=0; // we must remember the reset of the tree,used here **
			}
		}
		if (ricorsiva) {	// if a recursive string have to be emitted
			// emit first byte of the string just emitted, in order to obtain
			// a recursive string in output
			if (1 != write(par->text_file, emissione, 1))	// if write fail
				return -2;
			ricorsiva=0;	// recersive string emitted, unmark recursive flag
			contatore++;	// count how many byte we emitt
			curr_node=temp_cod; // *
		}
		else
			curr_node=in_cod; // **
		// release memory, for each emission we allocate it again
		free(emissione);
		
		// read next symbol from compressed file
		in_cod=bit_read(par->bitio_file, bit_per_symbol, &stat);
	}
	free(albero);	// release memory used for the tree

	if (par->verbose=='t')	// print addictional info
		printf("\tBit per symbol: %u\n\tCharacters emitted: %u\n", bit_per_symbol, contatore);
	
	bit_close(par->bitio_file);	// close input file
	close(par->text_file);	// close output file
	return 0;	// on success
}
