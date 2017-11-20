/*
 ============================================================================
 Name        : YAMA.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso YAMA
 ============================================================================
 */

#include "YAMA.h"

//Para probar los ips y puertos ejecutar el FS, yama y algunos datanodes poner en la consola del fs "format" para que pase a un estado estable
//Deberian aparecer las ips y los puertos

int main(void) {
	yamaIniciar();
	yamaAtender();
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso YAMA finalizado");
	return EXIT_SUCCESS;
}

void yamaIniciar() {
	pantallaLimpiar();
	estadoYama=ACTIVADO;
	imprimirMensajeProceso("# PROCESO YAMA");
	archivoLog=archivoLogCrear(RUTA_LOG, "YAMA");
	configuracion=malloc(sizeof(Configuracion));
	configurar();
	void sighandler(){ //como no maneja variables locales no importa que se vaya de scope
		configuracion->reconfigurar=true;
	}
	signal(SIGUSR1,sighandler);

	servidor = malloc(sizeof(Servidor));
	imprimirMensaje2(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	servidor->fileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);
	Mensaje* mensaje = mensajeRecibir(servidor->fileSystem);
	if(mensaje->header.operacion == ACEPTACION)
		imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con File System");
	else {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El File System no se encuentra estable"BLANCO);
		exit(EXIT_FAILURE);
	}
	workers=list_create();
	tablaEstados=list_create();
	tablaUsados=list_create();
	masters=list_create();
}

