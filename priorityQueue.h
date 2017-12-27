/* Popa Mircea-Marian 332CB */
#include "so_scheduler.h"
#include "list.h"
#include "so_scheduler.h"
#include <pthread.h>

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#define isSimpleQueueEmpty(p_q) (!(*(p_q->head)))

/* structura unei cozi simple */
typedef struct SimpleQueue {
	SimpleLinkedList_A head, tail;
} SimpleQueue_T, *SimpleQueue_P;

/* constrcutor */
SimpleQueue_P newSimpleQueue(void);

/* se adauga in coada */
int addToSimpleQueue(SimpleQueue_P q, threadInfo_P mutex);

/* se scoate din coada */
threadInfo_P popSimpleQueue(SimpleQueue_P q);

/* se elibereaza resursele cozii */
void freeSimpleQueue(SimpleQueue_P q);

/* structura unei cozi cu prioritati */
typedef struct PriorityQueue {
	SimpleQueue_P simpleQueueArray[SO_MAX_PRIO + 1];
	pthread_mutex_t accessMutex;
} PriorityQueue_T, *PriorityQueue_P;

/* constructor */
PriorityQueue_P initializePriorityQueue(void);

/* adauga la coada */
int addToPriorityQueue(PriorityQueue_P pq, threadInfo_P mutex,
	unsigned int prio);

/* intoarce prioritatea maxima din coada */
int getMaxAvailableItemPrio(PriorityQueue_P pq);

/* scoate capul listei si elibereaza resursele pentru el */
threadInfo_P popPriorityQueue(PriorityQueue_P pq);

/* se elibereaza resursele cozii */
void freePriorityQueue(PriorityQueue_P pq);

#endif
