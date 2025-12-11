#include "window.h"
#include "task.h"
#include "video.h"
#include "Utils.h"

#include "malloc.h"
#include "ListEntry.h"
#include "memory.h"
#include "mouse.h"
#include "apic.h"


extern "C" __declspec(dllexport)LPWINDOWSINFO gWindowsList = 0;


extern "C" __declspec(dllexport) POPUPMENU gPopupMenu = { 0 };

unsigned long g_window_lock = 0;


void initWindowList() {
	gWindowsList = (LPWINDOWSINFO)__kMalloc(WINDOW_LIST_BUF_SIZE);
	//gWindowsList = (LPWINDOWSINFO)WINDOW_INFO_BASE;
	__memset((char*)gWindowsList, 0, WINDOW_LIST_BUF_SIZE);

	InitListEntry((LPLIST_ENTRY)&gWindowsList->list);
}

LPWINDOWSINFO __FindWindow(char * wname) {
	if (gWindowsList == 0) {
		return 0;
	}
	LPWINDOWSINFO ptr = (LPWINDOWSINFO)gWindowsList->list.next;
	LPWINDOWSINFO hdr = ptr;
	do
	{
		if (ptr == 0) {
			break;
		}
		if (ptr->valid && ptr->window->winname[0] && __strcmp(ptr->window->winname,wname)== 0 )
		{
			return ptr;
		}
		else {
			ptr = (LPWINDOWSINFO)(ptr->list.next);
		}
	} while (ptr && ptr != hdr);

	return 0;
}



LPWINDOWSINFO __FindWindowID(DWORD wid) {
	if (gWindowsList == 0) {
		return 0;
	}
	LPWINDOWSINFO ptr = (LPWINDOWSINFO)gWindowsList->list.next;
	LPWINDOWSINFO hdr = ptr;
	do
	{
		if (ptr == 0)
		{
			break;
		}
		if (ptr->valid && ptr->window->id == wid )
		{
			return ptr;
		}
		else {
			ptr = (LPWINDOWSINFO)(ptr->list.next);
		}
	} while (ptr && ptr != hdr);

	return 0;
}


//must search in reverse oder
LPWINDOWSINFO __FindProcessWindow(int tid) {
	if (gWindowsList == 0) {
		return 0;
	}
	LPWINDOWSINFO ptr = (LPWINDOWSINFO)gWindowsList->list.prev;
	LPWINDOWSINFO hdr = ptr;
	do
	{
		if (ptr == 0)
		{
			break;
		}
		if (ptr->valid && ptr->window->tid == tid )
		{
			return ptr;
		}
		else {
			ptr = (LPWINDOWSINFO)(ptr->list.prev);
		}
	} while (ptr && ptr != hdr);

	return 0;
}

int getFreeWindow() {
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList;

	int cnt = WINDOW_LIST_BUF_SIZE / sizeof(WINDOWSINFO) ;
	for (int i = 1; i < cnt; i++)
	{
		if (info[i].valid == 0)
		{
			return  i;
		}
	}
	return 0;
}

int traversalWindow(char* outbuf) {
	char* buf = outbuf;
	int size = 0;
	if (gWindowsList == 0) {
		return 0;
	}
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList->list.next;
	LPWINDOWSINFO hdr = info;
	do
	{
		if (info == 0) {
			break;
		}
		if (info->valid )
		{
			int len = __printf(buf+size, "%s window id:%d,name:%s\r\n",__FUNCTION__, info->window->id, info->window->winname);
			size += len;
		}
		else {
			info = (LPWINDOWSINFO)(info->list.next);
		}
	} while (info && info != hdr);

	return size;
}


char* GetVideoBase() {
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	return proc->videoBase;
	//return (char*)gGraphBase;
}

char* SetVideoBase(char * buf) {
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	return proc->videoBase = buf;
	//return (char*)gGraphBase;
}

LPWINDOWSINFO GetProcessTextPos(int** x,int **y) {
	int tid = __GetCurrentTid();
	LPWINDOWSINFO winfo = __FindProcessWindow(tid);
	if (winfo)
	{
		WINDOWCLASS* window = winfo->window;
		if (window) {
			*x = (int*)&(window->showX);
			*y = (int*)&(window->showY);
			return winfo;
		}
	}

	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	*x = (int*)&proc->showX;
	*y = (int*)&proc->showY;
	
	return winfo;
}


