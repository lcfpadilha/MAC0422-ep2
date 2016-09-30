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
SLOT *track;
CYCLIST *c;
pthread_barrier_t barrera;
int raceEnd;

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
            c[i].lap = 0;
            c[i].pos = 0.0;
        }
        for (i = n; i < 2 * n; i++) {
            c[i].vel = 60;
            c[i].team = 1;
            c[i].lap = 0;
            c[i].pos = (float) (d / 2);
        }

        /* Inicializando o vetor track */
        track = malloc (d * sizeof (SLOT));
        for (i = 0; i < d; i++) {
            track[i].mainpos = 0;
            track[i].ultrapos = 0;
            track[i].mut = PTHREAD_MUTEX_INITIALIZER;
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
    int i, res;
    float nextPos;
    ARGS* p = (ARGS *) a;
    i = p->i;
    while (TRUE) {

        /* verificar se a corrida acabou */
        if (raceEnd) break;

        /* calcular a próxima posição do ciclista */
        if (vel == 60) {
            nextPos = (c[i].pos + 1) % n;
        }

        /* solicitar acesso ao próximo slot */
        pthread_mutex_lock (&track[nextPos].mut);
        {
            /* se não tiver ninguém nesse slot, vamos ocupar ele */
            if (track[nextPos].mainpos == 0) {
                track[nextPos] = c[i].id;
                c[i].pos = nextPos;

                /* verificar se houve um incremento de volta */
                if (nextPos == 0) c[i].lap++;
            }
        }
        pthread_mutex_unlock (&track[nexSl].mut);

        /* sincronizar na barreira para o intervalo seguinte */
        res = pthread_barrier_wait (&barrera);

        /* esta condição só vai ser executada em uma das threads */
        if(res == PTHREAD_BARRIER_SERIAL_THREAD) {
            //verificar se a corrida acabou (como?)
        }

        /* mais uma barreira, pra garantir 


        /*SE LAP == 16, FLAG = 1 */
    }

    return NULL;
}