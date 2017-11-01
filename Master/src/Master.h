/*
 * Master.h

 *
 *  Created on: 15/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

//--------------------------------------- Constantes -------------------------------------

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterLog.log"
#define EXITO 1
#define FRACASO 0
#define SCRIPT_TRANSFORMACION 501
#define SCRIPT_REDUCTOR 502
#define DIRSIZE sizeof(Direccion)
#define INTSIZE sizeof(int32_t)
#define TEMPSIZE 12

//--------------------------------------- Estructuras -------------------------------------

typedef enum {Solicitud,Transformacion=1,ReduccionLocal=2,ReduccionGlobal=3,Almacenamiento=4,Finalizacion,Aborto=6} Etapa;

typedef struct {
	char ipYama[20];
	char puertoYama[20];
} Configuracion;

typedef struct{
	Direccion direccion;
	int bloque;
	int bytes;
	char* temp;
}WorkerTransformacion;

typedef struct{
	Entero dirs_size;
	Lista dirs;
	Entero tmps_size;
	Lista tmps;
	Entero nombretemp_size;
	Lista nombretempsreduccion;
} WorkerReduccion;

//--------------------------------------- Globales -------------------------------------

Mutex errorBloque;
Mutex recepcionAlternativo;
String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYama;
Socket socketWorker;
int estadoMaster;
FILE* scriptTransformacion;
FILE* scriptReductor;
WorkerTransformacion alternativo;

//--------------------------------------- Funciones de Master -------------------------------------

void configuracionIniciar();
Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionImprimir(Configuracion* configuracion);
void configuracionIniciarLog();
void configuracionIniciarCampos();
void configuracionSenial();

//--------------------------------------- Funciones de Configuracion -------------------------------------

void masterIniciar();
void masterConectarAYama(String path);
void masterAtenderYama();
void masterFinalizar();
bool masterActivado();
bool masterDesactivado();
void masterActivar();
void masterDesactivar();
void masterAtender();
//--------------------------------------- Funciones de algo -------------------------------------

void etapaTransformacion();
void aborto();
void reduccionLocal(Mensaje* m);
void reduccionGlobal();

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
char* leerArchivo(FILE* f);
int archivoValido(FILE* f);
bool esUnArchivo(char* c);
void enviarArchivo(FILE* f);
char* leerCaracteresEntrantes();
void establecerConexiones();
