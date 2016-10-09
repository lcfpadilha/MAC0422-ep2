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
#define MODE_U -1
#define MODE_V 1
/********************************************************************/
/*********************** Estruturas usadas **************************/
/********************************************************************/
/* Estrutura do Ciclista */
typedef struct cyc {
    int id;             /* identificador do ciclista.               */
    int lap;            /* volta que o ciclista está.               */
    int vel;            /* velocidade que ele está.                 */
    int velM;           /* velocidade máxima que ele pode adquirir. */
    int team;           /* equipe que ele está.                     */
    int broken;         /* valor que diz se está quebrado ou não.   */
    int deaccel;        /* flag que diz se ele desaacelerou.        */
    int finalTick;      /* tempo que ele demorou para chegar.       */
    float pos;          /* posição da pista que ele está.           */
    pthread_t thread;   /* identificador da thread.                 */
} CYCLIST;

/* Estrutura de um slot da pista */
typedef struct sl {
    int mainpos;        /* slot da pista principal.                 */
    int ultrapos;       /* slot da pista para ultrapassagem.        */
    pthread_mutex_t mut;/* mutex para o slot.                       */
} SLOT;

/* Estrutura dos argumentos das threads */
typedef struct arguments {
    int i;
} ARGS;

/********************************************************************/
/************************ Variaveis Globais *************************/
/********************************************************************/
SLOT *pista; /* Vetor de slot pista */
CYCLIST *c;  /* Vetor de ciclista   */
int d;       /* Tamanho da pista    */
int mode;
int debug;   /* Parametro para debug*/
int cycsize; 
int raceEnd;
int timeTick;
int globalLap;
int runners[2];
int timeToBreak;
int timeToChange;
pthread_barrier_t barrera;
pthread_mutex_t randmutex;
pthread_mutex_t lapmutex;

/********************************************************************/
/*********************** Declaração de funções **********************/
/********************************************************************/
void *ciclista (void *a);
void *dummy ();
void waitForSync ();
void manageRace ();
void breakCyclist ();
void selectVelMax ();
void changeVel ();
float dist (CYCLIST cycl);
int comparator (const void *a, const void *b);

