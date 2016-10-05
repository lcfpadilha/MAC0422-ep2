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

/****************** Estruturas usadas ******************/
/* Estrutura do Ciclista */
typedef struct cyc {
    int id;
    int lap;
    int pos;
    int vel;
    int team;
    int broken;
    int finalTick;
    pthread_t thread;
} CYCLIST;

/* Estrutura de um slot da pista */
typedef struct sl {
    pthread_t mainpos;
    pthread_t ultrapos;
    pthread_mutex_t mut;
} SLOT;

/* Estrutura dos argumentos das threads */
typedef struct arguments {
    int i;
} ARGS;

/**************** Declaração de funções ****************/
void *ciclista (void *a);
void *dummy ();
void waitForSync ();
void manageRace ();
void breakCyclist ();
int comparator (const void *a, const void *b);

/****************** Variaveis Globais ******************/
SLOT *pista; /* Vetor de slot pista */
CYCLIST *c;  /* Vetor de ciclista   */
int d;       /* Tamanho da pista    */
int debug;   /* Parametro para debug*/
int cycsize; 
int raceEnd;
int timeTick;
int globalLap;
int runners[2];
int timeToBreak;
pthread_barrier_t barrera;
pthread_mutex_t randmutex;
pthread_mutex_t lapmutex;

int main (int argc, char **argv) {
    int n, i, winners[2], thirdTick[2];
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
    globalLap = 0;
    timeToBreak = 0;
    
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
            c[i].pos = d/2;
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
    }
    /* Inicializando a barreira para 2n threads e o mutex do random*/
    pthread_barrier_init (&barrera, NULL, 2*n);
    pthread_mutex_init (&randmutex, NULL);
    pthread_mutex_init (&lapmutex, NULL);
    
    /* Disparando as threads  */
    for (i = 0; i < 2 * n; i++) {
        thread_arg = malloc (sizeof (ARGS));
        thread_arg->i = i;
        pthread_create (&c[i].thread, NULL, &ciclista, thread_arg);
    }

    /* Esperando retornar*/
    for (i = 0; i < 2 * n; i++)
        pthread_join (c[i].thread, NULL);
    
    /* após o fim da corrida */
    printf("A corrida terminou!\n\nRanking:\n");
    
    /* ordenar o ranking de ciclistas */
    qsort(c, 2*n, sizeof(*c), comparator);
    winners[0] = winners[1] = thirdTick[0] = thirdTick[1] = 0;
    
    for(i = 0; i < cycsize; i++) {
        
        /* vamos contabilizar qual equipe venceu a corrida */
        winners[c[i].team]++;
        
        /* pra isso, temos que ver qual terceiro ciclista acabou antes */
        if(winners[c[i].team] == 3)
            thirdTick[c[i].team] = c[i].finalTick;
        
        /* exibindo ranking */
        if(c[i].broken)
            printf("%do: Ciclista %d (Equipe %d) - ACIDENTE (Volta %d)\n", 
                i+1, c[i].id, c[i].team+1, c[i].lap+1);
        else
            printf("%do: Ciclista %d (Equipe %d) - %.3fs\n", 
                i+1, c[i].id, c[i].team+1, (float) (c[i].finalTick*60)/1000);
    }
    
    /* exibimos o print da vitória, pra terminar o EP */
    if(thirdTick[0] < thirdTick[1])
        printf("\nVitória da equipe 1!\n\n");    
    else if(thirdTick[0] > thirdTick[1])
        printf("\nVitória da equipe 2!\n\n");
    else
        printf("\nEmpate!\n\n");
    
    return 0;
}