DWORD isTopWindow(int wid) {
	if (g_ScreenMode == 0) {
		return TRUE;
	}

	if (gWindowsList == 0) {
		return 0;
	}
	LPWINDOWSINFO window = & gWindowsList[ wid] ;
	LPWINDOWSINFO prev = (LPWINDOWSINFO)gWindowsList->list.prev;
	if (prev == 0) {
		return 0;
	}
	if (window == (LPWINDOWSINFO)prev)
	{
		if (prev->window->id == wid) 
		{
			return TRUE;
		}
		
	}
	return FALSE;

	//return (window == gWindowsList->list.prev ? TRUE : FALSE);
}


LPWINDOWSINFO getTopWindow() {
	LPWINDOWSINFO prev =(LPWINDOWSINFO)gWindowsList->list.prev;
	return prev;
}


int addWindow(WINDOWCLASS* ptrwindow,  char * wname) {
	char szout[1024];

	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int tid = proc->tid;
	WINDOWCLASS* lpwindow = (WINDOWCLASS*)linear2phyByPid((unsigned long)ptrwindow, tid);

	LPWINDOWSINFO window = __FindWindow(wname);
	if (window)
	{
		__printf(szout, "%s error id:%d name:%s\n", __FUNCTION__, window->window->id, window->window->winname);
		return FALSE;
	}

	int seq = getFreeWindow();
	if (seq == FALSE)
	{
		__printf(szout, "%s getFreeWindow error id:%d name:%s\n", __FUNCTION__, lpwindow->id, lpwindow->winname);
		return 0;
	}

	window = (LPWINDOWSINFO)&gWindowsList [seq];

	__enterSpinlock(&g_window_lock);

	window->window =(WINDOWCLASS*)lpwindow;

	window->valid = TRUE;
	//int seq = window - gWindowsList;
	window->window->id = seq;

	//__strncpy(window->window->winname, wname, WINDOW_NAME_LIMIT - 1);

	InsertListTail(&gWindowsList->list, &window->list);

	__leaveSpinlock(&g_window_lock);

 	__printf(szout, "%s id:%x,name:%s\r\n",__FUNCTION__, seq, window->window->winname );

	return seq;
}


int removeWindow(int id) {
	if (gWindowsList == 0) {
		return 0;
	}
	char szout[1024];
	
	LPWINDOWSINFO window = & gWindowsList [ id];
	if (window->valid) {

		__enterSpinlock(&g_window_lock);
		__printf(szout, "%s id:%x,name:%s\r\n", __FUNCTION__, window->window->id, window->window->winname);
		window->valid = FALSE;
		window->window = 0;

		RemoveList(&gWindowsList->list, &window->list);

		__leaveSpinlock(&g_window_lock);
	}

	return TRUE;
}



int MaximizeWindow(LPWINDOWCLASS window) {
	char szout[1024];
	__printf(szout, "%s window %x, name:%s\r\n", __FUNCTION__, window,window->winname);

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();

	if (proc->tid == window->tid) {
		proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	}else{	
		proc = tss + window->tid;
	}
	LPWINDOWSINFO topwinfo = getTopWindow();
	if (topwinfo == 0) {
		//error
		__printf(szout, "%s getTopWindow error\r\n", __FUNCTION__);
	}
	else {
		__printf(szout, "%s getTopWindow %s\r\n", __FUNCTION__,topwinfo->window->winname);

		if (topwinfo->window->minBuf) {
			LPPROCESS_INFO winproc = tss + topwinfo->window->tid;
			//winproc->videoBase = topwinfo->window->minBuf;
		}
	}

	__asm {cli}
	//char* videoBase = GetVideoBase();
	__kRestoreMouse();

	int width = window->width + window->frameSize;
	int height = window->height + window->frameSize + window->capHeight;
	char* src = (char*)gGraphBase + window->pos.y * gBytesPerLine + window->pos.x * gBytesPerPixel;
	char* dst =(char*)window->backBuf;
	for (int i = 0; i < height; i++) {
		for (int k = 0; k < width; k++) {
			for (int j = 0; j < gBytesPerPixel; j++) {
				*dst= src[i*gBytesPerLine + k * gBytesPerPixel + j];
				dst++;
			}
		}
	}

	int size = gVideoHeight * gVideoWidth * gBytesPerPixel;
	__memcpy((char*)gGraphBase, window->minBuf, size);
	__kRefreshMouseBackup();
	__kDrawMouse();

	enter_task_array_lock_cli();
	proc->videoBase = (char*)gGraphBase;
	leave_task_array_lock_sti();

	deletePopupItem(window);

	//make to be top window
	window->id = addWindow((WINDOWCLASS*)window, window->winname);

	__asm{sti}

	return 0;
}



