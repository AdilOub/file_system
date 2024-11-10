#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define FILENAME "hdd.bin"
#define NB_INDIRECTION 6

struct block{
    uint64_t info;
    uint64_t parent;
    uint64_t block_address;
    uint64_t* indirection;
};
typedef struct block block;
//info
//8 bits pour le type de block (0 = free, 1 = directory, 10 = data block, 11=indirection block)


char* read(char* buffer, int size, int offset){
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file in read\n");
        return NULL;
    }
    fseek(file, offset, SEEK_SET);
    fread(buffer, size, 1, file);
    fclose(file);
    return buffer;
}

uint64_t* read_uint64(int offset, int size) {
    char **buffer = malloc(sizeof(char) * 8);
    for(int i = 0; i<8 ; i++){
        buffer[i] = malloc(sizeof(char)*8);
    }
    
    for(int i=0; i<size; i++){
       read(buffer[i], 8, offset + i * 8);
    }
    return *(uint64_t*)buffer;
}
void write(char* buffer, int size, int offset) {
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file in writie\n");
        return;
    }
    fseek(file, 0, SEEK_SET);
    fwrite(buffer, size, 1, file);
    fclose(file);
    return;
}
uint64_t little_to_big_endian(uint64_t x){
    uint64_t b0, b1, b2, b3, b4, b5, b6, b7;
    b0 = (x & 0x00000000000000FF) << 56;
    b1 = (x & 0x000000000000FF00) << 40;
    b2 = (x & 0x0000000000FF0000) << 24;
    b3 = (x & 0x00000000FF000000) << 8;
    b4 = (x & 0x000000FF00000000) >> 8;
    b5 = (x & 0x0000FF0000000000) >> 24;
    b6 = (x & 0x00FF000000000000) >> 40;
    b7 = (x & 0xFF00000000000000) >> 56;
    return b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7;
    }

void read_block(block* block, int offset){
    char buffer[512];
    read(buffer, 512, offset);
    block->info = little_to_big_endian(*(uint64_t*)buffer);
    block->parent = little_to_big_endian(*(uint64_t*)(buffer + 8));
    block->block_address = offset;
    for (int i = 0; i < NB_INDIRECTION; i++) {
        block->indirection[i] = little_to_big_endian(*(uint64_t*)(buffer + 16 + i * 8));
    }
    return;
}

void print_bin(uint64_t n){
    for(int i = 63; i >= 0; i--){
        printf("%ld", (n >> i) & 1);
    }
    printf("\n");
}

int is_block_dir(block* block){
    uint64_t info = block->info>>56;
    return info == 0x80; //0b1000 0000
}

int is_free_block(block* block){
    uint64_t info = block->info>>56;
    return info == 0x0; //0x00 = 0000 0000
}

int is_data_block(block* block){
    uint64_t info = block->info>>56;
    return info == 0x40; //0x4 = 0100 0000
}

int is_indirection_block(block* block){
    uint64_t info = block->info>>56;
    return info == 0xC0; //0xC = 1100 0000
}



uint64_t get_block_size(block* block){
    return block->info & 0x00FFFFFFFFFFFFFF;
}

uint64_t get_block_address(block* block){
    return block->parent;
}

block* init_block(int type){
    block* block = malloc(sizeof(block));
    block->info = 0;
    block->info |= (1UL << 60);
    block->indirection = malloc(NB_INDIRECTION * sizeof(uint64_t));
    for(int i = 0; i < NB_INDIRECTION; i++){
        block->indirection[i] = 0;
    }
    return block;
}

char* read_block_data(block* block){
    if(is_block_dir(block)){
        fprintf(stderr, "Error: block is not a data block\n");
        return NULL;
    }
    uint64_t size = get_block_size(block);
    printf("Size: %lu\n", size);
    char* data = malloc(size);
    printf("Adresse: %lu\n", block->block_address+16);
    read(data, size, block->block_address + 16);
    return data;
}


uint64_t* read_block_indrection(block* block){
    if(!is_indirection_block(block)){
        fprintf(stderr, "Error: block is not a directory block\n");
        return NULL;
    }
    uint64_t size = get_block_size(block); //nombre d'indirection dans le block (max NB_INDIRECTION)
    if(size > NB_INDIRECTION){
        fprintf(stderr, "Error: block has too many indirection\n");
        return NULL;
    }
    uint64_t* data = malloc(size * sizeof(uint64_t)); //chaque data va contenir une addresse de block
    for(int i = 0; i < size; i++){
        data[i] = read_uint64(block->block_address + 16 + i * 8, size);
    }
    return data;
}   



int main() {
    block *racine = init_block(1);
    read_block(racine, 0);
    printf("Is_dir: %d\n", is_block_dir(racine));
    printf("Size: %lu\n", get_block_size(racine));
    for(int i = 0; i < NB_INDIRECTION; i++){
        printf("Indirection %d: %"PRIu64"\n", i, racine->indirection[i]);
    }

    return 0;
}