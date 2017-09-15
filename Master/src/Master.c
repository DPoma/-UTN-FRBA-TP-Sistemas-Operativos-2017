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

int main(void) {
	system("clear");
	imprimirMensajeProceso("# PROCESO MASTER");
	cargarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	Socket unSocket = socketCrearCliente(configuracion->ipYAMA, configuracion->puertoYAMA);
	printf("Conectado a YAMA en IP: %s | Puerto %s\n", configuracion->ipYAMA, configuracion->puertoYAMA);
	Socket otroSocket = socketCrearCliente("127.0.0.1", "5050");
	printf("Conectado a Master en IP: 127.0.0.1 | Puerto: 5050\n");
	estado = 1;
	char message[1000];
	senialAsignarFuncion(SIGINT, funcionSenial);
	while(estado){
		fgets(message, 1024, stdin);
			if (estado) {
				mensajeEnviar(unSocket, 4, message, strlen(message)+1);
				mensajeEnviar(otroSocket, 4, message, strlen(message)+1);
			}

	}
	close(unSocket);
	return 0;

}


Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipYAMA, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
	strcpy(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("IP YAMA: %s\n", configuracion->ipYAMA);
	printf("Puerto YAMA: %s\n", configuracion->puertoYAMA);
	puts("----------------------------------------------------------------");
}

void cargarCampos() {
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
}

void funcionSenial(int senial) {
	estado = 0;
	puts("");
	imprimirMensajeProceso("# PROCESO MASTER FINALIZADO");
	puts("Aprete enter para salir");
	puts("----------------------------------------------------------------");
}
