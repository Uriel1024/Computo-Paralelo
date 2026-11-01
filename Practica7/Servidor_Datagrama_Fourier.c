/*
 * Servidor_Datagrama_Fourier.c — Servidor UDP para cálculo distribuido de Armónicos de Fourier
 *
 * Compilación: gcc Servidor_Datagrama_Fourier.c -o servidor
 * Ejecución:   ./servidor
 *
 * Protocolo de mensajes:
 *   REQ:<indice>   — Cliente solicita el armónico (índice 0-9, equivalente a n = indice + 1)
 *   HAR:<n>,<x>    — Servidor envía el número de armónico n y el valor de x
 *   RES:<valor>    — Cliente devuelve el resultado flotante del armónico calculado
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define PUERTO_SERVIDOR 55555
#define LONGITUD_BUFER 1024
#define TOTAL_HARMONICOS 10

int main(void)
{
    int fd;
    struct sockaddr_in mi_dir, dir_cliente;
    socklen_t len_dir;
    char bufer[LONGITUD_BUFER];
    int n_bytes;

    /* Valor de evaluación x por defecto para la serie de Fourier */
    float x_valor = 1.0f;

    /* ── Crear socket UDP ── */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* ── Configurar dirección del servidor ── */
    mi_dir.sin_family = AF_INET;
    mi_dir.sin_port = htons(PUERTO_SERVIDOR);
    mi_dir.sin_addr.s_addr = INADDR_ANY;
    bzero(&(mi_dir.sin_zero), 8);

    if (bind(fd, (struct sockaddr *)&mi_dir, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    /* ── Pantalla de bienvenida ── */
    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║  SERVIDOR UDP — Armónicos de Fourier (10 PCs)    ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");
    printf(" Valor de evaluación x = %.2f\n", x_valor);
    printf(" Escuchando en el puerto %d...\n\n", PUERTO_SERVIDOR);

    /* ══════════════════════════════════════
       Bucle principal de atención a clientes
       ══════════════════════════════════════ */
    while (1)
    {
        len_dir = sizeof(struct sockaddr);

        n_bytes = recvfrom(fd, bufer, LONGITUD_BUFER - 1, 0,
                           (struct sockaddr *)&dir_cliente, &len_dir);
        if (n_bytes <= 0)
        {
            perror("recvfrom");
            continue;
        }
        bufer[n_bytes] = '\0';

        char *ip = inet_ntoa(dir_cliente.sin_addr);

        /* ────────────────────────────────────
           CASO 1: Solicitud de armónico "REQ:<indice>"
           ──────────────────────────────────── */
        if (strncmp(bufer, "REQ:", 4) == 0)
        {
            int indice = atoi(bufer + 4);
            printf("[REQ]  %s  →  solicita armónico índice %d\n", ip, indice);

            if (indice < 0 || indice >= TOTAL_HARMONICOS)
            {
                const char err[] = "ERR:Índice inválido. Use un valor entre 0 y 9.";
                sendto(fd, err, strlen(err), 0,
                       (struct sockaddr *)&dir_cliente, len_dir);
                printf("[ERR]  Índice fuera de rango: %d\n\n", indice);
            }
            else
            {
                int n = indice + 1; /* Mapeo: índice 0-9 -> armónico n de 1 a 10 */
                snprintf(bufer, LONGITUD_BUFER, "HAR:%d,%.4f", n, x_valor);
                sendto(fd, bufer, strlen(bufer), 0,
                       (struct sockaddr *)&dir_cliente, len_dir);
                printf("[HAR]  Enviado a %s  ←  Armónico n = %d, x = %.2f\n",
                       ip, n, x_valor);
            }

        /* ────────────────────────────────────
           CASO 2: Resultado recibido "RES:<valor>"
           ──────────────────────────────────── */
        }
        else if (strncmp(bufer, "RES:", 4) == 0)
        {
            float resultado = atof(bufer + 4);
            printf("[RES]  %s  →  Resultado del armónico recibido = %f\n\n", ip, resultado);

        /* ────────────────────────────────────
           CASO 3: Mensaje desconocido
           ──────────────────────────────────── */
        }
        else
        {
            printf("[???]  Mensaje desconocido de %s: \"%s\"\n\n", ip, bufer);
        }
    }

    close(fd);
    return 0;
}