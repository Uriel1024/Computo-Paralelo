/*
 * Servidor_Datagrama_Fourier.c — Servidor UDP para cálculo distribuido en PARALELO
 *
 * Compilación: gcc Servidor_Datagrama_Fourier.c -o servidor
 * Ejecución:   ./servidor
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

    /* Variables para la sincronización y almacenamiento */
    struct sockaddr_in clientes[TOTAL_HARMONICOS];
    int registrados[TOTAL_HARMONICOS] = {0}; // 0 = no, 1 = registrado, 2 = resuelto
    float resultados[TOTAL_HARMONICOS] = {0};
    int clientes_listos = 0;
    int resultados_recibidos = 0;
    float x_valor = 1.0f;

    /* ── Crear socket UDP ── */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    mi_dir.sin_family = AF_INET;
    mi_dir.sin_port = htons(PUERTO_SERVIDOR);
    mi_dir.sin_addr.s_addr = INADDR_ANY;
    bzero(&(mi_dir.sin_zero), 8);

    if (bind(fd, (struct sockaddr *)&mi_dir, sizeof(struct sockaddr)) == -1)
    {
        perror("bind");
        exit(1);
    }

    printf("╔══════════════════════════════════════════════════╗\n");
    printf("║  SERVIDOR UDP — Armónicos en PARALELO (10 PCs)   ║\n");
    printf("╚══════════════════════════════════════════════════╝\n\n");
    printf(" Valor de evaluación x = %.2f\n", x_valor);
    printf(" Escuchando en el puerto %d...\n\n", PUERTO_SERVIDOR);

    /* ════════════════════════════════════════════════
       FASE 1: Registro de clientes (Esperar a los 10)
       ════════════════════════════════════════════════ */
    printf("[*] FASE 1: Esperando la conexión de los %d clientes...\n", TOTAL_HARMONICOS);
    while (clientes_listos < TOTAL_HARMONICOS)
    {
        len_dir = sizeof(struct sockaddr);
        n_bytes = recvfrom(fd, bufer, LONGITUD_BUFER - 1, 0,
                           (struct sockaddr *)&dir_cliente, &len_dir);
        if (n_bytes <= 0) continue;
        bufer[n_bytes] = '\0';

        char *ip = inet_ntoa(dir_cliente.sin_addr);

        if (strncmp(bufer, "REQ:", 4) == 0)
        {
            int indice = atoi(bufer + 4);
            if (indice >= 0 && indice < TOTAL_HARMONICOS)
            {
                if (registrados[indice] == 0)
                {
                    clientes[indice] = dir_cliente;
                    registrados[indice] = 1;
                    clientes_listos++;
                    printf("    [REQ] %s registrado para armónico índice %d (%d/%d)\n", 
                           ip, indice, clientes_listos, TOTAL_HARMONICOS);
                }
            }
            else
            {
                printf("    [ERR] Índice fuera de rango (%d) desde %s\n", indice, ip);
            }
        }
    }

    /* ════════════════════════════════════════════════
       FASE 2: Liberar barrera (Start simultáneo)
       ════════════════════════════════════════════════ */
    printf("\n[*] FASE 2: Todos los clientes conectados. Enviando orden de inicio...\n");
    for (int i = 0; i < TOTAL_HARMONICOS; i++)
    {
        int n = i + 1;
        snprintf(bufer, LONGITUD_BUFER, "HAR:%d,%.4f", n, x_valor);
        sendto(fd, bufer, strlen(bufer), 0,
               (struct sockaddr *)&clientes[i], sizeof(struct sockaddr));
        printf("    [HAR] Orden enviada al índice %d (Armónico %d)\n", i, n);
    }

    /* ════════════════════════════════════════════════
       FASE 3: Recibir y agrupar resultados
       ════════════════════════════════════════════════ */
    printf("\n[*] FASE 3: Esperando resultados en paralelo...\n");
    while (resultados_recibidos < TOTAL_HARMONICOS)
    {
        len_dir = sizeof(struct sockaddr);
        n_bytes = recvfrom(fd, bufer, LONGITUD_BUFER - 1, 0,
                           (struct sockaddr *)&dir_cliente, &len_dir);
        if (n_bytes <= 0) continue;
        bufer[n_bytes] = '\0';

        if (strncmp(bufer, "RES:", 4) == 0)
        {
            int idx;
            float valor;
            // El cliente ahora manda RES:indice,valor
            if (sscanf(bufer + 4, "%d,%f", &idx, &valor) == 2)
            {
                if (idx >= 0 && idx < TOTAL_HARMONICOS && registrados[idx] == 1)
                {
                    resultados[idx] = valor;
                    registrados[idx] = 2; // Marcar como recibido
                    resultados_recibidos++;
                    printf("    [RES] Resultado recibido: Armónico n=%d -> %f (%d/%d)\n", 
                           idx + 1, valor, resultados_recibidos, TOTAL_HARMONICOS);
                }
            }
        }
    }

    printf("\n╔══════════════════════════════════════════════════╗\n");
    printf("║  TODOS LOS ARMÓNICOS CALCULADOS CON ÉXITO        ║\n");
    printf("╚══════════════════════════════════════════════════╝\n");
    for (int i = 0; i < TOTAL_HARMONICOS; i++)
    {
        printf("  Armónico n=%-2d : %-18.6f\n", i + 1, resultados[i]);
    }
    printf("\n");

    close(fd);
    return 0;
}