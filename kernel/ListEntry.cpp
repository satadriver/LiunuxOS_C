
#include "ListEntry.h"


void initListEntry(LIST_ENTRY * list) {
	list->next = 0;
	list->prev = 0;
}

int getListSize(LIST_ENTRY* list) {
	int size = 0;
	LIST_ENTRY* n = list->next;
	LIST_ENTRY* t = n;
	do  {
		if (n) {
			size++;
			n = n->next;
		}
	} while (n && n != t);

	return size;
}

//add after head
void addlistTail(LIST_ENTRY * head, LIST_ENTRY * list) {
	if (head == 0 || list == 0 || list == head)
	{
		return;
	}

	LIST_ENTRY * next = head->next;
	LIST_ENTRY* prev = head->prev;

	if (next ) {
		next->prev = list;
		if (prev) {
			prev->next = list;
		}
		else {
			//error
		}
		list->next = next;
		list->prev = prev;

		head->prev = list;
	}
	else {
		head->next = list;
		head->prev = list;

		list->prev = list;
		list->next = list;
	}
}

//add to head
void addlistHead(LIST_ENTRY * head, LIST_ENTRY * list) {
	if (head == 0 || list == 0 || list == head)
	{
		return;
	}

	LIST_ENTRY* next = head->next;
	LIST_ENTRY* prev = head->prev;

	if (next) {

		next->prev = list;
		if (prev) {
			prev->next = list;
		}
		else {
			//error
		}
		list->next = next;
		list->prev = prev;

		head->next = list;
	}
	else {
		head->next = list;
		head->prev = list;

		list->prev = list;
		list->next = list;
	}
}

LIST_ENTRY * searchList(LIST_ENTRY * head, LPLIST_ENTRY list) {
	if (head == list || head == 0 || list == 0)
	{
		return FALSE;
	}

	LPLIST_ENTRY n = head->next;

	do
	{
		if (n == 0) {
			break;
		}
		if ( n == list)
		{
			return n;
		}

		n = n->next;
		
	} while (TRUE);

	return FALSE;
}


void removelist(LPLIST_ENTRY h,LPLIST_ENTRY list) {
	LPLIST_ENTRY r = searchList(h, list);
	if (r == FALSE) {
		return ;
	}

	if (h->next == list && h->prev == list) {
		h->prev = 0;
		h->next = 0;
	}
	else if (h->next == list ) 
	{
		LPLIST_ENTRY next = list->next;
		LPLIST_ENTRY prev = list->prev;
		prev->next = next;
		next->prev = prev;
		//list->next = 0;
		//list->prev = 0;	
		h->next = next;
	}
	else if (h->prev == list) 
	{
		LPLIST_ENTRY next = list->next;
		LPLIST_ENTRY prev = list->prev;
		prev->next = next;
		next->prev = prev;

		h->prev = prev;
	}
	else {
		LPLIST_ENTRY next = list->next;
		LPLIST_ENTRY prev = list->prev;
		prev->next = next;
		next->prev = prev;
		//list->next = 0;
		//list->prev = 0;
	}
}



