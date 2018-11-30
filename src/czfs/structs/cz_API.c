#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include "cz_API.h"

/* VARIABLES GLOBALES */

char disk[1000];

/////////////////////////
/* FUNCIONES GENERALES */
/////////////////////////

// FUNCIONANDO
void cz_mount(const char* diskname)
{
	strcpy(disk, diskname);
}

// FUNCIONANDO
void cz_bitmap()
{
	int used_blocks = 0;
	int size = 8192;
	unsigned char buffer[size];
	FILE* ptr;
	ptr = fopen(disk,"rb");

	int n = 1;
	fseek(ptr, n * 2048, SEEK_SET);
	fread(buffer,sizeof(buffer),1,ptr);

	for (int k = 0; k < 4; k++) {
		for (int i = 0; i < 128; i++) {
			for (int j = 0; j < 16; j++) {
				int byte_array[8] = { 0 };
				byte_to_bits(buffer[(k * 2048) + (i * 16) + j], byte_array);
				for (int q = 0; q < 8; q++){
					if (byte_array[q] == 1) used_blocks++;
					fprintf( stderr, "%d", byte_array[q]);
				}
				fprintf(stderr, " ");
			}
			fprintf(stderr, "\n");
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "Used blocks: %d\n", used_blocks);
	fprintf(stderr, "Free blocks: %d\n", 65536 - used_blocks);
	fprintf(stderr, "\n");
	fclose(ptr);
}

// FUNCIONANDO
int cz_exists(const char* path)
{
	int pointer = search_path(path);

	return pointer != -1;
}

// FUNCIONANDO
void cz_ls(const char* path)
{
	if (is_folder(path) == 0) {
		fprintf(stderr, "[ERROR] La ruta no es un directorio.\n");
		return;
	}

	int pointer = search_path(path);
	if (pointer == -1) {
		fprintf(stderr, "[ERROR] Directorio no existe.\n");
		return;
	}
	else {
		unsigned char current_block[2048];
		get_block(pointer, current_block);

		for (int j = 0; j < 128; j++) {
			if (current_block[j * 16] == 0x02 || current_block[j * 16] == 0x04) {
				print_name(current_block, j * 16);
			}
		}
	}
}

//////////////////////////////////
/* FUNCIONES MANEJO DE ARCHIVOS */
//////////////////////////////////

czFILE* cz_open(const char* abs_path, const char mode)
{
	char path[1000];
	strcpy(path, abs_path);

	if (is_folder(path)) {
		fprintf(stderr, "[ERROR] La ruta entregada indica a un directorio.\n");
		return NULL;
	}

	char file_name[1000];
	strcpy(file_name, "");
	get_child_name(file_name, path);

	char parent_path[1000];
	strcpy(parent_path, "");
	get_parent_path(parent_path, path);

	int file_pointer = search_path(path);

	if (mode == 'r') {
		if (file_pointer == -1) {
			fprintf(stderr, "[ERROR] El archivo no existe.\n");
			return NULL;
		}

		czFILE* file = malloc(sizeof(czFILE));

		file->is_open = true;
		file->mode = 'r';
		file->index_pointer = file_pointer;
		file->last_byte_read = 0;
		file->last_byte_written = 0;
		strcpy(file->path, abs_path);
		return file;
	}
	else if (mode == 'w') {
		if (file_pointer != -1) {
			fprintf(stderr, "[ERROR] El archivo ya existe.\n");
			return NULL;
		}
		if (strlen(file_name) > 11)
		{
			fprintf(stderr, "[ERROR] El archivo tiene un nombre muy largo.\n");
			return NULL;
		}

		czFILE* file = malloc(sizeof(czFILE));

		file->is_open = true;
		file->mode = 'w';
		file->index_pointer = create_file_at_block(file_name, search_path(parent_path));
		file->last_byte_read = 0;
		file->last_byte_written = 0;
		strcpy(file->path, abs_path);

		return file;
	}

	return NULL;
}

int cz_read(czFILE* file_desc, unsigned char* buffer, const int nbytes)
{
	if (file_desc->mode != 'r') {
		fprintf(stderr, "[ERROR] El archivo no se encuentra en modo lectura.\n");
		return -1;
	}

	if (!file_desc->is_open) {
		fprintf(stderr, "[ERROR] El archivo no se encuentra abierto.\n");
		return -1;
	}

	if (file_desc == NULL) {
		fprintf(stderr, "[ERROR] El archivo no pudo abrirse.\n");
		return -1;
	}

	int bytes_read = 0;
	int blocks_read = 0;

	//accede al bloque indice y desde ahí a los bloques de datos:
	unsigned char index_block[2048];
	get_block(file_desc->index_pointer, index_block);

	for (int n = 2; n < 511; n++) {
		unsigned char data_block[2048];
		int block_int = (index_block[n * 4 + 2] << 8) | index_block[n * 4 + 3];
		get_block(block_int, data_block);

		read_data_block(file_desc, buffer, nbytes, data_block, &bytes_read, &blocks_read);

		if (bytes_read >= nbytes) break;
	}

	// Si le faltan bytes, va al bloque de direccionamiento indirecto
	if (bytes_read < nbytes) {
		unsigned char indirect_block[2048];
		int indirect_block_int = (index_block[2046] << 8) | index_block[2047];
		get_block(indirect_block_int, indirect_block);

		for (int n = 0; n < 512; n++){
			unsigned char data_block[2048];
			int block_int = (indirect_block[n * 4 + 2] << 8) | indirect_block[n * 4 + 3];
			get_block(block_int, data_block);
			read_data_block(file_desc, buffer, nbytes, data_block, &bytes_read, &blocks_read);

			if (bytes_read >= nbytes) break;
		}
	}

	return bytes_read;
}

int cz_write(czFILE* file_desc, unsigned char* buffer, const int nbytes)
{
	if(file_desc == NULL){
		fprintf(stderr, "[ERROR] El archivo no existe.\n");
		return -1;
	}
	if (file_desc->mode != 'w') {
		fprintf(stderr, "[ERROR] El archivo no se encuentra en modo escritura.\n");
		return -1;
	}

	if (!file_desc->is_open) {
		fprintf(stderr, "[ERROR] El archivo no se encuentra abierto\n");
		return -1;
	}

	int bytes_written = 0;
	int blocks_written = 0;

	//accede al bloque indice y desde ahí a los bloques de datos:
	unsigned char index_block[2048];
	get_block(file_desc->index_pointer, index_block);

	for (int n = 2; n < 511; n++) {
		int pointer = get_free_block();
		if (pointer == -1) {
			fprintf(stderr, "[ERROR] El disco está lleno.\n");
			return -1;
		}
		clean_block(pointer);

		unsigned char data_block[2048];
		get_block(pointer, data_block);

		change_block_in_bitmap(pointer, 1);

		unsigned char digits[2];
		digits[0] = pointer & 0xFF;
		digits[1] = (pointer >> 8) & 0xFF;
		write_byte(file_desc->index_pointer, (4 * n) + 2, digits[1]);
		write_byte(file_desc->index_pointer, (4 * n) + 3, digits[0]);
		write_data_block(file_desc, buffer, nbytes, pointer, &bytes_written, &blocks_written);
		if (bytes_written >= nbytes) break;
	}

	// Si le faltan bytes, va al bloque de direccionamiento indirecto
	if (bytes_written < nbytes) {
		int indirect_pointer = get_free_block();

		if (indirect_pointer == -1) {
			fprintf(stderr, "[ERROR] El disco está lleno.\n");
			return -1;
		}
		clean_block(indirect_pointer);

		unsigned char indirect_block[2048];
		get_block(indirect_pointer, indirect_block);

		change_block_in_bitmap(indirect_pointer, 1);

		unsigned char digits[2];
		digits[0] = indirect_pointer & 0xFF;
		digits[1] = (indirect_pointer >> 8) & 0xFF;
		write_byte(file_desc->index_pointer, 2046, digits[1]);
		write_byte(file_desc->index_pointer, 2047, digits[0]);

		for (int n = 0; n < 512; n++) {
			int pointer = get_free_block();

			if (pointer == -1) {
				fprintf(stderr, "[ERROR] El disco está lleno.\n");
				return -1;
			}
			clean_block(pointer);

			unsigned char data_block[2048];
			get_block(pointer, data_block);

			change_block_in_bitmap(pointer, 1);

			unsigned char digits[2];
			digits[0] = pointer & 0xFF;
			digits[1] = (pointer >> 8) & 0xFF;
			write_byte(indirect_pointer, (n * 4) + 2, digits[1]);
			write_byte(indirect_pointer, (n * 4) + 3, digits[0]);

			write_data_block(file_desc, buffer, nbytes, pointer, &bytes_written, &blocks_written);

			if (bytes_written >= nbytes) break;
		}

		if (bytes_written < nbytes) {
			fprintf(stderr, "[ERROR] El archivo no puede crecer más.\n");
			unsigned char file_size[4];
			file_size[0] = file_desc->last_byte_written & 0xFF;
			file_size[1] = (file_desc->last_byte_written >> 8) & 0xFF;
			file_size[2] = (file_desc->last_byte_written >> 16) & 0xFF;
			file_size[3] = (file_desc->last_byte_written >> 24) & 0xFF;
			write_byte(file_desc->index_pointer, 0, file_size[3]);
			write_byte(file_desc->index_pointer, 1, file_size[2]);
			write_byte(file_desc->index_pointer, 2, file_size[1]);
			write_byte(file_desc->index_pointer, 3, file_size[0]);

			unsigned char timestamp[4];
			time_t current_time = time(NULL);
			timestamp[0] = current_time & 0xFF;
			timestamp[1] = (current_time >> 8) & 0xFF;
			timestamp[2] = (current_time >> 16) & 0xFF;
			timestamp[3] = (current_time >> 24) & 0xFF;
			write_byte(file_desc->index_pointer, 4, timestamp[3]);
			write_byte(file_desc->index_pointer, 5, timestamp[2]);
			write_byte(file_desc->index_pointer, 6, timestamp[1]);
			write_byte(file_desc->index_pointer, 7, timestamp[0]);

			return bytes_written;
		}
	}

	unsigned char file_size[4];

	file_size[0] = file_desc->last_byte_written & 0xFF;
	file_size[1] = (file_desc->last_byte_written >> 8) & 0xFF;
	file_size[2] = (file_desc->last_byte_written >> 16) & 0xFF;
	file_size[3] = (file_desc->last_byte_written >> 24) & 0xFF;
	write_byte(file_desc->index_pointer, 0, file_size[3]);
	write_byte(file_desc->index_pointer, 1, file_size[2]);
	write_byte(file_desc->index_pointer, 2, file_size[1]);
	write_byte(file_desc->index_pointer, 3, file_size[0]);

	unsigned char timestamp[4];
	time_t current_time = time(0);
	timestamp[0] = current_time & 0xFF;
	timestamp[1] = (current_time >> 8) & 0xFF;
	timestamp[2] = (current_time >> 16) & 0xFF;
	timestamp[3] = (current_time >> 24) & 0xFF;
	write_byte(file_desc->index_pointer, 4, timestamp[3]);
	write_byte(file_desc->index_pointer, 5, timestamp[2]);
	write_byte(file_desc->index_pointer, 6, timestamp[1]);
	write_byte(file_desc->index_pointer, 7, timestamp[0]);

	return bytes_written;
}

int cz_close(czFILE* file_desc)
{
	if (file_desc == NULL) {
		fprintf(stderr, "[ERROR] Archivo no existe. \n");
		return -1;
	}

	file_desc->is_open = false;
	file_desc->mode = 'x';
	free(file_desc);

	return 0;
}

int cz_mv(const char* orig, const char *dest) {
	if (orig[strlen(orig) - 1] == '/') {
		fprintf(stderr, "[ERROR] Ruta origen debe ser un archivo. \n");
		return 1;
	}
	if (dest[strlen(dest) - 1] == '/') {
		fprintf(stderr, "[ERROR] Ruta destino debe ser un archivo. \n");
		return 1;
	}

	if (!cz_exists(orig)) {
		fprintf(stderr, "[ERROR] Archivo no encontrado. \n");
		return 1;
	}

	if (cz_exists(dest)) {
		fprintf(stderr, "[ERROR] Destino ya está en uso. \n");
		return 1;
	}

	char orig_parent_path[1000];
	char orig_file_name[1000];
	strcpy(orig_file_name, "");
	get_parent_path(orig_parent_path, orig);
	get_child_name(orig_file_name, orig);

	char dest_parent_path[1000];
	char dest_file_name[1000];
	strcpy(dest_file_name, "");
	get_parent_path(dest_parent_path, dest);
	get_child_name(dest_file_name, dest);

	if (strlen(dest_file_name) > 11) {
		fprintf(stderr, "[ERROR] Nombre archivo destino muy largo. \n");
		return 1;
	}

	if (!cz_exists(dest_parent_path)) {
		fprintf(stderr, "[ERROR] Carpeta destino incorrecta. \n");
		return 1;
	}

	bool change_name = (strcmp(orig_parent_path, dest_parent_path) == 0);

	int orig_parent_pointer = search_path(orig_parent_path);
	unsigned char parent_block[2048];
	get_block(orig_parent_pointer, parent_block);

	int dest_parent_pointer = search_path(dest_parent_path);
	unsigned char dest_parent_block[2048];
	get_block(dest_parent_pointer, dest_parent_block);

	for (int j = 0; j < 128; j++){
		if (get_byte_in_block(parent_block, j, 0) == 0x04){
			char file_name[1000];
			strcpy(file_name, "");
			get_name(parent_block, file_name, j * 16);
			char orig_file_name[1000];
			strcpy(orig_file_name, "");
			get_child_name(orig_file_name, orig);
			if (strcmp(orig_file_name, file_name) == 0) {  //Si encuentro mi archivo
				strcpy(dest_file_name, "");
				get_child_name(dest_file_name, dest);
				unsigned char byte_1 = get_byte_in_block(parent_block, j, 14);
				unsigned char byte_2 = get_byte_in_block(parent_block, j, 15);
				int bytes_pointer = (byte_1 << 8) | byte_2;
				if (change_name == 1) {
					write_line(orig_parent_pointer, j, 0x04, dest_file_name, bytes_pointer);
					return 0;
				}
				else {
					dest_parent_pointer = search_path(dest_parent_path);
					unsigned char dest_parent_block[2048];
					get_block(dest_parent_pointer, dest_parent_block);
					for (int k = 0; k < 128; k++) {
						if (dest_parent_block[k * 16] == 0x01 || dest_parent_block[k * 16] == 0x00) {
							write_line(dest_parent_pointer, k, 0x04, dest_file_name, bytes_pointer);
							write_line(orig_parent_pointer, j, 0x01, dest_file_name, bytes_pointer);
							return 0;
						}
					}
				}
			}
		}
	}

	fprintf(stderr, "[ERROR] No hay espacio en el bloque. \n");
	return 1;
}

int cz_cp(const char* orig, const char* dest)
{
	char parent_path[1000];
	strcpy(parent_path, "");
	get_parent_path(parent_path, dest);
	if (search_path(parent_path) == -1) {
		fprintf(stderr, "[ERROR] No existe la carpeta en la que quiere copiar el archivo. \n");
		return -1;
	}

	czFILE* read_file;
	read_file = cz_open(orig, 'r');
	if (read_file == NULL) {
		fprintf(stderr, "[ERROR] El archivo a copiar es nulo. \n");
		cz_close(read_file);
		return -1;
	}

	char dest_name[1000];
	strcpy(dest_name, "");
	get_child_name(dest_name, dest);
	if (strlen(dest_name) > 11) {
		fprintf(stderr, "[ERROR] Nombre del archivo es muy largo. \n");
		cz_close(read_file);
		return -1;
	}


	unsigned char index_block[2048];
	get_block(read_file->index_pointer, index_block);
	const int size = (index_block[0] << 24) | (index_block[1] << 16) | (index_block[2] << 8) | index_block[3];

	// unsigned char buffer[size];
	unsigned char* buffer = calloc(size, sizeof(unsigned char));
	if (cz_read(read_file, buffer, size) == -1) return -1;
	cz_close(read_file);

	czFILE* write_file;
	if ((write_file = cz_open(dest, 'w')) == NULL) {
		fprintf(stderr, "[ERROR] El archivo a escribir no existe. \n");
		free(buffer);
		return -1;
	}

	if (cz_write(write_file, buffer, size) == -1) {
		fprintf(stderr, "[ERROR] Se acabó el espacio de escritura. \n");
		cz_close(write_file);
		free(buffer);
		return -1;
	}
	cz_close(write_file);
	free(buffer);


	return 0;
}

int cz_rm(const char* path) {
	if (is_folder(path) == 1) {
		fprintf(stderr, "[ERROR] Este comando solo borra archivos. \n");
		return 1;
	}

	int pointer = search_path(path);
	if (pointer == -1) {
		fprintf(stderr, "[ERROR] Ruta incorrecta. \n");
		return 2;
	}

	char parent_path[1000];
	char file_name[1000];
	get_parent_path(parent_path, path);
	get_child_name(file_name, path);

	unsigned char current_block[2048];
	get_block(pointer, current_block);

	const int parent_pointer = search_path(parent_path);
	unsigned char parent_block[2048];
	get_block(parent_pointer, parent_block);

	for (int j = 0; j < 128; j++) {
		if (parent_block[j * 16] == 0x04){
			char name[12];
			strcpy(name, "");
			for (int l=1;l < 12;l++) {
				sprintf(name + strlen(name), "%c", parent_block[j*16 + l]);
			}

			if (strcmp(file_name, name) == 0) {
				write_byte(parent_pointer, j*16, 0x01);
				break;
			}
		}
	}

	for (int j = 0; j < 509; j++) {
		int data_pointer = (current_block[8 + j*4 + 2] << 8) | current_block[8 + j*4 + 3];
		if (data_pointer != 0) {
			change_block_in_bitmap(data_pointer,0);
		}
	}

	int indirect_block_pointer = (current_block[2046] << 8) | current_block[2047];
	unsigned char indirect_block[2048];
	get_block(indirect_block_pointer, indirect_block);

	if(indirect_block_pointer != 0) {
		for (int j = 0; j < 512; j++) {
			int data_pointer = (indirect_block[j*4 + 2] << 8) | indirect_block[j*4 + 3];
			if (data_pointer != 0) {
				change_block_in_bitmap(data_pointer, 0);
			}
		}
		if ((indirect_block_pointer) != 0) {
			change_block_in_bitmap(indirect_block_pointer, 0);
		}
	}
	change_block_in_bitmap(pointer, 0);

	return 0;
}

/////////////////////////////////////
/* FUNCIONES MANEJO DE DIRECTORIOS */
/////////////////////////////////////

// FUNCIONANDO
int cz_mkdir(const char* foldername)
{
	// MANEJO DE ERRORES
	if (cz_exists(foldername) && is_folder(foldername)) {
		fprintf(stderr, "[ERROR] Carpeta ya existe. \n");
		return 1;
	}
	if (!is_folder(foldername)) {
		fprintf(stderr, "[ERROR] Este comando solo crea carpetas, no archivos. \n");
		return 2;
	}
	if (get_free_block() == -1) {
		fprintf(stderr, "[ERROR] No queda memoria para asignar un bloque. \n");
		return 3;
	}

	char parent_path[1000];
	strcpy(parent_path, "");
	get_parent_path(parent_path, foldername);
	if (search_path(parent_path) == -1) {
		fprintf(stderr, "[ERROR] No se encuentra la carpeta padre de la carpeta a crear. \n");
		return 4;
	}

	char child_name[1000];
	strcpy(child_name, "");
	get_child_name(child_name, foldername);
	if (strlen(child_name) > 11) {
		fprintf(stderr, "[ERROR] El nombre de la carpeta es muy largo. \n");
	}

	// FIN MANEJO DE ERRORES

	char real_path[1000];
	int i;
	strcpy(real_path, foldername);
	char last_available_directory[1000];
	char current_directory[1000];
	strcpy(last_available_directory, "");
	strcpy(current_directory, "");
	int pointer = -1;
	for(i= 0; i < strlen(foldername); i++)
	{
		strncat(last_available_directory, &foldername[i], 1);
		strncat(current_directory, &foldername[i], 1);
		if(foldername[i] == 47)
		{
			if(cz_exists(last_available_directory) == 0)
			{
				current_directory[strlen(current_directory) - 1] = 0;
				for(int j = i; j < strlen(foldername); j++)
				{
					strncat(current_directory, &foldername[j], 1);
				}
				int result = create_folder_at_block(current_directory, pointer);
				return result;
			}else{
				strcpy(current_directory, "");
				pointer = search_path(last_available_directory);
			}
		}
	}
	return 1;
}


int cz_mvdir(const char* foldername, const char* dest)
{
	// MANEJO DE ERRORES
	if (!is_folder(foldername)) {
		fprintf(stderr, "[ERROR] carpeta original no es directorio. \n");
		return 1;
	}
	if (!is_folder(dest)) {
		fprintf(stderr, "[ERROR] carpeta destino no es directorio. \n");
		return 1;
	}
	if (search_path(foldername) == -1) {
		fprintf(stderr, "[ERROR] carpeta original no existe. \n");
		return 2;
	}

	char origin[1000];
	char origin_cmp[1000];
	char destine[1000];
	char destine_child[1000];
	strcpy(destine_child, "");
	get_child_name(destine_child, dest);
	if (strlen(destine_child) > 11) {
		fprintf(stderr, "[ERROR] carpeta de destino tiene un nombre muy largo. \n");
		return 3;
	}

	char destine_cmp[1000];
	char destine_parent[1000];
	strcpy(origin, foldername);
	strcpy(destine, dest);
	get_parent_path(destine_parent, destine);
	if (search_path(destine_parent) == -1) {
		fprintf(stderr, "[ERROR] existen carpetas intermedias en el destino que no han sido creadas. \n");
		return 4;
	}

	strcpy(origin_cmp, "");
	strcpy(destine_cmp, "");
	int start = 0;
	for (start = 0; start < strlen(origin); start++) {
		strncat(origin_cmp, &origin[start], 1);
		strncat(destine_cmp, &destine[start], 1);
	}
	if (strcmp(origin_cmp, destine_cmp) == 0) {
		fprintf(stderr, "[ERROR] No se puede mover la carpeta origen a una carpeta dentro de la carpeta origen. \n");
		return 5;
	}
	int result = cz_mkdir(destine);
	if (result != 0) {
		fprintf(stderr, "[ERROR] No hay espacio suficiente en disco para crear la carpeta. \n");
		return 6;
	}

	// FIN MANEJO DE ERRORES

	int origin_pointer = search_path(origin);
	int destine_pointer = search_path(destine);
	unsigned char origin_block[2048];
	get_block(origin_pointer, origin_block);
	for (int j = 0; j < 2048; j++) write_byte(destine_pointer, j, origin_block[j]);
	change_block_in_bitmap(origin_pointer, 0);
	char parent_folder[1000];
	char child_name[1000];
	strcpy(parent_folder, "");
	strcpy(child_name, "");
	get_parent_path(parent_folder, origin);
	get_child_name(child_name, origin);
	int parent_pointer = search_path(parent_folder);
	unsigned char parent_block[2048];
	get_block(parent_pointer, parent_block);

	for (int i = 0; i < 128; i++) {
		if (get_byte_in_block(parent_block, i, 0) == 0x02) {
			char name[1000];
			strcpy(name, "");
			get_name(parent_block, name, i * 16);
			if (strcmp(child_name, name) == 0) {
				write_byte(parent_pointer, i * 16, 0x01);
			}
		}
	}

	return 0;
}

int cz_cpdir(const char* foldername, const char* dest)
{
	// MANEJO DE ERRORES
	if (!is_folder(foldername)) {
		fprintf(stderr, "[ERROR] Ruta de origen no es carpeta.\n");
		return 1;
	}
	if (!is_folder(dest)) {
		fprintf(stderr, "[ERROR] Ruta de destino no es carpeta.\n");
		return 1;
	}
	char parent_folder[1000];
	strcpy(parent_folder, "");
	get_parent_path(parent_folder, dest);
	if (search_path(parent_folder) == -1) {
		fprintf(stderr, "[ERROR] Carpeta pariente de destino no existe.\n");
		return 2;
	}
	if (search_path(foldername) == -1) {
		fprintf(stderr, "[ERROR] Ruta de origen no existe.\n");
		return 3;
	}
	char child_path[1000];
	strcpy(child_path, "");
	get_child_name(child_path, dest);
	if (strlen(child_path) > 11) {
		fprintf(stderr, "[ERROR] nombre muy largo\n");
		return 4;
	}
	if (is_inside_itself(foldername, dest) == 1) {
		fprintf(stderr, "[ERROR] No puede copiar una carpeta dentro de sí misma.\n");
		return 5;
	}

	// FIN MANEJO DE ERRORES

	char dest_name[1000];
	strcpy(dest_name, "");
	get_child_name(dest_name, dest);
	int origin_pointer = search_path(foldername);
	unsigned char origin_block[2048];
	unsigned char dest_parent_block[2048];
	get_block(origin_pointer, origin_block);
	int free_block = get_free_block();
	change_block_in_bitmap(free_block, 1);
	int dest_parent_pointer = search_path(parent_folder);
	get_block(dest_parent_pointer, dest_parent_block);
	for (int i = 0; i < 128;i++) {
		if (get_byte_in_block(dest_parent_block, i, 0) != 0x02 && get_byte_in_block(dest_parent_block, i, 0) != 0x04) {
			write_line(dest_parent_pointer, i, 0x02, dest_name, free_block);
			break;
		}
	}
	for (int i = 0; i < 128; i++) {
		if (get_byte_in_block(origin_block, i, 0) == 0x02) {
			char name[1000];
			char old_folder_path[1000];
			char new_folder_path[1000];
			strcpy(name, "");
			strcpy(new_folder_path, "");
			strcpy(old_folder_path, "");
			strcpy(new_folder_path, dest);
			strcpy(old_folder_path, foldername);
			get_name(origin_block, name, i * 16);
			strcat(new_folder_path, name);
			strcat(old_folder_path, name);
			strcat(new_folder_path, "/");
			strcat(old_folder_path, "/");
			int cpdir_result = cz_cpdir(old_folder_path, new_folder_path);
			if (cpdir_result != 0) return 1;
		}
		else if(get_byte_in_block(origin_block, i, 0) == 0x04) {
			char name[1000];
			char old_file_path[1000];
			char new_file_path[1000];
			strcpy(name, "");
			strcpy(new_file_path, "");
			strcpy(old_file_path, "");
			strcpy(new_file_path, dest);
			strcpy(old_file_path, foldername);
			get_name(origin_block, name, i * 16);
			strcat(new_file_path, name);
			strcat(old_file_path, name);
			int cp_result = cz_cp(old_file_path, new_file_path);
			if (cp_result != 0) return 1;
		}
	}

	return 0;
}

int cz_rmdir(const char* path)
{
	// MANEJO DE ERRORES
	if (!is_folder(path)) {
		fprintf(stderr, "[ERROR] ruta no corresponde a una carpeta. \n");
		return 1;
	}
	int pointer = search_path(path);
	if (pointer == -1) {
		fprintf(stderr, "[ERROR] ruta no encontrada. \n");
		return 2;
	}
	char real_path[1000];
	strcpy(real_path, path);
	if (strcmp(real_path, "/") == 0) {
		fprintf(stderr, "[ERROR] No se puede eliminar el directorio raíz\n");
		return 3;
	}
	// FIN MANEJO DE ERRORES

	unsigned char block[2048];
	unsigned char parent_block[2048];
	get_block(pointer, block);
	char parent[1000];
	char child[1000];
	strcpy(parent, "");
	strcpy(child, "");
	get_parent_path(parent, real_path);
	get_child_name(child, real_path);
	int parent_pointer = search_path(parent);
	get_block(parent_pointer, parent_block);

	for (int i = 0; i < 128; i++) {
		char name[1000];
		char file_path[1000];
		strcpy(name, "");
		strcpy(file_path, "");
		strcpy(file_path, real_path);
		unsigned char byte = get_byte_in_block(block, i, 0);

		if (byte == 0x02) {  // Si es directorio…
			get_name(block, name, i * 16);
			strcat(file_path, name);
			strcat(file_path, "/");
			cz_rmdir(file_path);
		}
		else if (byte == 0x04) {  // Si es archivo…
			get_name(block, name, i * 16);
			strcat(file_path, name);
			cz_rm(file_path);
			strcpy(file_path, real_path);
		}
	}

	for (int i = 0; i < 128; i++) {
		if (get_byte_in_block(parent_block, i, 0) == 0x02) {
			char directory[1000];
			strcpy(directory, "");
			get_name(parent_block, directory, i * 16);
			if (strcmp(directory, child) == 0) write_byte(parent_pointer, i * 16, 0x01);
		}
	}
	change_block_in_bitmap(pointer, 0);

	return 0;
}

//////////////////////
/* FUNCIONES EXTRAS */
//////////////////////

void byte_to_bits(unsigned char byte, int* buff)
{
	for (int i = sizeof(unsigned char) * 8; i; byte >>= 1) buff[--i] = byte & 1;
}

unsigned char bits_to_byte(int* array)
{
	int final = 0;
	for (int i = 0; i < 8; i++) {
		int power = 1;
		for (int j = 6 - i; j >= 0; j--) power *= 2;
		final += power * array[i];
	}

	return (unsigned char) final;
}

void get_block(int n, unsigned char* block)
{
	int size = 2048;
	unsigned char buffer[size];
	FILE* ptr;
	ptr = fopen(disk,"rb");
	fseek(ptr, n * 2048, SEEK_SET);
	fread(buffer,sizeof(buffer),1,ptr);
	for (int i = 0; i < 128; i++) {
		for(int j = 0; j < 16; j++) {
			block[i * 16 + j] = buffer[i * 16 + j];
		}
	}
	fclose(ptr);
}


void print_name(unsigned char* block, int position)
{
	char name[12];
	strcpy(name, "");
	for (int l = 1; l < 12; l++) {
		sprintf(name + strlen(name), "%c", block[position + l]);
	}
	printf("%s \n", name);
}

int search_path(const char* path)
{
	int found = 0;
	int pointer = -1;
	unsigned char current_block[2048];
	get_block(0, current_block);

	char real_path[1000];
	strcpy(real_path, path);
	if (strlen(real_path) == 1 && real_path[0] == 47) return 0;

	int count = 0;
	for (int i = 0; real_path[i]; i++) count += (real_path[i] == '/');
	count--;

	char* pch = strtok(real_path, "/" );
	while (pch != NULL) {
		found = 0;
		int i = 0;
		while (i < 128) {
			if (current_block[i * 16] == 0x02 && count > 0) {
				char found_folder[12];
				strcpy(found_folder, "");
				for (int l = 1; l < 12; l++) {
					sprintf(found_folder + strlen(found_folder), "%c", current_block[i * 16 + l]);
				}
				if (strcmp(found_folder, pch) == 0) {
					pointer = (current_block[i * 16 + 14]<<8) | current_block[i * 16 + 15];
					get_block(pointer, current_block);
					i = 127;
					found = 1;
				}
			}
			else {
				if (current_block[i * 16] == 0x04 && count == 0) {
					char found_file[12];
					strcpy(found_file, "");
					for (int l = 1; l < 12; l++) {
						sprintf(found_file + strlen(found_file), "%c", current_block[i * 16 + l]);
					}
					if (strcmp(found_file, pch) == 0) {
						pointer = (current_block[i * 16 + 14]<<8) | current_block[i * 16 + 15];
						get_block(pointer, current_block);
						i = 127;
						found = 1;
					}
				}
			}
			i++;
		}
		if (found == 1) {
			pch = strtok (NULL, "/");
			count--;
		}
		else {
			break;
		}
	}


	return found == 1 ? pointer : -1;
}

int is_folder(const char* path)
{
	char real_path[1000];
	strcpy(real_path, path);

	if (real_path[(strlen(real_path)-1)] == 47) return 1;  // Es carpeta

	return 0;
}

void change_block_in_bitmap(int block, int value)
{
	// unsigned char byte_to_change;
	FILE *ptr;
	ptr = fopen(disk,"rb");
	int byte_position = block/8;
	int array_position = block%8;
	unsigned char byte[1];
	fseek(ptr, byte_position + 2048, SEEK_SET);
	fread(byte,sizeof(byte),1,ptr);
	int byte_array[8] = {0,0,0,0,0,0,0,0};
	byte_to_bits(byte[0], byte_array);
	fclose(ptr);
	byte_array[array_position] = value;
	unsigned char new_byte = bits_to_byte(byte_array);
	ptr = fopen(disk,"r+b");
	fseek(ptr, 2048 + byte_position, SEEK_SET);
	fwrite(&new_byte, 1, 1, ptr);
	fclose(ptr);
}

int get_free_block()
{
	int size = 8192;
	unsigned char buffer[size];
	FILE *ptr;
	ptr = fopen(disk,"rb");
	int n = 1;
	fseek(ptr, n * 2048, SEEK_SET);
	fread(buffer,sizeof(buffer),1,ptr);
	for (int k = 0; k < 4; k++) {
		for (int i = 0; i < 128; i++) {
			for (int j = 0; j < 16; j++) {
				int byte_array[8] = {0,0,0,0,0,0,0,0};
				byte_to_bits(buffer[(k * 2048) + (i * 16) + j], byte_array);
				for (int q = 0; q < 8; q++) {
					if (byte_array[q] == 0) {
						fclose(ptr);
						return (k*2048 + (i*16) + j)*8 + q;
					}
				}
			}
		}
	}
	fclose(ptr);

	return -1;
}

int create_folder_at_block(const char *foldername, int pointer)
{
	// foldername: es la carpeta que crearé. pointer: la carpeta en donde crearé foldername.
	int block_to_be_assigned = get_free_block();
	if (block_to_be_assigned == -1) {
		fprintf(stderr, "[ERROR] No queda espacio para crear carpeta.\n");
		return 1;
	}
	// Calculamos largo de la carpeta:
	unsigned char block[2048];
	get_block(pointer, block);
	FILE *ptr;
	ptr = fopen(disk, "r+b");
	for (int i = 0; i < 128; i++) {
		if (get_byte_in_block(block, i, 0) != 0x02 && get_byte_in_block(block, i, 0) != 0x04) {
			clean_block(block_to_be_assigned);
			char folder_name[1000];
			strcpy(folder_name, "");
			get_child_name(folder_name, foldername);
			write_line(pointer, i, 0x02, folder_name, block_to_be_assigned);
			change_block_in_bitmap(block_to_be_assigned, 1);
			fclose(ptr);

			return 0;
		}
	}

	fprintf(stderr, "[ERROR] No queda espacio en la carpeta para crear otra carpeta.\n");
	return 1;
}

void write_byte(int block, int byte_position, unsigned char byte)
{
	FILE* ptr;
	ptr = fopen(disk, "r+b");
	fseek(ptr, 2048* block + byte_position, SEEK_SET);
	fwrite(&byte, 1, 1, ptr);
	fclose(ptr);
}

void clean_block(int block)
{
	FILE* ptr;
	ptr = fopen(disk,"r+b");
	int start = block * 2048;
	fseek(ptr, start, SEEK_SET);
	for (int i = 0; i < 2048; i++) write_byte(block, i, 0x00);
	fclose(ptr);
}

void get_name(unsigned char *block, char *string, int line)
{
	char name[12];
	strcpy(name, "");
	for (int l = 1; l < 12; l++) {
		sprintf(name + strlen(name), "%c", block[line + l]);
	}
	strcpy(string, name);
}

void get_parent_path(char* file_name, const char* abs_path)
{
	char path[1000];
	strcpy(path, abs_path);

	char parent_path[1000];
	strcpy(parent_path, "");
	char* current_level = strtok(path, "/");

	int count = -2;
	for (int i = 0; abs_path[i]; i++) {
		count += (abs_path[i] == '/');
	}

	while (current_level != NULL) {
		if ((count > 0 && is_folder(abs_path)) || (count >= 0 && !is_folder(abs_path))) {
			strcat(parent_path, "/");
			strcat(parent_path, current_level);
			count--;
		}
		current_level = strtok(NULL, "/");
	}

	strcat(parent_path, "/");

	strcpy(file_name, parent_path);
}

void get_child_name(char* file_name, const char* abs_path)
{
	char path[1000];
	strcpy(path, "");
	strcpy(path, abs_path);

	char last_name[1000];
	char* current_level = strtok(path, "/");

	while (current_level != NULL) {
		strcpy(last_name, current_level);
		current_level = strtok(NULL, "/");
	}

	strcpy(file_name, last_name);
}

void recursive_ls(char* path, int recursion_depth)
{
	int k = 0;
	char start[1000];
	char folder_start[1000];
	char real_name[1000];
	strcpy(start, "");
	strcpy(folder_start, "");
	strcpy(real_name, "");
	get_child_name(real_name, path);
	if (recursion_depth == 0) printf("\n");

	while (k < recursion_depth) {
		strcat(start, "    ");
		if (k < recursion_depth -1) strcat(folder_start, "    ");
		k++;
	}

	printf("%s", folder_start);
	char real_path[1000];
	strcpy(real_path, path);
	printf("%s\n", real_name);

	if (is_folder(path) == 0) {
		printf("[ERROR] No es directorio. \n");
		return;
	}

	int pointer = search_path(path);
	if (pointer == -1) {
		printf("[ERROR] Directorio no existe. \n");
		return;
	}
	else {
		unsigned char current_block[2048];
		get_block(pointer, current_block);
		char child[1000];

		for (int j = 0; j < 128; j++) {
			if(current_block[j * 16] == 0x02){
				get_name(current_block, child, j * 16);
				strcat(real_path, child);
				strcat(real_path, "/");
				recursive_ls(real_path, recursion_depth + 1);
				strcpy(real_path, path);
			}

			if ((int) current_block[j * 16] == 4) {
				printf("%s", start);
				print_name(current_block, j * 16);
			}
		}
	}
	if (recursion_depth == 0) printf("\n");
}

void print_folder(char* folder)
{
	printf("FOLDER FOR %s:\n", folder);

	int pointer = search_path(folder);
	unsigned char block[2048];
	get_block(pointer, block);
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 16; j++) {
			printf("%x ", block[i * 16 + j]);
		}
		printf("\n");
	}
	printf("\n");
}