void configurar(){
	char* campos[8] = {"IP_PROPIO","PUERTO_MASTER","IP_FILESYSTEM","PUERTO_FILESYSTEM","RETARDO_PLANIFICACION","ALGORITMO_BALANCEO","DISPONIBILIDAD_BASE"};
	ArchivoConfig archivoConfig = archivoConfigCrear(RUTA_CONFIG, campos);
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
	stringCopiar(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
	configuracion->disponibilidadBase = archivoConfigEnteroDe(archivoConfig, "DISPONIBILIDAD_BASE");
	configuracion->reconfigurar=false;
	archivoConfigDestruir(archivoConfig);
}

bool nodoIguales(Dir a,Dir b){
	return stringIguales(a.ip,b.ip)&&stringIguales(a.port,b.port);//podría comparar solo ip
}

void yamaAtender() {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);
	imprimirMensaje1(archivoLog, "[CONEXION] Esperando conexiones de un Master (Puerto %s)", configuracion->puertoMaster);
	servidor->listenerMaster = socketCrearListener(configuracion->puertoMaster);
	listaSocketsAgregar(servidor->listenerMaster, &servidor->listaMaster);
	listaSocketsAgregar(servidor->fileSystem,&servidor->listaMaster);
	void servidorControlarMaximoSocket(Socket unSocket) {
		if(unSocket>servidor->maximoSocket)
			servidor->maximoSocket = unSocket;
	}
	servidorControlarMaximoSocket(servidor->fileSystem);
	servidorControlarMaximoSocket(servidor->listenerMaster);

	while(estadoYama==ACTIVADO){
		if(configuracion->reconfigurar)
			configurar();

		dibujarTablaEstados();

		servidor->listaSelect = servidor->listaMaster;
		socketSelect(servidor->maximoSocket, &servidor->listaSelect);
		Socket socketI;
		Socket maximoSocket = servidor->maximoSocket;
		for(socketI = 0; socketI <= maximoSocket; socketI++){
			retardo();
			if (listaSocketsContiene(socketI, &servidor->listaSelect)){ //se recibio algo
				//podría disparar el thread aca
				if(socketI==servidor->listenerMaster){

					Socket nuevoSocket;
					nuevoSocket = socketAceptar(socketI, ID_MASTER);
					if(nuevoSocket != ERROR) {
						log_info(archivoLog, "[CONEXION] Proceso Master %d conectado exitosamente",nuevoSocket);
						listaSocketsAgregar(nuevoSocket, &servidor->listaMaster);
						servidorControlarMaximoSocket(nuevoSocket);
					}
				}else if(socketI==servidor->fileSystem){
					Mensaje* mensaje = mensajeRecibir(socketI);
					if(mensaje->header.operacion==ERROR_ARCHIVO){
						log_info(archivoLog,"[ERROR] El path no existe en el File System");
						mensajeEnviar(*(Entero*)mensaje->datos,ABORTAR,NULL,0);
//					}else if(mensaje->header.operacion==666){ //todo esto no debería existir? worker me tendría que ir pasando los errores
//						log_info(archivoLog,"[RECEPCION] Un nodo se desconeto");
//						char nodoDesconectado[20];
//						strncpy(nodoDesconectado,mensaje->datos,20);
//						bool nodoDesconectadoF(Worker* worker){
//							return stringIguales(worker->nodo.ip,nodoDesconectado);
//						}
//						((Worker*)list_find(workers,nodoDesconectadoF))->conectado=false;
//						void cazarEntradasDesconectadas(Entrada* entrada){
//							if(stringIguales(entrada->nodo.ip,nodoDesconectado)){
//								actualizarTablaEstados(entrada,FRACASO);
//							}
//						}
						//list_iterate(tablaEstados,cazarEntradasDesconectadas);
						//podría romper por estar recorriendo una lista con una funcion
						//que puede modificar la lista, pero no deberia
					}else if(mensaje->header.operacion==DESCONEXION){
						imprimirMensaje(archivoLog,"[ERROR] FileSystem desconectado");
						abort();
					}else{
						int32_t masterid;
						memcpy(&masterid,mensaje->datos,INTSIZE);
						log_info(archivoLog, "[RECEPCION] lista de bloques para master #%d recibida",masterid);
						printf("%d\n", mensaje->header.tamanio);
						if(listaSocketsContiene(masterid,&servidor->listaMaster)) //por si el master se desconecto
							yamaPlanificar(masterid,mensaje->datos+INTSIZE,mensaje->header.tamanio-INTSIZE);
					}
					mensajeDestruir(mensaje);
				}else{ //master
					Mensaje* mensaje = mensajeRecibir(socketI);
					if(mensaje->header.operacion==SOLICITUD){
						log_info(archivoLog,"[RECEPCION] solicitud de master");
						int32_t masterid = socketI; //para pasarlo a 32, por las dudas
						//el mensaje es el path del archivo
						//aca le acoplo el numero de master y se lo mando al fileSystem
						//lo de acoplar esta por si uso hilos, sino esta al pedo

						void* pasoFs=malloc(mensaje->header.tamanio+INTSIZE);
						memcpy(pasoFs,&masterid,INTSIZE);
						memcpy(pasoFs+INTSIZE,mensaje->datos,mensaje->header.tamanio);
						mensajeEnviar(servidor->fileSystem,ENVIAR_BLOQUES,pasoFs,mensaje->header.tamanio+INTSIZE);
						log_info(archivoLog, "[ENVIO] path %s de master #%d enviado al fileSystem",mensaje->datos,socketI);
						free(pasoFs);
					}else if(mensaje->header.operacion==DESCONEXION){
						log_info(archivoLog,"[RECEPCION] desconexion de master");
						listaSocketsEliminar(socketI, &servidor->listaMaster);
						socketCerrar(socketI);
						if(socketI==servidor->maximoSocket)
							servidor->maximoSocket--; //no debería romper nada
						log_info(archivoLog, "[CONEXION] Proceso Master %d se ha desconectado",socketI);
					}else{
						actualizarTablaEstados(*(int32_t*)mensaje->datos,mensaje->datos+INTSIZE,mensaje->header.operacion,socketI);
						}
					mensajeDestruir(mensaje);
				}
			}
		}
	}
}

