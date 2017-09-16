/*
 * FileSystem.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include <stdbool.h>
#include "../../Biblioteca/src/Biblioteca.c"

#define TAMANIO_DATO_MAXIMO 1024
#define CLIENTES_ESPERANDO 5
#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/FileSystem/FileSystemConfig.conf"
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/FileSystem"
#define RUTA_LOG "/home/utnso/Escritorio/FileSystem.log"
#define CANTIDAD_PUERTOS 2

typedef struct {
	int comando;
	char argumento[1000];
} Instruccion;



typedef struct {
	char ipFileSystem[50];
	char puertoYAMA[50];
	char puertoDataNode[50];
	char rutaMetadata[100];
} Configuracion;

String campos[5];
String ipFileSystem;
String puertos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estado;

//--------------------------------------- Funciones para Servidor -------------------------------------

void controlServidorInicializar(ControlServidor* controlServidor);
int controlServidorCantidadSockets(ControlServidor* servidor);
void controlServidorSetearListaSelect(ControlServidor* servidor);
void controlServidorActualizarMaximoSocket(ControlServidor* servidor, Socket unSocket);
void controlServidorEjecutarSelect(ControlServidor* servidor);

void puertoActivarListener(Puerto* puerto, Conexion* conexion);
void puertoFinalizarConexionCon(Servidor* servidor, Socket unSocket);
void puertoAceptarCliente(Socket unSocket, Servidor* servidor);
void puertoRecibirMensajeCliente(Servidor* servidor, Socket unSocket);
void puertoActualizarSocket(Servidor* servidor, Socket unSocket);

void servidorActivarPuertos(Servidor* servidor, String* puertos);
void servidorSetearEstado(Servidor* servidor, int estado);
Servidor servidorCrear(String* puertos, int cantidadPuertos);
void servidorFinalizar();
bool servidorEstaActivo(Servidor servidor);
void servidorAtenderClientes(Servidor* servidor);
void servidorActualizarPuertos(Servidor* servidor);

void notificadorInformar(Socket unSocket);
void notificadorInicializar(ControlServidor* controlServidor);
Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void cargarCampos();
void cargarDatos();
void archivoConfigImprimir(Configuracion* configuracion);
void funcionSenial(int senial);
