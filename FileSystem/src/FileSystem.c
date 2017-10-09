/*
 ============================================================================
 Name        : FileSystem.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso File System
 ============================================================================
 */

#include "FileSystem.h"

int main(void) {
	fileSystemIniciar();
	fileSystemCrearConsola();
	fileSystemAtenderProcesos();
	fileSystemFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	archivoLog = archivoLogCrear(RUTA_LOG, "FileSystem");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System inicializado");
	fileSystemActivar();
	archivoConfigObtenerCampos();
	//senialAsignarFuncion(SIGPIPE, SIG_IGN);
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	listaDirectorios = listaCrear();
	directoriosDisponibles = 100;
}

void fileSystemCrearConsola() {
	hiloCrear(&hiloConsola, (Puntero)consolaAtenderComandos, NULL);
}

void fileSystemAtenderProcesos() {
	Servidor* servidor = memoriaAlocar(sizeof(Servidor));
	servidorInicializar(servidor);
	while(fileSystemActivado())
		servidorAtenderPedidos(servidor);
	servidorFinalizar(servidor);
}

void fileSystemFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System finalizado");
	//hiloEsperar(hiloConsola);
	archivoLogDestruir(archivoLog);
	memoriaLiberar(configuracion);
	//sleep(1.5);
}

//--------------------------------------- Funciones de Consola -------------------------------------


bool consolaEntradaRespetaLimiteEspacios(String entrada) {
	int indice;
	int contador = 0;
	for(indice = 0; caracterDistintos(entrada[indice], FIN); indice++)
		if(caracterIguales(entrada[indice],ESPACIO))
			contador++;
	return contador <=  4;
}

bool consolaEntradaSinEspacioEnExtremos(String entrada) {
	if(stringDistintos(entrada, VACIO))
		return caracterDistintos(entrada[0], ESPACIO) &&
				caracterDistintos(entrada[stringLongitud(entrada)-1],ESPACIO);
	else
		return false;
}

bool consolaEntradaEspaciosSeparados(String entrada) {
 	int indice;
	for(indice = 0; caracterDistintos(entrada[indice],FIN); indice++)
		if(caracterIguales(entrada[indice],ESPACIO) && (caracterIguales(entrada[indice-1],ESPACIO) ||
			caracterIguales(entrada[indice+1],ESPACIO)))
			return false;
	return true;
}


bool consolaEntradaEspaciosBienUtilizados(String entrada) {
	return consolaEntradaSinEspacioEnExtremos(entrada) &&
			consolaEntradaEspaciosSeparados(entrada);

}

bool consolaEntradaSinTabs(String entrada) {
	int indice;
	for(indice = 0; caracterDistintos(entrada[indice], FIN); indice++)
		if(caracterIguales(entrada[indice],TAB))
			return 0;
	return 1;
}

bool consolaEntradaTieneEspaciosNecesarios(String entrada) {
	return consolaEntradaRespetaLimiteEspacios(entrada) &&
			consolaEntradaSinTabs(entrada);
}

bool consolaEntradaLlena(String entrada) {
	return stringDistintos(entrada, VACIO);
}

bool consolaEntradaDecente(String entrada) {
	return consolaEntradaTieneEspaciosNecesarios(entrada) &&
			consolaEntradaEspaciosBienUtilizados(entrada) &&
			consolaEntradaLlena(entrada);
}

int consolaIdentificarComando(String comando) {
	if(stringNulo(comando))
		return ERROR;
	if(stringIguales(comando, C_FORMAT))
		return FORMAT;
	if(stringIguales(comando, C_INFO))
		return INFO;
	if(stringIguales(comando, C_RM))
		return RM;
	if(stringIguales(comando, C_RMB))
		return RMB;
	if(stringIguales(comando, C_RMD))
		return RMD;
	if(stringIguales(comando, C_RENAME))
		return RENAME;
	if(stringIguales(comando, C_MV))
		return MV;
	if(stringIguales(comando, C_CAT))
		return CAT;
	if(stringIguales(comando, C_MKDIR))
		return MKDIR;
	if(stringIguales(comando, C_CPFROM))
		return CPFROM;
	if(stringIguales(comando, C_CPTO))
		return CPTO;
	if(stringIguales(comando, C_CPBLOCK))
		return CPBLOCK;
	if(stringIguales(comando, C_MD5))
		return MD5;
	if(stringIguales(comando, C_LS))
		return LS;
	if(stringIguales(comando, C_INFO))
		return INFO;
	if(stringIguales(comando, C_RENAME))
		return RENAME;
	if(stringIguales(comando, C_EXIT))
		return EXIT;
	else
		return ERROR;
}