void yamaPlanificar(Socket master, void* listaBloques,int tamanio){
	typedef struct __attribute__((__packed__)){
		Dir nodo;
		int32_t bloque;
	}Bloque;
	int BLOCKSIZE=sizeof(Bloque);
	int i;
	Lista bloques=list_create();
	Lista byteses=list_create();
	for(i=0;i<tamanio;i+=BLOCKSIZE*2+INTSIZE){
		list_add(bloques,listaBloques+i);
		list_add(bloques,listaBloques+i+sizeof(Bloque));
		list_add(byteses,listaBloques+i+sizeof(Bloque)*2);

		char ip[20];
		char puerto[20];
		int32_t bloque;
		memcpy(ip,listaBloques+i,20);
		memcpy(puerto,listaBloques+i+20,20);
		memcpy(&bloque,listaBloques+i+DIRSIZE,INTSIZE);
		log_info(archivoLog,"[REGISTRO] bloque: %s | %s | %d",ip,puerto,(int)bloque);
		memcpy(ip,listaBloques+i+BLOCKSIZE,20);
		memcpy(puerto,listaBloques+i+20+BLOCKSIZE,20);
		memcpy(&bloque,listaBloques+i+DIRSIZE+BLOCKSIZE,INTSIZE);
		log_info(archivoLog,"[REGISTRO] bloque2: %s | %s | %d",ip,puerto,(int)bloque);

		void registrar(Dir* nodo){
			bool noRegistrado(Worker* worker){
				return !stringIguales(worker->nodo.ip,nodo->ip)||!stringIguales(worker->nodo.port,nodo->port);
			}
			if(list_all_satisfy(workers,noRegistrado)){
				Worker worker;
				worker.conectado=true;
				worker.carga=0;
				worker.tareasRealizadas=0;
				worker.nodo=*nodo;
				list_addM(workers,&worker,sizeof(Worker));
				log_info(archivoLog,"[REGISTRO] nodo direccion %s, puerto %s",nodo->ip,nodo->port);
			}
		}
		registrar(listaBloques+i);
		registrar(listaBloques+i+BLOCKSIZE);
	}
	log_info(archivoLog,"[REGISTRO] %d bloques recibidos",bloques->elements_count);
	log_info(archivoLog,"[REGISTRO] %d bytes recibidos",byteses->elements_count);
	Lista tablaEstadosJob=list_create();
	job++;//mutex (supongo que las variables globales se comparten entre hilos)
	for(i=0;i<bloques->elements_count/2;i++){
		Entrada entrada;
		entrada.job=job;
		entrada.masterid=master;
		darPathTemporal(&entrada.pathTemporal,'t');
		list_addM(tablaEstadosJob,&entrada,sizeof(Entrada));
	}

	if(stringIguales(configuracion->algoritmoBalanceo,"Clock")){
		void setearDisponibilidad(Worker* worker){
			worker->disponibilidad=configuracion->disponibilidadBase;
		}
		list_iterate(workers,setearDisponibilidad);
	}else if(stringIguales(configuracion->algoritmoBalanceo,"W-Clock")){
		int cargaMaxima=0;
		void obtenerCargaMaxima(Worker* worker){
			if(worker->carga>cargaMaxima)
				cargaMaxima=worker->carga;
		}
		void setearDisponibilidad(Worker* worker){
			worker->disponibilidad=configuracion->disponibilidadBase
					+cargaMaxima-worker->carga;
		}
		list_iterate(workers,obtenerCargaMaxima);
		list_iterate(workers,setearDisponibilidad);
	}else{
		imprimirMensaje(archivoLog,"[] no se reconoce el algoritmo");
		abort();
	}

	int clock=0;
	{
		int mayorDisponibilidad=0;
		void setearClock(Worker* worker){
			if(worker->disponibilidad>mayorDisponibilidad){
				mayorDisponibilidad=worker->disponibilidad;
				clock=dirToNum(worker->nodo);
			}
		}
		list_iterate(workers,setearClock);
	}
	Worker* obtenerWorker(int* pos){
		Worker* worker=list_get(workers,*pos);
		if(worker->conectado)
			return worker;
		*pos=*pos+1%workers->elements_count;
		return obtenerWorker(pos);
	}
	Worker* workerClock=obtenerWorker(&clock);
	for(i=0;i<bloques->elements_count;i+=2){
		Bloque* bloque0 = list_get(bloques,i);
		Bloque* bloque1 = list_get(bloques,i+1);
		int* bytes=list_get(byteses,i/2);
		void asignarBloque(Worker* worker,Bloque* bloque,Bloque* alt){
			worker->carga++; //y habría que usar mutex aca
			log_info(archivoLog,"bloque %s %s %d asignado a worker %s %s",bloque->nodo.ip,bloque->nodo.port,bloque->bloque,worker->nodo.ip,worker->nodo.port);
			worker->disponibilidad--;
			worker->tareasRealizadas++;
			Entrada* entrada=list_get(tablaEstadosJob,i/2);
			entrada->nodo=worker->nodo;
			entrada->bloque=bloque->bloque;
			entrada->bytes=*bytes;
			entrada->nodoAlt=alt->nodo;
			entrada->bloqueAlt=alt->bloque;
			entrada->etapa=TRANSFORMACION;
			entrada->estado=ENPROCESO;

		}

		bool encontrado=false;
		if(nodoIguales(workerClock->nodo,bloque0->nodo)){
			asignarBloque(workerClock,bloque0,bloque1);
			encontrado=true;
		}else if(nodoIguales(workerClock->nodo,bloque1->nodo)){
			asignarBloque(workerClock,bloque1,bloque0);
			encontrado=true;
		}
		if(encontrado){
			clock=(clock+1)%workers->elements_count;
			Worker* workerTest=obtenerWorker(&clock);
			if(workerTest->disponibilidad==0)
				workerTest->disponibilidad=configuracion->disponibilidadBase;
			continue;
		}
		int clockAdv=clock;
		while(1){
			clockAdv=(clockAdv+1)%workers->elements_count;
			if(clockAdv==clock){
				void sumarDisponibilidadBase(Worker* worker){
					worker->disponibilidad+=configuracion->disponibilidadBase;
				}
				list_iterate(workers,sumarDisponibilidadBase);
			}
			Worker* workerAdv=obtenerWorker(&clockAdv);
			if(workerAdv->disponibilidad>0){
				if(nodoIguales(workerAdv->nodo,bloque0->nodo)){
					asignarBloque(workerAdv,bloque0,bloque1);
					break;
				}else if(nodoIguales(workerClock->nodo,bloque1->nodo)){
					asignarBloque(workerAdv,bloque1,bloque0);
					break;
				}
			}
		}
	}

	int tamanioEslabon=BLOCKSIZE+INTSIZE+TEMPSIZE;//dir,bloque,bytes,temp
	int32_t tamanioDato=tamanioEslabon*tablaEstadosJob->elements_count;
	void* dato=malloc(tamanioDato);
	for(i=0;i<tamanioDato;i+=tamanioEslabon){
		Entrada* entrada=list_get(tablaEstadosJob,i/tamanioEslabon);
		memcpy(dato+i,&entrada->nodo,DIRSIZE);
		memcpy(dato+i+DIRSIZE,&entrada->bloque,INTSIZE);
		memcpy(dato+i+DIRSIZE+INTSIZE,&entrada->bytes,INTSIZE);
		memcpy(dato+i+DIRSIZE+INTSIZE*2,entrada->pathTemporal,TEMPSIZE);
	}
	mensajeEnviar(master,TRANSFORMACION,dato,tamanioDato);
	free(dato);

	list_add_all(tablaEstados,tablaEstadosJob); //mutex
	list_destroy(tablaEstadosJob);
	log_info(archivoLog,"[] planificacion terminada");
}

