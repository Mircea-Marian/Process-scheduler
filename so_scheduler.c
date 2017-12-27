/* Popa Mircea-Marian 332CB */
#include "scheduler_test.h"
#include "list.h"
#include "priorityQueue.h"
#include "util.h"
#include <fcntl.h>
#include <sys/types.h>
#include <semaphore.h>
#include <stdlib.h>
/* Se da drumul la urmatorul thread din coada. */
#define releaseNextThread() {\
	runningThread = popPriorityQueue(threadQueue);\
	sem_post(runningThread->sem);\
	}

/* Lista in care se tine istoria thread-urilor */
static SimpleLinkedList_A threadInfoList;

/* Coada pe care se fac operatii atomice */
static PriorityQueue_P threadQueue;

/* Vector de liste in care se vor tine thread-uri */
/* ce asteapta dupa un eveniment */
static SimpleLinkedList_A waitingLists[256];

static unsigned int initialQuantum, ioNo, waitingThreadsNo[256];
static unsigned char hasBeenInitiated = 0, startedFirstThread;

/* Vector de liste in care se vor tine thread-uri */
/* ce asteapta dupa un eveniment */
static threadInfo_P runningThread;

/* Element de sincronizare care asigura sa se iese din */
/* so_end() numai dupa ce toate thread-urile si-au */
/* terminat task-urile */
static pthread_mutex_t doneMutex = PTHREAD_MUTEX_INITIALIZER;


DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io)
{
	unsigned int i;

	/* Se verifica contitiile de eroare. */
	if (time_quantum == 0 || io > SO_MAX_NUM_EVENTS || hasBeenInitiated)
		return -1;

	/* Se initilizeaza datele. */
	ioNo = io;
	hasBeenInitiated = 1;
	initialQuantum = time_quantum;
	threadInfoList = newSimpleLinkedList();
	threadQueue = initializePriorityQueue();
	startedFirstThread = 0;

	/* Se initializeaza datele legate de IO */
	for (i = 0 ; i < io ; i++) {
		waitingThreadsNo[i] = 0;
		waitingLists[i] = newSimpleLinkedList();
	}

	return 0;
}

/* Se blocheaza executia programului daca este necesar. */
void continueOrBlockExecution(void)
{
	threadInfo_P aux;

	/* Daca cuanta de timp a expirat sau exista in capul cozii */
	/* un thread cu prioritate mai mare este dat la o parte */
	/* thread-ul curent. */
	if (!runningThread->quantum ||
		getMaxAvailableItemPrio(threadQueue) > runningThread->prio) {

		if (!runningThread->quantum)
			runningThread->quantum = initialQuantum;
		addToPriorityQueue(threadQueue, runningThread,
			runningThread->prio);

		aux = runningThread;

		releaseNextThread()

		sem_wait(aux->sem);
	}
}

/* Un destructor pentru "threadInfo_P" */
void freethreadInfo(void *thInfo)
{
	sem_destroy(((threadInfo_P)(thInfo))->sem);
	free(((threadInfo_P)(thInfo))->sem);
	free((threadInfo_P)thInfo);
}