bool consolaComandoTipoUno(String comando) {
	return stringIguales(comando, C_RM) ||
			stringIguales(comando, C_CAT) ||
			stringIguales(comando, C_MKDIR) ||
			stringIguales(comando, C_MD5) ||
			stringIguales(comando, C_LS) ||
			stringIguales(comando, C_INFO);
}

bool consolaComandoTipoDos(String comando) {
	return stringIguales(comando, C_RENAME) ||
			stringIguales(comando, C_MV) ||
			stringIguales(comando, C_CPFROM) ||
			stringIguales(comando, C_CPTO);
}

bool consolaComandoTipoTres(String comando) {
	return stringIguales(comando, C_CPBLOCK);
}


int consolaComandoCantidadArgumentos(String comando) {
	if(consolaComandoTipoUno(comando))
		return 1;
	if(consolaComandoTipoDos(comando))
		return 2;
	if(consolaComandoTipoTres(comando))
		return 3;
	else
		return 0;
}

bool consolaComandoExiste(String comando) {
	return consolaComandoTipoUno(comando) ||
			consolaComandoTipoDos(comando) ||
			consolaComandoTipoTres(comando) ||
			stringIguales(comando, C_FORMAT) ||
			stringIguales(comando, C_EXIT);
}


bool consolaValidarComandoSinTipo(String* subcadenas) {
	return stringNulo(subcadenas[1]);
}

bool consolaValidarComandoTipoUno(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) &&
			stringNulo(subcadenas[2]);
}

bool consolaValidarComandoTipoDos(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) &&
			stringNoNulo(subcadenas[2]) &&
		   stringNulo(subcadenas[3]);
}

bool consolaValidarComandoTipoTres(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) &&
		   stringNoNulo(subcadenas[2]) &&
		   stringNoNulo(subcadenas[3]) &&
		   stringNulo(subcadenas[4]);
}


bool consolaComandoEsRemoverFlag(String buffer) {
	return stringIguales(buffer, FLAG_B) ||
			stringIguales(buffer, FLAG_D);
}


bool consolaComandoRemoverFlagB(String* subcadenas) {
	return stringIguales(subcadenas[0], C_RM) &&
			stringIguales(subcadenas[1], FLAG_B);
}

bool consolaComandoRemoverFlagD(String* subcadenas) {
	return stringIguales(subcadenas[0], C_RM) &&
			stringIguales(subcadenas[1], FLAG_D);
}

bool consolaValidarComandoFlagB(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) &&
			   stringNoNulo(subcadenas[2]) &&
			   stringNoNulo(subcadenas[3]) &&
			   stringNoNulo(subcadenas[4]);
}

bool consolaValidarComandoFlagD(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) &&
			   stringNoNulo(subcadenas[2]) &&
			   stringNulo(subcadenas[3]) &&
			   stringNulo(subcadenas[4]);
}

bool consolaComandoControlarArgumentos(String* subcadenas) {
	if(consolaComandoRemoverFlagB(subcadenas))
		return consolaValidarComandoFlagB(subcadenas);
	if(consolaComandoRemoverFlagD(subcadenas))
		return consolaValidarComandoFlagD(subcadenas);
	if(consolaComandoTipoUno(subcadenas[0]))
		return consolaValidarComandoTipoUno(subcadenas);
	if(consolaComandoTipoDos(subcadenas[0]) )
		return consolaValidarComandoTipoDos(subcadenas);
	if(consolaComandoTipoTres(subcadenas[0]))
		return consolaValidarComandoTipoTres(subcadenas);
	else
		return consolaValidarComandoSinTipo(subcadenas);
}


void consolaNormalizarRemoverFlagB(String* buffer) {
	buffer[0] = stringDuplicar(C_RMB);
	buffer[1] = buffer[2];
	buffer[2] = buffer[3];
	buffer[3] = buffer[4];
	buffer[4] = NULL;
}

void consolaNormalizarRemoverFlagD(String* buffer) {
	buffer[0] = stringDuplicar(C_RMD);
	buffer[1] = buffer[2];
	buffer[2] = buffer[3];
	buffer[3] = buffer[4];
	buffer[4] = NULL;
}

void consolaNormalizarComando(String* buffer) {
	if(stringIguales(buffer[1], FLAG_B))
		consolaNormalizarRemoverFlagB(buffer);
	else
		consolaNormalizarRemoverFlagD(buffer);
}

