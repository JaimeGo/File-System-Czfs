#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "structs/cz_API.h"


int main(int argc, char* argv[])
{
	// Revisa si el archivo binario es valido o no.
	if (argc != 2) {
		printf("Modo de uso: %s ./cffs <bin file>\n", argv[0]);
		printf("\t[obligatorio] bin file: ruta archivo binario de entrada.\n");
		return 1;
	}

	FILE* ptr;
  if ((ptr = fopen(argv[1], "r"))) {
    fclose(ptr);
  }
  else {
    printf("Disco duro Inexistente. Se finaliza la ejecución.\n");
    return 1;
  }


  printf("Bienvenidos a main!\n");
  printf("Probaremos cada una de las funciones, viendo si se ejecutan o no, y si captan errores. \n");
 	printf("\n//////////////////////////////////////////////////\nEJECUTANDO FUNCIONES GENERALES\n//////////////////////////////////////////////////\n");


  printf("\n\n\nEJECUTANDO cz_mount...\n");
	cz_mount(argv[1]);


  printf("\n\n\nEJECUTANDO cz_bitmap...\n");
	cz_bitmap();


  printf("\n\n\nEJECUTANDO cz_exists...\n");
  	printf("cz_exists('/memes/') = %d \n", cz_exists("/memes/"));
	printf("cz_exists('memes/') = %d \n", cz_exists("memes/"));
	printf("cz_exists('/') = %d \n", cz_exists("/"));
	printf("cz_exists('/memes/touhou/chen.mp3') = %d \n", cz_exists("/memes/touhou/chen.mp3"));
	printf("cz_exists('memes/pepefrog/') = %d \n", cz_exists("/memes/pepefrog/"));
	printf("cz_exists('memes/tou') = %d \n", cz_exists("/memes/tou"));
	printf("cz_exists('memes/touhou') = %d \n", cz_exists("/memes/touhou/"));
	printf("cz_exists('/memes/lerolerolero/hehe.txt') = %d \n", cz_exists("/memes/lerolerolero/hehe.txt"));


  printf("\n\n\nEJECUTANDO cz_ls...\n");
  	printf("cz_ls('/memes/'): \n");
	cz_ls("/memes/");
	printf("cz_ls('/'): \n");
	cz_ls("/");
	printf("cz_ls(''): \n");
	cz_ls("");
	printf("cz_ls('/memes/lerolerolerolero/touhou/'): \n");
	cz_ls("/memes/lerolerolerolero/touhou/");
	printf("cz_ls('/memes/hehe'): \n");
	cz_ls("/memes/hehe");
	printf("cz_ls('/memes/touhou/'): \n");
	cz_ls("/memes/touhou/");


 	printf("\n//////////////////////////////////////////////////\nEJECUTANDO FUNCIONES DE MANEJO DE ARCHIVOS\n//////////////////////////////////////////////////\n");


	czFILE* write_file;
	czFILE* write_file_error;
	czFILE* read_file;


  printf("\n\n\nEJECUTANDO cz_open...\n");
  	printf("Abriendo archivo: '/test.txt', para escritura\n");
	write_file = cz_open("/test.txt", 'w'); // CORECTO
	if(write_file != NULL)
	{
		printf("Archivo abierto correctamente!\n");
	}
	printf("Abriendo archivo: '/nombredemasiadogrande.jpg', para escritura\n");
	write_file_error = cz_open("nombredemasiadogrande.jpg", "w"); //ERROR
	if(write_file_error == NULL)
	{
		printf("Archivo no pudo ser abierto!\n");
	}


  printf("\n\n\nEJECUTANDO cz_write...\n");
  unsigned char* write_buffer = calloc(8192, sizeof(unsigned char));
	strcpy((char*) write_buffer, "Hello, Foo!");
	printf("Escribiendo 'Hello, Foo!' en '/test.txt' = %d\n", cz_write(write_file, write_buffer, 16));
	printf("Escribiendo 'Hello, Foo!' en '/nombredemasiadogrande.jpg' (no existe)...\n");
	cz_write(write_file_error, write_buffer, 16); // ERROR
  free(write_buffer);


  printf("\n\n\nEJECUTANDO cz_close...\n");
  	printf("cz_close('/test.txt'):\n");
  	printf("%d\n", cz_close(write_file)); // CORRECTO
  	printf("cz_close('/nombredemasiadogrande.txt'):\n");
	printf("%d\n", cz_close(write_file_error)); // ERROR


  printf("\n\n\nEJECUTANDO cz_read...\n");
	read_file = cz_open("/test.txt", 'r');
	printf("Abriendo '/test.txt', en lectura\n");
  unsigned char* read_buffer = calloc(8192, sizeof(unsigned char));
	printf("cz_read('/test.txt') = %d\n", cz_read(read_file, read_buffer, 16));
	cz_close(read_file);
  free(read_buffer);


  printf("\n\n\nEJECUTANDO cz_mv...\n");
  	printf("cz_mv('/test.txt', '/bigtest.txt') = %d\n", cz_mv("/test.txt", "/bigtest.txt")); // CORRECTO (CAMBIO NOMBRE)
	printf("cz_mv('/bigtest.txt', '/memes/touhou/bigtest.txt') = %d\n", cz_mv("/bigtest.txt", "/memes/touhou/bigtest.txt")); // CORRECTO
	printf("cz_mv('/dawg.jpg', '/memes/noexisto/dawg.jpg'):\n");
	printf("%d\n", cz_mv("/dawg.jpg", "/memes/noexisto/dawg.jpg")); // ERROR
	printf("cz_mv('/dawg.jpg', '/memes/nombredemasiadogrande.jpg'):\n");
	printf("%d\n", cz_mv("/dawg.jpg", "/memes/nombredemasiadogrande.jpg")); // ERROR
	printf("cz_mv('/dawg.jpg/', '/memes/test.jpg'):\n");
	printf("%d\n",cz_mv("/dawg.jpg/", "/memes/test.jpg")); // ERROR


  printf("\n\n\nEJECUTANDO cz_cp...\n");
	printf("cz_cp('/memes/touhou/bigtest.txt', '/bigtest.txt') = %d\n", cz_cp("/memes/touhou/bigtest.txt", "/bigtest.txt")); // CORRECTO
	printf("cz_cp('/memes/touhou/chen.mp3', '/chen_copia.mp3'):\n");
	printf("%d\n", cz_cp("/memes/touhou/chen.mp3", "/chen_copia.mp3")); // ERROR
	printf("cz_cp('/dawg.jpg', '/dowg.jpg') = %d\n", cz_cp("/dawg.jpg", "/dowg.jpg")); // CORRECTO
	printf("cz_cp('/memes/dawg.jpg', '/memes/dawg.jpg'):\n");
	printf("%d\n", cz_cp("/memes/dawg.jpg", "/memes/dawg.jpg")); // ERROR
	printf("cz_cp('/memes/noexisto.uwu', '/memes/dawg.jpg'):\n");
	printf("%d\n", cz_cp("/memes/noexisto.uwu", "/memes/dawg.jpg")); // ERROR
	printf("cz_cp('/memes/dawg.jpg', '/memes/touhou/hola.txt'):\n");
	printf("%d\n", cz_cp("/memes/dawg.jpg", "/memes/touhou/hola.txt")); // ERROR
	printf("cz_cp('/ayylmao.mp4', '/memes/touhou/noexisto/ayylmao.mp4'):\n");
	printf("%d\n", cz_cp("/ayylmao.mp4", "/memes/touhou/noexisto/ayylmao.mp4")); //ERROR


  printf("\n\n\nEJECUTANDO cz_rm...\n");
	printf("cz_rm('/memes/touhou/bigtest.txt') = %d\n", cz_rm("/memes/touhou/bigtest.txt")); // CORRECTO
	printf("cz_rm('/memes/noexisto/hehe.txt'):\n");
	printf("%d\n", cz_rm("/memes/noexisto/hehe.txt")); // ERROR
	printf("cz_rm('/memes/'):\n");
	printf("%d\n", cz_rm("/memes/")); // ERROR


 	printf("\n//////////////////////////////////////////////////\nEJECUTANDO FUNCIONES DE MANEJO DE DIRECTORIOS\n//////////////////////////////////////////////////\n");


  printf("\n\n\nEJECUTANDO cz_mkdir...\n");
	printf("cz_mkdir('/dank/') = %d\n", cz_mkdir("/dank/")); // CORRECTO
	printf("cz_mkdir('/'):\n");
	printf("%d\n", cz_mkdir("/")); // ERROR
	printf("cz_mkdir('/memes/touhou/'):\n");
	printf("%d\n", cz_mkdir("/memes/touhou/")); // ERROR
	printf("cz_mkdir('/memes/touhou/noexisto/jajaj/'):\n");
	printf("%d\n", cz_mkdir("/memes/touhou/noexisto/jajaj/")); // ERROR
	printf("cz_mkdir('/memes/heha.txt'):\n");
	printf("%d\n", cz_mkdir("/memes/heha.txt")); // ERROR


  printf("\n\n\nEJECUTANDO cz_mvdir...\n");
	printf("cz_mvdir('/dank/', '/memes/dank/') = %d\n", cz_mvdir("/dank/", "/memes/dank/")); // CORRECTO
	printf("cz_mvdir('/memes/', '/memes/touhou/memes/'):\n");
	printf("%d\n", cz_mvdir("/memes/", "/memes/touhou/memes/")); // ERROR


  printf("cz_mv('/bigtest.txt', '/memes/dank/bigtest.txt') = %d\n", cz_mv("/bigtest.txt", "/memes/dank/bigtest.txt")); // CORRECTO
  printf("cz_mv('/dowg.jpg', '/memes/dank/dowg.jpg') = %d\n", cz_mv("/dowg.jpg", "/memes/dank/dowg.jpg")); // CORRECTO


  printf("\n\n\nEJECUTANDO cz_cpdir...\n");
  	printf("cz_cpdir('/memes/touhou/', 'memes/dank/touhoumemes/') = %d\n", cz_cpdir("/memes/touhou/", "/memes/dank/touhoumemes/")); // CORRECTO
	printf("cz_cpdir('/', '/memes/touhou2/newroot/'):\n");
	printf("%d\n", cz_cpdir("/", "/memes/touhou2/newroot/")); // ERROR
	printf("cz_cpdir('/', '/memes/noexisto/noexistotampoco/''):\n");
	printf("%d\n", cz_cpdir("/", "/memes/noexisto/noexistotampoco/")); // ERROR
	printf("cz_cpdir('/memes', '/memes/super/'):\n");
	printf("%d\n", cz_cpdir("/memes", "/memes/super/")); // ERROR
	printf("cz_cpdir('/', '/memes/''):\n");
	printf("%d\n", cz_cpdir("/", "/memes/")); // ERROR


  printf("\n\n\nEJECUTANDO cz_rmdir...\n");
	printf("cz_rmdir('/memes/dank/') = %d\n", cz_rmdir("/memes/dank/")); // CORRECTO
	printf("cz_rmdir('/'):\n");
	printf("%d\n", cz_rmdir("/")); // ERROR
	printf("cz_rmdir('/hehe.txt'):\n");
	printf("%d\n", cz_rmdir("/hehe.txt")); // ERROR

 	return 0;
}
//./czfs discolleno.bin

// PROCESO: Montamos disco -> Mostramos bitmap -> Ejecutamos exists -> Ejecutamos ls
// Creamos test.txt en root (/test.txt) -> Escribimos en /test.txt "Hello, Foo!"
// -> Cerramos /test.txt -> Leemos /test.txt en buffer -> Cambiamos de nombre a /bigtest.txt
// -> Lo movemos a /memes/touhou/bigtest.txt -> copiamos de /memes/touhou/bigtest.txt a /bigtest.txt
// -> copiamos /dawg.jpg a /dowg.jpg  -> eliminamos /memes/touhou/bigtest.txt
// -> creamos la carpeta /dank/ -> la movemos a /memes/dank/ -> Movemos /bigtest.txt y /dowg.txt a /memes/dank/
// -> copiamos /memes/touhou/ a /memes/dank/ -> Removemos /memes/dank/.