void actualizarTablaEstados(int etapa,void* datos,int actualizando,Socket masterid){
	Entrada* entradaA;
	void registrarActualizacion(char* s){
		log_info(archivoLog,"[RECEPCION] actualizacion de %s, %s",s,actualizando==EXITO?"exito":"fracaso");
	}
	if(etapa==TRANSFORMACION){
		registrarActualizacion("transformacion");
		Dir nodo=*((Dir*)datos);
		int32_t bloque=*((int32_t*)(datos+DIRSIZE));
		bool buscarEntrada(Entrada* entrada){
			return nodoIguales(entrada->nodo,nodo)&&entrada->bloque==bloque;
		}
		entradaA=list_find(tablaEstados,buscarEntrada);
	}else if(etapa==REDUCLOCAL){
		registrarActualizacion("reduccion local");
		Dir nodo=*((Dir*)datos);
		bool buscarEntrada(Entrada* entrada){
			return nodoIguales(entrada->nodo,nodo)&&entrada->masterid==masterid;
		}
		entradaA=list_find(tablaEstados,buscarEntrada);
	}else{
		if(etapa==REDUCGLOBAL) registrarActualizacion("reduccion global");
		else registrarActualizacion("almacenado");
		bool buscarEntrada(Entrada* entrada){
			return entrada->masterid==masterid;
		}
		entradaA=list_find(tablaEstados,buscarEntrada);
	}

	void moverAUsados(bool(*cond)(void*)){
		//mutex
		Entrada* entrada;
		while((entrada=list_remove_by_condition(tablaEstados,cond))){
			list_add(tablaUsados,entrada);
		}
	}
	void darDatosEntrada(Entrada* entrada){
		entrada->nodo=entradaA->nodo;
		entrada->job=entradaA->job;
		entrada->masterid=entradaA->masterid;
		entrada->estado=ENPROCESO;
		entrada->bloque=-1;
	}
	entradaA->estado=actualizando;
	if(actualizando==FRACASO||actualizando==ABORTADO){//todo sacar ABORTADO?
		void abortarJob(){
			bool abortarEntrada(Entrada* entrada){
				if(entrada->job==entradaA->job){
					entrada->estado=ABORTADO;
					return true;
				}
				return false;
			}
			moverAUsados(abortarEntrada);
			mensajeEnviar(entradaA->masterid,ABORTAR,nullptr,0);
		}
		if(entradaA->etapa==TRANSFORMACION&&actualizando==FRACASO){
			if(nodoIguales(entradaA->nodo,entradaA->nodoAlt)){
				abortarJob();
				return;
			}
			Entrada alternativa;
			darDatosEntrada(&alternativa);
			alternativa.nodo=entradaA->nodoAlt;
			alternativa.bloque=entradaA->bloqueAlt;
			char dato[DIRSIZE+INTSIZE*2];
			memcpy(dato,&alternativa.nodo,DIRSIZE);
			memcpy(dato+DIRSIZE,&alternativa.bloque,INTSIZE);
			memcpy(dato+DIRSIZE+INTSIZE,&alternativa.bytes,INTSIZE);
			mensajeEnviar(alternativa.masterid,TRANSFORMACION,dato,sizeof dato);
			list_addM(tablaEstados,&alternativa,sizeof(Entrada));
			bool buscarError(Entrada* entrada){
				return entrada->estado==FRACASO;
			}
			list_add(tablaUsados,list_remove_by_condition(tablaEstados,buscarError));//mutex
		}else{
			abortarJob();
		}
		return;
	}
	bool trabajoTerminado(bool(*cond)(void*)){
		bool aux(Entrada* entrada){
			return cond(entrada)&&entrada->estado!=EXITO;
		}
		return !list_any_satisfy(tablaEstados,aux);
	}
	bool mismoJob(Entrada* entrada){
		return entrada->job==entradaA->job;
	}
	bool mismoNodoJob(Entrada* entrada){
		return mismoJob(entrada)&&nodoIguales(entrada->nodo,entradaA->nodo);
	}
	if(entradaA->etapa==TRANSFORMACION&&trabajoTerminado(mismoNodoJob)){
		log_info(archivoLog,"[REDUCLOCAL] creando entrada");
		Entrada reducLocal;
		darDatosEntrada(&reducLocal);
		darPathTemporal(&reducLocal.pathTemporal,'l');
		reducLocal.etapa=REDUCLOCAL;
		Lista nodos=list_filter(tablaEstados,mismoNodoJob);
		int tamanio=TEMPSIZE*(nodos->elements_count+1)+DIRSIZE;
		void* dato=malloc(tamanio);
		memcpy(dato,&reducLocal.nodo,DIRSIZE);
		int i,j;
		for(i=DIRSIZE,j=0;i<tamanio-TEMPSIZE;i+=TEMPSIZE,j++)
			memcpy(dato+i,((Entrada*)list_get(nodos,j))->pathTemporal,TEMPSIZE);
		memcpy(dato+i,reducLocal.pathTemporal,TEMPSIZE);
		mensajeEnviar(reducLocal.masterid,REDUCLOCAL,dato,tamanio);
		moverAUsados(mismoNodoJob);
		list_addM(tablaEstados,&reducLocal,sizeof(Entrada));//mutex
	}else if(entradaA->etapa==REDUCLOCAL&&trabajoTerminado(mismoJob)){
		Entrada reducGlobal;
		darDatosEntrada(&reducGlobal);
		darPathTemporal(&reducGlobal.pathTemporal,'g');
		reducGlobal.etapa=REDUCGLOBAL;
		Dir nodoMenorCarga=entradaA->nodo;
		int menorCargaI=1000; //
		void menorCarga(Worker* worker){
			if(worker->carga<menorCargaI)
				nodoMenorCarga=worker->nodo;
				menorCargaI=worker->carga;
		}
		list_iterate(workers,menorCarga);
		Lista nodosReducidos=list_filter(tablaEstados,mismoJob);
		int tamanio=(DIRSIZE+TEMPSIZE)*(nodosReducidos->elements_count+1);
		void* dato=malloc(tamanio);
		memcpy(dato,&nodoMenorCarga,DIRSIZE);
		int i,j;
		Dir nodoFalso={"0","0"};
		for(i=DIRSIZE,j=0;i<tamanio-TEMPSIZE;i+=DIRSIZE+TEMPSIZE,j++){
			Dir* nodoActual=&((Entrada*)list_get(nodosReducidos,j))->nodo;
			memcpy(dato+i,nodoIguales(*nodoActual,nodoMenorCarga)?nodoActual:&nodoFalso,DIRSIZE);
			memcpy(dato+i+DIRSIZE,((Entrada*)list_get(nodosReducidos,j))->pathTemporal,TEMPSIZE);
		}
		memcpy(dato+i,reducGlobal.pathTemporal,TEMPSIZE);

		mensajeEnviar(reducGlobal.masterid,REDUCGLOBAL,dato,tamanio);
		moverAUsados(mismoJob);
		list_addM(tablaEstados,&reducGlobal,sizeof(Entrada));//mutex
	}else if(entradaA->etapa==REDUCGLOBAL){
		//no le veo sentido a que yama participe del almacenado final
		//master lo podría hacer solo,  ya esta grande
		Entrada* reducGlobal=list_find(tablaEstados,mismoJob);
		Entrada almacenado;
		darDatosEntrada(&almacenado);
		almacenado.pathTemporal="final";
		//aca podría ponerle el nombre de archivo final, pero no lo tengo
		//y no lo necesito, no vale la pena meter la comunicacion y la
		//logica por algo que es estetico nomas
		almacenado.etapa=ALMACENADO;
		char dato[DIRSIZE+TEMPSIZE];
		memcpy(dato,&reducGlobal->nodo,DIRSIZE);
		memcpy(dato+DIRSIZE,reducGlobal->pathTemporal,TEMPSIZE);
		mensajeEnviar(entradaA->masterid,ALMACENADO,dato,sizeof dato);
		list_add(tablaUsados,list_remove_by_condition(tablaEstados,mismoJob));
		list_addM(tablaEstados,&almacenado,sizeof(Entrada));
	}else if(entradaA->etapa==ALMACENADO){
		list_add(tablaUsados,list_remove_by_condition(tablaEstados,mismoJob));
		mensajeEnviar(entradaA->masterid,CIERRE,nullptr,0);
	}
}

