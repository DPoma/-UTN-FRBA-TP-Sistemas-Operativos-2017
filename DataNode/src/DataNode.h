/*
 * DataNode.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define TAMANIO_DATO_MAXIMO 1024
#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/DataNode"
#define RUTA_LOG "/home/utnso/Escritorio/DataNodeLog.log"

typedef struct {
	char ipFileSystem[50];
	char puertoFileSystem[50];
	char nombreNodo[50];
	char puertoWorker[50];
	char rutaDataBin[100];
} Configuracion;

String campos[4];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estado;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void cargarCampos();
