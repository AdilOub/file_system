#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>





#define FILENAME "hdd.bin"

#define FAT_TABLE_SIZE 65536 // nombre de bloc qu'on peut avoir


#define FREE_BLOCK 0
#define END_OF_FILE 0xFFFF
#define BAD_BLOCK 0xFFFE
#define NOT_USED 0xFFFD
#define FOLDER_SIGNATURE 0xFFFC

//#define BLOCK_SIZE 1024
#define BLOCK_SIZE_POW2 6
#define BLOCK_SIZE (2 << BLOCK_SIZE_POW2)

#define LINK_SIZE 0x10
#define MAX_INODE_PER_DIR (BLOCK_SIZE / LINK_SIZE)-1 

#pragma region str
char* pad_right(char* str, int size){
    int len = strlen(str);
    if(len >= size){
        return str;
    }

    char* buffer = (char*)malloc(size);
    for(int i=0; i<len; i++){
        buffer[i] = str[i];
    }
    for(int i=len; i<size; i++){
        buffer[i] = '\0';
    }
    return buffer;
}

char* uint16_to_hexstr(uint16_t n){
    char* buffer = (char*)malloc(5);
    sprintf(buffer, "%04x", n);
    return buffer;
}


#pragma endregion str

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
    FILE* file = fopen(FILENAME, "r+");
    if (file == NULL) {
        fprintf(stderr, "Error opening file in writie\n");
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(buffer, size, 1, file);
    fclose(file);
    return;
}

uint16_t* read_fat_table(){
    uint16_t *fat_table = (uint16_t*)malloc(FAT_TABLE_SIZE * sizeof(uint16_t));
    read((char*)fat_table, FAT_TABLE_SIZE * sizeof(uint16_t), 0);
    return fat_table;
}

uint16_t get_next_free_inode_nb(){
    uint16_t* fat_table = read_fat_table();
    for(uint16_t i = 0; i < FAT_TABLE_SIZE; i++){
        if(fat_table[i] == FREE_BLOCK){
            free(fat_table);
            return i;
        }
    }
    free(fat_table);
    return -1;
}

uint16_t get_nth_inode(uint16_t n){
    uint16_t* fat_table = read_fat_table();
    uint16_t r = fat_table[n];
    free(fat_table);
    return r;
}

void write_nth_inode(uint16_t n, uint16_t value){
    uint16_t* fat_table = read_fat_table();
    fat_table[n] = value;
    write((char*)fat_table, FAT_TABLE_SIZE * sizeof(uint16_t), 0);
    free(fat_table);
    return;
}


uint16_t find_not_used_in_folder(uint16_t parent){
    if(get_nth_inode(parent) != FOLDER_SIGNATURE){
        fprintf(stderr, "Parent is not a folder\n");
        return -1;
    }

    uint16_t* ptr = malloc(sizeof(uint16_t));
    *ptr = 0x0000;

    for(int i = 0; i<MAX_INODE_PER_DIR; i++){
        read((char*)ptr, sizeof(uint16_t), parent * BLOCK_SIZE + FAT_TABLE_SIZE + (i+1) * LINK_SIZE);
        if(*ptr == NOT_USED){
            free(ptr);
            return i;
        }
    }
    free(ptr);
    return -1;
}


uint16_t create_folder(uint16_t parent, char* name){
    //on l'ajoute à la fat table
    uint16_t new_inode = get_next_free_inode_nb();
    write_nth_inode(new_inode, FOLDER_SIGNATURE);

    //on écrit ensuite le parent dans le cluster du nouveau dossier
    uint16_t* ptr = (uint16_t*)malloc(sizeof(uint16_t));
    *ptr = parent;
    write((char*)ptr, sizeof(uint16_t), new_inode * BLOCK_SIZE + FAT_TABLE_SIZE);
    write("<-pof ", 6, new_inode * BLOCK_SIZE + FAT_TABLE_SIZE + sizeof(uint16_t)); //TODO rmv after debug
    write(name, strlen(name), new_inode * BLOCK_SIZE + FAT_TABLE_SIZE + sizeof(uint16_t) + 6); //TODO rmv after debug

    //le nouveau dossier est vide, on met tous ses inodes fils à NOT_USED
    *ptr = NOT_USED;
    for(int i = 0; i<MAX_INODE_PER_DIR; i++){
        write((char*)ptr, sizeof(uint16_t), new_inode * BLOCK_SIZE + FAT_TABLE_SIZE + (i+1) * LINK_SIZE);
    }
    
    free(ptr);
    return new_inode;
}

int create_folder_in_parent(uint16_t parent, char* name){
    //on récupère le premier liens libre dans le dossier parent
    uint16_t not_used = find_not_used_in_folder(parent);
    if(not_used == 0xFFFF){
        fprintf(stderr, "No more space in parent folder\n");
        return -1;
    }

    printf("Creating folder %s in parent %d at pos %d\n", name, parent, not_used);

    //on crée le dossier
    uint16_t new_inode = create_folder(parent, name);
    uint16_t* ptr = (uint16_t*)malloc(sizeof(uint16_t));
    *ptr = new_inode;

    //on écrit le nom du dossier et son lien dans le parent
    write((char*)ptr, sizeof(uint16_t), parent * BLOCK_SIZE + FAT_TABLE_SIZE + (not_used+1) * LINK_SIZE);
    write(name, strlen(name), parent * BLOCK_SIZE + FAT_TABLE_SIZE + (not_used+1) * LINK_SIZE + sizeof(uint16_t));

    return new_inode;
}


