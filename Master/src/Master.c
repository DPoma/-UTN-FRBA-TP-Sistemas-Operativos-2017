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
	imprimirMensaje2(archivoLog, "[CONEXION] Configuracion para conexion con YAMA (IP: %s | Puerto: %s)", configuracion->ipYama, configuracion->puertoYama);
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

	imprimirMensaje2(archivoLog,"[CONEXION] Estableciendo Conexion con YAMA...", configuracion->ipYama, configuracion->puertoYama);
	socketYama = socketCrearCliente(configuracion->ipYama, configuracion->puertoYama, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion establecida con YAMA, esperando instrucciones");
	mensajeEnviar(socketYama,SOLICITUD,argv[3],stringLongitud(argv[3])+1);



}

void iniciarClock(){
	tiempo=clock();
}


void masterAtender(){
	puts("ESPERANDO MENSAJE");
	Mensaje* mensaje=mensajeRecibir(socketYama);
	iniciarClock();
	if(mensaje->header.operacion == 301) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Path invalido, abortando proceso"BLANCO);
		abort();
	}
	imprimirMensaje(archivoLog, "[MENSAJE] Lista de bloques recibida");
	int i;
	bool mismoNodo(Dir a,Dir b){
		return stringIguales(a.ip,b.ip)&&stringIguales(a.port,b.port);//podría comparar solo ip
	}
	Lista listas=list_create();
	for(i=0;i<mensaje->header.tamanio;i+=DIRSIZE+INTSIZE*2+TEMPSIZE){
		WorkerTransformacion bloque;
		memcpy(&bloque.dir,mensaje->datos+i,DIRSIZE);
		memcpy(&bloque.bloque,mensaje->datos+i+DIRSIZE,INTSIZE);
		memcpy(&bloque.bytes,mensaje->datos+i+DIRSIZE+INTSIZE,INTSIZE);
		memcpy(bloque.temp,mensaje->datos+i+DIRSIZE+INTSIZE*2,TEMPSIZE);
		imprimirMensaje3(archivoLog, "[RECEPCION] bloque %s %s %d",bloque.dir.ip,bloque.dir.port,(int*)bloque.bloque);
		int j;
		bool flag=false;
		for(j=0;j<listas->elements_count;j++){
			Lista nodo=list_get(listas,j);
			WorkerTransformacion* cmp=list_get(nodo,0);
			if(mismoNodo(bloque.dir,cmp->dir)){
				list_addM(nodo,&bloque,sizeof bloque);
				flag=true;
				break;
			}
		}
		if(!flag){
			Lista nodo=list_create();
			list_addM(nodo, &bloque,sizeof(WorkerTransformacion));
			list_addM(listas,nodo,sizeof(t_list));
			imprimirMensaje3(archivoLog,"] lista para nodo %s %s armada, lista #%d",bloque.dir.ip,bloque.dir.port,(int*)listas->elements_count);
		}
	}

	for(i=0;i<listas->elements_count;i++){
		pthread_t hilo;
		pthread_create(&hilo,NULL,&transformaciones,list_get(listas,i));
	}
	while(estadoMaster==ACTIVADO) {
		Mensaje* m=mensajeRecibir(socketYama);
		switch(m->header.operacion){
		case ABORTAR:
			imprimirMensaje(archivoLog,"[ABORTO] Abortando proceso");
			abort(); //supongo que los hilos mueren aca
			//si no se mueren matarlos
			break;
		case CIERRE:
			imprimirMensaje(archivoLog,"[EJECUCION] Terminando proceso");
			estadoMaster=DESACTIVADO;
			break;
		case TRANSFORMACION://hubo error y se recibió un bloque alternativo
			memcpy(&alternativo.bloque,mensaje->datos+i+DIRSIZE,INTSIZE);
			memcpy(&alternativo.bytes,mensaje->datos+i+DIRSIZE+INTSIZE,INTSIZE);
			memcpy(&alternativo.temp,mensaje->datos+i+DIRSIZE+INTSIZE*2,TEMPSIZE);
			semaforoSignal(recepcionAlternativo);
			break;
		case REDUCLOCAL:
			reduccionLocal(m);
			break;
		case REDUCGLOBAL:
			reduccionGlobal(m);
			break;
		case ALMACENADO:
			almacenadoFinal(m);
		}
		mensajeDestruir(m);
	}

	tiempo= clock() - tiempo;
	double tiempotranscurrido = ((double)tiempo/CLOCKS_PER_SEC); //me da en segundos

	imprimirMensaje1(archivoLog,"[TIEMPO] El job tardo %f segundos en ejecutarse",&tiempotranscurrido);

}