unsigned char get_byte_in_block(unsigned char* block, int line, int index)
{
	return block[line * 16 + index];
}

int create_file_at_block(const char* file_name, const int pointer)
{
	int block_to_be_assigned = get_free_block();

	if (block_to_be_assigned == -1) {
		fprintf(stderr, "[ERROR] No queda espacio para crear archivo.\n");
		return -1;
	}

	unsigned char block[2048];
	get_block(pointer, block);

	FILE* ptr;
	ptr = fopen(disk, "r+b");

	for (int i = 0; i < 128; i++) {
		if (block[i * 16] != 0x02 && block[i * 16] != 0x04) {
			clean_block(block_to_be_assigned);

			change_block_in_bitmap(block_to_be_assigned, 1);
			write_line(pointer, i, 0x04, file_name, block_to_be_assigned);

			fclose(ptr);

			return block_to_be_assigned;
		}
	}

	return -1;
}

void read_data_block(czFILE* file_desc, unsigned char* buffer, int nbytes, unsigned char* data_block, int* bytes_read, int* blocks_read)
{
	for (int i = 0; i < 2048; i++) {
		if (file_desc->last_byte_read > 2048 * (*blocks_read) + i) continue;

		buffer[2048 * (*blocks_read) + i] = data_block[i];
		(*bytes_read)++;

		if (*bytes_read >= nbytes) break;
	}

	(*blocks_read)++;
}

