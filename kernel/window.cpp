#include "window.h"
#include "task.h"
#include "video.h"
#include "Utils.h"

#include "malloc.h"
#include "ListEntry.h"
#include "memory.h"




LPWINDOWSINFO gWindowsList = 0;





void initWindowList() {
	gWindowsList = (LPWINDOWSINFO)__kMalloc(WINDOW_LIST_BUF_SIZE);
	__memset((char*)gWindowsList, 0, WINDOW_LIST_BUF_SIZE);

	initListEntry((LPLIST_ENTRY)&gWindowsList->list);
}

LPWINDOWSINFO isWindowExist(char * wname) {
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList->list.next;
	LPWINDOWSINFO tmp = info;
	do
	{
		if (info == 0) {
			break;
		}
		if (info->valid && info->windowname[0] && __strcmp(info->windowname,wname)== 0 )
		{
			return info;
		}
		else {
			info = (LPWINDOWSINFO)(info->list.next);
		}
	} while (info && info != tmp);

	return 0;
}

LPWINDOWSINFO isWindowExist(DWORD wid) {
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList->list.next;
	LPWINDOWSINFO tmp = info;
	do
	{
		if (info == 0)
		{
			break;
		}
		if (info->valid && info->id == wid )
		{
			return info;
		}
		else {
			info = (LPWINDOWSINFO)(info->list.next);
		}
	} while (info && info != tmp);

	return 0;
}

LPWINDOWSINFO getFreeWindow() {
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList;

	int cnt = WINDOW_LIST_BUF_SIZE / sizeof(WINDOWSINFO);
	for (int i = 1; i < cnt; i++)
	{
		if (info[i].valid == 0)
		{
			return &info[i];
		}
	}
	return 0;
}


LPWINDOWCLASS getWindow(int wid) {
	LPWINDOWSINFO window = wid + gWindowsList;
	return window->window;
}

DWORD isTopWindow(int wid) {
	LPWINDOWSINFO window = wid + gWindowsList;
	if (window == (LPWINDOWSINFO)(gWindowsList->list.prev) )
	{
		return TRUE;
	}
	return FALSE;

	//return (window == gWindowsList->list.prev ? TRUE : FALSE);
}


DWORD getTopWindow() {
	LPWINDOWSINFO prev =(LPWINDOWSINFO)gWindowsList->list.prev;
	return prev->id;
}


int addWindow(DWORD wc, DWORD *x, DWORD *y, int color,char * wname) {
	char szout[1024];

	LPWINDOWSINFO window = isWindowExist(wname);
	if (window)
	{
		return FALSE;
	}

	window = getFreeWindow();
	if (window == FALSE)
	{
		__printf(szout, "getFreeWindow error,first:%x,last:%x\n", gWindowsList->list.next, gWindowsList->list.prev);
		return -1;
	}

	window->window =(WINDOWCLASS*) wc;

	window->valid = TRUE;
	int i = window - gWindowsList;

	window->id = i;

	window->cursorColor = color;

	window->cursorX = x;
	window->cursorY = y;

	__strncpy(window->windowname, wname, WINDOW_NAME_LIMIT - 1);

	addlistTail(&gWindowsList->list, &window->list);

// 	__printf(szout, "add windowid:%x,first:%x,top:%x\n", i, gWindowsList ,gWindowLast);

	return i;
}


int removeWindow(int id) {

	LPWINDOWSINFO window = gWindowsList + id;

	window->valid = FALSE;
	
	removelist(&gWindowsList->list, &window->list);

	return TRUE;
}
















