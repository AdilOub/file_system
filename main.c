#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define FILENAME "hdd.bin"

#define FAT_TABLE_SIZE 128 // nombre de bloc qu'on peut avoir

#define DISK_SIZE 4096 //4KB

#define MAX_INODE_PER_DIR 4


#define NAME_LENGTH 4

#define FOLDER_SIGNATURE 0xDE
#define FILE_SIGNATURE 0xF1

uint64_t offset = FAT_TABLE_SIZE;

struct inode{
    char name[NAME_LENGTH][MAX_INODE_PER_DIR];
    uint16_t starting_cluster[MAX_INODE_PER_DIR];
};
typedef struct inode inode;

#define BLOCK_SIZE sizeof(inode)

struct fat_entry{
    uint16_t signature;
};
typedef struct fat_entry fat_entry;

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

fat_entry* read_fat_table(){
    fat_entry* fat_table = (fat_entry*)malloc(FAT_TABLE_SIZE * sizeof(fat_entry));
    char* buffer = (char*)malloc(FAT_TABLE_SIZE * sizeof(fat_entry));
    read(buffer, FAT_TABLE_SIZE * sizeof(fat_entry), 0);
    fat_table = (fat_entry*)buffer;
    return fat_table;
}

void update_fat_table(uint16_t signature, uint16_t index){
    fat_entry* fat_table = read_fat_table();
    fat_table[index].signature = signature;
    write((char*)fat_table, FAT_TABLE_SIZE * sizeof(fat_entry), 0);
    free(fat_table);
    return;
}

void print_fat_table(){
    fat_entry* fat_table = read_fat_table();
    for(int i = 0; i < FAT_TABLE_SIZE; i++){
        printf("Index: %d, Signature: %d\n", i, fat_table[i].signature);
    }
    free(fat_table);
    return;
}

inode* read_inode(uint64_t inode_address){
    inode* node = (inode*)malloc(sizeof(inode));
    char* buffer = (char*)malloc(sizeof(inode));
    read(buffer, sizeof(inode), inode_address*sizeof(inode) + offset);
    node = (inode*)buffer;
    return node;
}

void write_inode(uint16_t inode_address, inode* node){
    write((char*)node, sizeof(inode), inode_address * sizeof(inode) + offset);
    return;
}

void write_data_to_inode(uint16_t inode_address, char* data){
    printf("Writing data to inode %d, '%s'\n", inode_address, data);
    //write(data, sizeof(inode), inode_address*sizeof(inode) + offset);
    return;
}

int create_folder(char* name, uint16_t starting_cluster){
    inode* node = read_inode(starting_cluster);
    fat_entry* fat_table = read_fat_table();
    int new_inode = -1;
    for(int i = 0; i < MAX_INODE_PER_DIR; i++){
        if(fat_table[i+starting_cluster].signature == 0){
            new_inode = i;
            for(int j = 0; j < NAME_LENGTH; j++){
                node->name[i][j] = name[j];
            }
            node->starting_cluster[i] = starting_cluster;
            write((char*)node, sizeof(inode), offset);
            update_fat_table(FOLDER_SIGNATURE, new_inode);
            free(node);
            free(fat_table);
            return new_inode;
        }
    }
    fprintf(stderr, "No more space in the inode\n");
    free(node);
    free(fat_table);
    return -1;
}

int create_file(char* name, uint16_t starting_cluster, char* data){
    inode* node = read_inode(starting_cluster);
    fat_entry* fat_table = read_fat_table();
    int new_inode = -1;
    for(int i = 0; i < MAX_INODE_PER_DIR; i++){
        if(fat_table[i+starting_cluster].signature == 0){
            printf("Find a free inode at %d in cluster %d\n", i+starting_cluster, starting_cluster);
            new_inode = i;
            for(int j = 0; j < NAME_LENGTH; j++){
                node->name[i][j] = name[j];
            }
            node->starting_cluster[i] = starting_cluster;
            write_inode(starting_cluster, node);
            update_fat_table(FILE_SIGNATURE, new_inode);
            write_data_to_inode(new_inode, data);
            free(node);
            free(fat_table);
            return new_inode;
        }
    }
    fprintf(stderr, "No more space in the inode\n");
    free(node);
    free(fat_table);
    return -1;
}

void print_inode(inode* node){
    for(int i = 0; i < MAX_INODE_PER_DIR; i++){
        printf("Name: %s\n", node->name[i]);
        printf("Starting cluster: %d\n", node->starting_cluster[i]);
    }
}

int main(){
    //create_folder(".", 0);
    int new_inode_usr = create_folder("usr", 0);
    printf("New inode for /usr/: %d\n", new_inode_usr);
    int new_inode_file = create_file("file", new_inode_usr, "data");
    printf("New inode for /file: %d\n", new_inode_file);
    return 0;
}