bool consolaValidarComando(String* buffer) {
	if(consolaComandoExiste(buffer[0]))
		return consolaComandoControlarArgumentos(buffer);
	else
		return false;
}

String consolaLeerEntrada() {
	int indice;
	int caracterLeido;
	char* cadena = memoriaAlocar(MAX);
	for(indice = 0; caracterDistintos((caracterLeido= caracterObtener()),ENTER); indice++)
		cadena[indice] = caracterLeido;
	cadena[indice] = FIN;
	return cadena;
}


void consolaIniciarArgumentos(String* argumentos) {
	argumentos[0] = NULL;
	argumentos[1] = NULL;
	argumentos[2] = NULL;
	argumentos[3] = NULL;
	argumentos[4] = NULL;
}

void consolaLiberarArgumentos(String* argumentos) {
	memoriaLiberar(argumentos[0]);
	memoriaLiberar(argumentos[1]);
	memoriaLiberar(argumentos[2]);
	memoriaLiberar(argumentos[3]);
	memoriaLiberar(argumentos[4]);
}

void consolaObtenerArgumentos(String* buffer, String entrada) {
	int indice;
	int indiceBuffer = 0;
	int ultimoEspacio = -1;
	for(indice = 0; caracterDistintos(entrada[indice], FIN); indice++)
		if(caracterIguales(entrada[indice],ESPACIO)) {
			buffer[indiceBuffer] = stringTomarCantidad(entrada, ultimoEspacio+1, indice-ultimoEspacio-1);
			ultimoEspacio = indice;
			indiceBuffer++;
		}
	buffer[indiceBuffer] = stringTomarCantidad(entrada, ultimoEspacio+1, indice-ultimoEspacio-1);
}


void consolaSetearArgumentos(String* argumentos, String* buffer) {
	(argumentos[0]=buffer[1]);
	(argumentos[1]=buffer[2]);
	(argumentos[2]=buffer[3]);
}

void consolaCrearComando(Comando* comando, String entrada) {
	consolaIniciarArgumentos(comando->argumentos);
	if(consolaEntradaDecente(entrada)) {
		consolaObtenerArgumentos(comando->argumentos, entrada);
	if(consolaValidarComando(comando->argumentos))
		comando->identificador = consolaIdentificarComando(comando->argumentos[0]);
	else
		comando->identificador = ERROR;
	} else {
		comando->identificador = ERROR;
	}
}

void consolaFinalizar() {
	 estadoFileSystem = 0;
	 socketCrearCliente(IP_LOCAL, configuracion->puertoDataNode, ID_DATANODE);
}

void consolaRealizarAccion(Comando* comando) {
	switch(comando->identificador) {
		case FORMAT: puts("COMANDO FORMAT"); break;
		case RM: puts("COMANDO RM"); break;
		case RMB: puts("COMANDO RM -B"); break;
		case RMD: puts("COMANDO RM -D"); break;
		case RENAME: puts("COMANDO RENAME"); break;
		case MV: puts("COMANDO  MV"); break;
		case CAT: puts("COMANDO CAT"); break;
		case MKDIR: directorioCrear(comando->argumentos[1]); break;
		case CPFROM: puts("COMANDO CPFROM"); break;
		case CPTO: puts("COMANDO CPTO"); break;
		case CPBLOCK:puts("COMANDO CPBLOCK"); break;
		case MD5: puts("COMANDO MD5"); break;
		case LS: puts("COMANDO LS"); break;
		case INFO: puts("COMANDO INFO"); break;
		case EXIT: consolaFinalizar();break;
		default: puts("COMANDO INVALIDO"); break;
	}
}

void consolaAtenderComandos() {
	hiloDetach(pthread_self());
	while(fileSystemActivado()) {
		char* entrada = consolaLeerEntrada();
		if(estadoFileSystem) {
			Comando comando;
			consolaCrearComando(&comando, entrada);
			consolaRealizarAccion(&comando);
			if(comando.identificador != ERROR) {

			printf("arg 0: %s\n", comando.argumentos[0]);
			printf("arg 1: %s\n", comando.argumentos[1]);
			printf("arg 2: %s\n", comando.argumentos[2]);
			printf("arg 3: %s\n", comando.argumentos[3]);
			printf("arg 4: %s\n", comando.argumentos[4]);
			}
			consolaLiberarArgumentos(comando.argumentos);
		}
		memoriaLiberar(entrada);

	}
}
//--------------------------------------- Funciones de Servidor -------------------------------------

