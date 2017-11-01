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

#define DIRSIZE sizeof(Direccion)


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
	Direccion nodo; //no me lo reconoce, por ahi estan mal los paths?
} Worker;
Lista workers;

typedef enum {Solicitud,Transformacion,ReducLocal,ReducGlobal,Cierre,Aborto} Etapa;
typedef enum {Error=-1,EnProceso=0,Terminado,Abortado} Estado;
int job=-1;
typedef struct{
	int job;
	Socket masterid;
	Direccion nodo;
	int32_t bloque;
	int32_t bytes;
	Direccion nodoAlt;
	int32_t bloqueAlt;
	Etapa etapa;
	char* pathTemporal; //podría usar char[12] y no usar memoria dinamica, despues ver
	Estado estado;
} Entrada;

typedef struct __attribute__((packed)) { //TODO borrar esta cosa
	Direccion direccionCopia1;
	Entero numeroBloqueCopia1;
	Direccion direccionCopia2;
	Entero numeroBloqueCopia2;
	Entero bytesUtilizados;
} BloqueYama;

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


