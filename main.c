#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define FILENAME "hdd.bin"


struct block{
    uint64_t info;
    uint64_t* indirection;
};
typedef struct block block;

char* read(char* buffer, int size, int offset){
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file in read\n");
        return NULL;
    }
    fseek(file, 0, SEEK_SET);
    fread(buffer, size, 1, file);
    fclose(file);
    return buffer;
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
    return ((x & 0x00000000000000FF) << 56) | ((x & 0x000000000000FF00) << 40) | ((x & 0x0000000000FF0000) << 24) | ((x & 0x00000000FF000000) << 8) | ((x & 0x000000FF00000000) >> 8) | ((x & 0x0000FF0000000000) >> 24) | ((x & 0x00FF000000000000) >> 40) | ((x & 0xFF00000000000000) >> 56);
}

void read_block(block* block, int offset){
    char buffer[512];
    read(buffer, 512, offset);
    block->info = little_to_big_endian(*(uint64_t*)buffer);
    for (int i = 0; i < 7; i++) {
        block->indirection[i] = little_to_big_endian(*(uint64_t*)(buffer + 8 + i * 8));
    }
    return;
}

int is_block_dir(block* block){
    return (block->info & (1UL << 60))!=0; //attention on lit en little endian
}

uint64_t get_block_size(block* block){
    return block->info & 0x00FFFFFFFFFFFFFF;
}

block* init_block(int type){
    block* block = malloc(sizeof(block));
    block->info = 0;
    block->info |= (1UL << 60);
    block->indirection = malloc(7 * sizeof(uint64_t));
    for(int i = 0; i < 7; i++){
        block->indirection[i] = 0;
    }
    return block;
}

//todo
char* read_block_data(block* block){
    if(is_block_dir(block)){
        fprintf(stderr, "Error: block is not a data block\n");
        return NULL;
    }
    uint64_t size = get_block_size(block);
    char* data = malloc(size);

}

//todo
char* read_block_indrection(block* block){
    if(!is_block_dir(block)){
        fprintf(stderr, "Error: block is not a directory block\n");
        return NULL;
    }
    uint64_t size = get_block_size(block); //nombre d'indirection dans le block (max 7)
    char* data = malloc(size * 8); //chaque data va contenir une addresse de block
}

void print_bin(uint64_t n){
    for(int i = 63; i >= 0; i--){
        printf("%ld", (n >> i) & 1);
    }
    printf("\n");
}

int main() {
    block *racine = init_block(1);
    read_block(racine, 0);
    printf("Is_dir: %d\n", is_block_dir(racine));
    printf("Size: %lu\n", get_block_size(racine));
    printf("Indirection: %lu\n", racine->indirection[0]);
    for(int i = 0; i < 7; i++){
        printf("Indirection %d: %lu\n", i, racine->indirection[i]);
    }
    return 0;
}