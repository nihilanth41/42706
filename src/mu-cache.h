/******************************************************************************/
/* CACHE STRUCTURE                                                            */
/******************************************************************************/
#define NUM_CACHE_BLOCKS 16
#define WORD_PER_BLOCK 4


typedef struct CacheBlock_Struct {

  int valid; //indicates if the given block contains a valid data. Initially, this is 0
  uint32_t tag; //this field should contain the tag, i.e. the high-order 32 - (2+2+4)  = 24 bits
  uint32_t words[WORD_PER_BLOCK]; //this is where actual data is stored. Each word is 4-byte long, and each cache block contains 4 blocks.
  
  
} CacheBlock;

typedef struct Cache_Struct {

  CacheBlock blocks[NUM_CACHE_BLOCKS]; // there are 16 blocks in the cache
  
} Cache;

// Write buffer for store instructions
uint32_t write_buffer[WORD_PER_BLOCK]; 

/***************************************************************/
/* CACHE STATS                                                 */
/***************************************************************/
uint32_t cache_misses; //need to initialize to 0 at the beginning of simulation start
uint32_t cache_hits;   //need to initialize to 0 at the beginning of simulation start


/***************************************************************/
/* CACHE OBJECT                                                */
/***************************************************************/
Cache L1Cache; //need to use this in the simulator 

// Already assume when calling this that the address is in the cache. 
// So just decode and return 
uint32_t cache_read_32(uint32_t addr) {
	uint32_t index = (addr & 0x000000F0) >> 4;
	uint32_t word_offset = (addr & 0x0000000C) >> 2;
 	uint32_t value = L1Cache.blocks[index].words[word_offset];
	printf("Cache read index: %u\tword offset: %u\tvalue: %u\n", index, word_offset, value);
	return value;
}

// Return true(1) if hit, false(0) if miss
int cache_isHit(uint32_t addr) {
	int i=0;
	uint32_t index = (addr & 0x000000F0) >> 4;
	uint32_t tag = (addr & 0xFFFFFF00) >> 8;
	//uint32_t word_offset = (addr & 0x0000000C) >> 2;
	//uint32_t byte_offset = (addr & 0x00000003);
	// Look for index in cache
	for(i=0; i<NUM_CACHE_BLOCKS; i++) {
		if( (L1Cache.blocks[index].tag == tag) && (1 == L1Cache.blocks[index].valid) ) // Tags match & Valid
		{ 
			printf("HIT!!!~!~!\n");
			cache_hits++;
			return 1; // hit
		}

	}
	cache_misses++; 
	printf("MISSSS!!!~!~!\n");
	return 0; // miss
}

// Call this on cache *miss*, load cache line from addr, and return the appropriate word
uint32_t cache_load_32(uint32_t addr) { 
	printf("Address: %u\n", addr);
	uint32_t index = (addr & 0x000000F0) >> 4; // Block number
	uint32_t tag = (addr & 0xFFFFFF00) >> 8;
	uint32_t word_offset = (addr & 0x0000000C) >> 2;
	int i=0;
	for(i=0; i<WORD_PER_BLOCK; i++) {
		uint32_t load_addr = (addr & 0xFFFFFFF0) + (i*4);
		L1Cache.blocks[index].tag = tag;
		L1Cache.blocks[index].valid = 1;
		L1Cache.blocks[index].words[i] = mem_read_32(load_addr);
	}
	// Return word that resulted in cache miss
	printf("Cache line: %8x\tindex: %u,\tword_offset: %u\n", L1Cache.blocks[index].words[word_offset], index, word_offset);
	return L1Cache.blocks[index].words[word_offset];
}

// This is to modify a single word in a cache block/line - for store instructions.
void cache_write_32(uint32_t addr, uint32_t value) {
	uint32_t index = (addr & 0x000000F0) >> 4;
	uint32_t word_offset = (addr & 0x0000000C) >> 2;
	L1Cache.blocks[index].words[word_offset] = value;
	printf("Cache write index: %u\tword offset: %u\tvalue: %u\n", index, word_offset, value);
}
