#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define FILENAME "hdd"
#define BLOCK_SIZE 512 //taille en byte !


char* read(char* buffer, int size, int offset){
    FILE* file = fopen(FILENAME, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file in read\n");
        return NULL;
    }
    fseek(file, offset, SEEK_SET);
    int r = fread(buffer, size, 1, file);
    fclose(file);
    return buffer;
}

void write(char* buffer, int size, int offset) {
    FILE* file = fopen(FILENAME, "w");
    if (file == NULL) {
        fprintf(stderr, "Error opening file in writie\n");
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(buffer, size, 1, file);
    fclose(file);
    return;
}

struct block{
    uint8_t type;
    uint64_t size;
    uint64_t* indirection;
};
typedef struct block block;

block* read_block_brut(uint64_t address){
    char* buffer = calloc(BLOCK_SIZE, sizeof(char));
    buffer = read(buffer, BLOCK_SIZE, address);
    printf("Info: '%s'\n", buffer);
    return NULL;

}

int main(){
    read_block_brut(0);
}