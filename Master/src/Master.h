/*
* Master.h


*
* Created on: 15/9/2017
* Author: Dario Poma
*/

#include "../../Biblioteca/src/Biblioteca.c"
#include <sys/time.h>
#include <sys/resource.h>

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterLog.log"

#define FRACASO -800
#define EXITO 1
#define DIRSIZE sizeof(Dir)
#define INTSIZE sizeof(int32_t)
#define TEMPSIZE 12


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
	char temp[TEMPSIZE];
} WorkerTransformacion;
WorkerTransformacion alternativo;

typedef struct{ //no se hasta que punto es util este struct
	Dir dir;
	char* temp;
	char* listaTemps;
} WorkerReduccion;

//--------------------------------------- Globales -------------------------------------

Semaforo* errorBloque;
Semaforo* recepcionAlternativo;

String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYama;
Socket socketWorker;
int estadoMaster;
char* scriptTransformacion;
int32_t lenTransformacion;
char* scriptReduccion;
int32_t lenReduccion;
WorkerTransformacion alternativo;
struct rusage uso;
struct timeval comienzo, fin;


//--------------------------------------- Funciones de Master -------------------------------------

void masterIniciar(char**);
void masterAtender();

void configuracionIniciar();
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
void transformaciones(Lista);
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
void reduccionLocal(Mensaje* m);
void reduccionGlobal(Mensaje* m);
