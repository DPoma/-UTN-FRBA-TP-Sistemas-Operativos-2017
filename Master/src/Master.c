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
	metricas.procesoC=clock();
	metricas.maxParalelo=metricas.paralelo=metricas.fallos=metricas.cantRedLoc=metricas.cantTrans=0;
	pantallaLimpiar();
	archivoLog = archivoLogCrear(RUTA_LOG, "Master");
	imprimirMensajeProceso("# PROCESO MASTER");
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
	Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig){
		Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
		stringCopiar(configuracion->ipYama, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
		stringCopiar(configuracion->puertoYama, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
		archivoConfigDestruir(archivoConfig);
		return configuracion;
	}
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	imprimirMensaje2(archivoLog, "[CONEXION] Configuracion para conexion con YAMA (IP: %s | Puerto: %s)", configuracion->ipYama, configuracion->puertoYama);
	void configuracionSenial(int senial){
		estadoMaster=DESACTIVADO;
	}
	senialAsignarFuncion(SIGINT, configuracionSenial);
	estadoMaster=ACTIVADO;
	void semaforoIniciar2(Semaforo* semaforo,int valor){
		semaforo=malloc(sizeof(Semaforo));
		semaforoIniciar(semaforo,valor);
	}
	semaforoIniciar2(errorBloque,1);
	semaforoIniciar2(recepcionAlternativo,0);
	semaforoIniciar2(metricas.paralelos,1);
	semaforoIniciar2(metricas.transformaciones,1);
	semaforoIniciar2(metricas.reducLocales,1);

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
	archivoSalida=argv[4];

	imprimirMensaje2(archivoLog,"[CONEXION] Estableciendo Conexion con YAMA...", configuracion->ipYama, configuracion->puertoYama);
	socketYama = socketCrearCliente(configuracion->ipYama, configuracion->puertoYama, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion establecida con YAMA, enviando solicitud");
	mensajeEnviar(socketYama,SOLICITUD,argv[3],stringLongitud(argv[3])+1);
}
void masterAtender(){
	Mensaje* mensaje=mensajeRecibir(socketYama);
	if(mensaje->header.operacion==ABORTAR){
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
		metricas.cantTrans++;
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
	mensajeDestruir(mensaje);
	for(i=0;i<listas->elements_count;i++){
		pthread_t hilo;
		pthread_create(&hilo,NULL,&transformaciones,list_get(listas,i));
	}
	while(estadoMaster==ACTIVADO){
		Mensaje* m=mensajeRecibir(socketYama);
		switch(m->header.operacion){
		case TRANSFORMACION://hubo error y se recibió un bloque alternativo
			metricas.fallos++;
			memcpy(&alternativo.bloque,mensaje->datos+i+DIRSIZE,INTSIZE);
			memcpy(&alternativo.bytes,mensaje->datos+i+DIRSIZE+INTSIZE,INTSIZE);
			memcpy(&alternativo.temp,mensaje->datos+i+DIRSIZE+INTSIZE*2,TEMPSIZE);
			semaforoSignal(recepcionAlternativo);
			mensajeDestruir(m);
			break;
		case REDUCLOCAL:{
			pthread_t hilo;
			pthread_create(&hilo,NULL,&reduccionLocal,m);
		}break;
		case REDUCGLOBAL:
			reduccionGlobal(m);
			break;
		case ALMACENADO:
			almacenado(m);
			break;
		case CIERRE:
			estadoMaster=DESACTIVADO;
			mensajeDestruir(m);
			break;
		case ABORTAR:
			imprimirMensaje(archivoLog,"[ABORTO] Abortando proceso");
			abort();
		}
	}
	metricas.proceso=transcurrido(metricas.procesoC);
	imprimirMensaje(archivoLog,"[EJECUCION]Terminando proceso");
	void mostrarTranscurrido(double dt,char* tarea){
		imprimirMensaje2(archivoLog,"[METRICA]%s tardo %f segundos en ejecutarse",tarea,&dt);
	}
	mostrarTranscurrido(metricas.proceso,"Job");
	mostrarTranscurrido(metricas.transformacionSum/metricas.cantTrans,"Transformacion promedio ");
	mostrarTranscurrido(metricas.reducLocalSum/metricas.cantRedLoc,"Reduccion local promedio");
	imprimirMensaje2(archivoLog,"[METRICA]Transformaciones: %d,Reducciones locales:%d",&metricas.cantTrans,&metricas.cantRedLoc);
	mostrarTranscurrido(metricas.reducGlobal,"Reducccion Global");
	mostrarTranscurrido(metricas.almacenado,"Almacenado");
	imprimirMensaje1(archivoLog,"[METRICA]Cantidad de fallos: %d",&metricas.fallos);
	imprimirMensaje1(archivoLog,"[METRICA]Tareas realizadas en paralelo: %d",&metricas.maxParalelo);
}
void transformaciones(Lista bloques){
	tareasEnParalelo(1);
	clock_t tiempo=clock();
	t_queue* clocks=queue_create();
	WorkerTransformacion* dir = list_get(bloques,0);
	socketWorker=socketCrearCliente(dir->dir.ip,dir->dir.port,ID_MASTER);
	imprimirMensaje2(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %s | PUERTO: %s",dir->dir.ip,dir->dir.port);
	mensajeEnviar(socketWorker,TRANSFORMACION,scriptTransformacion,lenTransformacion);
	int enviados=0,respondidos=0;
	semaforoWait(metricas.transformaciones);
	metricas.transformacionSum+=transcurrido(tiempo);
	semaforoSignal(metricas.transformaciones);
	do{
		for(;enviados<bloques->elements_count;enviados++){
			clock_t* inicio=malloc(sizeof(clock_t));
			*inicio=clock();
			queue_push(clocks,inicio);

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
			double dt=transcurrido(*(clock_t*)queue_pop(clocks));//si llegan desordenados no importa, la suma da lo mismo
			semaforoWait(metricas.transformaciones);
			metricas.transformacionSum+=dt;
			metricas.cantTrans++;
			semaforoSignal(metricas.transformaciones);
		}

	}while(enviados<bloques->elements_count);
	mensajeEnviar(socketWorker, EXITO, NULL, 0);
	socketCerrar(socketWorker);
	tareasEnParalelo(-1);
}
void reduccionLocal(Mensaje* m){
	tareasEnParalelo(1);
	clock_t tiempo=clock();
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
	mensajeDestruir(m);free(buffer);

	Mensaje* mensaje = mensajeRecibir(sWorker);
	imprimirMensaje(archivoLog,mensaje->header.operacion==EXITO?"[EJECUCION]REDUCCION LOCAL EXISTOSA"
			:"[ERROR]FALLO EN LA RE_D_UCCION LOCAL");
	mensajeEnviar(socketYama,mensaje->header.operacion,NULL,0);
	mensajeDestruir(mensaje);
	socketCerrar(sWorker);
	tareasEnParalelo(-1);

	semaforoWait(metricas.reducLocales);
	metricas.reducLocalSum+=transcurrido(tiempo);
	metricas.cantRedLoc++;
	semaforoSignal(metricas.reducLocales);
}
void reduccionGlobal(Mensaje* m){
	clock_t tiempo=clock();
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
	mensajeDestruir(m);free(buffer);

	Mensaje* mensaje = mensajeRecibir(sWorker);
	imprimirMensaje(archivoLog,mensaje->header.operacion==EXITO?"[EJECUCION]REDUCCION GLOBAL EXISTOSA"
			:"[ERROR]FALLO EN LA RE_D_UCCION GLOBAL");
	mensajeEnviar(socketYama,mensaje->header.operacion,NULL,0);
	mensajeDestruir(mensaje);
	socketCerrar(sWorker);
	metricas.reducGlobal=transcurrido(tiempo);
}
void almacenado(Mensaje* m){
	clock_t tiempo=clock();
	Dir nodo;
	memcpy(&nodo,m->datos,DIRSIZE);
	Socket sWorker=socketCrearCliente(nodo.ip,nodo.port,ID_MASTER);

	int32_t tamanio=TEMPSIZE+stringLongitud(archivoSalida)+1;
	void* buffer=malloc(tamanio);
	memcpy(buffer,m->datos+DIRSIZE,TEMPSIZE);
	memcpy(buffer+TEMPSIZE,archivoSalida,tamanio-TEMPSIZE);

	mensajeEnviar(sWorker,ALMACENADO,buffer,tamanio);
	free(buffer);

	Mensaje* mens=mensajeRecibir(sWorker);
	imprimirMensaje(archivoLog,mens->header.operacion==EXITO?"[EJECUCION] ALMACENADO FINAL EXITOSO":"[ERROR] ALMACENADO FINAL FALLIDO");

	mensajeEnviar(socketYama,mens->header.operacion, NULL,0);
	mensajeDestruir(mens);
	socketCerrar(sWorker);
	metricas.almacenado=transcurrido(tiempo);
}
void tareasEnParalelo(int dtp){
	semaforoWait(metricas.paralelos);
	metricas.paralelo+=dtp;
	if(metricas.paralelo>metricas.maxParalelo)
		metricas.maxParalelo=metricas.paralelo;
	semaforoSignal(metricas.paralelos);
}
double transcurrido(clock_t tiempo){
	//internet dice que en algunos linux esta resta no anda, ver que pasa
	return ((double)(clock()-tiempo)/CLOCKS_PER_SEC);
}
