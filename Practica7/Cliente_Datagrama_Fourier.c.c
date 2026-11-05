/*
 * Cliente_Datagrama_Fourier.c — Cliente UDP para cálculo distribuido en paralelo
 *
 * Compilación: gcc Cliente_Datagrama_Fourier.c -o cliente -lm
 * Ejecución:   ./cliente <IP_SERVIDOR> <indice 0-9>
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <math.h>

#define PUERTO_SERVIDOR 55555
#define LONGITUD_BUFER 1024
/* Aumentamos el timeout para esperar a que conectes a los demás nodos manualmente */
#define TIMEOUT_SEG 60 

#define A0_1 ((pow(M_PI, 4) / 25) + 1)

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in dir_servidor;
    struct hostent *he;
    char bufer[LONGITUD_BUFER];
    int n_bytes;
    socklen_t len_dir;

    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <hostname/IP> <indice 0-9>\n", argv[0]);
        exit(1);
    }

    int indice = atoi(argv[2]);
    if (indice < 0 || indice > 9)
    {
        fprintf(stderr, "Error: El índice debe ser un valor entre 0 y 9.\n");
        exit(1);
    }

    if ((he = gethostbyname(argv[1])) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    struct timeval tv = {.tv_sec = TIMEOUT_SEG, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_port = htons(PUERTO_SERVIDOR);
    dir_servidor.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(dir_servidor.sin_zero), 8);

    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  CLIENTE FOURIER  —  Índice solicitado: %d     ║\n", indice);
    printf("╚════════════════════════════════════════════════╝\n\n");

    /* ════════════════════════════════════════════════
       PASO 1 — Solicitar registro
       ════════════════════════════════════════════════ */
    snprintf(bufer, LONGITUD_BUFER, "REQ:%d", indice);
    n_bytes = sendto(fd, bufer, strlen(bufer), 0,
                     (struct sockaddr *)&dir_servidor, sizeof(struct sockaddr));
    if (n_bytes == -1)
    {
        perror("sendto (REQ)");
        close(fd);
        exit(1);
    }
    printf("[ → ] Solicitud enviada.\n");
    printf("[ ⌛ ] Esperando a que los demás clientes se conecten al servidor...\n");

    /* ════════════════════════════════════════════════
       PASO 2 — Esperar orden de inicio (Sincronización)
       ════════════════════════════════════════════════ */
    len_dir = sizeof(struct sockaddr);
    // El cliente se quedará bloqueado aquí hasta que el servidor haya recibido los 10 clientes
    n_bytes = recvfrom(fd, bufer, LONGITUD_BUFER - 1, 0,
                       (struct sockaddr *)&dir_servidor, &len_dir);
    if (n_bytes == -1)
    {
        fprintf(stderr, "\n[ ✗ ] Timeout. Pasaron %d segs sin que el servidor iniciara.\n", TIMEOUT_SEG);
        close(fd);
        exit(1);
    }
    bufer[n_bytes] = '\0';

    if (strncmp(bufer, "HAR:", 4) != 0)
    {
        fprintf(stderr, "[ ✗ ] Respuesta inesperada del servidor: \"%s\"\n", bufer);
        close(fd);
        exit(1);
    }

    int n;
    float x;
    if (sscanf(bufer + 4, "%d,%f", &n, &x) != 2)
    {
        fprintf(stderr, "[ ✗ ] Error al parsear los parámetros del armónico.\n");
        close(fd);
        exit(1);
    }
    printf("\n[ ← ] INICIO SIMULTÁNEO: n = %d, x = %.2f\n", n, x);

    /* ════════════════════════════════════════════════
       PASO 3 — Calcular
       ════════════════════════════════════════════════ */
    float AN = (2 * (((pow(M_PI, 4) + 5) * pow(n, 4) - 12 * pow(M_PI, 2) * pow(n, 2) + 24) * sin(M_PI * n))
                + 4 * M_PI * n * (pow(M_PI, 2) * pow(n, 2) - 6) * cos(M_PI * n))
               / (5 * M_PI * pow(n, 5));
    float BN = 0;
    float resultado = (AN * cos(n * x)) + (BN * sin(n * x));

    printf("  ┌──────────────────────────────────────────────────┐\n");
    printf("  │  Resultado del Armónico n=%-2d = %-18.6f │\n", n, resultado);
    printf("  └──────────────────────────────────────────────────┘\n\n");

    /* ════════════════════════════════════════════════
       PASO 4 — Enviar resultado y su índice
       ════════════════════════════════════════════════ */
    // Formato modificado para que el servidor sepa de qué índice es este resultado
    snprintf(bufer, LONGITUD_BUFER, "RES:%d,%f", indice, resultado);
    n_bytes = sendto(fd, bufer, strlen(bufer), 0,
                     (struct sockaddr *)&dir_servidor, sizeof(struct sockaddr));
    if (n_bytes == -1)
    {
        perror("sendto (RES)");
        close(fd);
        exit(1);
    }
    printf("[ → ] Resultado entregado al servidor.\n");

    close(fd);
    return 0;
}