uint16_t create_file(uint16_t parent, char* data, uint16_t nb_of_cluster){
    //on veut commencer par trouver nb_of_cluster clusters libres dnas la fat table
    uint16_t* clusters = (uint16_t*)malloc(nb_of_cluster * sizeof(uint16_t));

    for(int chunk = 0; chunk < nb_of_cluster; chunk++){
        clusters[chunk] = get_next_free_inode_nb();
        write_nth_inode(clusters[chunk], BAD_BLOCK);
        if(clusters[chunk] == 0xFFFF){
            fprintf(stderr, "No more space in the fat table\n");
            return -1;
        }
        write(data + chunk * BLOCK_SIZE, chunk == nb_of_cluster-1 ? strlen(data+chunk*BLOCK_SIZE) : BLOCK_SIZE, clusters[chunk] * BLOCK_SIZE + FAT_TABLE_SIZE);
    }

    for(int chunk = 0; chunk < nb_of_cluster-1; chunk++){
        printf("Writing %d to %d\n", clusters[chunk], clusters[chunk+1]);
        write_nth_inode(clusters[chunk], clusters[chunk+1]);
    }
    printf("Writing %d to %d(EOF)\n", clusters[nb_of_cluster-1], END_OF_FILE);
    write_nth_inode(clusters[nb_of_cluster-1], END_OF_FILE);

    uint16_t first_cluster = clusters[0];
    free(clusters);
    return first_cluster;
}

int create_file_in_parent(uint16_t parent, char* name, char* data){
    uint64_t size = strlen(data);
    uint16_t nb_of_cluster = (size / BLOCK_SIZE) + !!(size%BLOCK_SIZE); //j'aime bien, !! pour avoir 0 ou 1
    //uint16_t nb_of_cluster = (size >> BLOCK_SIZE_POW2) + !!(size & BLOCK_SIZE_MASK); //encore plus opti parceque why not mdr

    //on récupère le premier liens libre dans le dossier parent
    uint16_t not_used = find_not_used_in_folder(parent);
    if(not_used == 0xFFFF){
        fprintf(stderr, "No more space in parent folder\n");
        return -1;
    }
    printf("Creating file %s in parent %d at pos %d with %d clusters\n", name, parent, not_used, nb_of_cluster);


    //on crée le fichier
    uint16_t new_inode = create_file(parent, data, nb_of_cluster);
    uint16_t* ptr = (uint16_t*)malloc(sizeof(uint16_t));
    *ptr = new_inode;

    //on écrit le nom du fichier et son lien dans le parent
    write((char*)ptr, sizeof(uint16_t), parent * BLOCK_SIZE + FAT_TABLE_SIZE + (not_used+1) * LINK_SIZE);
    write(name, strlen(name), parent * BLOCK_SIZE + FAT_TABLE_SIZE + (not_used+1) * LINK_SIZE + sizeof(uint16_t));

    return new_inode;
}


void setup_root(){
    //on initialise le disque dur en overwritant le root
    write_nth_inode(0, FOLDER_SIGNATURE);

    uint16_t new_inode = 0; //OVERWRITE ROOT
    uint16_t parent = NOT_USED;

    uint16_t* ptr = (uint16_t*)malloc(sizeof(uint16_t));
    *ptr = parent;
    write((char*)ptr, sizeof(uint16_t), new_inode * BLOCK_SIZE + FAT_TABLE_SIZE);
    write("<-root", 6, new_inode * BLOCK_SIZE + FAT_TABLE_SIZE + sizeof(uint16_t)); //TODO rmv after debug

    *ptr = NOT_USED;
    for(int i = 0; i<MAX_INODE_PER_DIR; i++){
        write((char*)ptr, sizeof(uint16_t), new_inode * BLOCK_SIZE + FAT_TABLE_SIZE + (i+1) * LINK_SIZE);
    }

    
    free(ptr);
    return;
}

int main(){
    //on initialise le disque dur en overwritant le root
    setup_root(); //attention, doit être utiliser qu'une seule fois !!!


    int t = create_folder_in_parent(0, "test");
    create_folder_in_parent(0, "bin");
    create_folder_in_parent(0, "usr");
    create_folder_in_parent(t, "BITE");


    char* data = malloc(BLOCK_SIZE*2);
    for(int i = 0; i<BLOCK_SIZE; i++){
        data[i] = '-';
    }
    for(int i = BLOCK_SIZE; i<BLOCK_SIZE*2; i++){
        data[i] = '+';
    }
    data[BLOCK_SIZE*2-1] = '\0';
    
    create_file_in_parent(0, "test.txt", data);


    printf("Reading 7th: %d\n", get_nth_inode(7));

    //create_file_in_parent(0, "test.txt", "Hello World");
    //create_file_in_parent(0, "test2.txt", "Hello World2");

    return 0;
}



//Seul cas ou parent NOT_USED: le dossier root
//On écrit BAD_BLOCK pendant qu'on est en train de créer un fichier