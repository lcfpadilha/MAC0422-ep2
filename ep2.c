/*******************************************************************/
/*  EP1 de MAC0422 - Sistemas Operacionais                         */
/*                                                                 */
/*  Simulador de uma corrida - ep2.c                               */
/*                                                                 */         
/*  Autores: Gustavo Silva          (No USP 9298260)               */
/*           Leonardo Padilha       (No USP 9298295)               */
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
SLOT *pista;            /* Vetor de slot pista                       */
CYCLIST *c;             /* Vetor de ciclista                         */
CYCLIST t1[4], t2[4];   /* Os 3 melhores colocados de cada equipes   */ 
int d;                  /* Tamanho da pista                          */
int mode;               /* Modo da corrida                           */
int debug;              /* Parametro para debug                      */
int cycsize;            /* Quantidade de ciclistas na corrida.       */
int raceEnd;            /* Boolean que diz se a corrida terminou     */
int thirdEnd;           /* Indica que o terceiro ciclista ultrapassou*/
int timeTick;           /* Quantidade de 60ms decorridos.            */
int globalLap;          /* Volta no qual o primeiro da equipe está   */
int runners[2];         /* Quantidade de ciclistas em cada equipe.   */
int timeToBreak;        /* Boolean que diz se é hora de quebrar.     */
int timeToChange;       /* Boolean que diz se é hora de trocar as vel*/
pthread_barrier_t barrera;  /* Barreira de sincronização             */
pthread_mutex_t lapmutex;   /* Mutex para a volta.                   */

/********************************************************************/
/*********************** Declaração de funções **********************/
/********************************************************************/
void *ciclista (void *a);
void *dummy ();
void initializeCycs (int v);
void waitForSync ();
void manageRace ();
void printRanking ();
void breakCyclist ();
void selectVelMax ();
void changeVel ();
void posChange (int i, int nextPos);
float dist (CYCLIST cycl);
int comparator (const void *a, const void *b);

