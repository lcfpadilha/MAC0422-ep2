/*******************************************************************/
/*  EP1 de MAC0422 - Sistemas Operacionais                         */
/*                                                                 */
/*  Simulador de uma corrida - ep2.c                               */
/*                                                                 */         
/*  Autores: Gustavo Silva e Leonardo Padilha                      */
/*                                                                 */             
/*******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define TRUE  1
#define FALSE 0

/* Structs usadas */
typedef struct cyc {
    int vel;
    int team;
    float pos;
    pthread_t id;
} CYCLIST;

typedef struct arguments {
    int i;
} ARGS;

/* Declaração de funções        */
void *func (void *a);

/*      Variáveis globais       */
int** track;
CYCLIST* c;

int main (int argc, char **argv) {
    int n, d, debug, i;
    char type;
    ARGS* thread_arg;
    if (argc < 4) exit (EXIT_FAILURE);

    /* Inicialização das variáveis dadas na linha de comando*/
    n = atoi (argv[1]);
    d = atoi (argv[2]);
    type = argv[3][0];
    if (argc > 4 && strcmp (argv[4], "-d") == 0)
        debug = TRUE; 
    else
        debug = FALSE;

    /* Modo básico */
    if (type == 'v') {
        /* Inicializando os 2xN ciclistas */
        c = malloc (2 * n * sizeof (CYCLIST));
        for (i = 0; i < n; i++) {
            c[i].vel = 60;
            c[i].team = 0;
            c[i].pos = 0.0;
        }
        for (i = n; i < 2 * n; i++) {
            c[i].vel = 60;
            c[i].team = 1;
            c[i].pos = (float) (d / 2);
        }

        /* Inicializando o vetor track */
        track = malloc (d * sizeof (int *));
        for (i = 0; i < d; i++) {
            track[i] = malloc (2 * sizeof (int));
            track[i][0] = 0;
            track[i][1] = 0;
        }

        /* Disparando as threads  */
        for (i = 0; i < 2 * n; i++) {
            thread_arg = malloc (sizeof (ARGS));
            thread_arg->i = i;
            pthread_create (&c[i].id, NULL, &func, thread_arg);
        }

        /* Esperando retornar*/
        for (i = 0; i < 2 * n; i++)
            pthread_join (c[i].id, NULL);
    }
    return 0;
}

void *func (void *a) {
    int i;
    ARGS* p = (ARGS *) a;
    i = p->i;
    while (TRUE) {
        /*P(FLAG)*/
        /*SE FLAG FOR 1, PARA TUDO*/
        /*V(FLAG)*/

        /*CALCULA A POSIÇÃO*/
        /*P(TRACK)*/
        /*SE TRACK[POS] != 0 ATUALIZA TUDO*/
        /*v(TRACK)*/

        /*BARREIRA DE SINCRONIZAÇAO*/
        /*SE LAP == 16, FLAG = 1 */
    }

    return NULL;
}