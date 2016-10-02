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
    int lap;
    float pos;
    pthread_t id;
    int finalTick;
} CYCLIST;

typedef struct sl {
    pthread_t mainpos;
    pthread_t ultrapos;
    pthread_mutex_t mut;
} SLOT;

typedef struct arguments {
    int i;
} ARGS;

/* Declaração de funções        */
void *func (void *a);

/*      Variáveis globais       */
SLOT *pista;
CYCLIST *c;
int cycsize;
pthread_barrier_t barrera;
int raceEnd;
int timeTick;
int d;

int main (int argc, char **argv) {
    int n, debug, i;
    char type;
    ARGS* thread_arg;
    if (argc < 4) exit (EXIT_FAILURE);

    /* Inicialização das variáveis dadas na linha de comando*/
    d = atoi (argv[1]);
    n = atoi (argv[2]);
    type = argv[3][0];
    if (argc > 4 && strcmp (argv[4], "-d") == 0)
        debug = TRUE; 
    else
        debug = FALSE;

    /* Modo básico */
    if (type == 'v') {
        /* Inicializando os 2xN ciclistas */
        cycsize = 2*n;
        c = malloc (2 * n * sizeof (CYCLIST));
        for (i = 0; i < n; i++) {
            c[i].vel = 60;
            c[i].team = 0;
            c[i].lap = 0;
            c[i].pos = 0.0;
        }
        for (i = n; i < 2 * n; i++) {
            c[i].vel = 60;
            c[i].team = 1;
            c[i].lap = 0;
            c[i].pos = (float) (d / 2);
        }

        /* Inicializando o vetor pista */
        pista = malloc (d * sizeof (SLOT));
        for (i = 0; i < d; i++) {
            pista[i].mainpos = 0;
            pista[i].ultrapos = 0;
            pista[i].mut = PTHREAD_MUTEX_INITIALIZER;
        }

        /* Inicializando a barreira para 2n threads*/
        pthread_barrier_init (&barrera, NULL, 2*n);
        raceEnd = 0;

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
    int i, res, finish0 = 0, finish1 = 0, j, passedLap;
    int fastcyc1, fastcyc2, fast1ind, fast2ind, first, second;
    float nextPos;
    ARGS* p = (ARGS *) a;
    i = p->i;
    while (TRUE) {

        /* verificar se a corrida acabou */
        if (raceEnd) break;

        /* calcular a próxima posição do ciclista */
        if (vel == 60) {
            nextPos = (c[i].pos + 1) % d;
        }

        /* solicitar acesso ao próximo slot */
        pthread_mutex_lock (&pista[nextPos].mut);
        {
            /* se não tiver ninguém nesse slot, vamos ocupar ele */
            if (pista[nextPos].mainpos == 0) {
                pista[nextPos] = c[i].id;
                c[i].pos = nextPos;

                /* verificar se houve um incremento de volta */
                if (nextPos == 0) {
                    c[i].lap++;

                    /* caso ele terminou a corrida, marcamos isso */
                    if(c[i].lap == 16)
                        c[i].finalTick = timeTick;

                    /* verificamos se ele é o terceiro a passar */
                    passedLap = fastcyc1 = fastcyc2 = fast1ind = fast2ind = 0;
                    for (j = 0; j < cycsize; j++) {

                        /* guardamos tambem o primeiro e segundo colocados */
                        if (c[j].team == c[i].team && c[j].lap == c[i].lap && i != j) {
                            if (fastcyc1 == 0) {
                                fastcyc1 = (c[j].lap * d) + c[j].pos;
                                fast1ind = j;
                            } else {
                                fastcyc2 = (c[j].lap * d) + c[j].pos;
                                fast2ind = j;
                            }
                            passedLap++;

                        }
                    }

                    if (passedLap == 2) {
                        printf("Ciclista %d passou para a volta %d.\n", i, c[i].lap + 1);
                        if (fastcyc1 > fastcyc2) {
                            first = fastcyc1;
                            second = fastcyc2;
                        } else {
                            first = fastcyc2;
                            second = fastcyc1;
                        }

                        printf("Primeiro da equipe: %d\nSegundo da equipe: %d\n\n", first, second);

                    }
                }
            }
        }
        pthread_mutex_unlock (&pista[nextPos].mut);

        /* sincronizar na barreira para o intervalo seguinte */
        res = pthread_barrier_wait (&barrera);

        /* esta condição só vai ser executada em uma das threads */
        if (res == PTHREAD_BARRIER_SERIAL_THREAD) {

            /* aumentar o tempo atual */
            timeTick++; 

            /* verificar se a corrida acabou */
            raceEnd = 1;
            for (j = 0; j < cycsize; j++) {

                /* procuramos ciclistas que ainda nao acabaram */
                if(c[j].lap < 16) {
                    raceEnd = 0;
                    break;
                }
            }
        }

        /* mais uma barreira, pra garantir que ninguém vai recomeçar 
           a volta sem saber se a corrida acabou                      */
        pthread_barrier_wait (&barrera);
    }

    return NULL;
}