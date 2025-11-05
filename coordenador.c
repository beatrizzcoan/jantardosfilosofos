#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include "common.h"

#define TEMPO_JOGO 180

Filosofo filos[MAX_FILOS];
Garfo garfos[MAX_FILOS];
int N_FILOS;
int bonus_reflexao = 1;
int penaliza_inatividade = 1;
time_t inicio;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
FILE *log_moedas;

// ---------------------------------------------------
void log_moeda(const char *nome, int valor, const char *motivo) {
    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%H:%M:%S", tm_info);

    // grava no arquivo de log
    fprintf(log_moedas, "[%s] %s: %+d moeda (%s)\n", tbuf, nome, valor, motivo);
    fflush(log_moedas);

    // imprime no terminal com cor
    if (valor > 0)
        printf("\033[1;32m[%s] %s: +%d moeda (%s)\033[0m\n", tbuf, nome, valor, motivo);
    else
        printf("\033[1;31m[%s] %s: %d moeda (%s)\033[0m\n", tbuf, nome, valor, motivo);
    fflush(stdout);
}

// ---------------------------------------------------
void inicializa_garfos() {
    for (int i = 0; i < MAX_FILOS; i++) {
        garfos[i].livre = 1;
        garfos[i].dono = -1;
    }
}

void inicializa_filos() {
    for (int i = 0; i < MAX_FILOS; i++) {
        filos[i].ativo = 0;
        strcpy(filos[i].estado, "pensando");
        filos[i].moedas = 0;
        filos[i].erros = 0;
        filos[i].tempo_pensando = 0;
    }
}

// ---------------------------------------------------
void imprime_placar() {
    system("clear");
    time_t agora = time(NULL);
    long tempo_decorrido = (inicio > 0) ? (agora - inicio) : 0;
    printf("=== JANTAR DOS FILÓSOFOS v1.6.4 ===\n");
    printf("Registrados: %d/%d | Tempo: %lds/%d\n",
           N_FILOS, N_FILOS, tempo_decorrido, TEMPO_JOGO);
    printf("+----+-------------+L+R+Moedas+Err+Estado------+\n");
    for (int i = 0; i < N_FILOS; i++) {
        if (!filos[i].ativo) continue;

        // Garante que quem não tem garfos volta a "pensar"
        if (!filos[i].garfoL && !filos[i].garfoR)
            strcpy(filos[i].estado, "pensando");

        printf("| %2d |%-12s|%c|%c|%7d|%3d|%-10s|\n",
               i, filos[i].nome,
               filos[i].garfoL ? 'L' : '-',
               filos[i].garfoR ? 'R' : '-',
               filos[i].moedas,
               filos[i].erros,
               filos[i].estado);
    }
    printf("+----+-------------+L+R+Moedas+Err+Estado------+\n");
}

