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
#define nullptr NULL

#define ACEPTACION 200
#define SOLICITAR_BLOQUES 201
#define ERROR_ARCHIVO 202

#define DIRSIZE sizeof(Dir)


typedef struct {
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

#define ENPROCESO 1
#define TERMINADO 2
#define ABORTADO 3

typedef enum {Error,EnProceso,Terminado,Abortado} Estado;

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
	char* pathTemporal; //podría usar char[12] y no usar memoria dinamica, despues ver
	Estado estado;
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
void pantallaLimpiar();
void yamaIniciar();
void yamaAtender();
void yamaPlanificar(Socket,void*,int);
void actualizarTablaEstados(Entrada*,Estado);
void dibujarTablaEstados();
void retardo();


