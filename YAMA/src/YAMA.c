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
	yamaFinalizar();
	return EXIT_SUCCESS;
}

void yamaIniciar() {
	pantallaLimpiar();
	estadoYama=ACTIVADO;
	imprimirMensajeProceso("# PROCESO YAMA");
	archivoLog = archivoLogCrear(RUTA_LOG, "YAMA");
	archivoConfigObtenerCampos();

	Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
		Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
		stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
		stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
		stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
		configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
		stringCopiar(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
		configuracion->disponibilidadBase = archivoConfigEnteroDe(archivoConfig, "DISPONIBILIDAD_BASE");
		archivoConfigDestruir(archivoConfig);
		return configuracion;
	}
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);

	servidor = memoriaAlocar(sizeof(Servidor));
	imprimirMensajeDos(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	servidor->fileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con File System");

	//infoNodos es una lista de ips y puertos
	Mensaje* infoNodos=mensajeRecibir(servidor->fileSystem);
	mensajeObtenerDatos(infoNodos,servidor->fileSystem);
	int i=0;
	while(i<infoNodos->header.tamanio){ // dividido el tamaño del ip y puerto
		Worker worker;
		worker.conectado=true;
		worker.carga=0;
		worker.tareasRealizadas=0;
		worker.nodo=i; //suponiendo que el filesystem los enumere segun
		//le lleguen y me los mande asi
		memcpy(worker.ipYPuerto,infoNodos->datos+10*i,10);
		list_add(workers,&worker); //por ahi va sin &
	}
}

void yamaAtender() {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);

	void servidorControlarMaximoSocket(Socket unSocket) {
		if(socketEsMayor(unSocket, servidor->maximoSocket))
			servidor->maximoSocket = unSocket;
	}
	servidor->listenerMaster = socketCrearListener(configuracion->puertoMaster);
	log_info(archivoLog, "[CONEXION] Esperando conexiones de un Master (Puerto %s)", configuracion->puertoMaster);
	listaSocketsAgregar(servidor->listenerMaster, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor->fileSystem);
	servidorControlarMaximoSocket(servidor->listenerMaster);

	tablaEstados=list_create();
	tablaUsados=list_create();

	while(estadoYama==ACTIVADO){
		dibujarTablaEstados();

		servidor->listaSelect = servidor->listaMaster;
		socketSelect(servidor->maximoSocket, &servidor->listaSelect);
		Socket socketI;
		Socket maximoSocket = servidor->maximoSocket;
		for(socketI = 0; socketI <= maximoSocket; socketI++){
			if (listaSocketsContiene(socketI, &servidor->listaSelect)){ //se recibio algo
				//podría disparar el thread aca o antes de planificar
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
					mensajeObtenerDatos(mensaje,socketI);
					//aca debería estar el caso de que el mensaje sea
					//para avisar que se desconectó un nodo
					//y otro para avisar que se reconectó. (si pueden hacer eso)
					//podría hacer switch(mensaje->header.operacion)
					Socket masterid;
					memcpy(&masterid,mensaje->datos,INTSIZE);
					log_info(archivoLog, "[RECEPCION] lista de bloques para master #%d recibida",&masterid);
					void* listaBloques=malloc(mensaje->header.tamanio-INTSIZE);
					memcpy(listaBloques,mensaje->datos+INTSIZE,mensaje->header.tamanio-INTSIZE);
					if(listaSocketsContiene(masterid,&servidor->listaMaster)) //por si el master se desconecto
						yamaPlanificar(masterid,listaBloques,mensaje->header.tamanio-INTSIZE);
					mensajeDestruir(mensaje);
				}else{ //master
					Mensaje* mensaje = mensajeRecibir(socketI);
					mensajeObtenerDatos(mensaje,socketI);
					switch(mensaje->header.operacion){
					case SOLICITUD:{//se recibio solicitud de master #socketI
						int32_t masterid = socketI;
						//el mensaje es el path del archivo
						//aca le acoplo el numero de master y se lo mando al fileSystem
						mensaje=realloc(mensaje->datos,mensaje->header.tamanio+INTSIZE);
						memmove(mensaje->datos+INTSIZE,mensaje,mensaje->header.tamanio);
						memcpy(mensaje->datos,&masterid,INTSIZE);
						mensajeEnviar(servidor->fileSystem,SOLICITUD,mensaje->datos,mensaje->header.tamanio+INTSIZE);
						imprimirMensajeUno(archivoLog, "[ENVIO] path de master #%d enviado al fileSystem",&socketI);
					}break;case DESCONEXION:
						listaSocketsEliminar(socketI, &servidor->listaMaster);
						socketCerrar(socketI);
						if(socketI==servidor->maximoSocket)
							servidor->maximoSocket--; //no debería romper nada
						log_info(archivoLog, "[CONEXION] Proceso Master %d se ha desconectado",socketI);
					break;default:{
						int32_t nodo = *((int32_t*)mensaje->datos);
						int32_t bloque = *((int32_t*)(mensaje->datos+INTSIZE));
						actualizarTablaEstados(nodo,bloque,mensaje->header.operacion);
					}}
					mensajeDestruir(mensaje);
				}
			}
		}
	}
	memoriaLiberar(servidor);
}

void yamaFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso YAMA finalizado");
	archivoLogDestruir(archivoLog);
	memoriaLiberar(servidor); //no es al pedo liberar memoria si el
	memoriaLiberar(configuracion);//programa esta a punto de terminar?
}


void archivoConfigObtenerCampos() {
	campos[0] = "IP_PROPIO";
	campos[1] = "PUERTO_MASTER";
	campos[2] = "IP_FILESYSTEM";
	campos[3] = "PUERTO_FILESYSTEM";
	campos[4] = "RETARDO_PLANIFICACION";
	campos[5] = "ALGORITMO_BALANCEO";
	campos[6] = "DISPONIBILIDAD_BASE";
}//donde se usan los campos estos?


void yamaPlanificar(Socket master, void* listaBloques,int tamanio){
	int i=0;
	Lista bloques=list_create();
	while(sizeof(Bloque)*i<tamanio){
		list_add(bloques,(Bloque*)(listaBloques+sizeof(Bloque)*i));
		i++;
	}

	Lista tablaEstadosJob;
	job++;//mutex (supongo que las variables globales se comparten entre hilos)
	for(i=0;i<bloques->elements_count/2;i++){
		Entrada entrada;
		entrada.job=job;
		entrada.masterid=master;
//		entrada.pathArchivoTemporal ?
		list_add(tablaEstadosJob,&entrada);
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
				clock=worker->nodo;
			}
		}
		list_iterate(workers,setearClock);
	}
	Worker* workerClock=list_get(workers,clock); //alguna forma de no tener que hacer esto?
	for(i=0;i<bloques->elements_count;i+=2){
		void asignarBloque(Worker* worker,Bloque* bloque){
			worker->carga++; //y habría que usar mutex aca
			worker->disponibilidad--;
			worker->tareasRealizadas++;
			Entrada* entrada=list_get(tablaEstadosJob,i/2);
			entrada->nodo=worker->nodo;
			entrada->bloque=bloque->bloque;
			entrada->etapa=Transformacion;
			entrada->estado=EnProceso;
		}

		Bloque* bloque0 = list_get(bloques,i);
		Bloque* bloque1 = list_get(bloques,i+1);
		bool encontrado=false;

		if(workerClock->nodo==bloque0->nodo){
			asignarBloque(workerClock,bloque0);
			encontrado=true;
		}else if(workerClock->nodo==bloque1->nodo){
			asignarBloque(workerClock,bloque1);
			encontrado=true;
		}
		if(encontrado){
			clock=(clock+1)%workers->elements_count;
			Worker* workerTest=list_get(workers,clock);
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
			Worker* workerAdv=list_get(workers,clockAdv);
			if(workerAdv->disponibilidad>0){
				if(workerAdv->nodo==bloque0->nodo){
					asignarBloque(workerAdv,bloque0);
					break;
				}else if(workerClock->nodo==bloque1->nodo){
					asignarBloque(workerAdv,bloque1);
					break;
				}
			}
		}
	}//termina la planificacion

	int32_t tamanioDato=INTSIZE*2*tablaEstadosJob->elements_count;
	void* dato=malloc(tamanioDato);
	for(i=0;i<tablaEstadosJob->elements_count;i++){
		Entrada* entrada=list_get(tablaEstadosJob,i);
		memcpy(dato+INTSIZE*i*2,&entrada->nodo,INTSIZE);
		memcpy(dato+INTSIZE*i*2+1,&entrada->bloque,INTSIZE);
	}
	mensajeEnviar(master,TRANSFORMACION,dato,tamanioDato);
	free(dato);

	list_add_all(tablaEstados,tablaEstadosJob); //mutex
	list_destroy(tablaEstadosJob);
}

