#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "common.h"

int modo_quiet = 0;  // 0 = verbose, 1 = quiet

// -----------------------------------------------
// Função pensar agora aceita um tempo_fixo
// Se tempo_fixo = 0, usa o tempo aleatório normal (2-5s)
// Se tempo_fixo > 0, usa esse tempo (ex: 11s para reflexão)
void pensar(const char *nome, int tempo_fixo) {
    int t = tempo_fixo;
    if (t == 0) {
        t = rand() % 4 + 2; // 2–5 segundos (comportamento normal)
    }

    if (!modo_quiet)
        printf("Filosofo %s: pensando por %d s...\n", nome, t);
    fflush(stdout);
    sleep(t);
}

// Envia comando, exibe logs e armazena a resposta em 'resp_buf'
// Esta função é crucial para a lógica "prudente", pois permite verificar a resposta ("OK" ou "WAIT") do servidor.
int enviar(int sock, const char *nome, const char *cmd, char *resp_buf, int resp_size) {
    if (!modo_quiet)
        printf("Filosofo %s → envia: %s", nome, cmd);

    // Envia o comando para o servidor
    send(sock, cmd, strlen(cmd), 0);

    // Zera o buffer de resposta antes de receber
    resp_buf[0] = '\0';

    // Recebe a resposta do servidor
    int n = recv(sock, resp_buf, resp_size - 1, 0); // -1 para garantir espaço para o '\0'

    if (n > 0) {
        resp_buf[n] = '\0'; // Adiciona o terminador nulo
        if (!modo_quiet)
            printf("Filosofo %s ← resposta: %s", nome, resp_buf);
    } else {
        // Se houver erro ou desconexão, mantém o buffer vazio
        resp_buf[0] = '\0';
    }
    fflush(stdout);
    return n; // Retorna o número de bytes lidos
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

    // Buffers para enviar comandos e receber respostas
    char cmd_buf[MAXBUF];
    char resp_buf[MAXBUF];

    // --- Handshake ---
    // Faz o registro e espera o 'START' do coordenador
    snprintf(cmd_buf, sizeof(cmd_buf), "REGISTRAR %s\n", nome);
    enviar(sock, nome, cmd_buf, resp_buf, MAXBUF);

    enviar(sock, nome, "READY\n", resp_buf, MAXBUF);
    if (!modo_quiet) printf("Aguardando START...\n");

    int n;
    while ((n = recv(sock, resp_buf, MAXBUF, 0)) > 0) {
        resp_buf[n] = '\0';
        if (strstr(resp_buf, "START")) {
            if (!modo_quiet) printf("==> START recebido! Iniciando ações...\n");
            break;
        }
    }

    // === Loop principal (ESTRATÉGIA REFLEXIVA-PRUDENTE) ===
    while (1) {

        // Chance de 1 em 4 de fazer uma reflexão serena
        if (rand() % 4 == 0) {
            if (!modo_quiet)
                printf("Filosofo %s: Decidi fazer uma reflexão serena...\n", nome);

            // Pensa por 11 segundos para garantir o bônus de 10s
            pensar(nome, 11);

            // Apenas envia um STATUS para registrar atividade e evitar a penalidade de inatividade (15s).
            enviar(sock, nome, "STATUS\n", resp_buf, MAXBUF);
            continue; // Volta ao início do loop sem tentar comer
        }

        // --- Início da Lógica "Prudente" ---

        // Pensa normalmente (tempo 0 = aleatório 2-5s)
        pensar(nome, 0);

        // 1. Tenta pegar o garfo da Esquerda
        enviar(sock, nome, "PEDIR L\n", resp_buf, MAXBUF);

        // Se a resposta NÃO for "OK" (foi "WAIT" ou erro), desiste e volta a pensar.
        if (strstr(resp_buf, "OK") == NULL) {
            if (!modo_quiet)
                printf("Filosofo %s: Garfo L ocupado. Voltando a pensar...\n", nome);
            continue; // Volta ao início do while(1)
        }

        // Conseguiu o Garfo L!
        if (!modo_quiet)
             printf("Filosofo %s: Consegui o garfo L. Tentando o R...\n", nome);

        // Pausa aleatória (100-500ms) para dessincronizar os filósofos e reduzir a chance de todos pedirem R ao mesmo tempo.
        usleep((rand() % 400 + 100) * 1000);

        // 2. Tenta pegar o garfo da Direita
        enviar(sock, nome, "PEDIR R\n", resp_buf, MAXBUF);

        // Se a resposta NÃO for "OK" (foi "WAIT" ou erro)
        if (strstr(resp_buf, "OK") == NULL) {
            // *** A AÇÃO PRUDENTE ***
            // Não conseguiu o R, então LARGA o L que já tinha.
            if (!modo_quiet)
                printf("Filosofo %s: Garfo R ocupado. Largando L e voltando a pensar...\n", nome);

            enviar(sock, nome, "LARGAR L\n", resp_buf, MAXBUF); // Larga o garfo esquerdo
            continue; // Volta ao início do while(1)
        }

        // 3. Conseguiu AMBOS os garfos (L e R)
        if (!modo_quiet)
            printf("Filosofo %s: Consegui ambos os garfos! Comendo...\n", nome);

        enviar(sock, nome, "PEDIR_COMER\n", resp_buf, MAXBUF);

        if (!modo_quiet)
            printf("Filosofo %s: Comendo por 3 s...\n", nome);
        fflush(stdout);
        sleep(3); // Tempo comendo

        // 4. Devolver os garfos
        if (!modo_quiet)
            printf("Filosofo %s: Terminei de comer. Largando os garfos...\n", nome);
        enviar(sock, nome, "LARGAR L\n", resp_buf, MAXBUF);
        enviar(sock, nome, "LARGAR R\n", resp_buf, MAXBUF);

        sleep(1);
    }

    close(sock);
    return 0;
}