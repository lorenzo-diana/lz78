/*	Provide functions and data structure to compress a file and add to it time
 *	and owner info.
 */

#include "bitio.h"
#include "lz78.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <sys/stat.h>
#include <utime.h>
#include <time.h>	// for verbose info

// first 8 byte is the ID of the father, the last byte is the son value
#define DIM_KEY 9
#define SEED 0x776D85588A7C3252

// We consider hash table like circolar list. If a collision occur at certain
// element, we scroll the hash table untill we found an ampty element.
struct el {	// element of the hash table
	uint64_t figlio;			// id of the son of this node
	char collisione[DIM_KEY];	// id of the node
};

/*	Hash function by Daniel J. Bernstein with seed.
 *
 *	@param	str, string on which to calculate the hash.
 *			len, length of str.
 *			seed, value used to calculate the hash.
 *
 *	@return	hash
 */
uint64_t
djb(unsigned char *str, uint64_t len, uint64_t seed) {
	uint64_t i, hash = ((5381uLL << 5) + 5381uLL) + seed;
    for (i=0; i<len ; i++)
        hash = ((hash << 5) + hash) + *(str++); /* hash * 33 + c */
    return hash;
}

/*	Clean and initializes the hash table.
 *
 *	@param	tab, reference to the hash table to be initialized.
 *			hahs_table_size, dimension of the has table tab.
 *
 *	@return	void.
 */
void
init_h_table(struct el *tab, uint64_t hash_table_size) {
	int a;
	unsigned char key[ DIM_KEY ];
	uint64_t k;
	memset(tab, 0, hash_table_size * sizeof(struct el) );	// clean hash table
	memset(key, 0, DIM_KEY-1);								// clean key
	for (a=0; a<DIM_ALFABETO; a++) {	// for each element of the alphabet
		key[DIM_KEY-1]=a;	// set the key for element a
		k=djb(key, DIM_KEY, SEED)%hash_table_size;	// find its position
		while (tab[k].figlio)	// avoid collision
				k=(k+1) % hash_table_size;	// consider hash table like circolar list
		tab[k].figlio=a+1;	// set the entry for element a
		tab[k].collisione[DIM_KEY-1]=a;
	}
}

/*	Read time and owner info from input file and save it in compressed file.
 *	This function must be call after size of the tree was written.
 *
 *	@param	par, contain reference to info nedded to interact with input and
 *			output file.
 *
 *	@return	0 un success, -1 if it can't read time and user info from the
 *			input file.
 */
int
write_file_info(struct param * par) {
	struct stat info;
	if (fstat(par->text_file , &info))	// get stat from file
		return -1;	// if error, return
	
	// write last access time, second and nano second
	bit_write(par->bitio_file, (uint64_t)info.st_atim.tv_sec, sizeof(info.st_atim.tv_sec)*8);
	bit_write(par->bitio_file, (uint64_t)info.st_atim.tv_nsec, sizeof(info.st_atim.tv_nsec)*8);
	// write last modification time, second and nano second
	bit_write(par->bitio_file, (uint64_t)info.st_mtim.tv_sec, sizeof(info.st_mtim.tv_sec)*8);
	bit_write(par->bitio_file, (uint64_t)info.st_mtim.tv_nsec, sizeof(info.st_mtim.tv_nsec)*8);
	// write user and group ID
	bit_write(par->bitio_file, (uint64_t)info.st_uid, sizeof(info.st_uid)*8);
	bit_write(par->bitio_file, (uint64_t)info.st_gid, sizeof(info.st_gid)*8);
	
	if (par->verbose=='t') {	// if user wants additional info
		printf("Input file info:\n\tLast access time: %s", asctime(localtime(&info.st_atim.tv_sec)));
		printf("\tLast modification time: %s", asctime(localtime(&info.st_mtim.tv_sec)));
		printf("\tOwner user ID: %d\n\tOwner group ID: %d\n\n", info.st_uid, info.st_gid);
	}
	
	return 0; // on success
}

/*	Compress input file and write the result in output file, then close them.
 *	This function must be call after write_file_info().
 *	
 *	@param	par, contain reference to info nedded to interact with input and
 *			output file.
 *
 *	@return	0 un success, -1 if malloc fail.
 */
int
compress(struct param * par) {
	unsigned char key[DIM_KEY];	// key to be hash to find an element
	//denote how many bit is needed to identify a symbol,depend on alphabet size
	unsigned int bit_per_symbol=ceil(log2((double)par->albero_size));
	// curr_node -> node currently highlighted
	// contatore -> number of character read from input file
	// key_pre   -> pointer to the first 8 byte of the key
	uint64_t curr_node=0, next_free= DIM_ALFABETO + 1, K, contatore=0, *key_pre=(uint64_t *)key;
	char in_c;	// input symbol ID
	int libera;
	struct el *h_table=malloc( par->hash_table_size * sizeof(struct el) );
	if (!h_table)	// if malloc() fail
		return -1;
	init_h_table(h_table, par->hash_table_size);	// iniziatialize hash tabel
	
	while (read(par->text_file, &in_c, 1) != 0) {	// untill EOF is reached
		contatore++;
		*key_pre=curr_node;	// set the father part of the key
		key[DIM_KEY-1]=in_c;	// set the son part of the key
		K=djb(key, DIM_KEY, SEED)%par->hash_table_size;	// index of the element
		libera=1;	// assume no collision is detected initially
		// if element is already taken this is a collision, we scroll the table
		// to find the element we are looking for more ahead or an ampty element
		if (h_table[K].figlio) {
			while (memcmp(h_table[K].collisione, key, DIM_KEY) && (libera=memcmp(h_table[K].collisione, "\0\0\0\0\0\0\0\0\0", DIM_KEY)))
				K=(K+1) % par->hash_table_size;	// consider hash table like circolar list
			if (libera) { // if element is found
				curr_node=h_table[K].figlio;	// update current node
				continue ;
			} // else -> we have to add a new node
		}
		// emitt the ID of the current node
		bit_write(par->bitio_file, curr_node, bit_per_symbol);
		if (next_free==par->albero_size) {	// if the tree is full
			next_free= DIM_ALFABETO + 1;	// set next ID free
			init_h_table(h_table, par->hash_table_size); //initialize hash table
			*key_pre=0;	// reset key with the first node as father
			key[DIM_KEY-1]=in_c;	// and the last input character as son
			K=djb(key, DIM_KEY, SEED)%par->hash_table_size;
			curr_node=h_table[K].figlio;	// update current node
			continue ;
		}
		// if we reach this point the element rappresented by tha actual key
		// isn't in the tree, so we add it
		h_table[K].figlio=next_free++;
		memcpy(h_table[K].collisione, key, DIM_KEY);
		
		// set the new current node
		*key_pre=0;
		curr_node=h_table[djb(key, DIM_KEY, SEED)%par->hash_table_size].figlio;
	}
	free(h_table);	// free hash tsable
	bit_write(par->bitio_file, curr_node, bit_per_symbol); // emitt last node ID
	
	if (par->verbose=='t')	// print verbose info
		printf("\tBit per symbol: %u\n\tCharacters read: %lu\n", bit_per_symbol, contatore);
	
	close(par->text_file);	// close input file
	bit_close(par->bitio_file);	// close output file
	return 0;	// on success retorn 0
}
