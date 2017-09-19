/*
 * YAMA.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA/YAMAConfig.conf"
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA"
#define RUTA_LOG "/home/utnso/Escritorio/YAMALog.log"

typedef struct {
	char puertoMaster[50];
	char ipFileSystem[50];
	char puertoFileSystem[50];
	int retardoPlanificacion;
	char algoritmoBalanceo[50];
} Configuracion;


typedef struct {
	ListaSockets listaSelect;
	ListaSockets listaMaster;
	Socket maximoSocket;
	Socket listenerMaster;
} Servidor;


String campos[5];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketFileSystem;
int estadoYAMA;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void archivoConfigObtenerCampos();
void pantallaLimpiar();
void YAMAIniciar();
void YAMAAtenderMasters();
void YAMAConectarAFileSystem();
void servidorInicializar(Servidor* servidor);
void servidorFinalizar(Servidor* servidor);
void servidorAtenderPedidos(Servidor* servidor);
