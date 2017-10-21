/*
 * Master.h

 *
 *  Created on: 15/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterLog.log"


typedef enum {Solicitud,Transformacion,ReducLocal,ReducGlobal,Cierre} Etapa;

#define EXITOTRANSFORMACION 801
#define FRACASOTRANSFORMACION 802

typedef struct __attribute__((__packed__)){
	int32_t ip;
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



String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYAMA;
Socket socketWorker;
int estadoMaster;


Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void archivoConfigObtenerCampos();
void establecerConexiones();
void leerArchivoConfig();
void archivoConfigObtenerCampos();
void funcionSenial();

int hayWorkersParaConectar();
WorkerTransformacion* deserializarTransformacion(Mensaje* mensaje);
void confirmacionWorker(Socket unSocket);
void serializarYEnviar(int nroBloque, int nroBytes, char* nombretemp, Socket unSocket);
void establecerConexionConWorker(Lista);
void transformacion(Mensaje* mensaje);
Lista workersAConectar();
ListaSockets sockets();
void serializarYEnviar();


bool masterActivado();
bool masterDesactivado();
void masterActivar();
void masterDesactivar();

char* leerArchivo(FILE* f);
int archivoValido(FILE* f);
bool esUnArchivo(char* c);
void enviarArchivo(FILE* f);
char* leerCaracteresEntrantes();

