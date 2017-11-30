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

#define RUTA_CONFIG "Master.config"

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
	char rutaLog[150];
} Configuracion;

typedef struct{
	Dir dir;
	int32_t bloque;
	int32_t bytes;
	char temp[TEMPSIZE];
} WorkerTransformacion;


//--------------------------------------- Globales -------------------------------------

Semaforo* listaTransformandos;


String campos[3];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYama;
char* scriptTransformacion;
int32_t lenTransformacion;
char* scriptReduccion;
int32_t lenReduccion;
char* archivoSalida;

Lista transformandos;
typedef struct{
	pthread_t hilo;
	Dir dir;
	t_queue* bloquesExtra;
}HiloTransformacion;

struct{
	time_t procesoC;
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
HiloTransformacion* buscarHilo(Dir);
bool nodoIguales(Dir,Dir);
bool socketConectarMasterEspecialized(Conexion*,Socket);
Socket socketCrearClienteMasterEspecialized(String,String,int,String);
