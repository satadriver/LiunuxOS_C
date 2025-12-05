

#include "ListArray.h"




#include "ListEntry.h"
#include "utils.h"

void InitArrayList(LIST_ENTRY* list) {
	if (list == 0) {
		return;
	}

	list->next = 0;
	list->prev = 0;
}

LIST_ENTRY* GetEmptyItem(LIST_ENTRY* list) {
	return 0;
}


int GetArrayListSize(LIST_ENTRY* list) {
	if (list == 0) {
		return 0;
	}

	int size = 0;
	LIST_ENTRY* n = list->next;
	LIST_ENTRY* t = n;
	do {
		if (n) {
			size++;
			n = n->next;
		}
	} while (n && n != t);

	return size;
}


LIST_ENTRY* SearchArrayList(LIST_ENTRY* head, LPLIST_ENTRY list) {
	if (head == list || head == 0 || list == 0)
	{
		return FALSE;
	}

	LPLIST_ENTRY n = head->next;
	LPLIST_ENTRY base = n;

	do
	{
		if (n == 0) {
			break;
		}
		else if (n == list)
		{
			return n;
		}

		n = n->next;

	} while (n && n != base);

	return FALSE;
}

//add after head
void InsertArrayList(LIST_ENTRY* head, LIST_ENTRY* list, int type) {
	if (head == 0 || list == 0 || list == head)
	{
		return;
	}

	char szout[256];

	LIST_ENTRY* next = head->next;
	LIST_ENTRY* prev = head->prev;

	if ((next == 0 && prev != 0) || (next != 0 && prev == 0)) {

		__printf(szout, "%s node error!\r\n", __FUNCTION__);
		return;
	}

	if (next && prev) {
		next->prev = list;
		prev->next = list;

		list->next = next;
		list->prev = prev;

		
	}
	else {

		head->next = list;
		head->prev = list;

		list->prev = list;
		list->next = list;
	}

	if (type == 1) {
		head->prev = list;
	}
	else if (type == 2)
	{
		head->next = list;
	}
}



void RemoveArrayList(LPLIST_ENTRY h, LPLIST_ENTRY list) {
	LPLIST_ENTRY r = SearchList(h, list);
	if (r == FALSE) {
		return;
	}

	if (h->next == list && h->prev == list) {
		h->prev = 0;
		h->next = 0;
	}
	else if (h->next == list && h->prev != list)
	{
		LPLIST_ENTRY next = list->next;
		LPLIST_ENTRY prev = list->prev;
		prev->next = next;
		next->prev = prev;
		//list->next = 0;
		//list->prev = 0;	
		h->next = next;
	}
	else if (h->prev == list && h->next != list)
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



