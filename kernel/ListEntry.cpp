
#include "ListEntry.h"
#include "utils.h"

void InitListEntry(LIST_ENTRY * list) {
	if (list == 0) {
		return;
	}

	list->next = 0;
	list->prev = 0;
}

int GetListSize(LIST_ENTRY* list) {
	if (list == 0) {
		return 0;
	}

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


LIST_ENTRY* SearchList(LIST_ENTRY* head, LPLIST_ENTRY list) {
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
int InsertListTail(LIST_ENTRY * head, LIST_ENTRY * list) {
	int result = 0;
	if (head == 0 || list == 0 || list == head)
	{
		return result;
	}

	char szout[256];

	LIST_ENTRY * next = head->next;
	LIST_ENTRY* prev = head->prev;

	if ((next == 0 && prev != 0) || (next != 0 && prev == 0)) {

		__printf(szout, "%s node error!\r\n",__FUNCTION__);
		return result;
	}

	if (next && prev ) {
		next->prev = list;
		prev->next = list;

		list->next = next;
		list->prev = prev;

		head->prev = list;
		result = 1;
	}
	else {

		head->next = list;
		head->prev = list;

		list->prev = list;
		list->next = list;
		result = 2;
	}
	return result;
}

//add to head
int InsertListHead(LIST_ENTRY * head, LIST_ENTRY * list) {
	int result = 0;
	if (head == 0 || list == 0 || list == head)
	{
		return result;
	}

	LIST_ENTRY* next = head->next;
	LIST_ENTRY* prev = head->prev;
	char szout[256];
	if ((next == 0 && prev != 0) || (next != 0 && prev == 0) ) {

		__printf(szout, "%s node error!\r\n", __FUNCTION__);
		return result;
	}

	if (next && prev) {
		next->prev = list;
		prev->next = list;

		list->next = next;
		list->prev = prev;

		head->next = list;
		result = 1;
	}
	else {
		head->next = list;
		head->prev = list;

		list->prev = list;
		list->next = list;
		result = 2;
	}
	return result;
}


int RemoveList(LPLIST_ENTRY h,LPLIST_ENTRY list) {
	int result = 0;
	LPLIST_ENTRY r = SearchList(h, list);
	if (r == FALSE) {
		return result;
	}

	if (h->next == list && h->prev == list) {
		h->prev = 0;
		h->next = 0;
		result = 1;
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
		result = 2;
	}
	else if (h->prev == list && h->next != list) 
	{
		LPLIST_ENTRY next = list->next;
		LPLIST_ENTRY prev = list->prev;
		prev->next = next;
		next->prev = prev;

		h->prev = prev;
		result = 3;
	}
	else {
		LPLIST_ENTRY next = list->next;
		LPLIST_ENTRY prev = list->prev;
		prev->next = next;
		next->prev = prev;
		//list->next = 0;
		//list->prev = 0;
		result = 4;
	}
	return result;
}



