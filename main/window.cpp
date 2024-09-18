#include "window.h"
#include "task.h"
#include "video.h"
#include "Utils.h"
#include "screenUtils.h"
#include "slab.h"
#include "ListEntry.h"



LPWINDOWSINFO gWindowsList = 0;
LPWINDOWSINFO gWindowLast = 0;


LPWINDOWSINFO checkWindowExist(DWORD wid) {
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList;
	do
	{
		if (info->id == wid)
		{
			return info;
		}
		else {
			info = (LPWINDOWSINFO)(info->list.next);
		}
	} while (info != gWindowsList);

	return 0;
}

LPWINDOWSINFO getFreeWindow() {
	LPWINDOWSINFO info = (LPWINDOWSINFO)gWindowsList;

	int c = WINDOW_LIST_BUF_SIZE / sizeof(WINDOWSINFO);
	for (int i = 0; i < c; i++)
	{
		if (info[i].flag == 0)
		{
			info[i].flag = TRUE;
			return &info[i];
		}
	}
	return 0;
}


void initWindowList() {
	gWindowsList = (LPWINDOWSINFO)__kMalloc(WINDOW_LIST_BUF_SIZE);
	__memset((char*)gWindowsList, 0, WINDOW_LIST_BUF_SIZE);
	initListEntry((LPLIST_ENTRY)&gWindowsList->list);

// 	WINDOWSINFO winfo;
// 	winfo.id = -1;
// 	winfo.flag = TRUE;
	//addlistTail(&gWindowsList->list, &winfo.list);
	addWindow(FALSE, 0, 0, 0);

	gWindowLast = gWindowsList;
}

DWORD isTopWindow(int wid) {
	LPWINDOWSINFO window = wid + gWindowsList;
	if (window == gWindowLast)
	{
		return TRUE;
	}
	return FALSE;
	//return (window == gWindowLast ? TRUE : FALSE);
}


DWORD getTopWindow() {
	return gWindowLast - gWindowsList;
}

int addWindow(int active, DWORD *x, DWORD *y, int color) {

	LPWINDOWSINFO window = getFreeWindow();
	if (window == FALSE)
	{
		return -1;
	}

	int i = window - gWindowsList;

	window[i].flag = TRUE;

	window[i].id = i;

	window[i].cursorColor = color;

	DWORD *addrx = (DWORD *)linear2phy((DWORD)x);
	DWORD *addry = (DWORD *)linear2phy((DWORD)y);

	window[i].cursorX = addrx;
	window[i].cursorY = addry;

	if (active)
	{
		window[i].flag |= 0x80000000;
		setCursor(TRUE, window[i].cursorX, window[i].cursorY, window[i].cursorColor);
	}
	else {
		setCursor(FALSE, window[i].cursorX, window[i].cursorY, window[i].cursorColor);
	}

	addlistTail(&gWindowsList->list, &window->list);

	gWindowLast = &window[i];

	return i;
}


int removeWindow(int id) {

	LPWINDOWSINFO window = gWindowsList + id;

	window->flag = FALSE;

	removelist( &window->list);
	
	if (gWindowLast == window)
	{
		gWindowLast = (LPWINDOWSINFO)window->list.prev;
		if (gWindowLast->flag & 0x80000000)
		{
			setCursor(TRUE, gWindowLast->cursorX, gWindowLast->cursorY, gWindowLast->cursorColor);
		}
		else {
			setCursor(FALSE, gWindowLast->cursorX, gWindowLast->cursorY, gWindowLast->cursorColor);
		}
	}

	return TRUE;
}




