
#include "def.h"
#include "video.h"
#include "FileManagerWindow.h"
#include "window.h"
#include "malloc.h"
#include "task.h"
#include "Utils.h"


int restoreFileManager(LPFMWINDOW w) {
	__restoreRectangle(&w->window.pos, w->window.width, w->window.height,(unsigned char*) w->window.backGround);
	removeWindow(w->window.id);

	__kFree(w->window.backGround);

	//__terminatePid(w->pid);
	return 0;
}


int drawFileManager(LPFMWINDOW w) {
	w->window.capHeight = 0;
	w->window.frameSize = 0;

	w->window.next = 0;
	w->window.prev = 0;

	w->cpl = 3;
	w->window.color = 0xffffff;
	w->window.capColor = 0;
	w->window.fontcolor = 0;
	w->window.height = gVideoHeight;
	w->window.width = gVideoWidth;
	w->window.pos.x = 0;
	w->window.pos.y = 0;
	w->fsheight = GRAPHCHAR_HEIGHT * w->cpl;

	w->window.backsize = gBytesPerPixel*(w->window.width )*(w->window.height );

	w->window.backGround = (DWORD)__kMalloc(w->window.backsize);

	w->window.id = addWindow(TRUE,(DWORD*) &w->window.pos.x, (DWORD*)&w->window.pos.y, 0,w->window.winname);

	__drawRectangle(&w->window.pos, w->window.width, w->window.height, w->window.color, (unsigned char*)w->window.backGround);

	return 0;

}