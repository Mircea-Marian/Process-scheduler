
#ifndef LIST_H
#define LIST_H

#define MEMORY_ALLOC_ERR_CODE -1
#define NULL_LIST_PARAM -2
#define isListEmpty(a_l) (!(*a_l))

/* lista generica */
typedef struct SimpleLinkedList {
	void *info;
	struct SimpleLinkedList *next;
} SimpleLinkedList_T, *SimpleLinkedList_P, **SimpleLinkedList_A;

/* dealocator care nu face nimic */
void uselessDeallocator(void *p);

/* un alocator de celula */
SimpleLinkedList_P allocCell(void *info);

/* constructor de lista */
SimpleLinkedList_A newSimpleLinkedList(void);

/* adauga la lista */
int addToList(SimpleLinkedList_A list, void *info);

/* sterge capul listei */
void deleteHeadOfList(SimpleLinkedList_A list, void (*freeInfo)(void *));

/* intoarce capul listei si il sterge din lista */
void *getListHead(SimpleLinkedList_A list);

/* cauta si sterge din lista casuta cu informatia respectiva */
void deleteCellWithInfo(SimpleLinkedList_A list, void *info,
	void (*freeInfo)(void *));

/* sterge toate elementele din lista dar nu si lista in sine */
void clearList(SimpleLinkedList_A list, void (*freeInfo)(void *));

/* sterge cu totul lsita */
void deleteAllList(SimpleLinkedList_A list, void (*freeInfo)(void *));

#endif
