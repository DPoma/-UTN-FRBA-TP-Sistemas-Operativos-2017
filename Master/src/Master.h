/*
 * Master.h

 *
 *  Created on: 15/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define TAMANIO_DATO_MAXIMO 1024
#define CLIENTES_ESPERANDO 5
#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master"
#define RUTA_LOG "/home/utnso/Escritorio/MasterLog.log"
#define CANTIDAD_PUERTOS 2

typedef struct {
	char ipYAMA[50];
	char puertoYAMA[50];
} Configuracion;

String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYAMA;
Socket socketWorker;
int estado;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void cargarCampos();
