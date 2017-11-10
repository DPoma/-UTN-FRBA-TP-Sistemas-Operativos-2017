/*
 ============================================================================
 Name : Master.c
 Author : Dario Poma
 Version : 1.0
 Copyright : Todos los derechos reservados papu
 Description : Proceso YAMA
 ============================================================================
 */

#include "Master.h"

int main(int contadorArgumentos, String* argumentos) {
	masterIniciar(contadorArgumentos, argumentos);
	masterConectarAYama(argumentos[3]);
	masterAtender();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de Master -------------------------------------

void masterIniciar(int contadorArgumentos, String* argumentos) {
	configuracionError(contadorArgumentos);
	configuracionIniciar();
	scriptTransformacionLeer(argumentos[1]);
	scriptReduccionLeer(argumentos[2]);
}

void masterConectarAYama(String archivoDatos) {
	socketYama = socketCrearCliente(configuracion->ipYama, configuracion->puertoYama, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion establecida con YAMA, esperando instrucciones");
	mensajeEnviar(socketYama, SOLICITUD, archivoDatos, stringLongitud(archivoDatos)+1);
}

void masterAtender(){
	while(estadoMaster == ACTIVADO) {
		Mensaje* mensaje = mensajeRecibir(socketYama);
		switch(mensaje->header.operacion) {
		case DESCONEXION: masterFinalizar(); break;
		case TRANSFORMACION: transformacionEjecutar(mensaje); break;
		case REDUCCION_LOCAL: reduccionLocalEjecutar(mensaje); break;
		case REDUCCION_GLOBAL: reduccionGlobalEjecutar(mensaje); break;
		case 301: scriptInvalido(); break;
		}
	}
}

void masterFinalizar() {
	imprimirMensaje(archivoLog,"[EJECUCION] Proces Master finalizado");
	estadoMaster=DESACTIVADO;
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

void configuracionIniciar() {
	configuracionIniciarLog();
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionImprimir();
	configuracionIniciarSemaforos();
	senialAsignarFuncion(SIGINT, configuracionSenial);
	estadoMaster = ACTIVADO;
}

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipYama, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
	stringCopiar(configuracion->puertoYama, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionSenial(int senial) {
	estadoMaster=DESACTIVADO;
}

void configuracionIniciarLog() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO MASTER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Master");
}

void configuracionIniciarCampos() {
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
}

void configuracionImprimir() {
	imprimirMensaje2(archivoLog, "[CONFIGURACION] Conectandose a YAMA (IP: %s | Puerto: %s)", configuracion->ipYama, configuracion->puertoYama);
}

void configuracionIniciarSemaforos() {
	semaforoErrorBloque = memoriaAlocar(sizeof(Semaforo));
	semaforoRecepcionAlternativa = memoriaAlocar(sizeof(Semaforo));
	semaforoIniciar(semaforoErrorBloque,1);
	semaforoIniciar(semaforoRecepcionAlternativa,0);
}

void configuracionError(int contadorArgumentos) {
	if(contadorArgumentos != MAX_ARGS) {
		puts(ROJO"[ERROR] Argumentos invalidos"BLANCO);
		exit(EXIT_FAILURE);
	}
}

//--------------------------------------- Funciones de Script -------------------------------------

void scriptLeer(File archScript, String* script, Entero* tamanio) {
	fseek(archScript, 0, SEEK_END);
	long posicion = ftell(archScript);
	fseek(archScript, 0, SEEK_SET);
	*script = memoriaAlocar(posicion + 1);
	fread(*script, posicion, 1, archScript);
	(*script)[posicion] = '\0';
	*tamanio = stringLongitud(*script);
}

void scriptTransformacionLeer(String path) {
	File fileScript = fileAbrir(path, LECTURA);
	scriptLeer(fileScript,&scriptTransformacion,&tamanioScriptTransformacion);
	fileCerrar(fileScript);
}

void scriptReduccionLeer(String path) {
	File fileScript = fileAbrir(path, LECTURA);
	scriptLeer(fileScript,&scriptReduccion,&tamanioScriptReduccion);
	fileCerrar(fileScript);
}

void scriptInvalido() {
	imprimirMensaje(archivoLog, ROJO"[ERROR] Path invalido, abortando proceso"BLANCO);
	exit(EXIT_FAILURE);
}

//--------------------------------------- Funciones de Transformacion -------------------------------------

void transformacionEjecutar(Mensaje* mensaje) {
	imprimirMensaje(archivoLog, "[MENSAJE] Lista de bloques recibida");
	Lista listaMaster = listaCrear();
	int indice;
	for(indice=0; indice<mensaje->header.tamanio; indice+=DIRSIZE+INTSIZE*2+TEMPSIZE){
		BloqueTransformacion* bloque = transformacionCrearBloque(mensaje->datos+indice);
		imprimirMensaje3(archivoLog, "[TRANSFORMACION] Bloque N°%i recibido (%s|%s)",(int*)bloque->numeroBloque, bloque->direccion.ip, bloque->direccion.port);
		bool flag = false;
		int indiceNodos;
		for(indiceNodos=0; indiceNodos < listaMaster->elements_count; indiceNodos++){
			Lista listaNodo = listaObtenerElemento(listaMaster,indiceNodos);
			BloqueTransformacion* bloqueComparador = listaPrimerElemento(listaNodo);
			if(nodoIguales(bloque->direccion, bloqueComparador->direccion)){
				listaAgregarElemento(listaNodo, bloque);
				flag = true;
				break;
			}
		}
		if(!flag){
			Lista listaNodo = listaCrear();
			listaAgregarElemento(listaNodo, bloque);
			listaAgregarElemento(listaMaster ,listaNodo);
			imprimirMensaje3(archivoLog,"[TRANSFORMACION] Lista para Nodo %s|%s armada, lista size #%d",bloque->direccion.ip, bloque->direccion.port,(int*)listaMaster->elements_count);
		}
	}
	transformacionCrearHilos(listaMaster);
}

void transformacionHilo(Lista listaBloques) {
	BloqueTransformacion* bloque = listaPrimerElemento(listaBloques);
	Socket socketWorker = socketCrearCliente(bloque->direccion.ip,bloque->direccion.port,ID_MASTER);
	imprimirMensaje2(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %s | PUERTO: %s",bloque->direccion.ip,bloque->direccion.port);
	mensajeEnviar(socketWorker,TRANSFORMACION,scriptTransformacion,tamanioScriptTransformacion);
	int indice;
	for(indice=0; indice<listaBloques->elements_count;indice++){
		BloqueTransformacion* bloque = listaObtenerElemento(listaBloques,indice);
		transformacionEnviarBloque(bloque, socketWorker);
		Mensaje* mensaje = mensajeRecibir(socketWorker);
		switch(mensaje->header.operacion) {
			case EXITO: transformacionExito(mensaje, listaBloques); break;
			case FRACASO: transformacionFracaso(mensaje, listaBloques); break;
		}
		mensajeDestruir(mensaje);
	}
	mensajeEnviar(socketWorker, EXITO, VACIO, ACTIVADO);
	socketCerrar(socketWorker);
}

BloqueTransformacion* transformacionCrearBloque(Puntero datos) {
	BloqueTransformacion* bloque = memoriaAlocar(sizeof(BloqueTransformacion));
	memcpy(&bloque->direccion, datos, DIRSIZE);
	memcpy(&bloque->numeroBloque, datos+DIRSIZE, INTSIZE);
	memcpy(&bloque->bytesUtilizados, datos+DIRSIZE+INTSIZE,INTSIZE);
	memcpy(bloque->nombreTemporal, datos+DIRSIZE+INTSIZE*2,TEMPSIZE);
	return bloque;
}

BloqueTransformacion* transformacionBuscarBloque(Lista listaBloques, Entero numeroBloque) {

	bool bloqueBuscarNumero(BloqueTransformacion* bloque) {
		return bloque->numeroBloque == numeroBloque;
	}

	BloqueTransformacion* bloque = listaBuscar(listaBloques, (Puntero)bloqueBuscarNumero);
	return bloque;
}

void transformacionCrearHilos(Lista listaMaster) {
	int indice;
	for(indice=0; indice < listaMaster->elements_count; indice++){
		Hilo hilo;
		Lista listaNodo = listaObtenerElemento(listaMaster, indice);
		hiloCrear(&hilo, (Puntero)transformacionHilo, listaNodo);
	}
}

void transformacionNotificarYama(Mensaje* mensaje, Lista listaBloques) {
	Entero numeroBloque = (Entero)mensaje->datos;
	BloqueTransformacion* bloque = transformacionBuscarBloque(listaBloques, numeroBloque);
	Puntero datos = memoriaAlocar(INTSIZE+DIRSIZE);
	memcpy(datos, &bloque->direccion, DIRSIZE);
	memcpy(datos+DIRSIZE, &numeroBloque, INTSIZE);
	mensajeEnviar(socketYama,mensaje->header.operacion,mensaje->datos, DIRSIZE);
}

void transformacionEnviarBloque(BloqueTransformacion* bloqueTransformacion, Socket socketWorker) {
	BloqueWorker* bloque = bloqueCrear(bloqueTransformacion);
	mensajeEnviar(socketWorker, TRANSFORMACION, bloque, sizeof(BloqueWorker));
	imprimirMensaje2(archivoLog,"[TRANSFORMACION] Enviando bloque N°%d (Archivo temporal: %s)",(int*)bloque->numeroBloque, bloque->nombreTemporal);
}

void transformacionExito(Mensaje* mensaje, Lista listaBloques) {
	imprimirMensaje1(archivoLog, "[TRANSFORMACION] Transformacion realizada con exito en el Worker %s",(*(Dir*)mensaje->datos).ip);
	transformacionNotificarYama(mensaje, listaBloques);
}

void transformacionFracaso(Mensaje* mensaje, Lista listaBloques) {
	imprimirMensaje(archivoLog,"[TRANSFORMACION] Transformacion fallida en el Worker");
	transformacionNotificarYama(mensaje, listaBloques);
	//TODO ver alternativo
}


/*
void alternativo() {
	memcpy(&alternativo.numeroBloque,mensaje->datos+i+DIRSIZE,INTSIZE);
	memcpy(&alternativo.bytesUtilizados,mensaje->datos+i+DIRSIZE+INTSIZE,INTSIZE);
	memcpy(&alternativo.nombreTemporal,mensaje->datos+i+DIRSIZE+INTSIZE*2,TEMPSIZE);
	semaforoSignal(semaforoRecepcionAlternativa);
}
*/

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocalEjecutar(Mensaje* mensaje) {
	Dir* NODO;
	int canttemps;
	memcpy(NODO->ip, m->datos, sizeof(char)*20);
	memcpy(NODO->port,m->datos + sizeof(char)*20, sizeof(char)*20);
	memcpy(&canttemps, m->datos + sizeof(char)*40, sizeof(int32_t));
	int tamanio=TEMPSIZE*(canttemps+1)+sizeof(int32_t)+tamanioScriptReduccion;
	char* nuevoBuffer =malloc(tamanio);
	memcpy(&nuevoBuffer, scriptReduccion, tamanioScriptReduccion);
	memcpy(&nuevoBuffer + tamanioScriptReduccion, m->datos+ sizeof(char)*40, tamanio - tamanioScriptReduccion);
	Socket sWorker =socketCrearCliente(NODO->ip, NODO->port, ID_MASTER);
	mensajeEnviar(sWorker,REDUCCION_LOCAL,nuevoBuffer,tamanio);
	free(m);
	Mensaje* mensaje = mensajeRecibir(sWorker);
	switch(mensaje->header.operacion){
		case -802://Fracaso
		{
			mensajeEnviar(socketYama, FRACASO, NULL, 0); //MANDA A YAMA QUE FALLO
			free(mensaje);
			break;
		}
		case 802: //Exito
		{
			mensajeEnviar(socketYama, EXITO, NULL, 0); //MANDA A YAMA QUE FALLO
			free(mensaje);
			break;
		}
	}
	socketCerrar(sWorker);
}

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobalEjecutar(Mensaje* m) {
	Dir* NODO;
	int canttemps;
	memcpy(NODO->ip, m->datos, sizeof(char)*20);
	memcpy(NODO->port,m->datos + sizeof(char)*20, sizeof(char)*20);
	memcpy(&canttemps, m->datos + sizeof(char)*40, sizeof(int32_t));
	int tamanio=(DIRSIZE+TEMPSIZE)*(canttemps)+TEMPSIZE+sizeof(int32_t)+tamanioScriptReduccion;
	char* nuevoBuffer =malloc(tamanio);
	memcpy(&nuevoBuffer, scriptReduccion, tamanioScriptReduccion);
	memcpy(&nuevoBuffer + tamanioScriptReduccion, m->datos+ sizeof(char)*40, tamanio - tamanioScriptReduccion);
	Socket sWorker =socketCrearCliente(NODO->ip, NODO->port, ID_MASTER);
	mensajeEnviar(sWorker,REDUCCION_GLOBAL,nuevoBuffer,tamanio);
	free(m);
	Mensaje* mensaje = mensajeRecibir(sWorker);
	switch(mensaje->header.operacion){
		case -802://Fracaso
		{
			//imprimirMensaje(archivoLog, ("[EJECUCION] Tuve problemas para comunicarme con el Master (Pid hijo: %d)", pid)); //el hijo fallo en comunicarse con el master
			mensajeEnviar(socketYama, FRACASO, NULL, 0); //MANDA A YAMA QUE FALLO
			free(mensaje);
			break;
		}
		case 802: //Exito
		{
			//imprimirMensaje(archivoLog, ("[EJECUCION] Tuve problemas para comunicarme con el Master (Pid hijo: %d)", pid)); //el hijo fallo en comunicarse con el master
			mensajeEnviar(socketYama, EXITO, NULL, 0); //MANDA A YAMA QUE FALLO
			free(mensaje);
			break;
		}
	}
	socketCerrar(sWorker);
}

//--------------------------------------- Funciones de Worker -------------------------------------

BloqueWorker* bloqueCrear(BloqueTransformacion* transformacion) {
	BloqueWorker* bloque = memoriaAlocar(sizeof(BloqueWorker));
	memcpy(bloque,&transformacion->numeroBloque,INTSIZE);
	memcpy(bloque+INTSIZE,&transformacion->bytesUtilizados,INTSIZE);
	memcpy(bloque+INTSIZE*2,transformacion->nombreTemporal,TEMPSIZE);
	return bloque;
}

void iniciarMetricaJob(){
	getrusage(RUSAGE_SELF,&uso);//info del mismo proceso
	comienzo = uso.ru_utime;//tiempo que pasa en la cpu
}

void finMetricaJob(){
	getrusage(RUSAGE_SELF,&uso);
	fin = uso.ru_utime;
}

bool nodoIguales(Dir a, Dir b) {
	return stringIguales(a.ip,b.ip) && stringIguales(a.port,b.port);
}
//void reduccionLocal(Mensaje* m){
//	WorkerReduccion* wr= deserializarReduccion(m);
//	int i;
//	Lista list = wr->tmps;
//	pthread_t hilo;
//	pthread_create(&hilo,NULL,&conectarConWorkerReduccionL, NULL);
//
//	for(i=1;i<list_size(list);i++){
//
//		if(strcmp(list_get(list,i) , list_get(list,i++))){//comparo los nombres de los temporales donde se guarda la reduccion (uno por nodo)
//			//crear otro hilo
//		}else{
//
//
//		}
//
//	}
//
//
//}
//
//WorkerReduccion* deserializarReduccion(Mensaje* mensaje){
//	int size = mensaje->header.tamanio;
//	int tamanio_dirs;
//	Lista direcciones;
//	int tamaniolista;
//	Lista temporales;
//	int tamanionombrestemps;
//	Lista nombretemps;
//	WorkerReduccion* wr = malloc(size);
//
//	memcpy(&tamanio_dirs, mensaje->datos, sizeof(int));
//	memcpy(&direcciones,mensaje->datos + sizeof(int), tamanio_dirs);
//	memcpy(&tamaniolista, mensaje->datos + tamanio_dirs + sizeof(int), sizeof(int));
//	memcpy(&temporales, mensaje->datos + tamanio_dirs + sizeof(int)*2, tamaniolista);// no se si se puede esto
//	memcpy(&tamanionombrestemps, mensaje->datos + tamanio_dirs + sizeof(int)*3, sizeof(int));
//	memcpy(&nombretemps, mensaje->datos + tamanio_dirs + sizeof(int)*3 + tamanionombrestemps, tamanionombrestemps);
//
//
//	wr->dirs_size = tamaniolista;
//	wr->dirs = direcciones;
//	wr->tmps_size = tamaniolista;
//	wr->tmps = temporales;
//	wr->nombretemp_size = tamanionombrestemps;
//	wr-> nombretempsreduccion = nombretemps;
//
//	return wr;
//
//}



//int hayWorkersParaConectar(){
//	Mensaje* m = mensajeRecibir(socketYama);
//	if(m->header.operacion==-1){
//		imprimirMensaje(archivoLog,"No hay mas workers por conectar");
//		return -1;
//		}
//	else{
//		return 1;
//		}
//
//}

//Lista workersAConectar(){
//	Lista workers = list_create();
//	int pos=0;
//	Mensaje* mens;
//	WorkerTransformacion* wt;
//	while(hayWorkersParaConectar(socketYama)){
//		mens=mensajeRecibir(socketYama);
//		wt= deserializarTransformacion(mens);
//		listaAgregarEnPosicion(workers,(WorkerTransformacion*)wt, pos);
//		pos++;
//	}
//	return workers;
//}

//ListaSockets sockets(){
//	Lista workers = workersAConectar();
//	int size= listaCantidadElementos(workersAConectar());
//	WorkerTransformacion* wt;
//	int sWorker;
//	ListaSockets sockets;
//	int i;
//
//	for(i=0; i<size;i++){
//
//	 wt = listaObtenerElemento(workers,i);
//	 sWorker = socketCrearCliente(wt->dir.ip,string_itoa(wt->dir.port),ID_MASTER);
//	 listaSocketsAgregar(sWorker,&sockets);
//
//	}
//
//	return sockets;
//}