void actualizarTablaEstados(int nodo,int bloque,int actualizando){
	bool buscarEntrada(Entrada* entrada){
		return entrada->nodo==nodo&&entrada->bloque==bloque;
	}
	Entrada* entradaA=list_find(tablaEstados,buscarEntrada);
	entradaA->estado=actualizando;//actualizando es ERROR o TERMINADO
	if(actualizando==ERROR){
		if(entradaA->etapa==Transformacion){
			//mover entrada a la lista usados
			//replanificar
		}else{
			//abortar job, mover todas las entradas a la lista mala
		}
		return;
	}
	void darDatosEntrada(Entrada* entradaTo,Entrada* entradaFrom){
		entradaTo->nodo=entradaFrom->nodo;
		entradaTo->job=entradaFrom->job;
		entradaTo->masterid=entradaFrom->masterid;
		entradaTo->estado=EnProceso;
		//y habría que darle un nuevo path temporal
	}
	bool trabajoTerminadoB=true;
	void trabajoTerminado(bool(*cond)(void*)){
		void aux(Entrada* entrada){
			if(cond(entrada))
				trabajoTerminadoB=false;
		}
		list_iterate(tablaEstados,aux);
	}
	void moverAUsados(bool(*cond)(void*)){
		Entrada* entrada;
		while((entrada=list_remove_by_condition(tablaEstados,cond))){
			list_add(tablaUsados,entrada);
		}
	}
	if(entradaA->etapa==Transformacion){
		bool condicion(Entrada* entrada){
			if(entrada->nodo==entradaA->nodo&&entrada->job==entradaA->job)
				return true;
			return false;
		}
		trabajoTerminado(condicion);
		if(trabajoTerminadoB){
			moverAUsados(condicion);
			Entrada reducLocal;
			darDatosEntrada(&reducLocal,entradaA);
			reducLocal.etapa=ReduccionLocal;
			mensajeEnviar(reducLocal.masterid,REDUCLOCAL,&reducLocal.nodo,INTSIZE);
		}
	}else if(entradaA->etapa==ReduccionLocal){
		bool condicion(Entrada* entrada){
			if(entrada->job==entradaA->job)
				return true;
			return false;
		}
		trabajoTerminado(condicion);
		if(trabajoTerminadoB){
			moverAUsados(condicion);
			Entrada reducGlobal;
			darDatosEntrada(&reducGlobal,entradaA);
			reducGlobal.etapa=ReduccionGlobal;
			int32_t nodoMenorCarga=entradaA->nodo;
			int menorCargaI=100; //
			void menorCarga(Worker* worker){
				if(worker->carga<menorCargaI)
					nodoMenorCarga=worker->nodo;
					menorCargaI=worker->carga;
			}
			list_iterate(workers,menorCarga);
			mensajeEnviar(reducGlobal.masterid,REDUCGLOBAL,&nodoMenorCarga,INTSIZE);
		}
	}else{
		bool condicion(Entrada* entrada){
			if(entrada->job==entradaA->job)
				return true;
			return false;
		}
		list_add(tablaUsados,list_remove_by_condition(tablaEstados,condicion));
	}
}

void dibujarTablaEstados(){
	pantallaLimpiar();
	puts("Job    Master    Nodo    Bloque    Etapa    Temporal    Estado");
	void dibujarEntrada(Entrada* entrada){
		char* etapa,*estado;
		switch(entrada->etapa){
		case Transformacion: etapa="transformacion"; break;
		case ReduccionLocal: etapa="reduccion local"; break;
		default: etapa="reduccion global";
		}
		switch(entrada->estado){
		case EnProceso: estado="en proceso"; break;
		case Error: estado="error"; break;
		default: estado="terminado";
		}
		printf("%d     %d     %d     %d     %s     %s    %s",
				entrada->job,entrada->masterid,entrada->nodo,entrada->bloque,
				etapa,entrada->pathArchivoTemporal,estado);
	}
	list_iterate(tablaUsados,dibujarEntrada);
	list_iterate(tablaEstados,dibujarEntrada);
}


//faltaría hacer seguros por si el master se desconecta en
//cada etapa

//mover las entradas terminadas y con error a otro lado,
//asi no las recorro al pedo. No las voy a usar nunca mas
//y algunas busquedas andarian mal si consideran las entradas con error

//wat is dis
//void notificadorInformar(Socket unSocket) {
//	char buffer[BUF_LEN];
//	int length = read(unSocket, buffer, BUF_LEN);
//	int offset = 0;
//	while (offset < length) {
//		struct inotify_event *event = (struct inotify_event *) &buffer[offset];
//		if (event->len) {
//		if (event->mask & IN_MODIFY) {
//		if (!(event->mask & IN_ISDIR)) {
//		if(strcmp(event->name, "ArchivoConfig.conf"))
//			break;
//		ArchivoConfig archivoConfig = config_create(RUTA_CONFIG);
//		if(archivoConfigTieneCampo(archivoConfig, "RETARDO_PLANIFICACION")){
//			int retardo = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
//				if(retardo != configuracion->retardoPlanificacion){
//					puts("");
//					log_warning(archivoLog, "[CONFIG]: SE MODIFICO EL ARCHIVO DE CONFIGURACION");
//					configuracion->retardoPlanificacion = retardo;
//					log_warning(archivoLog, "[CONFIG]: NUEVA RUTA METADATA: %s\n", configuracion->retardoPlanificacion);
//
//				}
//				archivoConfigDestruir(archivoConfig);
//		}
//		}
//		}
//		}
//		offset += sizeof (struct inotify_event) + event->len;
//		}
//
//	//Esto se haria en otro lado
//
//	//inotify_rm_watch(file_descriptor, watch_descriptor);
//		//close(file_descriptor);
//}