void write_data_block(czFILE* file_desc, unsigned char* buffer, int nbytes, int pointer, int* bytes_written, int* blocks_written)
{
	for (int i = 0; i < 2048; i++) {
		write_byte(pointer, i, buffer[file_desc->last_byte_written]);
		(*bytes_written)++;
		(file_desc->last_byte_written)++;

		if (file_desc->last_byte_written >= nbytes) break;

	}
	(*blocks_written)++;
}

void write_line(int block_pointer, int line, unsigned char byte, const char* name, int pointer)
{
	write_byte(block_pointer, line * 16, byte);
	int len = strlen(name);
	for (int i = 0; i < 11 - len; i++) write_byte(block_pointer, line * 16 + i + 1, 0x00);
	for (int i = 0; i < len; i++) write_byte(block_pointer, line * 16 + i + 1 + (11 - len), name[i]);

	unsigned char digits[2];
	digits[0] = pointer & 0xFF;
	digits[1] = (pointer >> 8) & 0xFF;
	write_byte(block_pointer, line * 16 + 14, digits[1]);
	write_byte(block_pointer, line * 16 + 15, digits[0]);
}

int is_inside_itself(const char* orig, const char* dest)
{
	char origin_cmp[1000];
	char dest_cmp[1000];
	strcpy(origin_cmp, "");
	strcpy(dest_cmp, "");
	for (int start = 0; start < strlen(orig); start++){
		strncat(origin_cmp, &orig[start], 1);
		strncat(dest_cmp, &dest[start], 1);
	}

	if (strcmp(origin_cmp, dest_cmp) == 0) return 1;
	return 0;
}

