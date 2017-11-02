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
#define DIRSIZE sizeof(Dir)
#define INTSIZE sizeof(int32_t)
#define TEMPSIZE 12

//--------------------------------------- Estructuras -------------------------------------

typedef enum {Solicitud,Transformacion,ReducLocal,ReducGlobal,Almacenamiento,Cierre,Aborto} Etapa;

typedef struct {
	char ipYama[20];
	char puertoYama[20];
} Configuracion;

typedef struct{
	Dir dir;
	int bloque;
	int bytes;
	char* temp;
}WorkerTransformacion;

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
void transformaciones(Lista);
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
