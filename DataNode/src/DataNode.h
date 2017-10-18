/*
 * DataNode.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"
#include <sys/mman.h>

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/DataNode/DataNodeLog.log"

//Defines de operaciones

#define GETBLOQUE 101
#define SETBLOQUE 102


#define MB 1048576

/*
typedef struct {



} Serializado;
*/

typedef struct{
	int nroBloque;
/*		EL tamaño de un contenido es de 1 MB = 1024 Kb (no se si esta bien esto)

//Debo establecer el tamaño del contenido que se pueda guardar
	char* contenido = malloc(sizeof(char)*1024*1024*8);

	Bloque* sig;
	La pequeña linea de arriba da error, no recuedo como debía ser la estructura para que funque*/
}Bloque;

typedef struct {
	char ipFileSystem[50];
	char puertoFileSystem[50];
	char nombreNodo[50];
	char puertoWorker[50];
	char rutaDataBin[100];
} Configuracion;

String campos[5];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoDataNode;
FILE* dataBin;
Bloque bloques;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void archivoConfigObtenerCampos();
void dataNodeIniciar();
bool dataNodeActivado();
bool dataNodeDesactivado();
void dataNodeActivar();
void dataNodeDesactivar();
void finalizarDataNode();
void freeMemory();
void deserizalizar(Mensaje* mensaje);
void setBloque(int nroBloque, char* datos, int size);
char* getBloque(int nroBloque);
void guardarContenido(Bloque bloqueBuscado, Mensaje* mensajeAGuardar);
void atenderFileSystem(Socket unSocket);
