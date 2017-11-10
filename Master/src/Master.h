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

#define MAX_ARGS 4
#define FRACASO -800
#define EXITO 1
#define DIRSIZE sizeof(Dir)
#define INTSIZE sizeof(Entero)
#define TEMPSIZE 12


#define FRACASO -800
#define EXITO 1
#define DESCONEXION 0
#define ABORTAR 301
#define SOLICITUD 302
#define TRANSFORMACION 303
#define REDUCCION_LOCAL 304
#define REDUCCION_GLOBAL 305
#define ALMACENADO 306
#define CIERRE 307

typedef struct {
	char ipYama[50];
	char puertoYama[50];
	char ipWorker[50];
	char puertoWorker[50];
} Configuracion;

typedef struct{
	Dir direccion;
	int numeroBloque;
	int bytesUtilizados;
	char nombreTemporal[TEMPSIZE];
} BloqueTransformacion;

typedef struct{
	Dir direccion;
	String listaTemporales;
	char nombreTemporal[TEMPSIZE];
} BloqueReduccionLocal;

typedef struct{
	Entero numeroBloque;
	Entero bytesUtilizados;
	char nombreTemporal[TEMPSIZE];
} BloqueWorker;


//--------------------------------------- Globales -------------------------------------

String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Semaforo* semaforoErrorBloque;
Semaforo* semaforoRecepcionAlternativa;
Socket socketYama;
int estadoMaster;
String scriptTransformacion;
String scriptReduccion;
Entero tamanioScriptTransformacion;
Entero tamanioScriptReduccion;
BloqueTransformacion alternativo;
struct rusage uso;
struct timeval comienzo, fin;

//--------------------------------------- Funciones de Master -------------------------------------

void masterIniciar(int argc, String* argv);
void masterConectarAYama(String archivoDatos);
void masterAtender();
void masterFinalizar();

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionIniciar();
void configuracionIniciarLog();
void configuracionIniciarCampos();
void configuracionImprimir();
void configuracionIniciarSemaforos();
void configuracionError(int contadorArgumentos);

//--------------------------------------- Funciones de Script -------------------------------------

void scriptLeer(File archScript, String* script, Entero* tamanio);
void scriptTransformacionLeer(String path);
void scriptReduccionLeer(String path);
void scriptInvalido();

//--------------------------------------- Funciones de Transformacion -------------------------------------

void transformacionEjecutar(Mensaje* mensaje);
void transformacionCrearHilos(Lista listaMaster);
void transformacionNotificarYama(Mensaje* mensaje, Lista ListaBloques);
void transformacionEnviarBloque(BloqueTransformacion* bloqueTransformacion, Socket socketWorker);
void transformacionExito(Mensaje* mensaje, Lista listaBloques);
void transformacionFracaso(Mensaje* mensaje, Lista listaBloques);
void transformacionHilo(Lista listaBloques);
BloqueTransformacion* transformacionCrearBloque(Puntero datos);
BloqueTransformacion* transformacionBuscarBloque(Lista listaBloques, Entero numeroBloque);

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocalEjecutar(Mensaje* mensaje);

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobalEjecutar(Mensaje* mensaje);

//--------------------------------------- Funciones de Worker -------------------------------------

BloqueWorker* bloqueCrear(BloqueTransformacion* transformacion);
bool nodoIguales(Dir a, Dir b);