void dibujarTablaEstados(){
	if(list_is_empty(tablaEstados))
		return;
	pantallaLimpiar();
	puts("Job  Master  Nodo  Bloque  Etapa  Temporal  Estado\n");
	void dibujarEntrada(Entrada* entrada){
		char* etapa,*estado,*bloque; bool doFree=false;
		switch(entrada->etapa){
		case TRANSFORMACION: etapa="transformacion"; break;
		case REDUCLOCAL: etapa="reduccion local"; break;
		case REDUCGLOBAL: etapa="reduccion global";break;
		default: etapa="almacenado";
		}
		switch(entrada->estado){
		case EXITO: estado="terminado"; break;
		case FRACASO: estado="error"; break;
		default: estado="en proceso";
		}
		if(entrada->bloque==-1)
			bloque="-";
		else{
			bloque=string_itoa(entrada->bloque);
			doFree=true;
		}
		int masterToNum(int masterid){
			int index=-1,i=0;
			void buscarMaster(int* master){
				if(*master==masterid)
					index=i;
				i++;
			}
			list_iterate(masters,buscarMaster);
			if(index==-1){
				list_addM(masters,&masterid,sizeof(int));
				return i;
			}
			return index;
		}
		printf("%d   %d       %d     %s  %s  %s  %s\n",
				entrada->job,masterToNum(entrada->masterid),dirToNum(entrada->nodo),bloque,
				etapa,entrada->pathTemporal,estado);
		if(doFree)
			free(bloque);
	}
	list_iterate(tablaUsados,dibujarEntrada);
	list_iterate(tablaEstados,dibujarEntrada);
}