int compare_dir(const char* dest1, const char* dest2)
{
	unsigned char block1[2048];
	unsigned char block2[2048];
	get_block(search_path(dest1), block1);
	get_block(search_path(dest2), block2);
	for (int i = 0; i < 128; i++) {
		if (get_byte_in_block(block1, i, 0) == 0x04) {  //FILE
			char name1[1000];
			char name2[1000];
			char path1[1000];
			char path2[1000];
			strcpy(name1, "");
			strcpy(name2, "");
			strcpy(path1, "");
			strcpy(path2, "");
			strcpy(path1, dest1);
			strcpy(path2, dest2);
			get_name(block1, name1, i * 16);
			get_name(block2, name2, i * 16);
			strcat(path1, name1);
			strcat(path2, name2);
			if (compare_file(path1, path2) == 0) {
				printf("COMPARE DIR FAILED WITH FILE AT LINE %d\n", i);
				return 0;
			}
		}
		else if (get_byte_in_block(block1, i, 0) == 0x02) {
			char name1[1000];
			char name2[1000];
			char path1[1000];
			char path2[1000];
			strcpy(name1, "");
			strcpy(name2, "");
			strcpy(path1, "");
			strcpy(path2, "");
			strcpy(path1, dest1);
			strcpy(path2, dest2);
			get_name(block1, name1, i * 16);
			get_name(block2, name2, i * 16);
			strcat(path1, name1);
			strcat(path2, name2);
			strcat(path1, "/");
			strcat(path2, "/");
			if (compare_dir(path1, path2) == 0) {
				printf("COMPARE DIR FAILED WITH DIR AT LINE %d\n", i);
				return 0;
			}
		}
	}

	return 1;
}

