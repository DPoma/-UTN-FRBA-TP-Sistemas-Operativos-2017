/*
 * Master.h

 *
 *  Created on: 15/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Master/MasterLog.log"

#define TRANSFORMACION 702
#define REDUCCION_LOCAL 703
#define REDUCCION_GLOBAL 704

#define EXITOTRANSFORMACION 801
#define FRACASOTRANSFORMACION 802


typedef struct {
	char ipYama[50];
	char puertoYama[50];
	char ipWorker[50];
	char puertoWorker[50];
} Configuracion;

typedef struct{
	int size_ip;
	char* ip;
	int nroBloque;
	int nroBytes;
	int size_puerto;
	char* puerto;
	int size_nombretemp;
	char* nombretemp;
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
WorkerTransformacion* deserializar(Mensaje* mensaje);
void confirmacionWorker(Socket unSocket);
void serializarYEnviar(int nroBloque, int nroBytes, char* nombretemp, Socket unSocket);
void establecerConexionConWorker(WorkerTransformacion* wt);
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

