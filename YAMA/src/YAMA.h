/*
 * YAMA.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define TAMANIO_DATO_MAXIMO 1024
#define CLIENTES_ESPERANDO 5
#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA/YAMAConfig.conf"
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA"
#define RUTA_LOG "/home/utnso/Escritorio/YAMALog.log"
#define CANTIDAD_PUERTOS 2

typedef struct {
	char ipFileSystem[50];
	char puertoFileSystem[50];
	int retardoPlanificacion;
	char algoritmoBalanceo[50];
} Configuracion;

String campos[4];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estado;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void cargarCampos();
