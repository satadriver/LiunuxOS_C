#pragma once

#include "def.h"

#pragma pack(1)
typedef struct  _LIST_ENTRY
{
	_LIST_ENTRY * next;
	_LIST_ENTRY * prev;
}LIST_ENTRY, *LPLIST_ENTRY;

#pragma pack()


#ifdef DLL_EXPORT

extern "C"  __declspec(dllexport) void initListEntry(LIST_ENTRY * list);

extern "C"  __declspec(dllexport) void addlistHead(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllexport) void addlistTail(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllexport) int searchList(LIST_ENTRY * head, LPLIST_ENTRY list);

extern "C"  __declspec(dllexport) void removelist(LPLIST_ENTRY list);
#else
extern "C"  __declspec(dllimport) void initListEntry(LIST_ENTRY * list);

extern "C"  __declspec(dllimport) void addlistHead(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllimport) void addlistTail(LIST_ENTRY * head, LIST_ENTRY * list);

extern "C"  __declspec(dllimport) int searchList(LIST_ENTRY * head, LPLIST_ENTRY list);

extern "C"  __declspec(dllimport) void removelist(LPLIST_ENTRY list);
#endif