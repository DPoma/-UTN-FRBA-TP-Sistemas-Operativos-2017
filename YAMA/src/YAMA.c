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

int main(void) {
	yamaIniciar();
	yamaAtender();
	return EXIT_FAILURE;//nunca se va a leer
}

void yamaIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO YAMA");
	configuracion=malloc(sizeof(Configuracion));
	configurar();
	archivoLog=archivoLogCrear(configuracion->rutaLog, "YAMA");

	void sigreconfig(){
		configuracion->reconfigurar=true;
	}
	signal(SIGUSR1,sigreconfig);
	void  sigsalir(){
		puts("");
		imprimirMensaje(archivoLog, "[EJECUCION] Proceso YAMA finalizado");
		exit(EXIT_SUCCESS);
	}
	signal(SIGINT,sigsalir);

	flagUsados = false;
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
	mensajeDestruir(mensaje);
	workers=list_create();
	tablaEstados=list_create();
	tablaUsados=list_create();
	masters=list_create();
}

void configurar(){
	char* campos[8] = {"IP_PROPIA","PUERTO_MASTER","IP_FILESYSTEM","PUERTO_FILESYSTEM","RETARDO_PLANIFICACION","ALGORITMO_BALANCEO","DISPONIBILIDAD_BASE", "RUTA_LOG"};
	ArchivoConfig archivoConfig = archivoConfigCrear(RUTA_CONFIG, campos);
	stringCopiar(configuracion->ipPropia,archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
	stringCopiar(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
	configuracion->disponibilidadBase = archivoConfigEnteroDe(archivoConfig, "DISPONIBILIDAD_BASE");
	stringCopiar(configuracion->rutaLog, archivoConfigStringDe(archivoConfig, "RUTA_LOG"));
	if(!stringIguales(configuracion->algoritmoBalanceo,"Clock")&&!stringIguales(configuracion->algoritmoBalanceo,"W-Clock")){
		imprimirMensaje(archivoLog,"[ERRIR] no se reconoce el algoritmo");
		exit(EXIT_FAILURE);
	}
	printf("[CONFIGURACION] Algoritmo de planificacion: %s\n",configuracion->algoritmoBalanceo);
	printf("[CONFIGURACION] Retardo de planificacion: %d\n",configuracion->retardoPlanificacion);
	printf("[CONFIGURACION] Disponibilidad base: %d\n",configuracion->disponibilidadBase);
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
	servidor->listenerMaster = socketCrearListener(configuracion->ipPropia, configuracion->puertoMaster);
	listaSocketsAgregar(servidor->listenerMaster, &servidor->listaMaster);
	listaSocketsAgregar(servidor->fileSystem,&servidor->listaMaster);
	void servidorControlarMaximoSocket(Socket unSocket) {
		if(unSocket>servidor->maximoSocket)
			servidor->maximoSocket = unSocket;
	}
	servidorControlarMaximoSocket(servidor->fileSystem);
	servidorControlarMaximoSocket(servidor->listenerMaster);

	while(true){
		if(configuracion->reconfigurar)
			configurar();

		dibujarTablaEstados();

		servidor->listaSelect = servidor->listaMaster;
		socketSelect(servidor->maximoSocket, &servidor->listaSelect,0);
		Socket socketI;
		Socket maximoSocket = servidor->maximoSocket;
		for(socketI = 0; socketI <= maximoSocket; socketI++){
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
					}else if(mensaje->header.operacion==DESCONEXION){
						imprimirMensaje(archivoLog,"[ERROR] FileSystem desconectado");
						exit(EXIT_FAILURE);
					}else{
						int32_t masterid;
						memcpy(&masterid,mensaje->datos,INTSIZE);
						log_info(archivoLog, "[RECEPCION] lista de bloques para master #%d recibida",masterid);
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
						log_info(archivoLog, "[CONEXION] Proceso Master %d se ha desconectado",socketI);
						int jobDesconexion=-1;
						bool entradasDesconectadas(Entrada* entrada){
							if(entrada->masterid==socketI){
								jobDesconexion=entrada->job;
								return true;
							}
							return false;
						}
						moverAUsados((func)entradasDesconectadas);
						if(jobDesconexion!=-1){
							void cazarEntradasDesconectadas(Entrada* entrada){
								if(entrada->job==jobDesconexion){
									entrada->estado=ABORTADO;
								}
							}
							list_iterate(tablaUsados,(func)cazarEntradasDesconectadas);
							liberarCargas(jobDesconexion);
						}
						log_info(archivoLog,"[EJECUCION] trabajo terminado");

						listaSocketsEliminar(socketI, &servidor->listaMaster);
						socketCerrar(socketI);
						if(socketI==servidor->maximoSocket)
							servidor->maximoSocket--; //no debería romper nada
					}else{
						actualizarTablaEstados(mensaje,socketI);
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
			if(list_all_satisfy(workers,(func)noRegistrado)){
				Worker worker;
				worker.carga=0;
				worker.tareasRealizadas=0;
				worker.nodo=*nodo;
				worker.cargas=list_create();
				list_addM(workers,&worker,sizeof(Worker));
				log_info(archivoLog,"[REGISTRO] nodo direccion %s, puerto %s",nodo->ip,nodo->port);
			}
		}
		registrar(listaBloques+i);
		registrar(listaBloques+i+BLOCKSIZE);
	}
	log_info(archivoLog,"[REGISTRO] %d bloques recibidos",bloques->elements_count);
	log_info(archivoLog,"[REGISTRO] %d bytes recibidos",byteses->elements_count);

	usleep(configuracion->retardoPlanificacion);

	Lista tablaEstadosJob=list_create();
	job++;//mutex (supongo que las variables globales se comparten entre hilos)
	for(i=0;i<bloques->elements_count/2;i++){
		Entrada entrada;
		entrada.job=job;
		entrada.masterid=master;
		darPathTemporal(&entrada.pathTemporal,'t');
		list_addM(tablaEstadosJob,&entrada,sizeof(Entrada));
	}
	void agregarCargaJob(Worker* worker){
		CargaJob carga={job,0};
		list_addM(worker->cargas,&carga,sizeof(CargaJob));
	}
	list_iterate(workers,(func)agregarCargaJob);

	if(stringIguales(configuracion->algoritmoBalanceo,"Clock")){
		void setearDisponibilidad(Worker* worker){
			worker->disponibilidad=configuracion->disponibilidadBase;
		}
		list_iterate(workers,(func)setearDisponibilidad);
	}else{
		int cargaMaxima=0;
		void obtenerCargaMaxima(Worker* worker){
			if(worker->carga>cargaMaxima)
				cargaMaxima=worker->carga;
		}
		void setearDisponibilidad(Worker* worker){
			worker->disponibilidad=configuracion->disponibilidadBase+cargaMaxima-worker->carga;
		}
		list_iterate(workers,(func)obtenerCargaMaxima);
		list_iterate(workers,(func)setearDisponibilidad);
	}

	int clock=0;
	{
		Worker* aux=list_get(workers,0);
		void setearClock(Worker* worker){
			if(worker->disponibilidad>aux->disponibilidad||(worker->disponibilidad==aux->disponibilidad&&worker->tareasRealizadas<aux->tareasRealizadas)){
				aux=worker;
				int i=0;
				void numerarWorker(Worker* workerI){
					if(nodoIguales(workerI->nodo,aux->nodo))
						clock=i;
					i++;
				}
				list_iterate(workers,(func)numerarWorker);
			}
		}
		list_iterate(workers,(func)setearClock);
	}
	for(i=0;i<bloques->elements_count;i+=2){
		Worker* workerClock;
		Bloque* bloque0 = list_get(bloques,i);
		Bloque* bloque1 = list_get(bloques,i+1);
		int* bytes=list_get(byteses,i/2);
		void asignarBloque(Worker* worker,Bloque* bloque,Bloque* alt){
			aumentarCarga(worker,job,1);
			log_info(archivoLog,"bloque %s %s %d asignado a worker %s %s",bloque->nodo.ip,bloque->nodo.port,bloque->bloque,worker->nodo.ip,worker->nodo.port);
			worker->disponibilidad--;
			Entrada* entrada=list_get(tablaEstadosJob,i/2);
			entrada->nodo=worker->nodo;
			entrada->bloque=bloque->bloque;
			entrada->bytes=*bytes;
			entrada->nodoAlt=alt->nodo;
			entrada->bloqueAlt=alt->bloque;
			entrada->etapa=TRANSFORMACION;
			entrada->estado=ENPROCESO;
		}
		compararBloque:
		workerClock=list_get(workers,clock);
		if(nodoIguales(workerClock->nodo,bloque0->nodo)||nodoIguales(workerClock->nodo,bloque1->nodo)){
			if(workerClock->disponibilidad>0){
				clock=(clock+1)%workers->elements_count;
				if(nodoIguales(workerClock->nodo,bloque0->nodo)){
					asignarBloque(workerClock,bloque0,bloque1);
					continue;
				}
				if(nodoIguales(workerClock->nodo,bloque1->nodo)){
					asignarBloque(workerClock,bloque1,bloque0);
					continue;
				}
			}else{
				workerClock->disponibilidad=configuracion->disponibilidadBase;
				clock=(clock+1)%workers->elements_count;
				goto compararBloque;
			}
		}
		int clockAdv=clock;
		while(1){
			clockAdv=(clockAdv+1)%workers->elements_count;
			if(clockAdv==clock){
				void sumarDisponibilidadBase(Worker* worker){
					worker->disponibilidad+=configuracion->disponibilidadBase;
				}
				list_iterate(workers,(func)sumarDisponibilidadBase);
			}
			Worker* workerAdv=list_get(workers,clockAdv);
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
	int32_t tamanioDato=tamanioEslabon*tablaEstadosJob->elements_count+INTSIZE;
	void* dato=stringCrear(tamanioDato);
	memcpy(dato,&job,INTSIZE);
	int j;
	for(i=INTSIZE,j=0;i<tamanioDato;i+=tamanioEslabon,j++){
		Entrada* entrada=list_get(tablaEstadosJob,j);
		memcpy(dato+i,&entrada->nodo,DIRSIZE);
		memcpy(dato+i+DIRSIZE,&entrada->bloque,INTSIZE);
		memcpy(dato+i+DIRSIZE+INTSIZE,&entrada->bytes,INTSIZE);
		memcpy(dato+i+DIRSIZE+INTSIZE*2,entrada->pathTemporal,TEMPSIZE);
	}
	mensajeEnviar(master,TRANSFORMACION,dato,tamanioDato);
	free(dato);

	list_add_all(tablaEstados,tablaEstadosJob); //mutex
	list_destroy(tablaEstadosJob);list_destroy(bloques);list_destroy(byteses);
	log_info(archivoLog,"[] planificacion terminada");
}

void actualizarTablaEstados(Mensaje* mensaje,Socket masterid){
	Entrada* entradaA;
	log_info(archivoLog,"OPERACION: %d",mensaje->header.operacion);
	if(mensaje->header.operacion==DESCONEXION_NODO ){
		Dir* nodo=(Dir*)mensaje->datos;
		int jobA=*((int32_t*)mensaje->datos+DIRSIZE);
		log_info(archivoLog,"[CONEXION] Nodo %s desconectado %d",nodo->nombre,jobA);
		bool buscarEntrada(Entrada* entrada){
			return nodoIguales(entrada->nodo,*nodo)&&entrada->job==jobA;
		}
		while((entradaA=list_find(tablaEstados,(func)buscarEntrada))){
			if(entradaA->etapa==REDUCLOCAL){
				log_info(archivoLog,"[TANTEO] reduccion local de tanteo hizo efecto");
				bool limpiar(Entrada* entrada){
					return buscarEntrada(entrada)&&entrada->etapa==REDUCLOCAL;
				}
				free(list_remove_by_condition(tablaEstados,(func)limpiar));
				continue;
			}
			actualizarEntrada(entradaA,FRACASO,nullptr);
		}
		while((entradaA=list_find(tablaUsados,(func)buscarEntrada)))
			actualizarEntrada(entradaA,FRACASO,nullptr);

		log_info(archivoLog,"eliminando nodo %s",nodo->nombre);
		bool buscarWorker(Worker* worker){
			return nodoIguales(worker->nodo,*nodo);
		}
		Worker* about2die=list_remove_by_condition(workers,(func)buscarWorker);
		if(!about2die){
			puts(ROJO"!!!!SE PUDRIO TODO");
			return;
		}
		list_iterate(about2die->cargas,free);
		free(about2die);
		return;
	}

	int32_t etapa=*(int32_t*)mensaje->datos;
	void* datos=mensaje->datos+INTSIZE;
	int actualizando=mensaje->header.operacion;
	void registrarActualizacion(char* s){
		log_info(archivoLog,"[RECEPCION] actualizacion de %s, %s",s,actualizando==EXITO?"exito":"fracaso");
	}
	if(etapa==TRANSFORMACION){
		registrarActualizacion("transformacion");
		Dir nodo=*((Dir*)datos);
		int32_t bloque=*((int32_t*)(datos+DIRSIZE));
		bool buscarEntrada(Entrada* entrada){
			//printf("%s %s %d == %s %s %d\n",entrada->nodo.ip,entrada->nodo.port,entrada->bloque,nodo.ip,nodo.port,bloque);
			return nodoIguales(entrada->nodo,nodo)&&entrada->bloque==bloque&&entrada->masterid==masterid;
		}
		entradaA=list_find(tablaEstados,(func)buscarEntrada);
	}else if(etapa==REDUCLOCAL){
		registrarActualizacion("reduccion local");
		Dir nodo=*((Dir*)datos);
		bool buscarEntrada(Entrada* entrada){
			return nodoIguales(entrada->nodo,nodo)&&entrada->masterid==masterid;
		}
		entradaA=list_find(tablaEstados,(func)buscarEntrada);
	}else{
		if(etapa==REDUCGLOBAL) registrarActualizacion("reduccion global");
		else registrarActualizacion("almacenado");
		bool buscarEntrada(Entrada* entrada){
			return entrada->masterid==masterid;
		}
		entradaA=list_find(tablaEstados,(func)buscarEntrada);
	}
	if(!entradaA){
		imprimirMensaje(archivoLog,"[]ignorando actualizacion de job cancelado");
		return;
	}
	actualizarEntrada(entradaA,actualizando,mensaje);
}

void actualizarEntrada(Entrada* entradaA,int actualizando, Mensaje* mensaje){
	entradaA->estado=actualizando;

	void darDatosEntrada(Entrada* entrada){
		entrada->nodo=entradaA->nodo;
		entrada->job=entradaA->job;
		entrada->masterid=entradaA->masterid;
		entrada->estado=ENPROCESO;
		entrada->bloque=-1;
	}
	bool mismoJob(Entrada* entrada){
		return entrada->job==entradaA->job;
	}
	bool mismoNodoJob(Entrada* entrada){
		return mismoJob(entrada)&&nodoIguales(entrada->nodo,entradaA->nodo);
	}
	if(actualizando==FRACASO){
		void abortarJob(){
			void abortarEntrada(Entrada* entrada){
				if(mismoJob(entrada))
					entrada->estado=ABORTADO;
			}
			moverAUsados((func)mismoJob);
			list_iterate(tablaUsados,(func)abortarEntrada);
			log_info(archivoLog,"[] Abortando master #%d",(int)entradaA->masterid);
			mensajeEnviar(entradaA->masterid,ABORTAR,NULL,0);
			liberarCargas(entradaA->job);
		}
		if(entradaA->etapa==TRANSFORMACION&&actualizando==FRACASO){
			if(nodoIguales(entradaA->nodo,entradaA->nodoAlt)){
				log_info(archivoLog,"[] no hay mas copias para salvar el error");
				abortarJob();
				return;
			}
			Entrada alternativa;
			darDatosEntrada(&alternativa);
			alternativa.etapa=TRANSFORMACION;
			alternativa.nodo=entradaA->nodoAlt;
			alternativa.nodoAlt=entradaA->nodoAlt;
			alternativa.bloque=entradaA->bloqueAlt;
			alternativa.bytes=entradaA->bytes;
			darPathTemporal(&alternativa.pathTemporal,'t');
			char dato[DIRSIZE+INTSIZE*2+TEMPSIZE];
			memcpy(dato,&alternativa.nodo,DIRSIZE);
			memcpy(dato+DIRSIZE,&alternativa.bloque,INTSIZE);
			memcpy(dato+DIRSIZE+INTSIZE,&alternativa.bytes,INTSIZE);
			memcpy(dato+DIRSIZE+INTSIZE*2,alternativa.pathTemporal,TEMPSIZE);
			mensajeEnviar(alternativa.masterid,TRANSFORMACION,dato,sizeof dato);
			list_addM(tablaEstados,&alternativa,sizeof(Entrada));
			bool buscarError(Entrada* entrada){
				return entrada->estado==FRACASO;
			}
			list_add(tablaUsados,list_remove_by_condition(tablaEstados,(func)buscarError));//mutex
			bool buscarWorker(Worker* worker){
				return nodoIguales(worker->nodo,alternativa.nodo);
			}
			aumentarCarga(list_find(workers,(func)buscarWorker),alternativa.job,1);

			if(!list_any_satisfy(tablaEstados,(func)mismoNodoJob)){
				bool buscarWorker(Worker* worker){
					return nodoIguales(worker->nodo,entradaA->nodo);
				}
				log_info(archivoLog,"eliminando nodo %s",entradaA->nodo.nombre);
				Worker* about2die=list_remove_by_condition(workers,(func)buscarWorker);
				if(!about2die){
					puts(ROJO"!!!!SE PUDRIO TODO");
					return;
				}
				list_iterate(about2die->cargas,free);
				free(about2die);
			}
		}else{
			abortarJob();
			return;
		}
	}
	bool trabajoTerminado(bool(*cond)(void*)){
		bool aux(Entrada* entrada){
			return entrada->estado==EXITO;
		}
		Lista filtrada=list_filter(tablaEstados,cond);
		if(list_is_empty(filtrada)){
			list_destroy(filtrada);
			return false;
		}
		bool ret=list_all_satisfy(filtrada,(func)aux);
		list_destroy(filtrada);
		return ret;
	}
	if(entradaA->etapa==TRANSFORMACION||entradaA->etapa==REDUCLOCAL){
		bool hayTransformacion=false;
		bool hayQueReducir(Entrada* entrada){
			if(mismoNodoJob(entrada)){
				if(entrada->etapa==TRANSFORMACION)
					hayTransformacion=true;
			}
			return mismoNodoJob(entrada);
		}
		if(trabajoTerminado((func)hayQueReducir)&&hayTransformacion){
			log_info(archivoLog,"[REDUCLOCAL] creando entrada");
			Entrada reducLocal;
			darDatosEntrada(&reducLocal);
			darPathTemporal(&reducLocal.pathTemporal,'l');
			reducLocal.etapa=REDUCLOCAL;
			Lista nodos=list_filter(tablaEstados,(func)mismoNodoJob);
			int tamanio=TEMPSIZE*(nodos->elements_count+1)+DIRSIZE;
			void* dato=stringCrear(tamanio);
			memcpy(dato,&reducLocal.nodo,DIRSIZE);
			int i,j;
			for(i=DIRSIZE,j=0;i<tamanio-TEMPSIZE;i+=TEMPSIZE,j++)
				memcpy(dato+i,((Entrada*)list_get(nodos,j))->pathTemporal,TEMPSIZE);
			memcpy(dato+i,reducLocal.pathTemporal,TEMPSIZE);
			mensajeEnviar(reducLocal.masterid,REDUCLOCAL,dato,tamanio);
			moverAUsados((func)mismoNodoJob);
			free(dato);
			list_destroy(nodos);
			list_addM(tablaEstados,&reducLocal,sizeof(Entrada));//mutex
		}
	}
	if(entradaA->etapa==REDUCLOCAL){
		if(trabajoTerminado((func)mismoJob)){
			log_info(archivoLog,"[REDUCGLBAL] creando entrada");
			Entrada reducGlobal;
			darDatosEntrada(&reducGlobal);
			darPathTemporal(&reducGlobal.pathTemporal,'g');
			reducGlobal.etapa=REDUCGLOBAL;
			Worker* workerMenorCarga=list_get(workers,0);
			void menorCarga(Worker* worker){
				if(worker->carga<workerMenorCarga->carga)
					workerMenorCarga=worker;
			}
			list_iterate(workers,(func)menorCarga);
			reducGlobal.nodo=workerMenorCarga->nodo;
			int cantTemps=0;
			void contarTransformaciones(Entrada* entrada){
				if(entrada->etapa==TRANSFORMACION&&entrada->estado==EXITO&&entrada->job==entradaA->job)
					cantTemps++;
			}
			list_iterate(tablaUsados,(func)contarTransformaciones);
			aumentarCarga(workerMenorCarga,entradaA->job,ceil((double)cantTemps/2.0));

			Lista nodosReducidos=list_filter(tablaEstados,(func)mismoJob);

			int tamanio=(DIRSIZE+TEMPSIZE)*(nodosReducidos->elements_count+1);
			void* dato=stringCrear(tamanio);
			memcpy(dato,&workerMenorCarga->nodo,DIRSIZE);
			int i,j;
			for(i=DIRSIZE,j=0;i<tamanio-TEMPSIZE;i+=DIRSIZE+TEMPSIZE,j++){
				Dir* nodoActual=&((Entrada*)list_get(nodosReducidos,j))->nodo;
				memcpy(dato+i,nodoActual,DIRSIZE);
				memcpy(dato+i+DIRSIZE,((Entrada*)list_get(nodosReducidos,j))->pathTemporal,TEMPSIZE);
			}
			memcpy(dato+i,reducGlobal.pathTemporal,TEMPSIZE);

			mensajeEnviar(reducGlobal.masterid,REDUCGLOBAL,dato,tamanio);
			moverAUsados((func)mismoJob);
			free(dato);
			list_destroy(nodosReducidos);
			list_addM(tablaEstados,&reducGlobal,sizeof(Entrada));//mutex
		}
	}else if(entradaA->etapa==REDUCGLOBAL){
		log_info(archivoLog,"[ALMACENADO] creando entrada");
		//no le veo sentido a que yama participe del almacenado final
		//master lo podría hacer solo,  ya esta grande
		Entrada* reducGlobal=list_find(tablaEstados,(func)mismoJob);
		Entrada almacenado;
		darDatosEntrada(&almacenado);
		almacenado.etapa=ALMACENADO;
		almacenado.pathTemporal=malloc(mensaje->header.tamanio-INTSIZE-7); // -7 para sacar el yamafs: feo
		memcpy(almacenado.pathTemporal,mensaje->datos+INTSIZE+7,mensaje->header.tamanio-INTSIZE-7);
		char dato[DIRSIZE+TEMPSIZE];
		memcpy(dato,&reducGlobal->nodo,DIRSIZE);
		memcpy(dato+DIRSIZE,reducGlobal->pathTemporal,TEMPSIZE);
		mensajeEnviar(entradaA->masterid,ALMACENADO,dato,sizeof dato);
		list_add(tablaUsados,list_remove_by_condition(tablaEstados,(func)mismoJob));
		list_addM(tablaEstados,&almacenado,sizeof(Entrada));
	}else if(entradaA->etapa==ALMACENADO){
		list_add(tablaUsados,list_remove_by_condition(tablaEstados,(func)mismoJob));
		mensajeEnviar(entradaA->masterid,CIERRE,nullptr,0);
		liberarCargas(entradaA->job);
	}
}

void dibujarTablaEstados(){
	if(list_is_empty(tablaEstados)&&list_is_empty(tablaUsados))
		return;
	pantallaLimpiar();
	puts(" J  |  M |   N   |  B  |      ETAPA       |        SALIDA       |   ESTADO   |");
	puts("------------------------------------------------------------------------------");
	void dibujarEntrada(Entrada* entrada){
		char* etapa,*estado,*bloque; bool doFree=false;
		switch(entrada->etapa){
		case TRANSFORMACION: etapa="Transformacion"; break;
		case REDUCLOCAL: etapa="Reduccion local"; break;
		case REDUCGLOBAL: etapa="Reduccion global";break;
		default: etapa="Almacenado Final";
		}
		switch(entrada->estado){
		case EXITO: estado="Terminado"; break;
		case FRACASO: estado="Error"; break;
		case ABORTADO: estado="Abortado"; break;
		default: estado="Procesando";
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
			list_iterate(masters,(func)buscarMaster);
			if(index==-1){
				list_addM(masters,&masterid,sizeof(int));
				return i;
			}
			return index;
		}
		printf(" %2d | %2d | %5s | %3s | %16s | %19s | %10s |\n",
				entrada->job,masterToNum(entrada->masterid),entrada->nodo.nombre,bloque,
				etapa,entrada->pathTemporal,estado);
		if(doFree)
			free(bloque);
	}
	void dibujarCarga(Worker* worker){
		printf(AMARILLO"%s c: %d     "BLANCO,worker->nodo.nombre,worker->carga);
	}
	list_iterate(tablaUsados,(func)dibujarEntrada);
	//puts(ROJO"-.-|^|-.-"BLANCO);
	list_iterate(tablaEstados,(func)dibujarEntrada);
	list_iterate(workers,(func)dibujarCarga);
	puts("");
}

void darPathTemporal(char** ret,char pre){
	//mutex
	static char* anterior;
	static char agregado;
	char* temp=temporal_get_string_time();
	*ret=malloc(TEMPSIZE); //12
	int i,j=1;
	(*ret)[0]=pre;
	for(i=0;i<stringLongitud(temp);i++){
		if(temp[i]==':')
			continue;
		(*ret)[j]=temp[i];
		j++;
	}
	//log_info(archivoLog,"temp |%s|",temp);
	int endof=stringLongitud(temp)-3;
	(*ret)[endof+1]='0';
	(*ret)[endof+2]='\0';
	char* anteriorTemp=string_duplicate(*ret);
	//if(anterior) log_info(archivoLog,"ant |%s|, new |%s|",anterior,*ret);
	if(stringIguales(*ret,anterior))
		if(agregado=='9')
			agregado='a';
		else if(agregado=='z')
			agregado='A';
		else if(agregado=='Y'){
			usleep(10000);agregado++;
		}
		else
			agregado++;
	else
		agregado='0';
	(*ret)[endof+1]=agregado;
	//log_info(archivoLog,"then |%s|",*ret);
	free(anterior);free(temp);
	anterior=anteriorTemp;
}
void moverAUsados(bool(*cond)(void*)){
	//mutex
	Entrada* entrada;
	while((entrada=list_remove_by_condition(tablaEstados,cond))){
		list_add(tablaUsados,entrada);
	}
}
void aumentarCarga(Worker* worker,int jobb,int aumento){
	worker->carga+=aumento;
	worker->tareasRealizadas+=aumento;
	bool cargaJobActual(CargaJob* carga){
		return carga->job==jobb;
	}
	((CargaJob*)list_find(worker->cargas,(func)cargaJobActual))->carga+=aumento;
}
void liberarCargas(int jobb){
	void levantarCarga(Worker* worker){
		bool jobLiberado(CargaJob* jobA){
			return jobA->job==jobb;
		}
		CargaJob* levita=list_remove_by_condition(worker->cargas,(func)jobLiberado);
		worker->carga-=levita->carga;
		free(levita);
	}
	list_iterate(workers,(func)levantarCarga);
}

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
