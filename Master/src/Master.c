/*
 ============================================================================
 Name        : Master.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso YAMA
 ============================================================================
 */

#include "Master.h"

char* leerArchivo(FILE* archivo) {
	fseek(archivo, 0, SEEK_END);
	long posicion = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);
	char *texto = malloc(posicion + 1);
	fread(texto, posicion, 1, archivo);
	texto[posicion] = '\0';
	return texto;
}




int archivoValido(FILE* archivo) {
	return archivo != NULL;
}



bool esUnArchivo(char* c) {
	return string_contains(c, ".");
}


void enviarArchivo(FILE* archivo) {
	char* texto = leerArchivo(archivo);
	fileCerrar(archivo);
	mensajeEnviar(socketYAMA, 4, texto, strlen(texto)+1);
	mensajeEnviar(socketWorker, 4, texto, strlen(texto)+1);
	free(texto);
}


char* leerCaracteresEntrantes() {
	int i, caracterLeido;
	char* cadena = malloc(1000);
	for(i = 0; (caracterLeido= getchar()) != '\n'; i++)
		cadena[i] = caracterLeido;
	cadena[i] = '\0';
	return cadena;
}

void establecerConexiones(){
	imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo Conexion con YAMA (IP: %s | Puerto %s)", configuracion->ipYama, configuracion->puertoYama);
	socketYAMA = socketCrearCliente(configuracion->ipYama, configuracion->puertoYama, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion existosa con YAMA");
	imprimirMensajeDos(archivoLog, "[CONEXION] Estableciendo Conexion con Worker (IP: %s | Puerto: %s)", configuracion->ipWorker, configuracion->puertoWorker);
	socketWorker = socketCrearCliente(configuracion->ipWorker, configuracion->puertoWorker, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion existosa con Worker");

}

void leerArchivoConfig(){
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
	stringCopiar(configuracion->ipWorker, IP_LOCAL);
	stringCopiar(configuracion->puertoWorker, "5050");
	imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo Conexion con YAMA (IP: %s | Puerto %s)", configuracion->ipYama, configuracion->puertoYama);
}

int hayWorkersParaConectar(){
	Mensaje* m = mensajeRecibir(socketYAMA);
	if(*(int*)m->datos==-1){
		archivoLogInformarMensaje(archivoLog,"No hay mas workers por conectar");
		printf("No hay mas workers para conectar");
		return -1;
		}
	else{
		return 1;
		}

}

WorkerTransformacion* deserializar(){
	Mensaje* mensaje = mensajeRecibir(socketYAMA);
	int tamanio_ip;
	char* nroip;
	int numeroBloque;
	int numeropuerto;
	int tamanio_nombretemp;
	char* nombrearchivo;
	WorkerTransformacion* wt;

	memcpy(&tamanio_ip,mensaje->datos,sizeof(int));
	memcpy(&nroip,mensaje->datos + sizeof(int), wt->size_ip);
	memcpy(&numeroBloque, mensaje->datos + sizeof(int)+ wt->size_ip, sizeof(int));
	memcpy(&numeropuerto, mensaje->datos + (sizeof(int)*2) + wt->size_ip, sizeof(int));
	memcpy(&tamanio_nombretemp, mensaje->datos + (sizeof(int)*3) + wt->size_ip, sizeof(int));
	memcpy(&nombrearchivo, mensaje->datos+(sizeof(int)*3)+wt->size_ip+wt->size_nombretemp, wt->size_ip);

	wt->size_ip=tamanio_ip;
	wt->ip=nroip;
	wt->nroBloque=numeroBloque;
	wt->puerto=numeropuerto;
	wt->size_nombretemp=tamanio_nombretemp;
	wt->nombretemp=nombrearchivo;

	return wt;

}

Lista workersAConectar(){
	Lista workers = list_create();
	int pos=0;
	Mensaje* mens;
	WorkerTransformacion* wt;
	while(hayWorkersParaConectar(socketYAMA)){
		mens=mensajeRecibir(socketYAMA);
		wt= deserializar(mens);
		listaAgregarEnPosicion(workers,(WorkerTransformacion*)wt, pos);
		pos++;

	}

	return workers;
}


ListaSockets sockets(){
	Lista workers = workersAConectar();
	int size= listaCantidadElementos(workersAConectar());
	WorkerTransformacion* wt;
	int sWorker;
	ListaSockets sockets;
	int i;

	for(i=0; i<size;i++){

	 wt = listaObtenerElemento(workers,i);
	 sWorker = socketCrearCliente(wt->ip,wt->puerto,ID_MASTER);
	 listaSocketsAgregar(sWorker,&sockets);

	}

	return sockets;

}

/*
void conexion(){
	pthread_t hilo;
	Socket unSocket;
	hiloCrear(&hilo,*establecerConexionConWorker,&unSocket);

}
*/
void establecerConexionConWorker(Socket unSocket, WorkerTransformacion* wt){
	imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %s | PUERTO: %s", wt->ip, wt->puerto);
	socketWorker = socketCrearCliente(wt->ip, wt->puerto, ID_MASTER);

	serializarYEnviar(wt->nroBloque,wt->nombretemp,socketWorker);

}

void confirmacionWorker(Socket unSocket){
	Mensaje* mensaje = mensajeRecibir(unSocket);

	if(mensaje->header.operacion==EXITOTRANSFORMACION){
		imprimirMensajeUno(archivoLog, "[TRANSFORMACION] Transformacion realizada con exito en el Worker %i", &unSocket); //desp lo cambio
		mensajeEnviar(socketYAMA,EXITOTRANSFORMACION,&unSocket,sizeof(int));
	}
	else{
		imprimirMensajeUno(archivoLog,"[TRANSFORMACION] Transformacion fallida en el Worker %i",&unSocket);
		mensajeEnviar(socketYAMA,FRACASOTRANSFORMACION,&unSocket,sizeof(int));

	}
}



void serializarYEnviar(int nroBloque, char* nombretemp, Socket unSocket){
	int size_nombretemp = strlen(nombretemp)+1;
	char* data = malloc(sizeof(int)+size_nombretemp);
	int len = size_nombretemp+sizeof(int);

	memcpy(data,&nroBloque, sizeof(int));
	memcpy(data+sizeof(int),&nroBloque, sizeof(int));
	memcpy(data+(sizeof(int)*2),nombretemp,size_nombretemp);

	mensajeEnviar(unSocket,TRANSFORMACION,data,len);


}





int main(void) {
	pantallaLimpiar();
	leerArchivoConfig();
	establecerConexiones();

	masterActivar();

	senialAsignarFuncion(SIGINT, funcionSenial);
	mensajeEnviar(socketYAMA, HANDSHAKE, "HOLIII", 7);
	mensajeEnviar(socketWorker, HANDSHAKE, "HOLIII", 7);
	imprimirMensaje(archivoLog, "[MENSAJE] Mensaje enviado");

	while(masterActivado()){
		socketCerrar(socketYAMA);
		socketCerrar(socketWorker);
		archivoLogDestruir(archivoLog);
		memoriaLiberar(configuracion);
		}

	return EXIT_SUCCESS;

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipYama, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
	stringCopiar(configuracion->puertoYama, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
}

void funcionSenial(int senial) {
	masterDesactivar();
	puts("");
}



bool masterActivado() {
	return estadoMaster == ACTIVADO;
}

bool masterDesactivado() {
	return estadoMaster == DESACTIVADO;
}

void masterActivar() {
	estadoMaster= ACTIVADO;
}

void masterDesactivar() {
	estadoMaster = DESACTIVADO;
}