/* Functia care este rulata la inceputul vietii unui */
/* thread */
void *threadInit(void *arg)
{
	CreationInfo_P creationInfo = (CreationInfo_P)arg;
	threadInfo_P thInfo = (threadInfo_P)malloc(sizeof(threadInfo_T));
	so_handler *funcToExec = creationInfo->func;

	DIE(!thInfo, "malloc err !\n");

	thInfo->sem  = (sem_t *)malloc(sizeof(sem_t));
	DIE(!thInfo->sem , "malloc err !\n");

	/* Se initializeaza informatia despre thread-ul curent. */
	sem_init(thInfo->sem, 0, 0);
	thInfo->quantum = initialQuantum;
	thInfo->prio = creationInfo->prio;

	creationInfo->listCell = thInfo;

	addToPriorityQueue(threadQueue, thInfo, creationInfo->prio);

	addToList(threadInfoList, (void *)thInfo);

	/* Se anunta thread-ul parinte ca s-a finalizat initializarea */
	/* thread-ului copil. */
	sem_post(creationInfo->sem);

	/* Thread-ul copil se blocheaza pana v-a fii scos din coada. */
	sem_wait(thInfo->sem);

	funcToExec(thInfo->prio);

	/* S-a incheiat de rulat ultimul thread. */
	if (getMaxAvailableItemPrio(threadQueue) == -1)
		/* Se anunta ca s-a terminat si ultimul thread din */
		/* sistem. */
		pthread_mutex_unlock(&doneMutex);
	else
		/* Daca nu este ultimul thread din sistem atunci se */
		/* normal drumul thread-ului din capul listei. */
		releaseNextThread();
	return NULL;
}

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
	tid_t rez;
	CreationInfo_P arg = (CreationInfo_P)malloc(sizeof(CreationInfo_T));
	sem_t *sem = (sem_t *)malloc(sizeof(sem_t));

	DIE(!sem, "malloc error !\n");
	DIE(!arg, "malloc error !\n");

	/* In cazul in care se initializeaza primul thread */
	if (!startedFirstThread)
		pthread_mutex_lock(&doneMutex);

	/* Cazuri ce produc erori */
	if (func == NULL || priority > SO_MAX_PRIO) {
		free(sem);
		free(arg);
		pthread_mutex_unlock(&doneMutex);
		return INVALID_TID;
	}

	/* Se initializeaza semaforul care va ajuta */
	/* la sincronizarea dintre thread-ul tata si */
	/* thread-ul copil. */
	sem_init(sem, 0, 0);

	/* Se initializeaza datele necesare creeri thread-ului copil. */
	arg->func = func;
	arg->prio = priority;
	arg->sem = sem;

	DIE(
		pthread_create(&rez, NULL, &threadInit, (void *)arg) != 0,
		"pthread_create error !\n");

	/* Se blocheaza thread-ul parinte pana cand thread-ul copil ii */
	/* semnalizeaza ca si-a initializat datele. */
	sem_wait(sem);
	sem_destroy(sem);
	free(sem);

	arg->listCell->threadId = rez;
	free(arg);


	if (!startedFirstThread) {
		/* Nu se preempteaza primul thread. */
		startedFirstThread = 1;
		runningThread = popPriorityQueue(threadQueue);
		sem_post(runningThread->sem);
	} else {
		/* Se preempteaza thread-ul. */
		runningThread->quantum--;
		continueOrBlockExecution();
	}

	return rez;
}

DECL_PREFIX int so_wait(unsigned int io)
{
	threadInfo_P aux = runningThread;

	if (io >= ioNo)
		return -1;

	addToList(waitingLists[io], runningThread);
	releaseNextThread();

	/* Se procedeaza la fel ca atunci cand thread-ul este preemptat, */
	/* dar nu se baga thread-ul in coada de prioritati, ci intr-o lista */
	/* corespunzatoare unui eveniment IO. */
	aux->quantum = (aux->quantum - 1 == 0) ?
		initialQuantum : aux->quantum - 1;

	sem_wait(aux->sem);

	return 0;
}

DECL_PREFIX int so_signal(unsigned int io)
{
	threadInfo_P aux;
	int thNo = 0;

	if (io >= ioNo)
		return -1;

	/* Se baga thread-urile care erau in lista de asteptare */
	/* in coada cu prioritati pentru a fii valide pentru */
	/* rulare pe procesor. */
	aux = (threadInfo_P)getListHead(waitingLists[io]);
	while (aux) {

		addToPriorityQueue(threadQueue, aux, aux->prio);
		thNo++;

		aux = (threadInfo_P)getListHead(waitingLists[io]);
	}

	/* Preemptie */
	runningThread->quantum--;
	continueOrBlockExecution();

	return thNo;
}

DECL_PREFIX void so_exec(void)
{
	/* Preemptie */
	runningThread->quantum--;
	continueOrBlockExecution();
}

DECL_PREFIX void so_end(void)
{
	SimpleLinkedList_P iterator;
	unsigned int i;

	if (hasBeenInitiated) {
		/* Se asteapta pana ce toate task-urile s-au finalizat */
		pthread_mutex_lock(&doneMutex);
		pthread_mutex_unlock(&doneMutex);

		/* Se asteapta fiecare thread sa se termine si sa elibereze */
		/* resursele ocupate. */
		iterator = *threadInfoList;
		while (iterator) {
			DIE(pthread_join(
				((threadInfo_P)(iterator->info))->threadId
				, NULL) != 0, "pthread_join");
			iterator = iterator->next;
		}

		deleteAllList(threadInfoList, freethreadInfo);
		freePriorityQueue(threadQueue);
		hasBeenInitiated = 0;
		startedFirstThread = 0;
		for (i = 0 ; i < ioNo ; i++)
			deleteAllList(waitingLists[i], uselessDeallocator);

	}
}
