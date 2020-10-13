# ttsh (Triana & Toledo Shell)

## Funcionalidades:
* **basic:** funcionalidades básicas (3 puntos)
* **multi-pipe:** múltiples tuberías (1 punto)
* **background:** procesos ejecutándose en segundo plano (0.5 puntos)
* **spaces:** comandos con cualquier cantidad de espacios (0.5 puntos)
* **history:** historial de comandos (0.5 puntos)
* **help:** descripción de las funcionalidades del shell (1 punto)

## Comandos built-in
* **cd:** cambia de direcciones
* **exit:** termina el shell
* **history:** muestra el historial
* **fg:** devuelve al foreground un proceso del background
* **again:** ejecuta un comando del historial
* **help:** muestra la ayuda
* **jobs:** muestra todos los jobs en cola

## Ejecución del shell
Para ejecutar abra una consola y ejecute `make` y luego `./ttsh`

## Comandos
En el shell con el comando `exit` se puede cerrar el mismo, al cerrar salva los comandos en el historial de comandos. 

En el shell con el comando `history` se puede consultar los últimos 10 comandos en las ejecuciones del shell.

Para implementarlo se crea un archivo en el directorio **/home/USER** llamado *history.dat* donde se guardan los últimos 10 comandos. Es posible mostrar más de 10 comandos, solo es necesario definir la cantidad en la macro `HISTORY_LIMITS` del archivo **main.h**. Cuando se inicializa el shell, se carga el historial en una lista, y al cerrar el shell se guarda la lista en el archivo. Si se desea que un comando no se guarde en el historial solo es necesario introducir un espacio antes del comando. 

Por ejemplo:
```
ls
 echo hola mundo 
```
El comando `ls` se guarda en el historial pero `echo hola mundo` no se guarda.

En el shell con el comando `again <index>` se puede volver a ejecutar el comando que ocupa la posición `index` en el historial. Al ejecutar el comando lo que se guarda en el historial es el comando ha ejecutar por el `again`.

El comando `again` puede dar varios errores en ejecución:
	1. El primer error es que no exista ningún comando en el índice especificado y reportando el error: `'Unexistent command in history at index: <index>'`
	2. El segundo es que el índice esté fuera de la cantidad máxima de comandos en el historial, reportando el error: `'Index out of range of history'`

En el shell con el comando `cd <dir>` se puede cambiar la dirección del shell.

Si el comando `cd` no recibe ningún parámetro entonces cambia la dirección hacia **/home/USER**. Si recibe `<dir> = ..` cambia la dirección hacia el padre del actual directorio.

Alguno de los problemas es que no se puede pasar como parámetro una dirección que contenga espacios, por ejemplo: **/home/USER/proyecto shell**

En el shell con el comando `jobs` se puede obtener todos los comandos que se estan ejecutando en el background. Imprime por consola todos los procesos con el formato:
 `[<index>] <pid> <command> <state>`

En el shell con el comando `fg <index>` se puede traer un proceso de background hacia el foreground. Si no recibe ningún `<index>` el comando fg trae a foreground el último proceso que se envió a background. 

## Integrantes
Carlos Toledo Silva C-211

Ariel Alfonso Triana Pérez C-211
