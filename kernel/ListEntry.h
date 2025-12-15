#pragma once

#include "def.h"

#pragma pack(1)
typedef struct  _LIST_ENTRY
{
	_LIST_ENTRY * next;
	_LIST_ENTRY * prev;
}LIST_ENTRY, *LPLIST_ENTRY;

#pragma pack()

int GetListSize(LIST_ENTRY* list);


#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport) void InitListEntry(LIST_ENTRY * list);

extern "C"  __declspec(dllexport) int InsertListHead(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllexport) int InsertListTail(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllexport) LIST_ENTRY * SearchList(LIST_ENTRY * head, LPLIST_ENTRY list);

extern "C"  __declspec(dllexport) int RemoveList(LPLIST_ENTRY h,LPLIST_ENTRY list);
#else
extern "C"  __declspec(dllimport) void InitListEntry(LIST_ENTRY * head);

extern "C"  __declspec(dllimport) int InsertListHead(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllimport) int InsertListTail(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllimport) LIST_ENTRY * SearchList(LIST_ENTRY * head, LPLIST_ENTRY list);

extern "C"  __declspec(dllimport) int RemoveList(LPLIST_ENTRY h,LPLIST_ENTRY list);
#endif