int compare_file(const char* dest1, const char* dest2)
{
	unsigned char block1[2048];
	unsigned char block2[2048];
	unsigned char block3[2048];
	unsigned char block4[2048];
	int path_1 = search_path(dest1);
	int path_2 = search_path(dest2);
	get_block(path_1, block1);
	get_block(path_2, block2);
	for (int i = 2; i < 511;i++) {
		int pointer_1 = (block1[i * 4 + 2] << 8) | block1[i * 4 + 3];
		int pointer_2 = (block2[i * 4 + 2] << 8) | block2[i * 4 + 3];
		if (pointer_1 != 0 && (compare_block(pointer_1, pointer_2) == 0)) {
			printf("COMPARE FILE FAILED AT POINTER %d\n", i - 2);
			return 0;
		}
	}
	int indirect_pointer_1 = (block1[511 * 4 + 2] << 8) | block1[511 * 4 + 3];
	int indirect_pointer_2 = (block2[511 * 4 + 2] << 8) | block2[511 * 4 + 3];
	if (((indirect_pointer_1 == 0) && (indirect_pointer_2 != 0)) || ((indirect_pointer_2 == 0) && (indirect_pointer_1 != 0))) {
		printf("COMPARE FILE FAILED AT INDIRECT POINTER\n");
		return 0;
	}

	if (indirect_pointer_1 == 0 && indirect_pointer_2 == 0) return 1;

	get_block(indirect_pointer_1, block3);
	get_block(indirect_pointer_2, block4);
	for (int i = 0; i < 512;i++) {
		int pointer_1 = (block3[i * 4 + 2] << 8) | block3[i * 4 + 3];
		int pointer_2 = (block4[i * 4 + 2] << 8) | block4[i * 4 + 3];
		if (compare_block(pointer_1, pointer_2) == 0) {
			printf("COMPARE FILE FAILED AT INDIRECT POINTER BLOCK %d\n", i);
			return 0;
		}
	}

	return 1;
}

int compare_block(int pointer_1, int pointer_2)
{
	unsigned char block1[2048];
	unsigned char block2[2048];
	get_block(pointer_1, block1);
	get_block(pointer_2, block2);
	for (int i = 0; i < 2048;i++) {
		if (block1[i] != block2[i]) {
			printf("COMPARE BLOCK FAILED AT BYTE %d\n", i);
			return 0;
		}
	}

	return 1;
}

int get_size(unsigned char* block)
{
	int result = (block[0] << 24) | (block[1] << 16) | (block[2] << 8) | block[3];
	return result;
}
