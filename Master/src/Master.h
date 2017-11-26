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
#include "commons/collections/queue.h"

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
#define DESCONEXION_NODO 308

typedef void* func;

typedef struct {
	char ipYama[50];
	char puertoYama[50];
	char ipWorker[50];
	char puertoWorker[50];
} Configuracion;

typedef struct{
	Dir dir;
	int32_t bloque;
	int32_t bytes;
	char temp[TEMPSIZE];
} WorkerTransformacion;
WorkerTransformacion alternativo;


//--------------------------------------- Globales -------------------------------------

Semaforo* errorBloque;
Semaforo* recepcionAlternativo;


String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYama;
int estadoMaster;
char* scriptTransformacion;
int32_t lenTransformacion;
char* scriptReduccion;
int32_t lenReduccion;
char* archivoSalida;

struct{
	clock_t procesoC;
	double proceso;
	double almacenado;
	double reducGlobal;//por ahi es al pedo usar doubles
	double reducLocalSum;
	int cantRedLoc;
	double transformacionSum;
	int cantTrans;
	int fallos;
	int paralelo;
	int maxParalelo;
	Semaforo* paralelos;
	Semaforo* transformaciones;
	Semaforo* reducLocales;
}metricas;

//--------------------------------------- Funciones de Master -------------------------------------

void masterIniciar(char**);
void masterAtender();

void transformaciones(Lista);
void reduccionLocal(Mensaje*);
void reduccionGlobal(Mensaje*);
void almacenado(Mensaje*);

double transcurrido(clock_t);
void tareasEnParalelo(int);
