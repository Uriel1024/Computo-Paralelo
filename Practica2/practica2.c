#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define FILAS 3
#define COLUMNAS 9

int main() {
    // 1. El proceso padre crea/limpia el archivo donde se guardarán los resultados
    FILE *res_crear = fopen("resultados.txt", "w");
    if (res_crear == NULL) {
        perror("Error al crear resultados.txt");
        return 1;
    }
    fclose(res_crear);

    // 2 y 3. El proceso padre crea un proceso hijo por cada columna
    //        y le informa qué columna le toca a través de la variable 'col'
    for (int col = 0; col < COLUMNAS; col++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error al crear el proceso hijo");
            exit(1);
        }

        if (pid == 0) {
            // --- CÓDIGO DEL PROCESO HIJO ---
            
            // 4. Cada hijo abre el archivo y extrae la matriz
            FILE *archivo = fopen("matriz.txt", "r");
            if (archivo == NULL) {
                perror("Hijo: Error al abrir matriz.txt");
                exit(1);
            }

            int matriz[FILAS][COLUMNAS];
            for (int i = 0; i < FILAS; i++) {
                for (int j = 0; j < COLUMNAS; j++) {
                    if (fscanf(archivo, "%d", &matriz[i][j]) != 1) {
                        fclose(archivo);
                        exit(1);
                    }
                }
            }
            fclose(archivo);

            // 5. Multiplicar los elementos de la columna asignada
            long long producto = 1;
            for (int i = 0; i < FILAS; i++) {
                producto *= matriz[i][col];
            }

            // Guardar el resultado en el archivo de resultados
            FILE *resultados = fopen("resultados.txt", "a");
            if (resultados != NULL) {
                fprintf(resultados, "Columna %d (PID %d): Producto = %lld\n", col, getpid(), producto);
                fclose(resultados);
            }

            // El hijo termina su ejecución
            exit(0);
        }
    }

    // --- CÓDIGO DEL PROCESO PADRE ---

    // 6. El padre espera a que terminen todos los procesos hijos
    for (int i = 0; i < COLUMNAS; i++) {
        wait(NULL);
    }

    // El padre abre y muestra el archivo de resultados generado
    printf("\n=== RESULTADOS FINALES (Leidos por el Padre) ===\n");
    FILE *res_lectura = fopen("resultados.txt", "r");
    if (res_lectura == NULL) {
        perror("Error al leer resultados.txt");
        return 1;
    }

    char linea[256];
    while (fgets(linea, sizeof(linea), res_lectura) != NULL) {
        printf("%s", linea);
    }
    fclose(res_lectura);

    return 0;
}