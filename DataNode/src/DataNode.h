/*
 * DataNode.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

//--------------------------------------- Constantes -------------------------------------

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/DataNode/DataNodeLog.log"
#define GET_BLOQUE 101
#define SET_BLOQUE 102

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	char ipFileSystem[20];
	char puertoFileSystem[20];
	char nombreNodo[10];
	char puertoWorker[20];
	char rutaDataBin[255];
} Configuracion;

//--------------------------------------- Variables Globales -------------------------------------

int estadoDataNode;
int dataBinTamanio;
int dataBinBloques;
Puntero punteroDataBin;
String campos[5];
Socket socketFileSystem;
Configuracion* configuracion;
ArchivoLog archivoLog;
File dataBin;

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionImprimir(Configuracion* configuracion);
void configuracionIniciarCampos();

//--------------------------------------- Funciones de DataNode -------------------------------------

void dataNodeIniciar();
void dataNodeAtenderFileSystem();
void dataNodeFinalizar();
bool dataNodeActivado();
bool dataNodeDesactivado();
void dataNodeActivar();
void dataNodeDesactivar();
void dataNodeDesconectarFS();

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir();
void dataBinSetBloque(Puntero datos);
void dataBinGetBloque(Puntero datos);
Puntero dataBinMapear();
Puntero dataBinUbicarPuntero(Entero numeroBloque);

//--------------------------------------- Funciones Varias -------------------------------------

Puntero getBloque(Entero numeroBloque);
void setBloque(Entero numeroBloque, Puntero datos);
void dataBinCalcularBloques();
