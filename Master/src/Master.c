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

int main(int argc, String* argv) {
	if(argc != 5) {
		puts(ROJO"[ERROR] Faltan o sobran argumentos"BLANCO);
		abort();
	}
	masterIniciar(argv);
	masterAtender();
	return EXIT_SUCCESS;
}

void masterIniciar(String* argv) {
	pantallaLimpiar();
	archivoLog = archivoLogCrear(RUTA_LOG, "Master");
	imprimirMensajeProceso("# PROCESO MASTER");
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
	Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
		Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
		stringCopiar(configuracion->ipYama, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
		stringCopiar(configuracion->puertoYama, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
		archivoConfigDestruir(archivoConfig);
		return configuracion;
	}
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	imprimirMensajeDos(archivoLog, "[CONEXION] Configuracion para conexion con YAMA (IP: %s | Puerto: %s)", configuracion->ipYama, configuracion->puertoYama);
	void configuracionSenial(int senial) {
		estadoMaster=DESACTIVADO;
	}
	senialAsignarFuncion(SIGINT, configuracionSenial);
	estadoMaster=ACTIVADO;
	errorBloque = memoriaAlocar(sizeof(Semaforo));
	recepcionAlternativo = memoriaAlocar(sizeof(Semaforo));
	semaforoIniciar(errorBloque,1);
	semaforoIniciar(recepcionAlternativo,0);

	void leerArchivo(File archScript,char** script,int* len){
		fseek(archScript, 0, SEEK_END);
		long posicion = ftell(archScript);
		fseek(archScript, 0, SEEK_SET);
		*script=malloc(posicion + 1);
		fread(*script, posicion, 1, archScript);
		(*script)[posicion] = '\0';
		*len=strlen(*script);
		fclose(archScript);
	}

	leerArchivo(fopen(argv[1],"r+"),&scriptTransformacion,&lenTransformacion);
	leerArchivo(fopen(argv[2], "r+"),&scriptReduccion,&lenReduccion);

	imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo Conexion con YAMA...", configuracion->ipYama, configuracion->puertoYama);
	socketYama = socketCrearCliente(configuracion->ipYama, configuracion->puertoYama, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion establecida con YAMA, esperando instrucciones");
	mensajeEnviar(socketYama,Solicitud,argv[3],stringLongitud(argv[3])+1);
}

void masterAtender(){
	puts("ESPERANDO MENSAJE");
	Mensaje* mensaje=mensajeRecibir(socketYama);
	if(mensaje->header.operacion == 301) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Path invalido, abortando proceso"BLANCO);
		abort();
	}
	imprimirMensaje(archivoLog, "[MENSAJE] Lista de bloques recibida");
	int i;
	bool mismoNodo(Dir a,Dir b){
		return a.ip==b.ip&&a.port==b.port;//podría comparar solo ip
	}
	Lista listas=list_create();
	for(i=0;i<mensaje->header.tamanio;i+=DIRSIZE+INTSIZE*2+TEMPSIZE){
		WorkerTransformacion bloque;
		memcpy(&bloque.dir,mensaje->datos+i,DIRSIZE);
		memcpy(&bloque.bloque,mensaje->datos+i+DIRSIZE,INTSIZE);
		memcpy(&bloque.bytes,mensaje->datos+i+DIRSIZE+INTSIZE,INTSIZE);
		memcpy(&bloque.temp,mensaje->datos+i+DIRSIZE+INTSIZE*2,TEMPSIZE);
		int j;
		for(j=0;j<=listas->elements_count;j++){
			Lista nodo=list_get(listas,j);
			WorkerTransformacion* cmp=list_get(nodo,0);
			if(mismoNodo(bloque.dir,cmp->dir)){
				list_addM(nodo,&bloque,sizeof bloque);
			}
		}
		Lista nodo=list_create();
		list_addM(nodo, &bloque,sizeof bloque);
		list_add(listas,nodo);//creo que no hay que alocar nada
	}

	for(i=0;i<=listas->elements_count;i++){
		pthread_t hilo;
		pthread_create(&hilo,NULL,transformaciones,list_get(listas,i));
	}
	while(estadoMaster==ACTIVADO) {
		Mensaje* m = mensajeRecibir(socketYama);
		switch(m->header.operacion){
		case Aborto:
			imprimirMensaje(archivoLog,"[ABORTO] Abortando proceso");
			abort(); //supongo que los hilos mueren aca
			//si no se mueren matarlos
			break;
		case Cierre:
			imprimirMensaje(archivoLog,"[EJECUCION] Terminando proceso");
			estadoMaster=DESACTIVADO;
			break;
		case Transformacion://hubo error y se recibió un bloque alternativo
			memcpy(&alternativo.bloque,mensaje->datos+i+DIRSIZE,INTSIZE);
			memcpy(&alternativo.bytes,mensaje->datos+i+DIRSIZE+INTSIZE,INTSIZE);
			memcpy(&alternativo.temp,mensaje->datos+i+DIRSIZE+INTSIZE*2,TEMPSIZE);
			semaforoSignal(recepcionAlternativo);
			break;
		case ReducLocal:
			reduccionLocal(m);
			break;
		case ReducGlobal:
			reduccionGlobal(m);
			break;
		}
	}
}

void transformaciones(Lista bloques){
	WorkerTransformacion* dir = list_get(bloques,0);
	socketWorker=socketCrearCliente(dir->dir.ip,dir->dir.port,ID_MASTER);
	mensajeEnviar(socketWorker,Transformacion,scriptTransformacion,lenTransformacion);
	int enviados=0,respondidos=0;
	enviarBloques:
	for(;enviados<bloques->elements_count;enviados++){
		WorkerTransformacion* wt=list_get(bloques,enviados);
		int tamanio=sizeof(WorkerTransformacion)-DIRSIZE;
		char data[tamanio];
		memcpy(data,&wt->bloque,INTSIZE);
		memcpy(data+INTSIZE,&wt->bytes,INTSIZE); //a worker le interesan los bytes?
		memcpy(data+INTSIZE*2,&wt->temp,TEMPSIZE);
		mensajeEnviar(socketWorker,Transformacion,data,tamanio);
		imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %s | PUERTO: %d",wt->dir.ip,wt->dir.port);
	}
	for(;respondidos<enviados;respondidos++){
		Mensaje* mensaje = mensajeRecibir(socketWorker);
		//a demas de decir exito o fracaso devuelve el numero de bloque
		void enviarActualizacion(){
			mensaje=realloc(mensaje,mensaje->header.tamanio+DIRSIZE+sizeof(Header));
			memmove(mensaje->datos+DIRSIZE,mensaje->datos,mensaje->header.tamanio);
			memcpy(mensaje->datos,&dir->dir,DIRSIZE);
			mensajeEnviar(socketYama,mensaje->header.operacion,mensaje->datos,mensaje->header.tamanio+DIRSIZE);
			mensajeDestruir(mensaje);
		}
		if(mensaje->header.operacion==EXITO){
			imprimirMensajeUno(archivoLog, "[TRANSFORMACION] Transformacion realizada con exito en el Worker %s",(*(Dir*)mensaje->datos).ip);
			enviarActualizacion();
		}else{
			semaforoWait(errorBloque);
			enviarActualizacion();
			imprimirMensajeUno(archivoLog,"[TRANSFORMACION] Transformacion fallida en el Worker %i",&socketWorker);
			semaforoWait(recepcionAlternativo);
			list_addM(bloques,&alternativo,sizeof alternativo);
			semaforoSignal(errorBloque);
			//se podría modificar para que se puedan procesar varios errores
			//en paralelo, pero no vale la pena porque agrega mucho codigo
			//y se supone que estos errores son casos raros
			//para hacer eso se tendrian que sacar el semaforo y
			//enviar el numero de thread en el mensaje, para diferenciar despues
			respondidos++;
			goto enviarBloques;
		}
	}
	mensajeEnviar(socketWorker, EXITO, NULL, 0);
	socketCerrar(socketWorker);
}

void reduccionLocal(Mensaje* m){
	Dir* NODO;
	int canttemps;
	memcpy(NODO->ip, m->datos, sizeof(char)*20);
	memcpy(NODO->port,m->datos + sizeof(char)*20, sizeof(char)*20);
	memcpy(&canttemps, m->datos + sizeof(char)*40, sizeof(int32_t));
	int tamanio=TEMPSIZE*(canttemps+1)+sizeof(int32_t)+lenReduccion;
	char* nuevoBuffer =malloc(tamanio);
	memcpy(&nuevoBuffer, scriptReduccion, lenReduccion);
	memcpy(&nuevoBuffer + lenReduccion, m->datos+ sizeof(char)*40, tamanio - lenReduccion);
	Socket sWorker =socketCrearCliente(NODO->ip, NODO->port, ID_MASTER);
	mensajeEnviar(sWorker,ReducLocal,nuevoBuffer,tamanio);
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

void reduccionGlobal(Mensaje* m){
	Dir* NODO;
	int canttemps;
	memcpy(NODO->ip, m->datos, sizeof(char)*20);
	memcpy(NODO->port,m->datos + sizeof(char)*20, sizeof(char)*20);
	memcpy(&canttemps, m->datos + sizeof(char)*40, sizeof(int32_t));
	int tamanio=(DIRSIZE+TEMPSIZE)*(canttemps)+TEMPSIZE+sizeof(int32_t)+lenReduccion;
	char* nuevoBuffer =malloc(tamanio);
	memcpy(&nuevoBuffer, scriptReduccion, lenReduccion);
	memcpy(&nuevoBuffer + lenReduccion, m->datos+ sizeof(char)*40, tamanio - lenReduccion);
	Socket sWorker =socketCrearCliente(NODO->ip, NODO->port, ID_MASTER);
	mensajeEnviar(sWorker,ReducLocal,nuevoBuffer,tamanio);
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







//char* leerCaracteresEntrantes() {
//	int i, caracterLeido;
//	char* cadena = malloc(1000);
//	for(i = 0; (caracterLeido= getchar()) != '\n'; i++)
//		cadena[i] = caracterLeido;
//	cadena[i] = '\0';
//	return cadena;
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
