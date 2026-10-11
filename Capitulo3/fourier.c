#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#define N 100            // Número de procesos
#define SALTO 0.1        // Salto entre cada X
#define COLUMNAS 65      // Valores entre -PI y PI
#define MAX_CONCURRENTE 8 // Procesos simultáneos permitidos

// Prototipo de la función
float funcion(float x, int n, int caso);

float funcion(float x, int n, int caso) {
    switch (caso) {
    case 1: {
        float AN = (2 * (((pow(M_PI, 4) + 5) * pow(n, 4) - 12 * pow(M_PI, 2) * pow(n, 2) + 24) * sin(M_PI * n))
                    + 4 * M_PI * n * (pow(M_PI, 2) * pow(n, 2) - 6) * cos(M_PI * n))
                   / (5 * M_PI * pow(n, 5));
        float BN = 0;
        return (AN * cos(n * x)) + (BN * sin(n * x));
    }
    case 2:
        return (-1.0f / 15.0f) * (1.0f / pow(n, 3) * (((5.0f * pow(M_PI * n, 2)) - (6.0f * pow(n, 2)) - 30.0f) * pow(-1, n)) * sin(n * x));
    case 3:
        return -(4.0f / 3.0f) * ((pow(M_PI * n, 2) - 6.0f) * pow(-1, n)) * (1.0f / pow(n, 4)) * cos(n * x);
    case 4:
        return (2.0f * pow(-1, n) * ((1.0f - pow(M_PI, 2)) * pow(n, 2) + 6.0f) * sin(n * x)) / pow(n, 3);
    default:
        return -1.0f;
    }
}

void calcular_fourier(float A0, char nombre[], int caso) {
    char archivo[50];
    snprintf(archivo, sizeof(archivo), "resultados_%s.txt", nombre);

    size_t shm_size = sizeof(float) * N * COLUMNAS;
    float (*resultados)[COLUMNAS] = mmap(
        NULL,
        shm_size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    sem_t *semaforo = mmap(
        NULL,
        sizeof(sem_t),
        PROT_READ | PROT_WRITE,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0
    );

    if (semaforo == MAP_FAILED || resultados == MAP_FAILED) {
        perror("Error en mmap");
        exit(1);
    }

    sem_init(semaforo, 1, MAX_CONCURRENTE);

    for (int i = 1; i <= N; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("Error al crear el proceso hijo");
            exit(1);
        }

        if (pid == 0) {
            sem_wait(semaforo);

            int j = 0;
            float x = -M_PI;
            while (x < M_PI + 0.1 && j < COLUMNAS) {
                resultados[i - 1][j] = funcion(x, i, caso);
                x += SALTO;
                j++;
            }

            sem_post(semaforo);
            exit(0);
        }
    }

    for (int i = 0; i < N; i++) {
        wait(NULL);
    }

    float Fourier[COLUMNAS];
    for (int x = 0; x < COLUMNAS; x++) {
        float sum_n = 0;
        for (int n = 0; n < N; n++) {
            sum_n += resultados[n][x];
        }
        Fourier[x] = A0 + sum_n;
    }

    FILE *res_escribir = fopen(archivo, "w");
    if (res_escribir != NULL) {
        for (int x = 0; x < COLUMNAS - 1; x++) {
            fprintf(res_escribir, "%f\n", Fourier[x]);
        }
        fclose(res_escribir);
    }

    sem_destroy(semaforo);
    munmap(semaforo, sizeof(sem_t));
    munmap(resultados, shm_size);
}

int main() {
    char nombres[4][50] = {
        "ernesto",
        "uriel",
        "omar",
        "diego"
    };

    float A0_ernesto = (pow(M_PI, 4) / 25) + 1;
    float A0_uriel = 0.0f;
    float A0_omar = -(pow(M_PI, 4) - 150.0f) * (1.0f / 30.0f);
    float A0_diego = 3.0f;
    float A0[4] = {A0_ernesto, A0_uriel, A0_omar, A0_diego};

    for (int i = 0; i < 4; i++) {
        calcular_fourier(A0[i], nombres[i], i + 1);
    }

    FILE *csv = fopen("resultados_fourier.csv", "w");
    if (csv == NULL) {
        perror("Error al crear el CSV");
        return 1;
    }

    fprintf(csv, "x,ernesto,uriel,omar,diego\n");

    FILE *archivos[4];
    for (int i = 0; i < 4; i++) {
        char nombre_archivo[50];
        snprintf(nombre_archivo, sizeof(nombre_archivo), "resultados_%s.txt", nombres[i]);
        archivos[i] = fopen(nombre_archivo, "r");
        if (archivos[i] == NULL) {
            perror("Error al abrir archivo de resultados");
            fclose(csv);
            return 1;
        }
    }

    for (int i = 0; i < COLUMNAS - 1; i++) {
        float res[4];
        for (int j = 0; j < 4; j++) {
            fscanf(archivos[j], "%f", &res[j]);
        }
        float x = -M_PI + (i * SALTO);
        fprintf(csv, "%f,%f,%f,%f,%f\n", x, res[0], res[1], res[2], res[3]);
    }

    fclose(csv);

    for (int i = 0; i < 4; i++) {
        fclose(archivos[i]);
    }

    return 0;
} 