int dirToNum(Dir nodo){
	int index,i=0;
	void buscarIp(Worker* worker){
		if(nodoIguales(worker->nodo,nodo))
			index=i;
		i++;
	}
	list_iterate(workers,buscarIp);
	return index;
}

void darPathTemporal(char** ret,char pre){
	//mutex
	static char* anterior;
	static char agregado;
	char* temp=temporal_get_string_time();
	*ret=malloc(TEMPSIZE); //12
	int i,j=1;
	(*ret)[0]=pre;
	for(i=0;i<12;i++){
		if(temp[i]==':')
			continue;
		(*ret)[j]=temp[i];
		j++;
	}
	(*ret)[10]='0';
	(*ret)[11]='\0';
	char* anteriorTemp=string_duplicate(*ret);
	if(stringIguales(*ret,anterior))
		if(agregado=='9')
			agregado='a';
		else if(agregado=='z')
			agregado='A';
		else if(agregado=='Y')
			usleep(1000);
		else
			agregado++;
	else
		agregado='0';
	(*ret)[10]=agregado;
	free(anterior);
	anterior=anteriorTemp;
}

void retardo(){usleep(configuracion->retardoPlanificacion);}

//version mas eficiente, negada y olvidada
//int64_t darPathTemporal(int64_t ret){ //debería ser != 0
//	//mutex
//	static int64_t anterior;
//	static int agregado;
//	char* temp=temporal_get_string_time();
//	int i;
//	for(i=0;i<12;i++){
//		if(temp[i]==':')
//			continue;
//		ret=ret*10+temp[i]-'0';
//	}
//	int64_t anteriorTemp=ret;
//	if(anterior==ret) agregado++;
//	else agregado=0;
//	ret=ret*10+agregado;
//	anterior=anteriorTemp;
//	return ret;
//}
