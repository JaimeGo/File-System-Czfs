#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Estructuras */

struct czfile {
  bool is_open;
  char mode;
  int index_pointer;
  char path[1024];
  int last_byte_read;
  int last_byte_written;
};

typedef struct czfile czFILE;

/* Funciones generales */
void cz_mount(const char* diskname);
void cz_bitmap();
int cz_exists(const char* path);
void cz_ls(const char* path);

/* Funciones manejo de archivos */
int cz_cp(const char* orig, const char* dest);
int cz_rm(const char* path);
czFILE* cz_open(const char* path, const char mode);
int cz_read(czFILE* file_desc, unsigned char* buffer, const int nbytes);
int cz_write(czFILE* file_desc, unsigned char* buffer, const int nbytes);
int cz_close(czFILE* file_desc);
int cz_mv(const char* orig, const char *dest);

/* Funciones manejo de directorios */
int cz_mkdir(const char *foldername);
int cz_mvdir(const char *foldername, const char *dest);
int cz_cpdir(const char* foldername, const char* dest);
int cz_rmdir(const char* path);


/* Funciones extras */

// Cambia un byte (0x00 a 0xFF) a su representacion binaria (int array[8]), en el buff.
void byte_to_bits(unsigned char byte, int* buff);
// Cambia un array de 8 ints (int array[8]) a su representacion en byte (0x00 a 0xFF)
unsigned char bits_to_byte(int* array);

// Guarda en "block" la informacion contenida en el bloque n (0 - 65536).
void get_block(const int n, unsigned char* block);
// Obtiene el siguiente bloque libre para ser utilizado.
int get_free_block();

// Busca si existe el path "path", y si existe, entrega el bloque en el que esta (ya sea archivo o directorio).
int search_path(const char* path);

// Retorna 1 si path es carpeta, 0 si path es archivo.
int is_folder(const char* path);
int is_inside_itself(const char *orig, const char *dest);

// Cambia el bloque (Desde 0 a 65536) en el bitmap, a 0 o a 1.
void change_block_in_bitmap(int block, int value);

// Crea la carpeta, en el directorio apuntado por pointer (Desde bloque 0 a 65536).
int create_folder_at_block(const char *foldername, int pointer);
int create_file_at_block(const char* file_name, const int pointer);

// Escribe en la posicion byte_position (Desde 0 a 134217728), el byte "byte".
void write_byte(int block, int byte_position, unsigned char byte);
void write_line(int block_pointer, int line, unsigned char byte, const char* name, int pointer);

void clean_block(int block);

void get_parent_path(char* file_name, const char* abs_path);
void get_child_name(char* file_name, const char* abs_path);

void recursive_ls(char *path, int recursion_depth);

void print_name(unsigned char* block, const int position);
void print_folder(char *folder);

unsigned char get_byte_in_block(unsigned char* block, int line, int index);

void read_data_block(czFILE* file_desc, unsigned char* buffer, int nbytes, unsigned char* data_block, int* bytes_read, int* blocks_read);
void write_data_block(czFILE* file_desc, unsigned char* buffer, int nbytes, int pointer, int* bytes_written, int* blocks_written);

void get_name(unsigned char *block, char *string, int line);
int get_size(unsigned char* block);

int compare_dir(const char* dest1, const char* dest2);
int compare_file(const char* dest1, const char* dest2);
int compare_block(int pointer_1, int pointer_2);
