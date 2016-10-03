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
#include <time.h>
#define TRUE  1
#define FALSE 0

/* Structs usadas */
typedef struct cyc {
    int vel;
    int team;
    int lap;
    int pos;
    pthread_t thread;
    int id;
    int finalTick;
    int broken;
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
void *ciclista (void *a);
void *dummy ();
void waitForSync ();
void manageRace ();

/*      Variáveis globais       */
SLOT *pista;
CYCLIST *c;
int runners[2];
int cycsize;
pthread_barrier_t barrera;
pthread_mutex_t randmutex;
int raceEnd;
int timeTick;
int d;
int debug;

int main (int argc, char **argv) {
    int n, i;
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

    /* inicializando nosso gerador */
    srand(time(NULL));
    
    /* inicializando variaveis globais */
    raceEnd = 0;
    runners[0] = n;
    runners[1] = n;
    cycsize = 2*n;
    
    /* Modo básico */
    if (type == 'v') {
        
        /* Inicializando os 2xN ciclistas */
        c = malloc (2 * n * sizeof (CYCLIST));
        for (i = 0; i < n; i++) {
            c[i].vel = 60;
            c[i].team = 0;
            c[i].lap = 0;
            c[i].pos = 0;
            c[i].id = i + 1;
            c[i].finalTick = 0;
            c[i].broken = 0;
        }
        for (i = n; i < 2 * n; i++) {
            c[i].vel = 60;
            c[i].team = 1;
            c[i].lap = 0;
            c[i].pos = 0;
            c[i].id = i + 1;
            c[i].finalTick = 0;
            c[i].broken = 0;
        }

        /* Inicializando o vetor pista */
        pista = malloc (d * sizeof (SLOT));
        for (i = 0; i < d; i++) {
            pista[i].mainpos = 0;
            pista[i].ultrapos = 0;
            pthread_mutex_init ( &pista[i].mut, NULL);
        }

        /* Inicializando a barreira para 2n threads e o mutex do random*/
        pthread_barrier_init (&barrera, NULL, 2*n);
        pthread_mutex_init ( &randmutex, NULL);
        
        /* Disparando as threads  */
        for (i = 0; i < 2 * n; i++) {
            thread_arg = malloc (sizeof (ARGS));
            thread_arg->i = i;
            pthread_create (&c[i].thread, NULL, &ciclista, thread_arg);
        }

        /* Esperando retornar*/
        for (i = 0; i < 2 * n; i++)
            pthread_join (c[i].thread, NULL);
    }
    return 0;
}

void *ciclista (void *a) {
    int i, j, passedLap, nextPos, random;
    int clock1, clock2, index1, index2, first, second;
    pthread_t dummyid;
    ARGS* p = (ARGS *) a;
    i = p->i;
    while (TRUE) {

        /* verificar se a corrida acabou */
        if (raceEnd) break;

        
        /* calcular a próxima posição do ciclista */
        if (60 == 60) {
            nextPos = (c[i].pos + 1) % d;
        }

        /* solicitar acesso ao próximo slot */
        pthread_mutex_lock (&pista[nextPos].mut);
        {
            /* se não tiver ninguém nesse slot, vamos ocupar ele e desocupar o anterior */
            if (pista[nextPos].mainpos == 0) {
                pista[nextPos].mainpos = c[i].id;
                pista[c[i].pos].mainpos = 0;
                c[i].pos = nextPos;

                /* verificar se houve um incremento de volta */
                if (nextPos == 0) {
                    c[i].lap++;

                    /* caso ele terminou a corrida, marcamos isso */
                    if(c[i].lap == 16)
                        c[i].finalTick = timeTick;

                    /* verificamos se ele é o terceiro a passar */
                    passedLap = clock1 = clock2 = index1 = index2 = 0;
                    for (j = 0; j < cycsize; j++) {
                        
                        if(c[j].broken) continue;
                        
                        /* guardamos tambem o primeiro e segundo colocados */
                        if (c[j].team == c[i].team && c[j].lap == c[i].lap && i != j) {
                            if (clock1 == 0) {
                                clock1 = (c[j].lap * d) + c[j].pos;
                                index1 = c[j].id;
                            } else {
                                clock2 = (c[j].lap * d) + c[j].pos;
                                index2 = c[j].id;
                            }
                            passedLap++;

                        }
                    }

                    /* exibimos o print de nova volta */
                    if (passedLap == 2) {
                        if (clock1 > clock2) {
                            first = index1;
                            second = index2;
                        } else {
                            first = index2;
                            second = index1;
                        }

                        printf("Ciclista %d passou para a volta %d.\n", c[i].id, c[i].lap + 1);
                        printf("Primeiro da equipe: %d\nSegundo  da equipe: %d\n\n", first, second);

                    }
                    
                    /* a cada 4 voltas ele pode sofrer um infeliz acidente */
                    if(c[i].lap < 16 && c[i].lap % 4 == 0) {
                        
                        /* solicitamos acesso ao mutex do random */
                        pthread_mutex_lock(&randmutex);
                        {
                            random = rand() % 10;
                        }
                        pthread_mutex_unlock(&randmutex);
                        
                        /* a chance é 10% */
                        if(random < 1) {
                            
                            /* a equipe tambem deve ter mais do que 3 ciclistas */
                            if(runners[c[i].team] > 3) {
                                pista[nextPos].mainpos = 0;
                                c[i].broken = 1;
                                printf("Ciclista %d sofreu um acidente!\n\n", c[i].id);
                                runners[c[i].team]--;
                            }
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock (&pista[nextPos].mut);

        /* sincronizar threads */
        waitForSync();
        
        /* verificamos se este ciclista quebrou e deve ser desligado */
        if(c[i].broken) {
            pthread_create(&dummyid, NULL, &dummy, NULL);
            break;
        }

    }

    return NULL;
}

/* thread vazia, apenas para substituir os ciclistas quebrados */
void *dummy () {
    while(TRUE) {
        
        /* verificar se a corrida acabou */
        if (raceEnd) break; 
        
        /* sincronizar com o resto */
        waitForSync();
    }
    
    return NULL;
}

/* função que sincroniza todas as thread após uma iteração.
   utiliza duas barreiras pra isso, recheadas por código de controle
   do funcionamento da corrida.                                      */
void waitForSync() {    
    
    /* sincronizar na barreira para o intervalo seguinte */
    int res = pthread_barrier_wait (&barrera);

    /* esta condição só vai ser executada em uma das threads */
    if (res == PTHREAD_BARRIER_SERIAL_THREAD) {
        manageRace();
    }

    /* mais uma barreira, pra garantir que ninguém vai recomeçar 
       a volta sem saber se a corrida acabou                      */
    pthread_barrier_wait (&barrera);
}

/* código que gerencia o funcionamento da corrida ao fim de cada
   iteração dos ciclistas. Só é executado em uma thread.         */
void manageRace () {
    int j;
    
    /* printar tabuleiro caso o debug esteja habilitado */
    if(debug) {
        printf("Instante: %.3f\n", (float) (timeTick*60)/1000);
        for (j = 0; j < d; j++) {
            printf("[%3.d][%3.d] %d\n", (int) pista[j].mainpos, (int) pista[j].ultrapos, j);
        }
        printf("\n\n");
    }


    /* aumentar o tempo atual */
    timeTick++; 

    /* verificar se a corrida acabou */
    raceEnd = 1;
    for (j = 0; j < cycsize; j++) {

        if(c[j].broken) continue;

        /* procuramos ciclistas que ainda nao acabaram */
        if(c[j].lap < 16) {
            raceEnd = 0;
            break;
        }
    }

    /* se a corrida acabou, imprimimos o ranking */
    if(raceEnd) {

    }
}