// ---------------------------------------------------
void *placar_thread(void *arg) {
    while (1) {
        sleep(2);
        pthread_mutex_lock(&lock);
        imprime_placar();

        if (inicio == 0) { // jogo ainda não começou
            pthread_mutex_unlock(&lock);
            continue;
        }

        time_t agora = time(NULL);
        for (int i = 0; i < N_FILOS; i++) {
            if (!filos[i].ativo) continue;

            // penalidade por inatividade
            if (penaliza_inatividade && difftime(agora, filos[i].ultima_acao) > 15.0) {
                int esq = filos[i].viz_esq;
                int dir = filos[i].viz_dir;
                if (esq != i && dir != i) {
                    filos[esq].moedas++;
                    filos[dir].moedas++;
                    log_moeda(filos[esq].nome, +1, "inatividade vizinho");
                    log_moeda(filos[dir].nome, +1, "inatividade vizinho");
                }
                filos[i].ultima_acao = agora;
            }

            // bonificação por reflexão (sem garfos)
            if (bonus_reflexao && !filos[i].garfoL && !filos[i].garfoR) {
                filos[i].tempo_pensando += 2;
                if (filos[i].tempo_pensando >= 10) {
                    filos[i].moedas++;
                    log_moeda(filos[i].nome, +1, "reflexão serena");
                    filos[i].tempo_pensando = 0;
                }
            } else {
                filos[i].tempo_pensando = 0;
            }
        }

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// ---------------------------------------------------
void *atende_filosofo(void *arg) {
    int id = *(int *)arg;
    int sock = filos[id].sock;
    char buf[MAXBUF];

    // --- Handshake inicial ---
    int n = recv(sock, buf, MAXBUF, 0);
    if (n > 0) {
        buf[n] = '\0';
        if (strncmp(buf, "REGISTRAR", 9) == 0) {
            char nome[32];
            sscanf(buf, "REGISTRAR %31s", nome);
            strcpy(filos[id].nome, nome);
            char resp[128];
            sprintf(resp, "OK id=%d viz_esq=%d viz_dir=%d\n", id,
                    filos[id].viz_esq, filos[id].viz_dir);
            send(sock, resp, strlen(resp), 0);
        } else {
            send(sock, "ERRO registro\n", 14, 0);
        }
    }

    // Espera READY
    n = recv(sock, buf, MAXBUF, 0);
    if (n > 0) {
        buf[n] = '\0';
        if (strncmp(buf, "READY", 5) == 0)
            send(sock, "OK READY\n", 9, 0);
        else
            send(sock, "ERRO esperado_READY\n", 21, 0);
    }

    // --- Loop principal ---
    while (1) {
        int n = recv(sock, buf, MAXBUF, 0);
        if (n <= 0) break;
        buf[n] = '\0';
        filos[id].ultima_acao = time(NULL);

        if (strncmp(buf, "PEDIR L", 7) == 0) {
            int idx = id;
            if (garfos[idx].livre) {
                garfos[idx].livre = 0;
                garfos[idx].dono = id;
                filos[id].garfoL = 1;
                send(sock, "OK\n", 3, 0);
            } else {
                send(sock, "WAIT\n", 5, 0);
            }
        }

        else if (strncmp(buf, "PEDIR R", 7) == 0) {
            int idx = (id + 1) % N_FILOS;
            if (garfos[idx].livre) {
                garfos[idx].livre = 0;
                garfos[idx].dono = id;
                filos[id].garfoR = 1;
                send(sock, "OK\n", 3, 0);
            } else {
                send(sock, "WAIT\n", 5, 0);
            }
        }

        else if (strncmp(buf, "LARGAR L", 8) == 0) {
            int idx = id;
            if (filos[id].garfoL) {
                garfos[idx].livre = 1;
                garfos[idx].dono = -1;
                filos[id].garfoL = 0;
                strcpy(filos[id].estado, "pensando");
                send(sock, "OK\n", 3, 0);
            } else send(sock, "ERRO nao_tem_L\n", 15, 0);
        }

        else if (strncmp(buf, "LARGAR R", 8) == 0) {
            int idx = (id + 1) % N_FILOS;
            if (filos[id].garfoR) {
                garfos[idx].livre = 1;
                garfos[idx].dono = -1;
                filos[id].garfoR = 0;
                strcpy(filos[id].estado, "pensando");
                send(sock, "OK\n", 3, 0);
            } else send(sock, "ERRO nao_tem_R\n", 15, 0);
        }

        else if (strncmp(buf, "PEDIR_COMER", 11) == 0) {
            if (filos[id].garfoL && filos[id].garfoR) {
                send(sock, "OK comendo\n", 11, 0);
                filos[id].moedas += 2;
                strcpy(filos[id].estado, "comendo");
                log_moeda(filos[id].nome, +2, "comendo");
            } else {
                send(sock, "ERRO falta_garfo\n", 17, 0);
                filos[id].erros++;
                int esq = filos[id].viz_esq;
                int dir = filos[id].viz_dir;
                filos[esq].moedas++;
                filos[dir].moedas++;
                log_moeda(filos[esq].nome, +1, "erro vizinho");
                log_moeda(filos[dir].nome, +1, "erro vizinho");
            }
        }

        else if (strncmp(buf, "STATUS", 6) == 0) {
            char resp[128];
            sprintf(resp, "INFO moedas=%d garfos=%s erros=%d\n",
                    filos[id].moedas,
                    (filos[id].garfoL && filos[id].garfoR) ? "LR" :
                    (filos[id].garfoL ? "L" :
                    (filos[id].garfoR ? "R" : "")),
                    filos[id].erros);
            send(sock, resp, strlen(resp), 0);
        }

        else {
            send(sock, "ERRO comando_invalido\n", 23, 0);
            filos[id].erros++;
            int esq = filos[id].viz_esq;
            int dir = filos[id].viz_dir;
            filos[esq].moedas++;
            filos[dir].moedas++;
            log_moeda(filos[esq].nome, +1, "erro vizinho");
            log_moeda(filos[dir].nome, +1, "erro vizinho");
        }
    }

    close(sock);
    pthread_mutex_lock(&lock);
    filos[id].ativo = 0;
    pthread_mutex_unlock(&lock);
    return NULL;
}

// ---------------------------------------------------
int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <num_filos> [--bonus-reflexao] [--penaliza-inatividade]\n", argv[0]);
        return 1;
    }

    N_FILOS = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--bonus-reflexao") == 0) bonus_reflexao = 1;
        if (strcmp(argv[i], "--no-bonus") == 0) bonus_reflexao = 0;
        if (strcmp(argv[i], "--penaliza-inatividade") == 0) penaliza_inatividade = 1;
        if (strcmp(argv[i], "--no-inatividade") == 0) penaliza_inatividade = 0;
    }

    log_moedas = fopen("log_moedas.txt", "w");
    inicializa_filos();
    inicializa_garfos();

    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(PORTA);
    serv.sin_addr.s_addr = INADDR_ANY;
    bind(server, (struct sockaddr *)&serv, sizeof(serv));
    listen(server, 5);

    printf("Aguardando %d filósofos...\n", N_FILOS);

    pthread_t placar_tid;
    pthread_create(&placar_tid, NULL, placar_thread, NULL);

    int i = 0;
    while (i < N_FILOS) {
        int cli = accept(server, NULL, NULL);
        pthread_mutex_lock(&lock);
        filos[i].sock = cli;
        filos[i].ativo = 1;
        sprintf(filos[i].nome, "team%d", i + 1);
        filos[i].viz_esq = (i - 1 + N_FILOS) % N_FILOS;
        filos[i].viz_dir = (i + 1) % N_FILOS;
        filos[i].ultima_acao = time(NULL);
        pthread_mutex_unlock(&lock);

        pthread_t t;
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&t, NULL, atende_filosofo, id);
        pthread_detach(t);
        i++;
    }

    // Espera todos conectarem e envia START
    sleep(2);
    for (int j = 0; j < N_FILOS; j++) {
        if (filos[j].ativo)
            send(filos[j].sock, "START\n", 6, 0);
    }

    // inicia o tempo real do jogo
    inicio = time(NULL);

    while (time(NULL) - inicio < TEMPO_JOGO)
        sleep(1);

    printf("[FIM] Tempo esgotado (%ds).\n", TEMPO_JOGO);
    fclose(log_moedas);
    close(server);
    return 0;
}

