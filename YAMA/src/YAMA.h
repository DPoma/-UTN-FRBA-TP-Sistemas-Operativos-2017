/*
 * YAMA.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA";
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA/YAMAConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/YAMALog.log"

#define INTSIZE sizeof(int32_t)

typedef struct {
	char puertoMaster[50];
	char ipFileSystem[50];
	char puertoFileSystem[50];
	int retardoPlanificacion;
	char algoritmoBalanceo[50];
	char disponibilidadBase[50];
} Configuracion;


typedef struct {
	ListaSockets listaSelect;
	ListaSockets listaMaster;
	Socket maximoSocket;
	Socket listenerMaster;
	Socket fileSystem;
} Servidor;
Servidor* servidor;

typedef struct {
	int nodo;
	int bloque;
} Bloque;

typedef struct{
	int numero;
	bool conectado;
	int disponibilidad;
} Nodo;
Lista nodos;

String campos[6];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketFileSystem;
int estadoYama;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig);
void archivoConfigObtenerCampos();
void pantallaLimpiar();
void yamaIniciar();
void yamaConectarAFileSystem();
void yamaAtenderMasters();
void servidorInicializar();
void servidorAtenderPedidos();
void servidorControlarMaximoSocket(Socket);

void yamaPlanificar(Socket,void*,int);

void yamaFinalizar();