/********************************************************************/
/************************ Código das funções ************************/
/********************************************************************/
int main (int argc, char **argv) {
    int n, i;
    char type;
    ARGS** thread_arg;

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
    runners[0] = n;
    runners[1] = n;
    cycsize = 2*n;
    globalLap = 0;
    raceEnd = thirdEnd = timeToChange = timeToBreak = FALSE;

    /* Inicializando os 2xN ciclistas */
    c = malloc (2 * n * sizeof (CYCLIST));

    /* Modo de velocidades uniformes */
    if (type == 'u') {
        initializeCycs (60);
        mode = MODE_U;
    }
    /* Modo de velocidades variadas */
    else if (type == 'v') {
        initializeCycs (30);
        mode = MODE_V;
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
    pthread_mutex_init (&lapmutex, NULL);
    
    /* Disparando as threads  */
    thread_arg = malloc (2 * n * sizeof (ARGS *));
    for (i = 0; i < 2 * n; i++) {
        thread_arg[i] = malloc (sizeof (ARGS));
        thread_arg[i]->i = i;
        pthread_create (&c[i].thread, NULL, &ciclista, thread_arg[i]);
    }

    /* Esperando a corrida terminar.    */
    for (i = 0; i < 2 * n; i++)
        pthread_join (c[i].thread, NULL);

    /* Imprimimos o ranking após o final da corrida */
    printRanking ();

    /* Desalocamos toda a memória alocada */
    for (i = 0; i < cycsize; i++)
        free (thread_arg[i]);
    free (thread_arg);
    free (c);
    free (pista);
    
    return 0;
}

/* ciclista: função que contém toda a lógica da thread do ciclista.     */
void *ciclista (void *a) {
    int i, nextPos, changeIndex;
    float next;
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
            /* Verifica se a próxima posição do ciclista é a que ele está (de ul- */
            /*trapassagem).                                                       */
            else if (pista[nextPos].ultrapos == c[i].id) {
                c[i].pos += next;
                if (c[i].pos >= d)
                    c[i].pos = c[i].pos - d;
            }
            /* Se o indice da posição do vetor for alterado, chamamos a função    */
            /*responsável por fazer as verificações de mudança de posição.        */
            if (changeIndex) 
                posChange (i, nextPos);
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

/* dummy: função para uma thread vazia, que servirá apenas para a barreira*/
/*de sincronização, substituindo os ciclistas quebrados.                  */
void *dummy () {
    while (TRUE) {
        
        /* verificar se a corrida acabou */
        if (raceEnd) break; 
        
        /* sincronizar com o resto */
        waitForSync();
    }
    
    return NULL;
}

/* initializeCycs: recebe um inteiro v e inicializa o vetor de ciclistas*/
/*com velocidade v.                                                     */
void initializeCycs (int v) {
    int i;

    for (i = 0; i < cycsize / 2; i++) {
        c[i].vel = v;
        c[i].velM = v;
        c[i].deaccel = FALSE;
        c[i].team = 0;
        c[i].lap = 0;
        c[i].pos = 0.0;
        c[i].id = i + 1;
        c[i].finalTick = 0;
        c[i].broken = 0;
    }
    for (i = cycsize / 2; i < cycsize; i++) {
        c[i].vel = v;
        c[i].velM = v;
        c[i].deaccel = FALSE;
        c[i].team = 1;
        c[i].lap = 0;
        c[i].pos = d/2;
        c[i].id = i + 1;
        c[i].finalTick = 0;
        c[i].broken = 0;
    }
}

void printRanking () {
    int i, winners[2], thirdTick[2];

    printf("A corrida terminou!\n\nRanking:\n");

    /* Ordenamos os ciclistas com base na função comparadora */
    qsort ((void *) c, cycsize, sizeof (CYCLIST), comparator);
    winners[0] = winners[1] = thirdTick[0] = thirdTick[1] = 0;
    
    for (i = 0; i < cycsize; i++) {
        /* Vamos contabilizar qual equipe venceu a corrida, caso */
        /*a corrida tenha acabado após todos passarem na linha de*/
        /*chegada.                                               */
        winners[c[i].team]++;
        
        /* Para isso, temos que ver qual terceiro ciclista acabou antes */
        if (winners[c[i].team] == 3)
            thirdTick[c[i].team] = c[i].finalTick;
        
        /* Exibindo ranking */
        if (c[i].broken)
            printf("%do: Ciclista %d (Equipe %d) - ACIDENTE (Volta %d)\n", 
                i+1, c[i].id, c[i].team+1, c[i].lap+1);
        else if (thirdEnd == 0)
            printf("%do: Ciclista %d (Equipe %d) - %.3fs\n", 
                i+1, c[i].id, c[i].team+1, (float) (c[i].finalTick*60)/1000);
        else
            printf("%do: Ciclista %d (Equipe %d) - distancia percorrida: %.3fm\n", 
                i+1, c[i].id, c[i].team+1, dist(c[i]));
    }
    
    /* Se a flag para fim de corrida pela ultrapassagem do terceiro ciclista */
    /*for diferente de 0, então imprimimos qual equipe foi vitoriosa.        */
    if (thirdEnd == 1) {
        printf("\nO terceiro da equipe 1 ultrapassou o terceiro da equipe 2!\n");
        printf("Vitória da equipe 1!\n\n"); 
    }
    else if (thirdEnd == 2) {
        printf("\nO terceiro da equipe 2 ultrapassou o terceiro da equipe 1!\n");
        printf("Vitória da equipe 2!\n\n"); 
    }
    else if (thirdTick[0] < thirdTick[1])
        printf("\nVitória da equipe 1!\n\n");    
    else if(thirdTick[0] > thirdTick[1])
        printf("\nVitória da equipe 2!\n\n");
    else
        printf("\nEmpate!\n\n");
}

/* waitForSync: função que sincroniza todas as thread após uma iteração.   */
/*utiliza duas barreiras pra isso, entre essas barreiras temos um código de*/
/*controle do funcionamento da corrida.                                    */
void waitForSync() {    
    
    /* sincronizar na barreira para o intervalo seguinte */
    int res = pthread_barrier_wait (&barrera);

    /* esta condição só vai ser executada em uma das threads */
    if (res == PTHREAD_BARRIER_SERIAL_THREAD) {
        manageRace();
    }

    /* mais uma barreira, pra garantir que ninguém vai recomeçar */
    /*a volta sem saber se a corrida acabou                      */
    pthread_barrier_wait (&barrera);
}
/* posChange: recebe um inteiro i e faz as verificações relativas à mu- */
/*dança de posição do ciclista identificado por i para a posição nextPos*/
void  posChange (int i, int nextPos) {
    int j, index1, index2, first, second, passedLap;
    float dist1, dist2;

    /* Verificar se houve um incremento de volta.                      */
    if ((c[i].team == 0 && nextPos == 0 && c[i].pos == 0.0) || 
        (c[i].team == 1 && nextPos == d/2 && c[i].pos == d/2)) {
        c[i].lap++;
        
        /* Vverificamos se ele é o primeiro a mudar de volta */
        pthread_mutex_lock(&lapmutex);
        {
            if(c[i].lap > globalLap) {
                globalLap++;
                timeToChange = TRUE;

                /* A cada quatro voltas, chance de alguem quebrar */
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
/* manageRace: código que gerencia o funcionamento da corrida ao fim */
/*de cada iteração dos ciclistas. Só é executado em uma thread.      */
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
        printf("\n\n");
    }
    
    /* Verificar se é necessario quebrar algum ciclista */
    if (timeToBreak) {
        timeToBreak = FALSE;
        breakCyclist();
    }

    /* Verifica se é necessário alterar as velocidades */
    if (mode == MODE_V && timeToChange) {
        timeToChange = FALSE;
        selectVelMax ();
    }
    /* Determina os 3 primeiros corredores de cada equipe   */
    t1[0] = c[0];
    t1[1] = c[1];
    t1[2] = c[2];
    qsort ((void *) t1, 3, sizeof (CYCLIST), comparator);

    /* Insere cada ciclista no final do vetor t1 e reordena,  */
    /*assim garantimos que no final da iteração os 3 primeiros*/
    /*elementos do vetor são os 3 primeiros da equipe         */
    for (j = 3; j < cycsize / 2; j++) {
        t1[3] = c[j];
        qsort ((void *) t1, 4, sizeof (CYCLIST), comparator);
    }

    /* Fazemos de forma semelhante para a equipe 2.           */
    t2[0] = c[j];
    t2[1] = c[j + 1];
    t2[2] = c[j + 2];
    qsort ((void *) t2, 3, sizeof (CYCLIST), comparator);
    for (j = j + 3; j < cycsize; j++) {
        t2[3] = c[j];
        qsort ((void *) t2, 4, sizeof (CYCLIST), comparator);
    }

    /* Se o terceiro ciclista de uma equipe tiver ultrapassado o terceiro*/
    /*ciclista de outra, a corrida acaba.                                */
    if (dist(t1[2]) >= d/2 + dist(t2[2])) {
        raceEnd = 1;
        thirdEnd = 1;
        return;
    }
    else if (dist(t2[2]) >= d/2 + dist(t1[2])) {
        raceEnd = 1;
        thirdEnd = 2;
        return;
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

/* comparator: função que ordena os ciclistas por ordem de che- */
/*gada, comparando dois a dois:                                 */
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
    
    /* Caso os ciclistas tenham passado pela linha de chegada, ordenamos*/
    /*pelo finalTick.                                                   */
    if (c1->finalTick != 0 && c2->finalTick != 0)
        return c1->finalTick - c2->finalTick;
    else return dist(*c2) - dist (*c1);
}