void transformaciones(Lista bloques){
	WorkerTransformacion* dir = list_get(bloques,0);
	socketWorker=socketCrearCliente(dir->dir.ip,dir->dir.port,ID_MASTER);
	imprimirMensaje2(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %s | PUERTO: %s",dir->dir.ip,dir->dir.port);
	mensajeEnviar(socketWorker,TRANSFORMACION,scriptTransformacion,lenTransformacion);
	int enviados=0,respondidos=0;
	do{
		for(;enviados<bloques->elements_count;enviados++){
			WorkerTransformacion* wt=list_get(bloques,enviados);
			int tamanio=INTSIZE*2+TEMPSIZE;
			char data[tamanio];
			memcpy(data,&wt->bloque,INTSIZE);
			memcpy(data+INTSIZE,&wt->bytes,INTSIZE); //a worker le interesan los bytes?
			memcpy(data+INTSIZE*2,wt->temp,TEMPSIZE);
			mensajeEnviar(socketWorker,TRANSFORMACION,data,tamanio);
			imprimirMensaje2(archivoLog,"[CONEXION] Enviando bloque %d %s",(int*)wt->bloque,wt->temp);
		}
		for(;respondidos<enviados;respondidos++){
			Mensaje* mensaje = mensajeRecibir(socketWorker);
			//a demas de decir exito o fracaso devuelve el numero de bloque
			void enviarActualizacion(){
				imprimirMensaje2(archivoLog,"se recibio actualizacion de Worker %d %d",(int32_t*)mensaje->datos,&mensaje->header.operacion);
				mensaje=realloc(mensaje,mensaje->header.tamanio+DIRSIZE+sizeof(Header));
				memmove(mensaje->datos+DIRSIZE,mensaje->datos,mensaje->header.tamanio);
				memcpy(mensaje->datos,&dir->dir,DIRSIZE);
				mensajeEnviar(socketYama,mensaje->header.operacion,mensaje->datos,mensaje->header.tamanio+DIRSIZE);
				mensajeDestruir(mensaje);
			}
			if(mensaje->header.operacion==EXITO){
				imprimirMensaje1(archivoLog, "[TRANSFORMACION] Transformacion realizada con exito en el Worker %s",(*(Dir*)mensaje->datos).ip);
				enviarActualizacion();
			}else{
				semaforoWait(errorBloque);
				enviarActualizacion();
				imprimirMensaje1(archivoLog,"[TRANSFORMACION] Transformacion fallida en el Worker %i",&socketWorker);
				semaforoWait(recepcionAlternativo);
				list_addM(bloques,&alternativo,sizeof alternativo);
				semaforoSignal(errorBloque);
				//se podría modificar para que se puedan procesar varios errores
				//en paralelo, pero no vale la pena porque agrega mucho codigo
				//y se supone que estos errores son casos raros
				//para hacer eso se tendrian que sacar el semaforo y
				//enviar el numero de thread en el mensaje, para diferenciar despues
			}
			mensajeDestruir(mensaje);
		}
	}while(enviados<bloques->elements_count);
	mensajeEnviar(socketWorker, EXITO, NULL, 0);
	socketCerrar(socketWorker);
}
void reduccionLocal(Mensaje* m){
	Dir* nodo;
	memcpy(nodo,m->datos,DIRSIZE);
	int32_t cantTemps=(m->header.tamanio-DIRSIZE-TEMPSIZE)/TEMPSIZE;
	int tamanio=cantTemps*TEMPSIZE+TEMPSIZE+lenReduccion+INTSIZE*2;
	void* buffer=malloc(tamanio);
	memcpy(buffer,&lenReduccion,INTSIZE);//script
	memcpy(buffer+INTSIZE,scriptReduccion,lenReduccion);

	memcpy(buffer+INTSIZE+lenReduccion,&cantTemps,INTSIZE);//origen
	memcpy(buffer+INTSIZE*2+lenReduccion,m->datos+DIRSIZE,cantTemps*TEMPSIZE);

	memcpy(buffer+INTSIZE*2+lenReduccion+cantTemps*TEMPSIZE,m->datos+DIRSIZE+cantTemps*TEMPSIZE,TEMPSIZE);//destino

	Socket sWorker=socketCrearCliente(nodo->ip,nodo->port,ID_MASTER);
	mensajeEnviar(sWorker,REDUCLOCAL,buffer,tamanio);

	Mensaje* mensaje = mensajeRecibir(sWorker);
	imprimirMensaje(archivoLog,mensaje->header.operacion==EXITO?"[EJECUCION]REDUCCION LOCAL EXISTOSA"
			:"[ERROR]FALLO EN LA RE_D_UCCION LOCAL");
	mensajeEnviar(socketYama,mensaje->header.operacion,NULL,0);
	mensajeDestruir(mensaje);
	socketCerrar(sWorker);
}
void reduccionGlobal(Mensaje* m){
	Dir* nodo;
	memcpy(nodo,m->datos,DIRSIZE);
	int cantTemps=(m->header.tamanio-DIRSIZE-TEMPSIZE)/TEMPSIZE;
	int tamanio=(DIRSIZE+TEMPSIZE)*(cantTemps)+TEMPSIZE+INTSIZE+lenReduccion;
	void* buffer=malloc(tamanio);
	memcpy(buffer,&lenReduccion,INTSIZE);//script
	memcpy(buffer+INTSIZE,scriptReduccion,lenReduccion);

	memcpy(buffer+INTSIZE+lenReduccion,&cantTemps,INTSIZE);//origen
	memcpy(buffer+INTSIZE*2+lenReduccion,m->datos+DIRSIZE,tamanio-DIRSIZE);//y destino

	Socket sWorker=socketCrearCliente(nodo->ip,nodo->port,ID_MASTER);
	mensajeEnviar(sWorker,REDUCGLOBAL,buffer,tamanio);
	Mensaje* mensaje = mensajeRecibir(sWorker);
	imprimirMensaje(archivoLog,mensaje->header.operacion==EXITO?"[EJECUCION]REDUCCION GLOBAL EXISTOSA"
			:"[ERROR]FALLO EN LA RE_D_UCCION GLOBAL");
	mensajeEnviar(socketYama,mensaje->header.operacion,NULL,0);
	mensajeDestruir(mensaje);
	socketCerrar(sWorker);
}

void almacenadoFinal(Mensaje* m){
	Dir* nodo = malloc(DIRSIZE);
	char* temp = malloc(TEMPSIZE);
	memcpy(nodo, m->datos,DIRSIZE);
	memcpy(temp, m->datos + DIRSIZE, TEMPSIZE);

	Socket worker = socketCrearCliente(nodo->ip, nodo->port, ID_MASTER);

	void* data = malloc(DIRSIZE+TEMPSIZE);

	memcpy(data, nodo, DIRSIZE);
	memcpy(data + TEMPSIZE, temp,TEMPSIZE);

	mensajeEnviar(worker, ALMACENADO, data, DIRSIZE+TEMPSIZE);

	Mensaje* mens = mensajeRecibir(worker);
	if(mens->header.operacion == EXITO){
		imprimirMensaje(archivoLog,"[EJECUCION] ALMACENADO FINAL EXITOSO");
	}
	else{
		imprimirMensaje(archivoLog, "[ERROR] ALMACENADO FINAL FALLIDO");
	}

	mensajeEnviar(socketYama, mens->header.operacion, NULL, 0 );
	mensajeDestruir(mens);
	free(data);
	free(temp);
	free(nodo);

	socketCerrar(worker);

	estadoMaster = DESACTIVADO;

}





void iniciarMetricaJob(){
	getrusage(RUSAGE_SELF,&uso);//info del mismo proceso
	comienzo = uso.ru_utime;//tiempo que pasa en la cpu
}
void finMetricaJob(){
	getrusage(RUSAGE_SELF,&uso);
	fin = uso.ru_utime;
}
