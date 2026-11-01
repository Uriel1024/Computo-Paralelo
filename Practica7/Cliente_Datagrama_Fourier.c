/*
 * Cliente_Datagrama_Fourier.c — Cliente UDP para cálculo distribuido de un armónico
 *
 * Compilación: gcc Cliente_Datagrama_Fourier.c -o cliente -lm
 * Ejecución:   ./cliente <IP_SERVIDOR> <indice 0-9>
 *
 * Ejemplo (en 10 PCs distintas en paralelo):
 *   PC 1:  ./cliente 192.168.1.10 0   (Calcula armónico n=1)
 *   PC 2:  ./cliente 192.168.1.10 1   (Calcula armónico n=2)
 *   ...
 *   PC 10: ./cliente 192.168.1.10 9   (Calcula armónico n=10)
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
#define TIMEOUT_SEG 5

#define A0_1 ((pow(M_PI, 4) / 25) + 1)

int main(int argc, char *argv[])
{
    int fd;
    struct sockaddr_in dir_servidor;
    struct hostent *he;
    char bufer[LONGITUD_BUFER];
    int n_bytes;
    socklen_t len_dir;

    /* ── Validar argumentos ── */
    if (argc != 3)
    {
        fprintf(stderr, "Uso: %s <hostname/IP> <indice 0-9>\n", argv[0]);
        fprintf(stderr, "  Ej: %s 192.168.1.10 0\n", argv[0]);
        exit(1);
    }

    int indice = atoi(argv[2]);
    if (indice < 0 || indice > 9)
    {
        fprintf(stderr, "Error: El índice debe ser un valor entre 0 y 9.\n");
        exit(1);
    }

    /* ── Resolver IP del servidor ── */
    if ((he = gethostbyname(argv[1])) == NULL)
    {
        herror("gethostbyname");
        exit(1);
    }

    /* ── Crear socket UDP ── */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* ── Timeout de recepción ── */
    struct timeval tv = {.tv_sec = TIMEOUT_SEG, .tv_usec = 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    dir_servidor.sin_family = AF_INET;
    dir_servidor.sin_port = htons(PUERTO_SERVIDOR);
    dir_servidor.sin_addr = *((struct in_addr *)he->h_addr);
    bzero(&(dir_servidor.sin_zero), 8);

    printf("╔════════════════════════════════════════════════╗\n");
    printf("║  CLIENTE FOURIER  —  Índice solicitado: %d     ║\n", indice);
    printf("╚════════════════════════════════════════════════╝\n\n");
    printf("Servidor: %s:%d\n\n", inet_ntoa(dir_servidor.sin_addr), PUERTO_SERVIDOR);

    /* ══════════════════════════════════════
       PASO 1 — Solicitar el índice al servidor
       ══════════════════════════════════════ */
    snprintf(bufer, LONGITUD_BUFER, "REQ:%d", indice);
    n_bytes = sendto(fd, bufer, strlen(bufer), 0,
                     (struct sockaddr *)&dir_servidor, sizeof(struct sockaddr));
    if (n_bytes == -1)
    {
        perror("sendto (REQ)");
        close(fd);
        exit(1);
    }
    printf("[ → ] Solicitud enviada para índice %d (Armónico n = %d)\n", indice, indice + 1);

    /* ══════════════════════════════════════
       PASO 2 — Recibir los parámetros (n y x)
       ══════════════════════════════════════ */
    len_dir = sizeof(struct sockaddr);
    n_bytes = recvfrom(fd, bufer, LONGITUD_BUFER - 1, 0,
                       (struct sockaddr *)&dir_servidor, &len_dir);
    if (n_bytes == -1)
    {
        fprintf(stderr, "[ ✗ ] Tiempo de espera agotado. ¿El servidor está activo?\n");
        close(fd);
        exit(1);
    }
    bufer[n_bytes] = '\0';

    if (strncmp(bufer, "ERR:", 4) == 0)
    {
        fprintf(stderr, "[ ✗ ] Error del servidor: %s\n", bufer + 4);
        close(fd);
        exit(1);
    }

    if (strncmp(bufer, "HAR:", 4) != 0)
    {
        fprintf(stderr, "[ ✗ ] Respuesta inesperada: \"%s\"\n", bufer);
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
    printf("[ ← ] Parámetros recibidos: n = %d, x = %.2f\n", n, x);

    /* ══════════════════════════════════════
       PASO 3 — Calcular el armónico de Fourier
       ══════════════════════════════════════ */
    float AN = (2 * (((pow(M_PI, 4) + 5) * pow(n, 4) - 12 * pow(M_PI, 2) * pow(n, 2) + 24) * sin(M_PI * n))
                + 4 * M_PI * n * (pow(M_PI, 2) * pow(n, 2) - 6) * cos(M_PI * n))
               / (5 * M_PI * pow(n, 5));
    float BN = 0;
    float resultado = (AN * cos(n * x)) + (BN * sin(n * x));

    if (n == 1)
    {
        printf("  [i] Nota: Término constante A0_1 asociado = %f\n", A0_1);
    }

    printf("\n");
    printf("  ┌──────────────────────────────────────────────────┐\n");
    printf("  │  Resultado del Armónico n=%-2d = %-18.6f │\n", n, resultado);
    printf("  └──────────────────────────────────────────────────┘\n\n");

    /* ══════════════════════════════════════
       PASO 4 — Enviar el resultado al servidor
       ══════════════════════════════════════ */
    snprintf(bufer, LONGITUD_BUFER, "RES:%f", resultado);
    n_bytes = sendto(fd, bufer, strlen(bufer), 0,
                     (struct sockaddr *)&dir_servidor, sizeof(struct sockaddr));
    if (n_bytes == -1)
    {
        perror("sendto (RES)");
        close(fd);
        exit(1);
    }
    printf("[ → ] Resultado enviado al servidor con éxito.\n");

    close(fd);
    return 0;
}