bool servidorObtenerMaximoSocket(Servidor* servidor) {
	return servidor->maximoSocket;
}

void servidorSetearListaSelect(Servidor* servidor) {
	servidor->listaSelect = servidor->listaMaster;
}

void servidorControlarMaximoSocket(Servidor* servidor, Socket unSocket) {
	if(socketEsMayor(unSocket, servidor->maximoSocket))
		servidor->maximoSocket = unSocket;
}

void servidorEsperarSolicitud(Servidor* servidor) {
	socketSelect(servidor->maximoSocket, &servidor->listaSelect);
}

bool socketEsDataNode(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaDataNodes);
}

bool socketEsYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->procesoYAMA, unSocket);
}

void servidorFinalizarConexion(Servidor* servidor, Socket unSocket) {
	listaSocketsEliminar(unSocket, &servidor->listaMaster);
	if(socketEsDataNode(servidor, unSocket)) {
		listaSocketsEliminar(unSocket, &servidor->listaDataNodes);
		imprimirMensaje(archivoLog, "[CONEXION] Un proceso Data Node se ha desconectado");
	}
	else {
		imprimirMensaje(archivoLog, "[CONEXION] El proceso YAMA se ha desconectado");
	}

	socketCerrar(unSocket);
}

void servidorRegistrarConexion(Servidor* servidor, Socket unSocket) {
	if(unSocket != ERROR) {
		listaSocketsAgregar(unSocket, &servidor->listaMaster);
		servidorControlarMaximoSocket(servidor, unSocket);
	}

}

Socket servidorAceptarYAMA(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(unSocket, ID_YAMA);
	if(nuevoSocket != ERROR) {
		servidor->procesoYAMA = nuevoSocket;
		imprimirMensaje(archivoLog, "[CONEXION] Proceso YAMA conectado exitosamente");
	}
	return nuevoSocket;
}

bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerDataNode, unSocket);
}


void servidorAceptarConexion(Servidor* servidor, Socket unSocket) {

}

void servidorRecibirMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeDesconexion(mensaje))
		servidorFinalizarConexion(servidor, unSocket);
	else
		puts("[MENSAJE]");
	mensajeDestruir(mensaje);
}


bool socketEsListenerYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerYAMA, unSocket);
}

bool socketEsListener(Servidor* servidor, Socket unSocket) {
	return socketEsListenerDataNode(servidor, unSocket) || socketEsListenerYAMA(servidor, unSocket);
}

bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaSelect);
}

void servidorControlarSocket(Servidor* servidor, Socket unSocket) {

}

void servidorActivarListenerYAMA(Servidor* servidor) {
	servidor->listenerYAMA = socketCrearListener(configuracion->puertoYAMA);
	listaSocketsAgregar(servidor->listenerYAMA, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerYAMA);
}

void servidorActivarListenerDataNode(Servidor* servidor) {
	servidor->listenerDataNode = socketCrearListener(configuracion->puertoDataNode);
	listaSocketsAgregar(servidor->listenerDataNode, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerDataNode);
}

void servidorActivarListeners(Servidor* servidor) {
	servidorActivarListenerDataNode(servidor);
	servidorActivarListenerYAMA(servidor);
}

void servidorInicializar(Servidor* servidor) {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);
	listaSocketsLimpiar(&servidor->listaDataNodes);
	servidorActivarListeners(servidor);
}

void servidorFinalizar(Servidor* servidor) {
	memoriaLiberar(servidor);
}

void servidorRegistrarDataNode(Servidor* servidor, Socket nuevoSocket) {
	if(nuevoSocket != ERROR) {
		listaSocketsAgregar(nuevoSocket, &servidor->listaDataNodes);
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Data Node conectado exitosamente");
	}
}

