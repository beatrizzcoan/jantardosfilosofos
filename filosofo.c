#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "common.h"

int modo_quiet = 0;  // 0 = verbose, 1 = quiet

// -----------------------------------------------
void pensar(const char *nome) {
    int t = rand() % 4 + 2;  // 2–5 segundos
    if (!modo_quiet)
        printf("Filosofo %s: pensando por %d s...\n", nome, t);
    fflush(stdout);
    sleep(t);
}

int possui_ambos(const char *buf) {
    return strstr(buf, "garfos=LR") != NULL;
}

// Envia comando e exibe logs
void enviar(int sock, const char *nome, const char *cmd) {
    char buf[MAXBUF];
    if (!modo_quiet)
        printf("Filosofo %s → envia: %s", nome, cmd);

    send(sock, cmd, strlen(cmd), 0);

    int n = recv(sock, buf, MAXBUF, 0);
    if (n > 0) {
        buf[n] = '\0';
        if (!modo_quiet)
            printf("Filosofo %s ← resposta: %s", nome, buf);
    }
    fflush(stdout);
}

// -----------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <ip_coordenador> <nome> [-q]\n", argv[0]);
        return 1;
    }

    if (argc == 4 && strcmp(argv[3], "-q") == 0)
        modo_quiet = 1;

    srand(time(NULL) ^ getpid());
    const char *ip = argv[1];
    const char *nome = argv[2];

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("socket"); return 1; }

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORTA);
    inet_pton(AF_INET, ip, &serv.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("Erro ao conectar");
        return 1;
    }

    char buf[MAXBUF];
    snprintf(buf, sizeof(buf), "REGISTRAR %s\n", nome);
    enviar(sock, nome, buf);

    enviar(sock, nome, "READY\n");
    if (!modo_quiet) printf("Aguardando START...\n");

    int n;
    while ((n = recv(sock, buf, MAXBUF, 0)) > 0) {
        buf[n] = '\0';
        if (strstr(buf, "START")) {
            if (!modo_quiet) printf("==> START recebido! Iniciando ações...\n");
            break;
        }
    }

    // === Loop principal ===
    while (1) {
        pensar(nome);

        enviar(sock, nome, "PEDIR L\n");
        enviar(sock, nome, "PEDIR R\n");

        enviar(sock, nome, "STATUS\n");  // resposta já lida dentro de enviar()

        // Analisar o último STATUS armazenado
        // (como o coordenador manda resposta curta, podemos pedir novamente para testar)
        send(sock, "STATUS\n", 8, 0);
        n = recv(sock, buf, MAXBUF, 0);
        if (n <= 0) break;
        buf[n] = '\0';

        if (possui_ambos(buf)) {
            if (!modo_quiet)
                printf("Filosofo %s: possui ambos os garfos → vai comer!\n", nome);
            enviar(sock, nome, "PEDIR_COMER\n");
            if (!modo_quiet)
                printf("Filosofo %s: comendo por 3 s...\n", nome);
            fflush(stdout);
            sleep(3);
        } else {
            if (!modo_quiet)
                printf("Filosofo %s: não conseguiu ambos os garfos (esperando...)\n", nome);
        }

        // devolver garfos
        enviar(sock, nome, "LARGAR L\n");
        enviar(sock, nome, "LARGAR R\n");

        sleep(1);
    }

    close(sock);
    return 0;
}

