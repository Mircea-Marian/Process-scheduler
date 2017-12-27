#include <stdlib.h>
#include "list.h"

void uselessDeallocator(void *p)
{
}

SimpleLinkedList_P allocCell(void *info)
{
	SimpleLinkedList_P rez =
	    (SimpleLinkedList_P) malloc(sizeof(SimpleLinkedList_T));

	if (!rez)
		return NULL;

	rez->info = info;
	rez->next = NULL;

	return rez;
}

SimpleLinkedList_A newSimpleLinkedList(void)
{
	SimpleLinkedList_A rez = (SimpleLinkedList_A)
	    malloc(sizeof(SimpleLinkedList_P));
	*rez = NULL;
	return rez;
}

int addToList(SimpleLinkedList_A list, void *info)
{
	SimpleLinkedList_P rez;

	if (!list)
		return NULL_LIST_PARAM;

	rez = allocCell(info);
	if (!rez)
		return MEMORY_ALLOC_ERR_CODE;

	if (!(*list))
		*list = rez;
	else {
		rez->next = *list;
		*list = rez;
	}

	return 0;
}

void deleteHeadOfList(SimpleLinkedList_A list, void (*freeInfo) (void *))
{
	SimpleLinkedList_P aux;

	if (list && *list) {
		aux = *list;
		*list = aux->next;

		(*freeInfo) (aux->info);

		free(aux);
	}
}

void *getListHead(SimpleLinkedList_A list)
{
	void *rez;

	if (list && *list) {
		rez = (*list)->info;
		deleteHeadOfList(list, uselessDeallocator);
		return rez;
	}
	return NULL;
}

void deleteCellWithInfo(SimpleLinkedList_A list, void *info,
			void (*freeInfo) (void *))
{
	SimpleLinkedList_P iteratorBehind, iterator;

	if (list && *list) {
		if ((*list)->info == info)
			deleteHeadOfList(list, freeInfo);
		else if ((*list)->next) {
			iteratorBehind = *list;
			iterator = (*list)->next;

			while (iterator && iterator->info != info) {
				iterator = iterator->next;
				iteratorBehind = iteratorBehind->next;
			}

			if (iterator) {
				iteratorBehind->next = iterator->next;
				(*freeInfo) (iterator->info);
				free(iterator);
			}
		}
	}
}

void clearList(SimpleLinkedList_A list, void (*freeInfo) (void *))
{
	if (list) {
		while ((*list))
			deleteHeadOfList(list, freeInfo);
	}
}

void deleteAllList(SimpleLinkedList_A list, void (*freeInfo) (void *))
{
	clearList(list, freeInfo);

	if (list)
		free(list);
}
