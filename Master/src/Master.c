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
	if(m->header.operacion==-1){
		imprimirMensaje(archivoLog,"No hay mas workers por conectar");
		printf("No hay mas workers para conectar");
		return -1;
		}
	else{
		return 1;
		}

}

WorkerTransformacion* deserializar(Mensaje* mensaje){
	int size = mensaje->header.tamanio;
	int numeroip;
	int numeropuerto;
	int numeroBloque;
	int numeroBytes;
	char* nombretemporal;
	WorkerTransformacion* wt= malloc(size);

	memcpy(&numeroip, mensaje->datos,sizeof(int));
	memcpy(&numeropuerto, mensaje->datos + sizeof(int), sizeof(int));
	memcpy(&numeroBloque, mensaje->datos + sizeof(int)*2 , sizeof(int));
	memcpy(&numeroBytes, mensaje->datos + sizeof(int)*3, sizeof(int));
	memcpy(&nombretemporal, mensaje->datos + sizeof(int)*3 +12, 12);//tamaño del nombre del temporal es 12

	wt->ip = numeroip;
	wt->puerto = numeropuerto;
	wt->nroBloque = numeropuerto;
	wt->nroBytes = numeroBytes;
	wt->nombretemp = nombretemporal;

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
	 //sWorker = socketCrearCliente(wt->ip,wt->puerto,ID_MASTER);  falta pasar a char* ip y puerto
	 //listaSocketsAgregar(sWorker,&sockets);

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

void serializarYEnviar(int nroBloque, int nroBytes, char* nombretemp, Socket unSocket){
	int size_nombretemp = strlen(nombretemp)+1;
	char* data = malloc(sizeof(int)*2+size_nombretemp);
	int len = size_nombretemp+sizeof(int)*2;

	memcpy(data,&nroBloque, sizeof(int));
	memcpy(data+sizeof(int),&nroBytes, sizeof(int));
	memcpy(data+sizeof(int)*2,&nroBloque, sizeof(int));
	memcpy(data+(sizeof(int)*3),nombretemp,size_nombretemp);

	mensajeEnviar(unSocket,TRANSFORMACION,data,len);


}




void establecerConexionConWorker( WorkerTransformacion* wt){

	//falta convertir a char* ip y puerto
	//imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %d | PUERTO: %d", wt->ip, wt->puerto);
	//socketWorker = socketCrearCliente(wt->ip, wt->puerto, ID_MASTER);
	serializarYEnviar(wt->nroBloque,wt->nroBytes, wt->nombretemp,socketWorker);

	confirmacionWorker(socketWorker);

}



void transformacion(Mensaje* mens){
	WorkerTransformacion* wt = deserializar(mens);
	pthread_t hilo;

	pthread_create(&hilo,NULL,(void*)establecerConexionConWorker,&wt);


}



void reduccionLocal(){

}

void reduccionGlobal(){

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
		Mensaje* m = mensajeRecibir(socketYAMA);
		switch(m->header.operacion){

			case -1:
			imprimirMensaje(archivoLog,"[EJECUCION] Error en Yama");
			masterDesactivar();
			break;

			case TRANSFORMACION:

			transformacion(m);
			break;

			case REDUCCION_LOCAL:

			reduccionLocal();
			break;

			case REDUCCION_GLOBAL:

			reduccionGlobal();
			break;

		}




	}

	socketCerrar(socketYAMA);
	socketCerrar(socketWorker);
	archivoLogDestruir(archivoLog);
	memoriaLiberar(configuracion);


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

