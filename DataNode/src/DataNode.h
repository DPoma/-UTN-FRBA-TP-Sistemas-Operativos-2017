/*
 * DataNode.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"
#include <sys/mman.h>

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/DataNodeLog.log"

//Defines de operaciones

#define GETBLOQUE 101
#define SETBLOQUE 102

/*
typedef struct {



} Serializado;
*/


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

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void archivoConfigObtenerCampos();
void dataNodeIniciar();
bool dataNodeActivado();
bool dataNodeDesactivado();
void dataNodeActivar();
void dataNodeDesactivar();
void finalizarDataNode();
void deserizalizar(Mensaje* mensaje);
void setBloque();
void getBloque();
