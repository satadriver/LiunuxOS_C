
#include "ListEntry.h"


void initListEntry(LIST_ENTRY * list) {
	list->next = list;
	list->prev = list;
}

//add after head
void addlistTail(LIST_ENTRY * head, LIST_ENTRY * list) {
	if (head->next == 0 || head->prev == 0 || list == head) 
	{
		head->next = list;
		head->prev = list;
		return;
	}

	LIST_ENTRY * headnext = head->next;

	list->prev = head;
	list->next = headnext;

	head->next = list;
	headnext->prev = list;
}

//add to head
void addlistHead(LIST_ENTRY * head, LIST_ENTRY * list) {
	if (head->next == 0 || head->prev == 0 || list == head)
	{
		head->next = list;
		head->prev = list;
		return;
	}
	LIST_ENTRY * headprev = head->prev;

	list->prev = headprev;
	list->next = head;

	headprev->next = list;
	head->prev = list;
}

int searchList(LIST_ENTRY * head, LPLIST_ENTRY list) {
	if (head == list)
	{
		return TRUE;
	}

	LPLIST_ENTRY e = head->next;
	do
	{
		if (e == 0) {
			break;
		}
		if ( e == list)
		{
			return TRUE;
		}

		e = e->next;
		
	} while (TRUE);

	return FALSE;
}


void removelist(LPLIST_ENTRY list) {

	if (list->next == 0 && list->prev == 0)
	{
		//error
		return;
	}
	else if (list->next == 0 && list->prev)
	{
		//error
		return;
	}
	else if (list->prev == 0 && list->next)
	{
		//error
		return;
	}
	else {
		if (list->next == list && list->prev == list) {
			list->next = 0;
			list->prev = 0;
		}
		else {
			LPLIST_ENTRY next = list->next;
			LPLIST_ENTRY prev = list->prev;
			list->prev->next = next;
			list->next->prev = prev;
			//list->next = 0;
			//list->prev = 0;
		}
	}
}