/*
int destroyWindows() {
	int cnt = 0;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;

	LPWINDOWCLASS windows = p->window;

	if (windows)
	{
		LPWINDOWCLASS next = windows;

		while (next->next)
		{
			next = next->next;
		}

		while (next)
		{
			__removeWindow(next);
			next = next->prev;
			cnt++;
		}
	}

	return cnt;
}


LPWINDOWCLASS getWindowFromName(char * winname) {

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	for (int i = 0; i < TASK_LIMIT_TOTAL; i++) {

		if ((tss[i].status == TASK_RUN))
		{
			LPWINDOWCLASS window = tss[i].window;
			while (window)
			{
				if (__strcmp(window->caption, winname) == 0)
				{
					return window;
				}
				window = window->next;
			}
		}
	}

	return 0;
}

LPWINDOWCLASS insertProcWindow(LPWINDOWCLASS window) {

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	LPWINDOWCLASS w = p->window;
	if (w)
	{
		while (w->next)
		{
			w = w->next;
		}
		w->next = window;

		window->prev = w;
	}
	else {
		p->window = window;

		window->prev = 0;
	}
	window->next = 0;
	return window;
}

LPWINDOWCLASS removeProcWindow() {
	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	LPWINDOWCLASS w = p->window;
	while (w)
	{
		LPWINDOWCLASS t = w;
		w = w->next;
		__kFree((DWORD)t);
	}

	return w;
}

int getOverlapRect(LPRECT r1, LPRECT r2, LPRECT res) {
	if ((r1->right > r2->left && r1->bottom > r2->top) || (r2->right > r1->left && r2->bottom > r1->top)) {

	}
	else {
		res->bottom = 0;
		res->left = 0;
		res->top = 0;
		res->right = 0;
		return FALSE;
	}

	int l, t, r, b;

	if (r1->left > r2->left) {
		l = r1->left;
	}
	else {
		l = r2->left;
	}

	if (r1->top > r2->top) {
		t = r1->top;
	}
	else {
		t = r2->top;
	}

	if (r1->right > r2->right) {
		l = r2->right;
	}
	else {
		l = r1->right;
	}

	if (r1->bottom > r2->bottom) {
		l = r2->bottom;
	}
	else {
		l = r1->bottom;
	}
	res->bottom = b;
	res->left = l;
	res->right = r;
	res->top = t;
	return TRUE;
}

LPWINDOWCLASS getLastWindow() {
	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	LPWINDOWCLASS w = p->window;
	while (w&& w->next)
	{
		w = w->next;
	}

	return w;
}

int placeFocus(int x,int y) {
	int res = 0;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	LPWINDOWCLASS w = getLastWindow();
	while (w)
	{
		if ((w->left <= x && w->right >= x) && (w->top <= y && w->bottom >= y)) {

			if (w->prev) {
				w->prev->next = w->next;
			}
			
			if (w->next) {
				w->next->prev = w->prev;
			}
			
			w->prev = 0;
			w->next = p->window;
			p->window = w;

			LPWINDOWCLASS surf = w;
			if (surf && surf->next) {
				do {
					surf = surf->next;

					RECT r;
					RECT r1;
					RECT r2;
					r1.bottom = surf->bottom;
					r1.top = surf->top;
					r1.left = surf->left;
					r1.right = surf->right;
					r2.bottom = w->bottom;
					r2.top = w->top;
					r2.left = w->left;
					r2.right = w->right;
					res = getOverlapRect(&r1,&r2,&r);

					char* ol_src = (char*)surf->backBuf + ( (r.top - surf->top) * surf->width + r.left - surf->left)*gBytesPerPixel;
					int olsize = (r.bottom - r.top) * (r.right - r.left) * gBytesPerPixel;

					char * ol_dst = (char*)__getpos(w->pos.x,w->pos.y) + ((r.top - surf->top) * w->width + r.left - w->left) * gBytesPerPixel;
			
					__memcpy(ol_dst, ol_src, olsize);


					char* src = (char*)w->backBuf + ((r.top - w->top) * w->width + r.left - w->left) * gBytesPerPixel;

					char* dst = ol_src;

					__memcpy(dst, src, olsize);
					
				} while (surf && surf->next);
			}
		}
		else {
			w = w->prev;
		}
	}

	return 0;
}

*/