void servidorAtenderPedidos(Servidor* servidor) {
	servidorSetearListaSelect(servidor);
	servidorEsperarSolicitud(servidor);
	Socket unSocket;
	Socket maximoSocket = servidor->maximoSocket;
	for(unSocket = 0; unSocket <= maximoSocket; unSocket++)
		if (socketRealizoSolicitud(servidor, unSocket)) {
			if(socketEsListener(servidor, unSocket)) {
				Socket nuevoSocket;
				if(socketEsListenerDataNode(servidor, unSocket)) {
					nuevoSocket = socketAceptar(unSocket, ID_DATANODE);
					if(fileSystemDesactivado())
						break;
					servidorRegistrarDataNode(servidor, nuevoSocket);
				}
				else
					nuevoSocket = servidorAceptarYAMA(servidor, unSocket);

				servidorRegistrarConexion(servidor, nuevoSocket);
			}
			else
				servidorRecibirMensaje(servidor, unSocket);
		}
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	stringCopiar(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	stringCopiar(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexion de YAMA (Puerto: %s)", configuracion->puertoYAMA);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)", configuracion->puertoDataNode);
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Ruta Metadata: %s", configuracion->rutaMetadata);
}

void archivoConfigObtenerCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
	campos[2] = "RUTA_METADATA";
}

bool fileSystemActivado() {
	return estadoFileSystem == ACTIVADO;
}

bool fileSystemDesactivado() {
	return estadoFileSystem == DESACTIVADO;
}

void fileSystemActivar() {
	estadoFileSystem = ACTIVADO;
}

void fileSystemDesactivar() {
	estadoFileSystem = DESACTIVADO;
}


//--------------------------------------- Funciones de Directorio -------------------------------------
/*
Directorio directorioCrear(int indice, int padre, String nombre) {
	Directorio directorio;
	directorio.indice = indice;
	directorio.padre = padre;
	stringCopiar(directorio.nombre, nombre);
	return directorio;
}
*/

Directorio directorioLeerDeArchivo(Archivo unArchivo) {
   Directorio directorio;
   fread(&directorio, 1, sizeof(Directorio), unArchivo);
   return directorio;
}

void listaDirectorioAgregar(Directorio* directorio) {
	listaAgregarElemento(listaDirectorios, directorio);
}

/*
void directorioGuardarEnArchivo(Archivo unArchivo, Directorio unDirectorio) {
   fwrite(&unDirectorio, 1, sizeof(Directorio), unArchivo);
}
*/

void directorioPosicionarEnRegistro(Archivo archivo, int posicion) {
   fseek(archivo, posicion*sizeof(Directorio),SEEK_SET);
}

long directorioCantidadRegistros(Archivo archivo) {
   long curr=ftell(archivo);
   fseek(archivo,0,SEEK_END);
   long ultimo=ftell(archivo);
   fseek(archivo,curr,SEEK_SET);
   return ultimo/sizeof(Directorio);
}

long directorioObtenerPosicionActualArchivo(Archivo archivo) {
   return ftell(archivo)/sizeof(Directorio);
}

//Bitmap cabecita, nico estaria orgulloso
int directorioBuscarIndiceLibre() {
	int indice;
	for(indice = 0; indice < 100 && indiceDirectorios[indice] == 1; indice++);
	if (indice < 100)
		return indice;
	else
		return ERROR;
}


void directorioPersistir(Directorio directorio){
	Archivo archivoDirectorio = archivoAbrir("Directorios.dat","a+");
	String indice = stringConvertirEntero(directorio.indice);
	String padre = stringConvertirEntero(directorio.padre);
	String buffer = stringCrear();
	stringConcatenar(&buffer,indice);
	stringConcatenar(&buffer, ";");
	stringConcatenar(&buffer,directorio.nombre);
	stringConcatenar(&buffer,";");
	stringConcatenar(&buffer,padre);
	fprintf(archivoDirectorio,"%s", buffer);
	archivoCerrar(archivoDirectorio);
	free(indice);
	free(padre);
}

int directorioBuscarEnLista(String nombreDirectorioABuscar, int idPadre) {
	Directorio* directorio;
	int indiceDirectorio  = ERROR;
	int tamanioLista = listaCantidadElementos(listaDirectorios);
	int indice = 0;
	while (indiceDirectorio == ERROR && indice < tamanioLista) {
		directorio = listaObtenerElemento(listaDirectorios, indice);
		 //Busco en la lista de directorios el nombre
		if (stringIguales(directorio->nombre, nombreDirectorioABuscar)) {
			 //Veo si tiene el mismo padre, puede que tenga el mismo nombre pero este en otra carpeta
			if (directorio->padre == idPadre)
				indiceDirectorio = directorio->indice;
		}
		indice++;
	}
	return indiceDirectorio;
}