int MinimizeWindow(WINDOWCLASS* lpwindow) {
	char szout[1024];
	__printf(szout, "%s window %x, name:%s\r\n", __FUNCTION__, lpwindow, lpwindow->winname);

	int size = gVideoHeight * gVideoWidth * gBytesPerPixel;

	LPPROCESS_INFO proc = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	int tid = proc->tid;
	WINDOWCLASS* window = (WINDOWCLASS *)linear2phyByPid((unsigned long)lpwindow,tid);
	//char* videoBase = GetVideoBase();
	if (window->minBuf == 0) {
		window->minBuf = (char*)__kMalloc(gVideoHeight * gVideoWidth * gBytesPerPixel);
	}

	__asm {cli}

	__kRestoreMouse();
	//int size = (window->height + window->frameSize + window->capHeight) * (window->width + window->frameSize) * gBytesPerPixel;
	__memcpy(window->minBuf, (char*)gGraphBase, size);

	int width = window->width + window->frameSize;
	int height = window->height + window->frameSize + window->capHeight;
	char* dst = (char*)gGraphBase + window->pos.y * gBytesPerLine + window->pos.x * gBytesPerPixel;
	char* src = (char*)window->backBuf;
	for (int i = 0; i < height; i++) {
		for (int k = 0; k < width; k++) {
			for (int j = 0; j < gBytesPerPixel; j++) {
				dst[i * gBytesPerLine + k * gBytesPerPixel + j] = *src;
				src++;
			}
		}
	}

	__kRefreshMouseBackup();
	__kDrawMouse();
	//__restoreWindow(window);

	proc->videoBase = window->minBuf;

	insertPopupItem(window);

	removeWindow(window->id);

	__asm {sti}

	return 0;
}


int insertPopupItem(LPWINDOWCLASS window) {
	int n = 0;
	int cnt = LEFTCLICK_MENU_HEIGHT / 2 / GRAPHCHAR_HEIGHT;
	for (int i = 0; i < cnt; i++) {
		if (gPopupMenu.item[i].valid == 0 && gPopupMenu.item[i].window == 0 ) {
			gPopupMenu.item[i].valid = 1;

			gPopupMenu.item[i].window = window;

			gPopupMenu.cnt++;

			n = i;
			break;
		}
	}

	return n;
}


int deletePopupItem(LPWINDOWCLASS window) {
	int n = 0;
	int cnt = LEFTCLICK_MENU_HEIGHT / 2 / GRAPHCHAR_HEIGHT;
	for (int i = 0; i < cnt; i++) {
		if (gPopupMenu.item[i].valid  && gPopupMenu.item[i].window == window) {
			gPopupMenu.item[i].valid = 0;
			gPopupMenu.item[i].window = 0;
			gPopupMenu.cnt--;

			n = i;
			break;
		}
	}
	return n;
}




/*
int destroyWindows() {
	int cnt = 0;

	LPPROCESS_INFO p = (LPPROCESS_INFO)GetCurrentTaskTssBase();

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

	LPPROCESS_INFO p = (LPPROCESS_INFO)GetCurrentTaskTssBase();
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
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetCurrentTaskTssBase();
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
	LPPROCESS_INFO p = (LPPROCESS_INFO)GetCurrentTaskTssBase();
	LPWINDOWCLASS w = p->window;
	while (w&& w->next)
	{
		w = w->next;
	}

	return w;
}

int placeFocus(int x,int y) {
	int res = 0;

	LPPROCESS_INFO p = (LPPROCESS_INFO)GetCurrentTaskTssBase();
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