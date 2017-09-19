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

FILE* archivoAbrir(char* path) {
	FILE* archivo = fopen(path, "r");
	return archivo;
}

int archivoValido(FILE* archivo) {
	return archivo != NULL;
}

void archivoCerrar(FILE* archivo) {
	fclose(archivo);
}

bool esUnArchivo(char* c) {
	return string_contains(c, ".");
}


void enviarArchivo(FILE* archivo) {
	char* texto = leerArchivo(archivo);
	archivoCerrar(archivo);
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

int main(void) {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO MASTER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Master");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
	stringCopiar(configuracion->ipWorker, IP_LOCAL);
	stringCopiar(configuracion->puertoWorker, "5050");
	imprimirMensajeDos(archivoLog,"[CONEXION] Estableciendo Conexion con YAMA (IP: %s | Puerto %s)", configuracion->ipYAMA, configuracion->puertoYAMA);
	socketYAMA = socketCrearCliente(configuracion->ipYAMA, configuracion->puertoYAMA, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion existosa con YAMA");
	imprimirMensajeDos(archivoLog, "[CONEXION] Estableciendo Conexion con Worker (IP: %s | Puerto: %s)", configuracion->ipWorker, configuracion->puertoWorker);
	socketWorker = socketCrearCliente(configuracion->ipWorker, configuracion->puertoWorker, ID_MASTER);
	imprimirMensaje(archivoLog, "[CONEXION] Estableciendo Conexion con Worker");
	estadoMaster = 1;
	senialAsignarFuncion(SIGINT, funcionSenial);
	while(estadoMaster);
	socketCerrar(socketYAMA);
	socketCerrar(socketWorker);
	return EXIT_SUCCESS;

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipYAMA, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
	stringCopiar(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
}

void funcionSenial(int senial) {
	estadoMaster = 0;
	puts("");
}
