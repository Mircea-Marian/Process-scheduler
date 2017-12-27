/* Popa Mircea-Marian 332CB */
#include "priorityQueue.h"
#include "list.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#define PRIO_ERR_CODE -3
SimpleQueue_P newSimpleQueue(void)
{
	SimpleQueue_P rez;

	rez = (SimpleQueue_P)malloc(sizeof(SimpleQueue_T));
	if (!rez)
		return NULL;

	rez->head = newSimpleLinkedList();
	if (!rez->head) {
		free(rez);
		return NULL;
	}

	rez->tail = newSimpleLinkedList();
	if (!rez->tail) {
		free(rez->head);
		free(rez);
		return NULL;
	}

	return rez;
}


int addToSimpleQueue(SimpleQueue_P q, threadInfo_P mutex)
{
	SimpleLinkedList_P aux;

	if (!q)
		return NULL_LIST_PARAM;

	if (!(*(q->head))) {
		addToList(q->head, (void *)mutex);
		*(q->tail) = *(q->head);
		return 0;
	}

	if (*(q->tail) == *(q->head)) {
		aux = allocCell((void *)mutex);
		(*(q->head))->next = aux;
		*(q->tail) = aux;
		return 0;
	}

	aux = allocCell((void *)mutex);
	(*(q->tail))->next = aux;
	*(q->tail) = aux;

	return 0;
}

threadInfo_P popSimpleQueue(SimpleQueue_P q)
{
	threadInfo_P mutex;

	if (!q)
		return NULL;

	if (isListEmpty(q->head))
		return NULL;

	if (*(q->tail) == *(q->head)) {
		mutex = (threadInfo_P)((*(q->head))->info);
		deleteHeadOfList(q->head, uselessDeallocator);
		*(q->tail) = NULL;
		return mutex;
	}

	mutex = (threadInfo_P)((*(q->head))->info);
	deleteHeadOfList(q->head, uselessDeallocator);
	return mutex;
}

void freeSimpleQueue(SimpleQueue_P q)
{
	if (q) {
		if (!isListEmpty(q->head)) {
			deleteAllList(q->head, uselessDeallocator);
			*q->head = *q->tail = NULL;
			free(q->tail);
		} else {
			free(q->tail);
			free(q->head);
		}
		free(q);
	}
}

PriorityQueue_P initializePriorityQueue(void)
{
	PriorityQueue_P p_q;
	unsigned int i, j;

	p_q = (PriorityQueue_P)malloc(sizeof(PriorityQueue_T));
	if (!p_q)
		return NULL;

	for (i = 0 ; i < SO_MAX_PRIO + 1 ; i++) {
		p_q->simpleQueueArray[i] = newSimpleQueue();

		if (!p_q->simpleQueueArray[i]) {
			for (j = 0 ; j < i ; j++)
				freeSimpleQueue(p_q->simpleQueueArray[j]);
			free(p_q);
			return NULL;
		}
	}

	pthread_mutex_init(&(p_q->accessMutex), NULL);

	return p_q;
}

int addToPriorityQueue(PriorityQueue_P pq, threadInfo_P mutex,
	unsigned int prio)
{

	if (!pq)
		return NULL_LIST_PARAM;

	if (prio > SO_MAX_PRIO)
		return PRIO_ERR_CODE;

	pthread_mutex_lock(&(pq->accessMutex));
	if (addToSimpleQueue(pq->simpleQueueArray[prio], mutex)) {
		pthread_mutex_unlock(&(pq->accessMutex));
		return MEMORY_ALLOC_ERR_CODE;
	}
	pthread_mutex_unlock(&(pq->accessMutex));
	return 0;
}

int getMaxAvailableItemPrio(PriorityQueue_P pq)
{
	int i;

	for (i = SO_MAX_PRIO ; i >= 0 ; i--)
		if (!isSimpleQueueEmpty(pq->simpleQueueArray[i]))
			return i;
	return -1;
}

threadInfo_P popPriorityQueue(PriorityQueue_P pq)
{
	int i;

	pthread_mutex_lock(&(pq->accessMutex));
	for (i = SO_MAX_PRIO ; i >= 0 ; i--)
		if (!isSimpleQueueEmpty(pq->simpleQueueArray[i])) {
			pthread_mutex_unlock(&(pq->accessMutex));
			return popSimpleQueue(pq->simpleQueueArray[i]);
		}
	pthread_mutex_unlock(&(pq->accessMutex));
	return NULL;
}

void freePriorityQueue(PriorityQueue_P pq)
{
	unsigned int i;

	if (pq) {
		for (i = 0 ; i < SO_MAX_PRIO + 1 ; i++)
			freeSimpleQueue(pq->simpleQueueArray[i]);
		pthread_mutex_destroy(&(pq->accessMutex));
		free(pq);
	}
}
