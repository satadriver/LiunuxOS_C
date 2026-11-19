
#include "cursor.h"
#include "window.h"
#include "apicTimer.h"



#if 0
int gPrevX = 0;
int gPrevY = 0;

int* gCursorX = 0;
int* gCursorY = 0;

int gCursorColor = 0;

unsigned char* gCursorBackup = 0;

int g_cursorID = 0;

int gTag = 0;


void setCursor(int* x, int* y, unsigned int color) {

	gCursorX = x;
	gCursorY = y;
	gCursorColor = color;
	gCursorBackup = (unsigned char*)CURSOR_GRAPH_BASE;

	gPrevX = *gCursorX;
	gPrevY = *gCursorY;

	g_cursorID = __kAddExactTimer((DWORD)drawCursor, CURSOR_REFRESH_MILLISECONDS, 0, 0, 0, 0);
}



int removeCursor() {

	__kRemoveExactTimer(g_cursorID);

	return 0;
}

int drawCursor(int p1, int p2, int p3, int p4) {

	int ret = 0;

	int ch = GRAPHCHAR_HEIGHT / 2;
	int cw = GRAPHCHAR_WIDTH;

	POINT p;

	if (gTag) {
		if (gPrevX != *gCursorX || gPrevY != *gCursorY) {
			p.x = gPrevX;
			p.y = gPrevY + GRAPHCHAR_HEIGHT;
		}
		else {
			p.x = *gCursorX;
			p.y = *gCursorY + GRAPHCHAR_HEIGHT;
		}

		ret = __DestroyRectWindow(&p, cw, ch, (unsigned char*)gCursorBackup);

		gTag = FALSE;
	}
	else {
		p.x = *gCursorX;
		p.y = *gCursorY + GRAPHCHAR_HEIGHT;
		ret = __drawRectWindow(&p, cw, ch, gCursorColor, (unsigned char*)gCursorBackup);
		gTag = TRUE;
	}

	gPrevX = *gCursorX;
	gPrevY = *gCursorY;

	return 0;
}

#endif


int windowCursor(WINDOWCLASS* w, int p2, int p3, int p4) {

	int ret = 0;

	//WINDOWSINFO* wif = isWindowExist(w->id);

	int ch = GRAPHCHAR_HEIGHT / 2;
	int cw = GRAPHCHAR_WIDTH;

	POINT p;

	if (w->tag) {
		if (w->showBakX != w->showX || w->showBakY != w->showY) {
			p.x = w->showBakX;
			p.y = w->showBakY + GRAPHCHAR_HEIGHT;
		}
		else {
			p.x = w->showX;
			p.y = w->showY + GRAPHCHAR_HEIGHT;
		}

		ret = __DestroyRectWindow(&p, cw, ch, (unsigned char*)w->cursorBuf);

		w->tag = FALSE;
	}
	else {
		p.x = w->showX;
		p.y = w->showY + GRAPHCHAR_HEIGHT;
		ret = __drawRectWindow(&p, cw, ch, w->cursorColor, (unsigned char*)w->cursorBuf);
		w->tag = TRUE;
	}

	w->showBakX = w->showX;
	w->showBakY = w->showY;

	return 0;
}


int RemoveCursor(WINDOWCLASS* w) {

	__kRemoveApicTimer(w->cursorID);

	return 0;
}