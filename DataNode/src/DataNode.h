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

#define ACEPTACION 100
#define LEER_BLOQUE 101
#define ESCRIBIR_BLOQUE 102
#define COPIAR_BLOQUE 103
#define COPIAR_ARCHIVO 104
#define FINALIZAR 105
#define SOLICITAR_CONEXION 106

//--------------------------------------- Estructuras -------------------------------------

typedef void* Bloque;

typedef struct {
	char ipFileSystem[20];
	char puertoFileSystemDataNode[20];
	char puertoFileSystemWorker[20];
	char puertoMaster[20];
	char nombreNodo[10];
	char rutaDataBin[255];
	char ipPropia[20];
} Configuracion;


//--------------------------------------- Variables Globales -------------------------------------

int estadoDataNode;
int dataBinTamanio;
Entero dataBinBloques;
Puntero punteroDataBin;
String campos[7];
Socket socketFileSystem;
Configuracion* configuracion;
ArchivoLog archivoLog;
File dataBin;
Hilo hiloControl;

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionImprimir(Configuracion* configuracion);
void configuracionIniciarCampos();
void configuracionIniciar();

//--------------------------------------- Funciones de DataNode -------------------------------------

void dataNodeIniciar();
void dataNodeAtenderFileSystem();
void dataNodeFinalizar();
bool dataNodeActivado();
bool dataNodeDesactivado();
void dataNodeActivar();
void dataNodeDesactivar();
void dataNodeDesconectarFS();
void dataNodeConectarAFS();
void dataNodeAceptado();

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir();
void configuracionCalcularBloques();
Puntero dataBinMapear();
Puntero dataBinUbicarPuntero(Entero numeroBloque);
void dataBinConfigurar();

//--------------------------------------- Funciones de Bloque -------------------------------------

Bloque bloqueBuscar(Entero numeroBloque);
void bloqueEscribir(Puntero datos);
void bloqueObtenerParaLeer(Puntero datos);
void bloqueObtenerParaCopiar(Puntero datos);
void bloqueObtenerParaCopiarArchivo(Puntero datos);

//--------------------------------------- Interfaz con File System -------------------------------------

Puntero getBloque(Entero numeroBloque);
void setBloque(Entero numeroBloque, Puntero datos);
