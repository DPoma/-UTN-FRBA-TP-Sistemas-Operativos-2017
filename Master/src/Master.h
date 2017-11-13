/*
* Master.h


*
* Created on: 15/9/2017
* Author: Dario Poma
*/

#include "../../Biblioteca/src/Biblioteca.c"
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterLog.log"

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

//--------------------------------------- Globales -------------------------------------

Semaforo* errorBloque;
Semaforo* recepcionAlternativo;
Semaforo* paralelos;

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
char* archivoSalida;
int paralelo;
int maxParalelo;
//struct rusage uso;
//struct timeval comienzo, fin;

//--------------------------------------- Funciones de Master -------------------------------------

void masterIniciar(char**);
void masterAtender();

void transformaciones(Lista);
void reduccionLocal(Mensaje*);
void reduccionGlobal(Mensaje*);
void almacenadoFinal(Mensaje*);
