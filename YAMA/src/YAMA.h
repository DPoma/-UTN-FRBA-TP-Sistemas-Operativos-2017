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
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/YAMA/YAMALog.log"

#define INTSIZE sizeof(int32_t)
#define TEMPSIZE 12
#define DIRSIZE sizeof(Dir)
#define nullptr NULL

#define ACEPTACION 200
#define SOLICITAR_BLOQUES 201
#define ERROR_ARCHIVO 202
#define ENVIAR_BLOQUES 201
#define ERROR_ARCHIVO 202

#define FRACASO -800
#define EXITO 1
#define DESCONEXION 0
#define ABORTAR 301
#define SOLICITUD 302
#define TRANSFORMACION 303
#define REDUCLOCAL 304
#define REDUCGLOBAL 305
#define ALMACENADO 306
#define CIERRE 307

#define DESCONEXION_NODO 308

#define ENPROCESO 2
#define ABORTADO 3

typedef void* func;

typedef struct {
	char ipPropia[50];
	char puertoMaster[50];
	char puertoErrores[50];
	char ipFileSystem[50];
	char puertoFileSystem[50];
	int retardoPlanificacion;
	char algoritmoBalanceo[50];
	int disponibilidadBase;
	bool reconfigurar;
} Configuracion;

typedef struct {
	ListaSockets listaSelect;
	ListaSockets listaMaster;
	Socket maximoSocket;
	Socket listenerMaster;
	Socket fileSystem;
} Servidor;
Servidor* servidor;

typedef struct{
	bool conectado;
	uint32_t carga; //son uint32_t porque lo pide el tp, yo usaria ints
	uint32_t tareasRealizadas;
	uint32_t disponibilidad;
	Dir nodo;
} Worker;
Lista workers;

Lista masters;

int job=-1;
typedef struct{
	int job;
	Socket masterid;
	Dir nodo;
	int32_t bloque;
	int32_t bytes;
	Dir nodoAlt;
	int32_t bloqueAlt;
	int etapa;
	char* pathTemporal;
	int estado;
} Entrada;

Lista tablaEstados;
Lista tablaUsados; //entradas Abortadas, Error, o Terminadas que ya
//no se necesitan procesar, solo se dibujan

Configuracion* configuracion; //es un puntero por algun motivo?
ArchivoLog archivoLog;
int estadoYama;

int ipToNum(char*);

void darPathTemporal(char**,char);
void configurar();
void archivoConfigObtenerCampos();
void yamaIniciar();
void yamaAtender();
void yamaPlanificar(Socket,void*,int);
void actualizarTablaEstados(Mensaje*,Socket);
void actualizarEntrada(Entrada*,int,Mensaje*);
void dibujarTablaEstados();
int dirToNum(Dir);
void moverAUsados(bool(*)(void*));
void retardo();


