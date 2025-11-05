#ifndef JANTARDOSFILOSOFOS_COMMON_H
#define JANTARDOSFILOSOFOS_COMMON_H

#include <time.h>

#define MAXBUF 256
#define MAX_FILOS 10
#define PORTA 4040

typedef struct {
    int livre;
    int dono;
} Garfo;

typedef struct {
    int ativo;
    int sock;
    char nome[32];
    int moedas;
    int erros;
    int garfoL, garfoR;
    int viz_esq, viz_dir;
    time_t ultima_acao;
    int tempo_pensando;  // usado na reflexão serena
    char estado[16];
} Filosofo;

#endif
