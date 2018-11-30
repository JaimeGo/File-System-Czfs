# Proyecto 1: Sistemas Operativos y Redes 2018-2.

### Integrantes del equipo

* Ian Fieldhouse Becerra, 16203615

* Ignacio Sarovic, 10633855

* Cristóbal Espinoza Jimenez, 16635205

* Jaime González, 11636475

### Decisiones de diseño

El programa busca simular un sistema de archivos simplificado sobre un disco virtual a través de la creación de un API que entrega las funcionalidades básicas y necesarias para la implementación de este tipo de sistemas. La representación del disco radica en una variable global que contiene la información organizada en conjuntos de bytes llamados bloques.

Para estructurar el código las distintas funciones y métodos dentro del archivo principal CZ_API se agruparon en 4 categorías: funciones generales, de archivos, de directorios y auxiliares (incluyendo métodos extras de testeo). Lo anterior realizado para mantener el orden entregado en el enunciado del proyecto entregando, a su vez, mayor facilidad en la corrección del mismo.

Sobre las primeras 3 categoras mencionadas cabe hacer algunos comentarios. La lógica relacionada a la funcionalidad del método cz_cp radica en la lectura del archivo de origen en un buffer seguido de una posterior escritura en la dirección de destino señalada. Para el método cz_mv se utilizó otra estrategia. Primero se escribe en la línea del bloque destino el nombre asignado en los parámetros junto con la dirección del bloque de direccionamiento. Luego se invalida el bloque de tipo directorio de origen (siempre y cuando este no coincida con la ruta de destino, en cuyo caso sólo hay cambio de nombre).Respecto a las funciones asociadas a directorios, tales como cz_rmdir, cz_mvdir y cz_cpdir, fueron implementadas mediante recursión. Estos son los métodos más complejos que tiene el programa y utilizan a su vez otras funciones básicas requeridas por el sistema como cz_rm,cz_cp y cz_mv cuando se encuentra con un archivo dentro del procedimiento recursivo.

Respecto a la funciones auxiliares ubicadas en la zona inferior de CZ_API,éstas fueron creadas en base a dos objetivos: en primer lugar para facilitar el desarrollo de las funciones solicitadas mediante la reutilización efectiva de código y en segundo lugar para entregar herramientas extras de testeo en el desarrollo del programa. Algunos ejemplos del primer grupo de funciones son search_path (obtiene el puntero de una entrada dada un path), get_block (retorna un bloque específico), get_name(entrega el nombre de una archivo/subdirectorio ), entre otras, y son utilizadas prácticamente en casi todos los métodos mínimos requeridos. Dentro del segundo grupo se peude hacer uso de compare_block, compare_file y compare_dir, cuyos objetivos son autoexplicativos.

Considerando los puntos mencionados previamente se puede observar que el programa está desarrollado para poder entregar las funcionalidades de manera simple, en base a un código ordenado y claro, permitiendo a su vez, la posibilidad de una futura extensión del mismo sin mayores inconvenientes.

### Sobre al archivo de prueba entregado

Sobre el archivo main.c entregado en esta entrega se utilizan todas las funciones básicas requeridas para corroborar el funcionamiento de la API de la siguiente manera (desarrollado en base al archivo binario simdiskfilled.bin):

1) Se monta el disco. 

2) Se imprime el Bitmap y la información general asociada al disco.

3) Se verifica la existencia de algunos archivos y directorios.

4) Se ve el contenido de algunas carpetas.

5) Se crea y abre un archivo llamado test.txt.

6) Se escribe sobre éste.

7) Se cierra.

8) Posterioremente se lee el contenido recién escrito.

9) Se toma el mismo archivo test.txt y se le cambia el nombre a bigtest.txt.

10) Luego se realiza una copia de bigtest.txt.

11) Se copia el archivo dawn.jpg a uno con nombre down.jpg (dentro de la misma carpeta contenedora).

11) Se elimina el archivo bigtest.txt original.

12) Ahora se crea un directorio llamado "dank".

13) Se mueve hacia otra posición.

14) Se copia otro directorio y se ubica dentro de "dank".

15) Finalmente se elimina recursivamente la carpeta de prueba "dank".


### Supuestos adicionales

1) En relación a las rutas siemrpe se debe colocar un "/" al final si es que se está haciendo referencia a un directorio. Por ejemplo, si quiero ver el contenido de la carpeta llamada "Vacaciones" la ruta entrega al método cz_ls debe ser de la forma "/Vacaciones/". Para los archivos simplemente se indica el nombre el archivo y su extensión como se hace regularmente.

2) Dentro de los bloques de tipo directorio se permite utilizar los 11 Bytes para indicar el nombre del archivo o subdirectorio. Por este motivo existirán casos en que no existe un byte 0x00 para señalar el fin del contenido.

3) Se espera que para el uso de la API se utilice el archivo simdiskfilled.bin. Esto ya que el archivo main.c que contiene todo el procedimiento de testeo de las funciones requeridas fue construido en base a la estructura basal de aquel archivo. En caso de utilizar otro archivo binario para representar el disco se deberá modificar main.c para que quede en relación a éste, pudiendo entonces corroborar efectivamente que las funciones cumplan su cometido.

4) La función cz_cpdir toma un tiempo extendido de ejecución (varios minutos, dependiendo o no del uso de valgrind), por lo que se debe tener un poco de paciencia en algunos casos en el que se utilice este método.

5) Se considera que para poder copiar y mover archivos/subdirectorios la ruta de destino no puede haber carpetas inexistentes entre medio. Lo anterior también aplica al concepto de creación de los mismos: sólo se podrá crear un archivo/subdirectorio siempre que la ruta previa al mismo exista.