/********************************************************************/
/************************ Código das funções ************************/
/********************************************************************/
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
    timeToChange = timeToBreak = FALSE;

    /* Inicializando os 2xN ciclistas */
    c = malloc (2 * n * sizeof (CYCLIST));

    /* Modo básico */
    if (type == 'v') {
        for (i = 0; i < n; i++) {
            c[i].vel = 60;
            c[i].velM = 60;
            c[i].deaccel = FALSE;
            c[i].team = 0;
            c[i].lap = 0;
            c[i].pos = 0.0;
            c[i].id = i + 1;
            c[i].finalTick = 0;
            c[i].broken = 0;
        }
        for (i = n; i < 2 * n; i++) {
            c[i].vel = 60;
            c[i].velM = 60;
            c[i].deaccel = FALSE;
            c[i].team = 1;
            c[i].lap = 0;
            c[i].pos = d/2;
            c[i].id = i + 1;
            c[i].finalTick = 0;
            c[i].broken = 0;
        }
        mode = MODE_V;
    }
    /* Modo de velocidades variadas */
    else if (type == 'u') {
        for (i = 0; i < n; i++) {
            c[i].vel = 30;
            c[i].velM = 30;
            c[i].deaccel = FALSE;
            c[i].team = 0;
            c[i].lap = 0;
            c[i].pos = 0.0;
            c[i].id = i + 1;
            c[i].finalTick = 0;
            c[i].broken = 0;
        }
        for (i = n; i < 2 * n; i++) {
            c[i].vel = 30;
            c[i].velM = 30;
            c[i].deaccel = FALSE;
            c[i].team = 1;
            c[i].lap = 0;
            c[i].pos = d/2;
            c[i].id = i + 1;
            c[i].finalTick = 0;
            c[i].broken = 0;
        }
        mode = MODE_U;
    }
    /* ERRO */
    else
        return EXIT_FAILURE;
        
    /* Inicializando o vetor pista */
    pista = malloc (d * sizeof (SLOT));
    for (i = 0; i < d; i++) {
        pista[i].mainpos = 0;
        pista[i].ultrapos = 0;
        pthread_mutex_init (&pista[i].mut, NULL);
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
    qsort ((void *) c, 2*n, sizeof (CYCLIST), comparator);
    winners[0] = winners[1] = thirdTick[0] = thirdTick[1] = 0;
    
    for (i = 0; i < cycsize; i++) {
        
        /* vamos contabilizar qual equipe venceu a corrida */
        winners[c[i].team]++;
        
        /* pra isso, temos que ver qual terceiro ciclista acabou antes */
        if (winners[c[i].team] == 3)
            thirdTick[c[i].team] = c[i].finalTick;
        
        /* exibindo ranking */
        if (c[i].broken)
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
    int index1, index2, first, second;
    float dist1, dist2, next, changeIndex;
    pthread_t dummyid;
    ARGS* p = (ARGS *) a;
    i = p->i;
    while (TRUE) {
        /* Resetamos a flag de mudança de índice da posição do ciclista */
        changeIndex = FALSE;

        /* Verificar se a corrida acabou */
        if (raceEnd) break;

        /* Calcular a próxima posição do ciclista e quantos */
        /*passos ele vai dar no vetor.                      */
        if (c[i].vel == 60) {
            nextPos = (int) (c[i].pos + 1) % d;
            next = 1.0; 
        }
        else {
            nextPos = (int) (c[i].pos + 0.5) % d;
            next = 0.5;
        }

        /* Solicitar acesso ao próximo slot */
        pthread_mutex_lock (&pista[nextPos].mut);
        {
            /* Se não tiver ninguém no slot principal, vamos ocupar ele e desocupar */
            /*o anterior.                                                           */
            if (pista[nextPos].mainpos == 0) {
                /* Verifica se ele estava na posição anterior para poder zera-la*/
                if (pista[(int) c[i].pos].mainpos == c[i].id)
                    pista[(int) c[i].pos].mainpos = 0;
                else if (pista[(int) c[i].pos].ultrapos == c[i].id)
                    pista[(int) c[i].pos].ultrapos = 0;

                /* O ciclista insere seu id na posição principal */
                pista[nextPos].mainpos = c[i].id;

                /* Calculo da proxima posição*/
                c[i].pos += next;
                if (c[i].pos >= d)
                    c[i].pos = c[i].pos - d;

                /* Como houve a alteração da posição do ciclista, setamos a flag. */
                changeIndex = TRUE;
            }
            /* Verifica se a próxima posição do ciclista é a que ele está.        */
            else if (pista[nextPos].mainpos == c[i].id) {
                c[i].pos += next;
                if (c[i].pos >= d)
                    c[i].pos = c[i].pos - d;
            }
            /* Se a posição principal que o ciclista quer acessar está ocupada pos*/
            /*um ciclista de outra equipe, tentamos inserir o ciclista na pista de*/
            /*ultrapassagem.                                                      */
            else if (pista[nextPos].ultrapos == 0 && 
                    (c[pista[nextPos].mainpos-1].team != c[i].team || 
                    c[pista[nextPos].mainpos-1].vel < c[i].vel)) {
                /* O ciclista insere seu id na posição de ultrapassagem.          */
                pista[nextPos].ultrapos = c[i].id;
                
                /* Verifica se ele estava na posição anterior para poder zera-la. */
                if (pista[(int) c[i].pos].mainpos == c[i].id)
                    pista[(int) c[i].pos].mainpos = 0;
                else if (pista[(int) c[i].pos].ultrapos == c[i].id)
                    pista[(int) c[i].pos].ultrapos = 0;

                /* Calculo da proxima posição */
                c[i].pos += next;
                if (c[i].pos >= d)
                    c[i].pos = c[i].pos - d;

                /* Como houve a alteração da posição do ciclista, setamos a flag. */
                changeIndex = TRUE;
            }
            /* Verifica se a próxima posição do ciclista é a que ele está (de ul-*/
            /*trapassagem).                                                      */
            else if (pista[nextPos].ultrapos == c[i].id) {
                c[i].pos += next;
                if (c[i].pos >= d)
                    c[i].pos = c[i].pos - d;
            }
            /* Se o indice, */
            if (changeIndex) {
                /* verificar se houve um incremento de volta.                     */
                if ((c[i].team == 0 && nextPos == 0 && c[i].pos == 0.0) || 
                    (c[i].team == 1 && nextPos == d/2 && c[i].pos == d/2)) {
                    c[i].lap++;
                    
                    /* verificamos se ele é o primeiro a mudar de volta */
                    pthread_mutex_lock(&lapmutex);
                    {
                        if(c[i].lap > globalLap) {
                            globalLap++;
                            timeToChange = TRUE;

                            /* a cada quatro voltas, chance de alguem quebrar */
                            if(globalLap % 4 == 0 && globalLap < 16) {
                                timeToBreak = TRUE;
                            }
                        }
                    }
                    pthread_mutex_unlock(&lapmutex);

                    /* caso ele terminou a corrida, marcamos isso */
                    if(c[i].lap == 16)
                        c[i].finalTick = timeTick + 1;

                    /* verificamos se ele é o terceiro a passar */
                    passedLap = index1 = index2 = 0;
                    dist1 = dist2 = 0.0;
                    for (j = 0; j < cycsize; j++) {
                        
                        if(c[j].broken) continue;
                        
                        /* Guardamos tambem o primeiro e segundo colocados */
                        if (c[j].team == c[i].team && c[j].lap >= c[i].lap && i != j) {
                            if (dist1 == 0.0) {
                                /* O ultimo fator da soma corrige a diferença do ponto */
                                /*de largada das duas equipes.                         */
                                dist1 = (c[j].lap * d) + c[j].pos - c[j].team*(d/2);
                                index1 = c[j].id;
                            } 
                            else {
                                dist2 = (c[j].lap * d) + c[j].pos - c[j].team*(d/2);
                                index2 = c[j].id;
                            }
                            passedLap++;
                        }
                    }

                    /* Exibimos o print de nova volta */
                    if (passedLap == 2) {
                        if (dist1 > dist2) {
                            first = index1;
                            second = index2;
                        } 
                        else {
                            first = index2;
                            second = index1;
                        }

                        printf("Ciclista %d (Equipe %d) finalizou a volta %d no instante %.3fs.\n", 
                               c[i].id, c[i].team+1, c[i].lap, (float) (timeTick*60)/1000);
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

    /* Aumentar o tempo atual */
    timeTick++; 
    
    /* Printar tabuleiro caso o debug esteja habilitado */
    if(debug) {
        printf("Instante: %.3f\n", (float) (timeTick*60)/1000);
        for (j = 0; j < d; j++) {
            printf("[%3.d][%3.d] %d\n", (int) pista[j].mainpos, (int) pista[j].ultrapos, j);
        }
        printf("\n");
        printf("EQUIPE 1\n");
        for (j = 0; j < cycsize / 2; j++)
            printf("Ciclista %d, velocidade %d (%d), posição %f\n", j+1, c[j].vel, c[j].velM, dist(c[j]));
        printf ("EQUIPE 2\n");
        for (j = cycsize / 2; j < cycsize; j++)
            printf("Ciclista %d, velocidade %d (%d), posição %f\n", j+1, c[j].vel, c[j].velM, dist(c[j]));
        printf("\n\n");
    }
    
    /* Verificar se é necessario quebrar algum ciclista */
    if (timeToBreak) {
        timeToBreak = FALSE;
        breakCyclist();
    }

    /* Verifica se é necessário alterar as velocidades */
    if (mode == MODE_U && timeToChange) {
        timeToChange = FALSE;
        selectVelMax ();
    }

    /* Verificar se a corrida acabou */
    raceEnd = 1;
    for (j = 0; j < cycsize; j++) {

        if(c[j].broken) continue;

        /* Procuramos ciclistas que ainda nao acabaram */
        if(c[j].lap < 16) {
            raceEnd = 0;
            break;
        }
    }
}

/* breakCyclist: quebra um ciclista aleatório com chance de 10% */
void breakCyclist () {
    int startpoint = 0, endpoint = cycsize, chance, lucky;
    
    /* Verificamos se as equipes são grandes o suficiente */
    if(runners[0] == 3) 
        startpoint = cycsize/2;
    if(runners[1] == 3) 
        endpoint = cycsize/2;
    
    if(startpoint == endpoint) return;
    
    chance = rand() % 10;  

    /* Chance é de 10% */
    if(chance < 1) {
        do {
            /* Normalizamos o random pra escolher um dos ciclistas */
            lucky = (rand() % (endpoint - startpoint)) + startpoint;
        } while(c[lucky].broken);

        c[lucky].broken = 1;
        pista[(int) c[lucky].pos].mainpos = 0;
        printf("Ciclista %d (Equipe %d) sofreu um acidente!\n\n", 
            c[lucky].id, c[lucky].team + 1);
        runners[c[lucky].team]--;
    }
}

/* selectVelMax: função que altera todas as velocidades máximas*/
/*dos 2*n ciclistas. As velocidades podem ser 30 ou 60 (com 50%*/
/*de chance de ser cada uma.                                   */
void selectVelMax () {
    int j, prob;
    for (j = 0; j < cycsize; j++) {
        /* Se o ciclista etiver quebrado, pulamos para o próximo */
        if (c[j].broken) continue;

        /* Alteramos a velocidade com base na probabilidade prob.*/
        prob = rand() % 10;
        if (prob >= 5) {
            c[j].vel = 60;
            c[j].velM = 60;
        }
        else {
            c[j].vel  = 30;
            c[j].velM = 30;
        }
    }

    /* Fazemos todos os ciclistas que estão atrás daqueles com */
    /*30km/h diminuirem para 30km.                             */
    for (j = 0; j < cycsize; j++)
        if (c[j].velM == 30)
            changeVel (j);

    /* Todos os ciclistas que não desaceleraram setam sua flag */
    /*de desaceleração.                                        */
    for (j = 0; j < cycsize; j++)
        if (c[j].velM == c[j].vel) c[j].deaccel = FALSE;
}

/* changeVel: recebe um inteiro i e altera todas as velocidades  */
/*dos ciclistas que são da mesma equipe e que estão atrás do ci- */
/*clista identificado por j por 30km/.                           */
void changeVel (int i) {
    int j;
    for (j = 0; j < cycsize; j++) {
        if (c[j].team == c[i].team && dist (c[j]) < dist (c[i]))
            if (c[j].velM > 30 && !c[j].deaccel) {
                c[j].vel = 30;
                c[j].deaccel = TRUE;
            }
    }
}

/* dist: recebe um CYCLIST cycl e retorna a distancia percorrida */
/*por cycl.                                                      */
float dist (CYCLIST cycl) {
    float dist;
    if (cycl.team == 1)
        if (cycl.pos >= d/2)
            dist = cycl.lap*d + cycl.pos - d/2;
        else
            dist = cycl.lap*d + cycl.pos + d/2;
    else dist = cycl.lap*d + cycl.pos;
    return dist;
}

/* função que ordena os ciclistas por ordem de chegada, compa-  */
/*rando dois a dois:                                            */
/* - retorna um valor < 0 se A deve vir antes de B              */
/* - retorna um valor > 0 se B deve vir antes de A              */
/* - retorna 0 se são iguais                                    */
int comparator (const void *a, const void *b) {
    CYCLIST *c1, *c2;
        
    /* Fazemos o cast para ponteiro de ciclista */
    c1 = (CYCLIST*) a;
    c2 = (CYCLIST*) b;
    
    /* Se ambos estiverem quebrados, predomina o lap em que quebraram.*/ 
    if(c1->broken && c2->broken)
        return c2->lap - c1->lap;
    
    /* Se apenas um estiver quebrado, ele vai pro fundo da lista.     */
    if(c1->broken)
        return 1;
    if(c2->broken)
        return -1;
    
    /* A comparação tradicional é por distancia percorrida, ja que mesmo */
    /*apos passar a linha de chegada eles continuam andando              */
    return c1->finalTick - c2->finalTick;
}