/*
void listarDirectoriosCreadosRecursiva(int id, char path[200]){
	t_list* listaHijos = list_create();
	listaHijos = obtenerHijos(id);
	int cantidadHijos = list_size(listaHijos);
	int i;
	char auxPath[200];
	if(cantidadHijos == 0){
		//imprimo
		printf("%s \n" , path);
	}
	else{
		//para cada hijo vuelvo a llamar a esta funcion
		for (i = 0; i < cantidadHijos; i++){
			memset(auxPath, '\0', 200);
			//Genero String Auxiliar
			strcpy(auxPath, path);
			//obtengo directorio
			t_dir* nodoHijo = list_get(listaHijos,i);
			//concateno
			strcat(auxPath,"/");
			strcat(auxPath,nodoHijo->nombre);
			listarDirectoriosCreadosRecursiva(nodoHijo->id, auxPath);
		}
	}
}


void listarDirectoriosCreados(){
	int cantDirectorios;
	cantDirectorios = list_size(directorios);
	char path[200];
	int i;
	int cantidadHijosDeRaiz = 0;;
	if (cantDirectorios > 0){
		t_list* listaHijosDeRaiz =list_create();
		listaHijosDeRaiz = obtenerHijos(0);
		printf ("Listado de directorios en MDFS\n\n");
		cantidadHijosDeRaiz = list_size(listaHijosDeRaiz);
		for (i = 0; i < cantidadHijosDeRaiz; i++){
			memset(path,'\0',200);
			//Seteo la / inicial de raiz
			strcpy(path,"/");
			t_dir* nodoHijo = list_get(listaHijosDeRaiz,i);
			//concateno nombre hijo
			strcat(path,nodoHijo->nombre);
			listarDirectoriosCreadosRecursiva(nodoHijo->id, path);
		}
	}
	else printf ("No hay directorios creados\n");
}
*/

//Hay que ponerlo lindo
void directorioCrear(String path) {
	int idPadre;
	String* directorioNuevo;
	Directorio* directorio;
	int cantDirACrear = 0;
	int indiceEncontrado;
	imprimirMensajeUno(archivoLog, "Creando el directorio %s",path);
	if (stringIguales(path,"/")){
		imprimirMensajeUno(archivoLog, "No se pudo el directorio %s",path);
		return;
	}
	directorioNuevo = string_split((char*) path, "/"); //Devuelve un array del path del directorio a crear
	int indiceVectorDirNuevo = 0; //empiezo por el primero del split
	while (directorioNuevo[indiceVectorDirNuevo] != NULL) {
		if (indiceVectorDirNuevo == 0) //el primero del split siempre va a ser hijo de raiz
			idPadre = 0;
		indiceEncontrado = directorioBuscarEnLista(directorioNuevo[indiceVectorDirNuevo], idPadre);
		if (indiceEncontrado != ERROR) { //quiere decir que existe
			if (directorioNuevo[indiceVectorDirNuevo + 1] == NULL) {
				imprimirMensaje(archivoLog, "El directorio ya existe");
				//listarDirectoriosCreados();
			} else
				idPadre = indiceEncontrado;
			indiceVectorDirNuevo++;
		} else { //hay que crear directorio
			int indiceDirectoriosNuevos;
			for (indiceDirectoriosNuevos = indiceVectorDirNuevo;directorioNuevo[indiceDirectoriosNuevos] != NULL;indiceDirectoriosNuevos++) {
				cantDirACrear++;
			}
			if (cantDirACrear <= directoriosDisponibles) { //controlo que no supere la cantidad maxima que es 1024
				while (directorioNuevo[indiceVectorDirNuevo] != NULL) {
					directorio = memoriaAlocar(sizeof(Directorio));
					stringCopiar(directorio->nombre, directorioNuevo[indiceVectorDirNuevo]);
					directorio->padre = idPadre;
					int id = directorioBuscarIndiceLibre(indiceDirectorios); //el nuevo id será el menor libre del vector de indices de directorios, siempre menor a 1024
					directorio->indice = id;
					indiceDirectorios[id] = 1; //marco como ocupado el indice correspondiente
					directoriosDisponibles--; //actualizo mi variable para saber cantidad de directorios máximos a crear
					idPadre = directorio->indice;
					listaAgregarElemento(listaDirectorios, directorio);
					indiceVectorDirNuevo++;
					directorioPersistir(*directorio);
				}
				imprimirMensajeUno(archivoLog, "El directorio %s se creo correctamente", path);
				//listarDirectoriosCreados();
			} else {
				imprimirMensaje(archivoLog,"No se pudo crear el directorio por superar el limite permitido");
				//No puede pasarse de 1024 directorios
				//listarDirectoriosCreados();
			}
		}
	}
}