void *ciclista (void *a) {
    int i, j, passedLap, nextPos;
    int clock1, clock2, index1, index2, first, second;
    pthread_t dummyid;
    ARGS* p = (ARGS *) a;
    i = p->i;
    while (TRUE) {

        /* verificar se a corrida acabou */
        if (raceEnd) break;

        
        /* calcular a próxima posição do ciclista */
        if (c[i].vel == 60) {
            nextPos = (c[i].pos + 1) % d;
        }

        /* solicitar acesso ao próximo slot */
        pthread_mutex_lock (&pista[nextPos].mut);
        {
            /* se não tiver ninguém nesse slot, vamos ocupar ele e desocupar o anterior */
            if (pista[nextPos].mainpos == 0) {
                /* O ciclista insere seu id na posição principal */
                pista[nextPos].mainpos = c[i].id;
                pista[c[i].pos].mainpos = 0;
                c[i].pos = nextPos;

                /* verificar se houve um incremento de volta */
                if ((c[i].team == 0 && nextPos == 0) || (c[i].team == 1 && nextPos == d/2)) {
                    c[i].lap++;
                    
                    /* verificamos se ele é o primeiro a mudar de volta */
                    pthread_mutex_lock(&lapmutex);
                    {
                        if(c[i].lap > globalLap) {
                            globalLap++;
                            
                            /* a cada quatro voltas, chance de alguem quebrar */
                            if(globalLap % 4 == 0 && globalLap < 16 ) {
                                timeToBreak = 1;
                            }
                        }
                    }
                    pthread_mutex_unlock(&lapmutex);

                    /* caso ele terminou a corrida, marcamos isso */
                    if(c[i].lap == 16)
                        c[i].finalTick = timeTick;

                    /* verificamos se ele é o terceiro a passar */
                    passedLap = clock1 = clock2 = index1 = index2 = 0;
                    for (j = 0; j < cycsize; j++) {
                        
                        if(c[j].broken) continue;
                        
                        /* guardamos tambem o primeiro e segundo colocados */
                        if (c[j].team == c[i].team && c[j].lap >= c[i].lap && i != j) {
                            if (clock1 == 0) {
                                
                                /* O ultimo fator da soma corrige a diferença do ponto de largada
                                   das duas equipes */
                                clock1 = (c[j].lap * d) + c[j].pos - c[j].team*(d/2);
                                index1 = c[j].id;
                            } else {
                                clock2 = (c[j].lap * d) + c[j].pos - c[j].team*(d/2);
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

                        printf("Ciclista %d passou para a volta %d no instante %.3fs.\n", 
                               c[i].id, c[i].lap + 1, (float) (timeTick*60)/1000);
                        printf("Primeiro da equipe: %d\nSegundo  da equipe: %d\n\n", first, second);

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
    
    /* verificar se é necessario quebrar algum ciclista */
    if (timeToBreak) {
        timeToBreak = 0;
        breakCyclist();
    }

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
}

/* rola um dado e com chance de 10% quebra algum ciclista */
void breakCyclist () {
    int startpoint = 0, endpoint = cycsize, chance, lucky;
    
    /* verificamos se as equipes são grandes o suficiente */
    if(runners[0] == 3) 
        startpoint = cycsize/2;
    if(runners[1] == 3) 
        endpoint = cycsize/2;
    
    if(startpoint == endpoint) return;
    
    /* solicitamos acesso ao mutex do random */
    pthread_mutex_lock(&randmutex);
    {
        chance = rand() % 10;
    }
    pthread_mutex_unlock(&randmutex);
    /* chance é de 10% */
    if(chance < 1) {
        pthread_mutex_lock(&randmutex);
        {
            do {
                
                /* normalizamos o random pra escolher um dos ciclistas */
                lucky = (rand() % (endpoint - startpoint)) + startpoint;
            } while(c[lucky].broken);
        }
        pthread_mutex_unlock(&randmutex);
        c[lucky].broken = 1;
        pista[c[lucky].pos].mainpos = 0;
        printf("Ciclista %d sofreu um acidente!\n\n", c[lucky].id);
        runners[c[lucky].team]--;
    }
}

/* função que ordena os ciclistas por ordem de chegada,
   comparando dois a dois:
   - retorna x < 0 se A deve vir antes de B
   - retorna X > 0 se B deve vir antes de A
   - retorna X = 0 se são iguais
   */
int comparator (const void *a, const void *b) {
    CYCLIST *c1, *c2;
    int dist1, dist2;
        
    /* fazemos o cast para ponteiro de ciclista */
    c1 = (CYCLIST*) a;
    c2 = (CYCLIST*) b;
    
    /* se ambos estiverem quebrados, predomina o lap em que quebraram */ 
    if(c1->broken && c2->broken)
        return c2->lap - c1->lap;
    
    /* se apenas um estiver quebrado, ele vai pro fundo da lista */
    if(c1->broken)
        return 1;
    if(c2->broken)
        return -1;
    
    /* a comparação tradicional é por distancia percorrida,
       ja que mesmo apos passar a linha de chegada eles continuam andando */
    dist1 = c1->lap*d + c1->pos - c1->team*(d/2);
    dist2 = c2->lap*d + c2->pos - c2->team*(d/2);
    return dist2 - dist1;
}
