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
#define BLOQUE 1048576
#define GET_BLOQUE 101
#define SET_BLOQUE 102

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	char ipFileSystem[50];
	char puertoFileSystem[50];
	char nombreNodo[50];
	char puertoWorker[50];
	char rutaDataBin[100];
} Configuracion;

typedef struct{
	Entero numeroBloque;
	Entero tamanioDatos;
	Puntero datos;
} Bloque;

//--------------------------------------- Variables Globales -------------------------------------

int estadoDataNode;
int tamanioDataBin;
Puntero punteroDataBin;
String campos[5];
Socket socketFileSystem;
Configuracion* configuracion;
ArchivoLog archivoLog;
File dataBin;
Bloque bloques;

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
Puntero dataBinUbicarPuntero(Entero numeroBloque);
void dataBinSetBloque(Puntero datos);
Puntero dataBinGetBloque(Puntero datos);
Bloque* dataBinCrearBloque(Puntero puntero);
Puntero dataBinMapear();

//--------------------------------------- Funciones Varias -------------------------------------

Puntero getBloque(Entero numeroBloque);
void setBloque(Entero numeroBloque, String datos);
