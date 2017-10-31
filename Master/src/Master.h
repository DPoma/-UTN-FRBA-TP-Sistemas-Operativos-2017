/*
 * Master.h

 *
 *  Created on: 15/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterLog.log"

typedef enum {Solicitud,Transformacion=1,ReducLocal=2,ReducGlobal=3,Almacenamiento=4,Cierre,Aborto=6} Etapa;

#define EXITO 1
#define FRACASO 0

#define SCRIPT_TRANSFORMACION 501
#define SCRIPT_REDUCTOR 502

typedef struct __attribute__((__packed__)){
	char ip[20];
	int32_t port;
} Dir;

#define DIRSIZE sizeof(Dir)
#define INTSIZE sizeof(int32_t)
#define TEMPSIZE 12

typedef struct {
	char ipYama[50];
	char puertoYama[50];
	char ipWorker[50];
	char puertoWorker[50];
} Configuracion;

typedef struct{
	Dir dir;
	int bloque;
	int bytes;
	char* temp;
}WorkerTransformacion;
WorkerTransformacion alternativo;

typedef struct{
	Dir dir;
	Entero list_size;
	Lista tmps;
	char* nombretemp;
}WorkerReduccion;


Mutex errorBloque;
Mutex recepcionAlternativo;

String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYAMA;
Socket socketWorker;
int estadoMaster;

FILE* scriptTransformacion;
FILE* scriptReductor;

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void archivoConfigObtenerCampos();
void establecerConexiones();
void leerArchivoConfig();
void archivoConfigObtenerCampos();
void funcionSenial();

int hayWorkersParaConectar();
WorkerTransformacion* deserializarTransformacion(Mensaje* mensaje);
WorkerReduccion* deserializarReduccion(Mensaje* mensaje);
void confirmacionWorker(Socket unSocket);
void serializarYEnviar(Entero nroBloque, Entero nroBytes, char* nombretemp, Socket unSocket);
void establecerConexionConWorker(Lista);
void transformacion(Mensaje* mensaje);
Lista workersAConectar();
ListaSockets sockets();
void serializarYEnviar();
void enviarScript(Socket unSocket, FILE* script, Entero operacion);


bool masterActivado();
bool masterDesactivado();
void masterActivar();
void masterDesactivar();

char* leerArchivo(FILE* f);
int archivoValido(FILE* f);
bool esUnArchivo(char* c);
void enviarArchivo(FILE* f);
char* leerCaracteresEntrantes();

