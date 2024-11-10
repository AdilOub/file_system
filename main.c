#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define FILENAME "hdd.bin"
#define BLOCK_SIZE 128 //block size en bytes

#define DISK_SIZE 0x1000000 //16MB

uint64_t offset = 0;


struct inode{
    uint64_t type;
    uint64_t size;
    uint64_t blockPointers[12];
    uint64_t singleIndirectPointer;
    uint64_t parent;
};
typedef struct inode inode;

struct inode_descriptor{
    uint64_t name_high;
    uint64_t name_low;
    uint64_t creation_time;
    uint64_t modification_time;
    uint64_t access_rights;
    uint64_t inode_pointer;
    uint64_t align[10]; //ulgy ass hack to make the struct 128 bytes :sob:
};

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

inode* read_inode(uint64_t inode_address){
    inode* node = (inode*)malloc(sizeof(inode));
    char* buffer = (char*)malloc(sizeof(inode));
    read(buffer, sizeof(inode), inode_address);
    node = (inode*)buffer;
    return node;
}

void print_inode(inode* node){
    printf("Type: %lu\n", node->type);
    printf("Size: %lu\n", node->size);
    printf("Block Pointers: \n");
    for (int i = 0; i < 12; i++){
        printf("%d: %lu\n", i, node->blockPointers[i]);
    }
    printf("\n");
    printf("Single Indirect Pointer: %lu\n", node->singleIndirectPointer);
    printf("Parent: %lu\n", node->parent);
}

//TODO return inode_descriptor** instead of inode**
inode** read_dir(inode* dir){
    if(dir->type != 1){
        fprintf(stderr, "Not a directory\n");
        return NULL;
    }

    inode** nodes = (inode**)malloc(sizeof(inode*) * 12);
    for (int i = 0; i < dir->size; i++){
        nodes[i] = read_inode(dir->blockPointers[i]);
    }
    return nodes;
}

char* read_file(inode* file){
    if(file->type != 2){
        fprintf(stderr, "Not a file\n");
        return NULL;
    }

    char* buffer = (char*)malloc(file->size);
    for (int i = 0; i < file->size; i++){
        char temp;
        read(&temp, sizeof(char), file->blockPointers[i / BLOCK_SIZE] + i % BLOCK_SIZE);
        buffer[i] = temp;
    }
    return buffer;
}

uint64_t find_free_inode_block(){
    uint64_t address=0;
    while(address<DISK_SIZE){
        inode* node = read_inode(address);
        if(node->type == 0){
            return address;
        }
        address += sizeof(inode);
    }
    return -1;
}

uint64_t find_free_data_block(){
    uint64_t address=DISK_SIZE/2; //on laisse de la place pour les inodes au début
    while(address<DISK_SIZE){
        inode* node = read_inode(address);
        if(node->type == 0){
            return address;
        }
        address += sizeof(inode);
    }
    return -1;
}

uint64_t write_inode(inode* node){
    uint64_t address = find_free_inode_block();
    printf("Free inode block at %lu\n", address);
    if(address == -1){
        fprintf(stderr, "No free inode block\n");
        return -1;
    }
    write((char*)node, sizeof(inode), address);
    return address;
}

int main(int argc, char* argv[]){
    inode* racine = read_inode(0);
    print_inode(racine);

    printf("Reading directory\n");
    inode** nodes = read_dir(racine);
    for (int i = 0; i < racine->size; i++){
        printf("Node %d\n\n\n", i);
        print_inode(nodes[i]);
    }

    if(racine->size == 0){
        printf("Racine vide\n");
        return 0;
    }
    
    inode* file = nodes[0];
    printf("Reading file\n");
    char* buffer = read_file(file);
    printf("%s\n", buffer);

    printf("Writing new file\n");
    inode* new_file = (inode*)malloc(sizeof(inode));
    new_file->type = 2;
    new_file->size = 5;
    new_file->blockPointers[0] = find_free_data_block();
    write_inode(new_file);


   
    return 0;
}

/*
C'est une V1 très inéficace, par exemple pour modifier un fichier, on doit le lire, le supprimer, le réécrire avec les modifications, et le réécrire dans le répertoire
*/
