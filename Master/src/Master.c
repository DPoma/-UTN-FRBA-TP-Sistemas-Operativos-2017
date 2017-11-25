/*
 * Master.c
 *
 *  Created on: 15/11/2017
 *      Author: utnso
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
	//senialAsignarFuncion(SIGINT, configuracionSenial);
	estadoMaster=ACTIVADO;
	void semaforoIniciar2(Semaforo** semaforo,int valor){
		*semaforo=malloc(sizeof(Semaforo));
		semaforoIniciar(*semaforo,valor);
	}
	semaforoIniciar2(&errorBloque,1);
	semaforoIniciar2(&recepcionAlternativo,0);
	semaforoIniciar2(&metricas.paralelos,1);
	semaforoIniciar2(&metricas.transformaciones,1);
	semaforoIniciar2(&metricas.reducLocales,1);

	void leerArchivo(File archScript,char** script,int* len){
		//todo validar
		fseek(archScript, 0, SEEK_END);
		long posicion = ftell(archScript);
		fseek(archScript, 0, SEEK_SET);
		*script=malloc(posicion + 1);
		fread(*script, posicion, 1, archScript);
		(*script)[posicion] = '\0';
		*len=strlen(*script)+1;
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
	}else if(mensaje->header.operacion==DESCONEXION){
		imprimirMensaje(archivoLog,"[ERROR] yama desconectado");
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
			list_add(listas,nodo);
			imprimirMensaje3(archivoLog,"] lista para nodo %s %s armada, lista #%d",bloque.dir.ip,bloque.dir.port,(int*)listas->elements_count);
		}
	}
	mensajeDestruir(mensaje);
	for(i=0;i<listas->elements_count;i++){
		pthread_t hilo;
		pthread_create(&hilo,NULL,(func)&transformaciones,list_get(listas,i));
	}
	while(estadoMaster==ACTIVADO){
		Mensaje* m=mensajeRecibir(socketYama);
		switch(m->header.operacion){
		case TRANSFORMACION://hubo error y se recibió un bloque alternativo
			metricas.fallos++;
			memcpy(&alternativo.dir,m->datos,DIRSIZE);
			memcpy(&alternativo.bloque,m->datos+DIRSIZE,INTSIZE);
			memcpy(&alternativo.bytes,m->datos+DIRSIZE+INTSIZE,INTSIZE);
			memcpy(&alternativo.temp,m->datos+DIRSIZE+INTSIZE*2,TEMPSIZE);
			semaforoSignal(recepcionAlternativo);
			mensajeDestruir(m);
			break;
		case REDUCLOCAL:{
			pthread_t hilo;
			pthread_create(&hilo,NULL,(func)&reduccionLocal,m);
		}break;
		case REDUCGLOBAL:{
			void list_obliterate(t_list* list){
				list_destroy_and_destroy_elements(list,free);
			}
			list_destroy_and_destroy_elements(listas,(func)list_obliterate);}
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
		printf("[METRICA]%s tardo %f segundos en ejecutarse\n",tarea,dt);
		log_info(archivoLog,"[METRICA]%s tardo %f segundos en ejecutarse",tarea,dt);
	}
	mostrarTranscurrido(metricas.proceso,"Job");
	mostrarTranscurrido(metricas.transformacionSum/metricas.cantTrans,"Transformacion promedio ");
	mostrarTranscurrido(metricas.reducLocalSum/metricas.cantRedLoc,"Reduccion local promedio");
	imprimirMensaje2(archivoLog,"[METRICA]Transformaciones: %d,Reducciones locales:%d",(void*)metricas.cantTrans,(void*)metricas.cantRedLoc);
	mostrarTranscurrido(metricas.reducGlobal,"Reducccion Global");
	mostrarTranscurrido(metricas.almacenado,"Almacenado");
	imprimirMensaje1(archivoLog,"[METRICA]Cantidad de fallos: %d",(void*)metricas.fallos);
	imprimirMensaje1(archivoLog,"[METRICA]Tareas realizadas en paralelo: %d",(void*)metricas.maxParalelo);
}
void transformaciones(Lista bloques){
	clock_t tiempo=clock();
	t_queue* clocks=queue_create();
	WorkerTransformacion* dir = list_get(bloques,0);
	imprimirMensaje2(archivoLog,"[EJECUCION] comenzando transformacion de nodo %s %s",&dir->dir.ip,&dir->dir.port);
	socketWorker=socketCrearCliente(dir->dir.ip,dir->dir.port,ID_MASTER);
	imprimirMensaje2(archivoLog,"[CONEXION] Estableciendo conexion con Worker (IP: %s | PUERTO: %s",dir->dir.ip,dir->dir.port);
	mensajeEnviar(socketWorker,TRANSFORMACION,scriptTransformacion,lenTransformacion);
	int enviados=0,respondidos=0;
	semaforoWait(metricas.transformaciones);
	metricas.transformacionSum+=transcurrido(tiempo);
	semaforoSignal(metricas.transformaciones);
	do{
		for(;enviados<bloques->elements_count;enviados++){
			tareasEnParalelo(1);
			clock_t* inicio=malloc(sizeof(clock_t));
			*inicio=clock();
			queue_push(clocks,inicio);

			WorkerTransformacion* wt=list_get(bloques,enviados);
			int tamanio=INTSIZE*2+TEMPSIZE;
			char data[tamanio];
			memcpy(data,&wt->bloque,INTSIZE);
			memcpy(data+INTSIZE,&wt->bytes,INTSIZE);
			memcpy(data+INTSIZE*2,wt->temp,TEMPSIZE);
			mensajeEnviar(socketWorker,TRANSFORMACION,data,tamanio);
			imprimirMensaje2(archivoLog,"[CONEXION] Enviando bloque %d %s",(int*)wt->bloque,wt->temp);
		}
		for(;respondidos<enviados;respondidos++){
			Mensaje* mensaje = mensajeRecibir(socketWorker);
			if(mensaje->header.operacion==DESCONEXION){
				puts("UR DONE");
				mensajeEnviar(socketYama,DESCONEXION_NODO,&dir->dir,DIRSIZE);
				pthread_detach(pthread_self());
				return;
			}
			//a demas de decir exito o fracaso devuelve el numero de bloque
			void enviarActualizacion(){
				char buffer[INTSIZE*2+DIRSIZE];
				int32_t op=TRANSFORMACION;
				memcpy(buffer,&op,INTSIZE);
				memcpy(buffer+INTSIZE,&dir->dir,DIRSIZE);
				memcpy(buffer+INTSIZE+DIRSIZE,mensaje->datos,INTSIZE);
				mensajeEnviar(socketYama,mensaje->header.operacion,buffer,sizeof buffer);
				mensajeDestruir(mensaje);
				tareasEnParalelo(-1);
			}
			if(mensaje->header.operacion==EXITO){
				imprimirMensaje1(archivoLog, "[TRANSFORMACION] Transformacion realizada con exito en el Worker %s",dir->dir.ip);
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
			clock_t* reloj=(clock_t*)queue_pop(clocks);//si llegan desordenados no importa, la suma da lo mismo
			double dt=transcurrido(*reloj);
			free(reloj);
			semaforoWait(metricas.transformaciones);
			metricas.transformacionSum+=dt;
			metricas.cantTrans++;
			semaforoSignal(metricas.transformaciones);
		}

	}while(enviados<bloques->elements_count);
	mensajeEnviar(socketWorker, EXITO, NULL, 0);
	socketCerrar(socketWorker);
	queue_destroy(clocks);
	imprimirMensaje2(archivoLog,"[EJECUCION] transformacion terminada de nodo %s %s",dir->dir.ip,dir->dir.port);
	pthread_detach(pthread_self());
}
void reduccionLocal(Mensaje* m){
	tareasEnParalelo(1);
	clock_t tiempo=clock();
	Dir nodo;
	memcpy(&nodo,m->datos,DIRSIZE);
	imprimirMensaje2(archivoLog,"[EJECUCION] comenzando reduccion local de nodo %s %s",nodo.ip,nodo.port);
	int32_t cantTemps=(m->header.tamanio-DIRSIZE-TEMPSIZE)/TEMPSIZE;
	int tamanio=cantTemps*TEMPSIZE+TEMPSIZE+lenReduccion+INTSIZE*2;
	void* buffer=malloc(tamanio);
	memcpy(buffer,&lenReduccion,INTSIZE);//script
	memcpy(buffer+INTSIZE,scriptReduccion,lenReduccion);

	memcpy(buffer+INTSIZE+lenReduccion,&cantTemps,INTSIZE);//origen
	memcpy(buffer+INTSIZE*2+lenReduccion,m->datos+DIRSIZE,cantTemps*TEMPSIZE);

	memcpy(buffer+INTSIZE*2+lenReduccion+cantTemps*TEMPSIZE,m->datos+DIRSIZE+cantTemps*TEMPSIZE,TEMPSIZE);//destino

	Socket sWorker=socketCrearCliente(nodo.ip,nodo.port,ID_MASTER);
	mensajeEnviar(sWorker,REDUCLOCAL,buffer,tamanio);
	mensajeDestruir(m);free(buffer);

	Mensaje* mensaje = mensajeRecibir(sWorker);
	if(mensaje->header.operacion==DESCONEXION){
		imprimirMensaje(archivoLog,"[ERROR] Worker desconectado");
		mensaje->header.operacion=FRACASO;
	}
	imprimirMensaje(archivoLog,mensaje->header.operacion==EXITO?"[EJECUCION] reduccion local exitosa"
			:"[ERROR] fallo en la reduccion local");
	char bufferY[INTSIZE+DIRSIZE];
	int32_t op=REDUCLOCAL;
	memcpy(bufferY,&op,INTSIZE);
	memcpy(bufferY+INTSIZE,&nodo,DIRSIZE);
	mensajeEnviar(socketYama,mensaje->header.operacion,bufferY,sizeof bufferY);
	mensajeDestruir(mensaje);
	socketCerrar(sWorker);
	tareasEnParalelo(-1);

	semaforoWait(metricas.reducLocales);
	metricas.reducLocalSum+=transcurrido(tiempo);
	metricas.cantRedLoc++;
	semaforoSignal(metricas.reducLocales);
	imprimirMensaje2(archivoLog,"[EJECUCION] terminando reduccion local de nodo %s %s",nodo.ip,nodo.port);
	pthread_detach(pthread_self());
}
void reduccionGlobal(Mensaje* m){
	imprimirMensaje(archivoLog,"[EJECUCION] comenzando reduccion global");
	clock_t tiempo=clock();
	Dir nodo;
	memcpy(&nodo,m->datos,DIRSIZE);
	int cantDuplas=(m->header.tamanio-DIRSIZE-TEMPSIZE)/(TEMPSIZE+DIRSIZE);
	int tamanio=(DIRSIZE+TEMPSIZE)*cantDuplas+TEMPSIZE+INTSIZE*2+lenReduccion;
	void* buffer=malloc(tamanio);
	memcpy(buffer,&lenReduccion,INTSIZE);//script
	memcpy(buffer+INTSIZE,scriptReduccion,lenReduccion);

	memcpy(buffer+INTSIZE+lenReduccion,&cantDuplas,INTSIZE);//origen
	memcpy(buffer+INTSIZE*2+lenReduccion,m->datos+DIRSIZE,m->header.tamanio-DIRSIZE);//y destino

	Socket sWorker=socketCrearCliente(nodo.ip,nodo.port,ID_MASTER);
	mensajeEnviar(sWorker,REDUCGLOBAL,buffer,tamanio);
	mensajeDestruir(m);free(buffer);

	Mensaje* mensaje = mensajeRecibir(sWorker);
	if(mensaje->header.operacion==DESCONEXION){
		imprimirMensaje(archivoLog,"[ERROR] Worker desconectado");
		mensaje->header.operacion=FRACASO;
	}
	imprimirMensaje(archivoLog,mensaje->header.operacion==EXITO?"[EJECUCION] reduccion global exitosa"
			:"[ERROR] reduccion global fallida");
	int32_t op=REDUCGLOBAL;
	tamanio=INTSIZE+stringLongitud(archivoSalida)+1;
	buffer=malloc(tamanio);
	memcpy(buffer,&op,INTSIZE);
	memcpy(buffer+INTSIZE,archivoSalida,stringLongitud(archivoSalida)+1);
	mensajeEnviar(socketYama,mensaje->header.operacion,buffer,tamanio);
	free(buffer);
	mensajeDestruir(mensaje);
	socketCerrar(sWorker);
	metricas.reducGlobal=transcurrido(tiempo);
	imprimirMensaje(archivoLog,"[EJECUCION] reduccion global terminada");
}
void almacenado(Mensaje* m){
	imprimirMensaje(archivoLog,"[EJECUCION] comenzando almacenado");
	clock_t tiempo=clock();
	Dir nodo;
	memcpy(&nodo,m->datos,DIRSIZE);
	Socket sWorker=socketCrearCliente(nodo.ip,nodo.port,ID_MASTER);

	int32_t tamanio=TEMPSIZE+stringLongitud(archivoSalida)+1;
	void* buffer=malloc(tamanio);
	memcpy(buffer,m->datos+DIRSIZE,TEMPSIZE);
	memcpy(buffer+TEMPSIZE,archivoSalida,tamanio-TEMPSIZE);

	mensajeEnviar(sWorker,ALMACENADO,buffer,tamanio);
	mensajeDestruir(m);free(buffer);

	Mensaje* mens=mensajeRecibir(sWorker);
	if(mens->header.operacion==DESCONEXION){
		imprimirMensaje(archivoLog,"[ERROR] Worker desconectado");
		mens->header.operacion=FRACASO;
	}
	imprimirMensaje(archivoLog,mens->header.operacion==EXITO?"[EJECUCION] ALMACENADO EXITOSO":"[ERROR] ALMACENADO FALLIDO");

	int32_t op=ALMACENADO;
	mensajeEnviar(socketYama,mens->header.operacion,&op,INTSIZE);
	mensajeDestruir(mens);
	socketCerrar(sWorker);
	metricas.almacenado=transcurrido(tiempo);
	imprimirMensaje(archivoLog,"[EJECUCION] almacenado terminado");
}
void tareasEnParalelo(int dtp){
	semaforoWait(metricas.paralelos);
	metricas.paralelo+=dtp;
	if(metricas.paralelo>metricas.maxParalelo)
		metricas.maxParalelo=metricas.paralelo;
	semaforoSignal(metricas.paralelos);
}
double transcurrido(clock_t tiempo){
	return ((double)(clock()-tiempo)/CLOCKS_PER_SEC);
}
