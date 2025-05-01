#include "Utils.h"
#include "guiHelper.h"
#include "def.h"
#include "video.h"
#include "keyboard.h"
#include "mouse.h"
#include "window.h"
#include "malloc.h"
#include "task.h"
#include "Utils.h"
#include "cmd.h"



int isTxtFile(char * filename, int fnlen) {
	if (__memcmp(filename + fnlen - 4, ".txt", 4) == 0 || __memcmp(filename + fnlen - 4, ".asm", 4) == 0 ||
		__memcmp(filename + fnlen - 4, ".json", 4) == 0 || __memcmp(filename + fnlen - 4, ".cpp", 4) == 0 ||
		__memcmp(filename + fnlen - 2, ".c", 2) == 0 || 
		__memcmp(filename + fnlen - 4, ".htm", 4) == 0 || __memcmp(filename + fnlen - 5, ".html", 5) == 0 ||
		__memcmp(filename + fnlen - 4, ".php", 4) == 0 || __memcmp(filename + fnlen - 5, ".java", 5) == 0 ||
		__memcmp(filename + fnlen - 3, ".js", 3) == 0 || __memcmp(filename + fnlen - 3, ".py", 3) == 0 ||
		__memcmp(filename + fnlen - 4, ".log", 4) == 0 || __memcmp(filename + fnlen - 4, ".ini", 4) == 0 || 
		__memcmp(filename + fnlen - 4, ".bat", 4) == 0)
	{
		return TRUE;
	}
	return FALSE;
}


int getPrevPath(char * path) {
	int len = __strlen(path);
	int flag = 0;
	for (int i = len - 1; i >= 0; i --)
	{
		if (path[i] == '/' || path[i] == '\\')
		{
			if (flag)
			{
				*(path + i + 1) = 0;
				return i;
			}
			else {
				flag = TRUE;
			}
		}
	}

	*path = 0;
	return FALSE;
}





int beEndWith(char * str,char * flag) {
	int len = __strlen(str);
	int flaglen = __strlen(flag);
	if (__memcmp(str + len - flaglen,flag,flaglen) == 0)
	{
		return TRUE;
	}
	return FALSE;
}




void initTaskbarWindow(WINDOWCLASS* window, char* filename, int tid) {

	__memset((char*)window, 0, sizeof(WINDOWCLASS));

	window->capColor = 0;
	window->capHeight = 0;
	window->color = TASKBARCOLOR;
	window->frameSize = 0;
	window->frameColor = 0;
	__strcpy(window->caption, filename);

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = gWindowHeight;

	window->width = gVideoWidth + window->frameSize;

	window->height = gVideoHeight - gWindowHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = proc->pid;

	window->left = window->pos.x + (window->frameSize >> 1);
	window->top = window->pos.y + (window->frameSize >> 1) + window->capHeight;
	window->right = gVideoWidth - (window->frameSize >> 1) - 1;
	window->bottom = gVideoHeight - (window->frameSize >> 1) - 1;

	window->showX = window->pos.x + (window->frameSize >> 1);
	window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;

	window->cursorColor = ~window->color;

	int ret = __drawRectWindow(&window->pos, window->width, window->height, window->color, (unsigned char*)window->backBuf);

	window->prev = 0;
	window->next = 0;
}


void initDesktopWindow(WINDOWCLASS* window, char* name, int tid) {
	__memset((char*)window, 0, sizeof(WINDOWCLASS));

	window->capColor = 0;
	window->capHeight = 0;
	window->color = BACKGROUND_COLOR;
	window->frameSize = 0;
	window->frameColor = 0;
	__strcpy(window->caption, name);
	__strcpy(window->winname, name);

	window->backBuf = 0;

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = FULLWINDOW_LEFT;

	window->width = gVideoWidth + window->frameSize;

	window->height = gWindowHeight + window->frameSize + window->capHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = proc->pid;


	window->left = (window->frameSize >> 1) + window->pos.x;
	window->top = (window->frameSize >> 1) + window->capHeight + window->pos.y;
	window->right = window->left + window->width - 1;
	window->bottom = window->top + window->height - 1;

	window->showX = window->left;
	window->showY = window->top;
	window->cursorX = window->showX;
	window->cursorY = window->showY;
	window->cursorColor = ~window->color;

	window->minBuf = 0;

	window->id = addWindow((DWORD)window, window->winname);

	int ret = __drawRectWindow(&window->pos, window->width, window->height, window->color, (unsigned char*)window->backBuf);

	window->prev = 0;
	window->next = 0;

	proc->window = window->id;
	tss[tid].window = window->id;
}


void initFullWindow(WINDOWCLASS* window, char* functionname, int tid,int fill) {

	window->capColor = 0x00ffff;
	window->capHeight = GRAPHCHAR_HEIGHT * 2;
	window->color = CONSOLE_FONT_COLOR;
	window->frameSize = GRAPHCHAR_WIDTH;
	window->frameColor = FOLDERFONTBGCOLOR;
	__strcpy(window->caption, functionname);
	__strcpy(window->winname, functionname);

	window->pos.x = FULLWINDOW_TOP;

	window->pos.y = FULLWINDOW_LEFT;

	window->width = gVideoWidth - window->frameSize;

	int height = gWindowHeight;
	if (fill) {
		height = gVideoHeight;
	}

	window->height = height - window->frameSize - window->capHeight;

	window->zoomin = 1;

	window->tid = tid;

	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = proc->pid;

	window->left = window->frameSize >> 1;
	window->top = (window->frameSize >> 1) + window->capHeight;
	window->right = gVideoWidth - (window->frameSize >> 1) - 1;
	window->bottom = height - (window->frameSize >> 1) - 1;

	window->showX = window->pos.x + (window->frameSize >> 1);
	window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;
	window->minBuf = 0;

	window->cursorColor = ~window->color;

	window->prev = 0;
	window->next = 0;

	__drawWindow(window);

	proc->window = window->id;
	tss[tid].window = window->id;
}


void initConsoleWindow(WINDOWCLASS* window, char* filename, int tid) {
	window->capColor = 0x00ffff;
	window->capHeight = GRAPHCHAR_HEIGHT * 2;
	
	window->frameSize = GRAPHCHAR_WIDTH;
	window->frameColor = FOLDERFONTBGCOLOR;

	window->color = DEFAULT_FONT_COLOR;

	__strcpy(window->caption, filename);
	__strcpy(window->winname, "__Console");

	window->width = gVideoWidth / 2;
	window->height = gVideoHeight / 2;

	window->pos.x = gVideoWidth / 4;

	window->pos.y = gVideoHeight / 4;

	window->left = window->pos.x + window->frameSize;
	window->top = window->pos.y + (window->frameSize >> 1) + window->capHeight;

	window->right = window->pos.x + window->frameSize + window->width - 1;
	window->bottom = window->pos.y + (window->frameSize >> 1) + +window->capHeight + window->height - 1;

	window->zoomin = 1;
	window->tid = tid;
	LPPROCESS_INFO tss = (LPPROCESS_INFO)TASKS_TSS_BASE;
	LPPROCESS_INFO proc = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window->pid = proc->pid;

	window->cursorColor = ~window->color;
	window->showX = window->pos.x + (window->frameSize >> 1);
	window->showY = window->pos.y + (window->frameSize >> 1) + window->capHeight;
	window->minBuf = 0;

	window->prev = 0;
	window->next = 0;

	__drawWindow(window);

	proc->window = window->id;
	tss[tid].window = window->id;
}


void initIcon(FILEICON* clickitem, char* name, int tid, int id, int x, int y) {
	__memset((char*)clickitem, 0, sizeof(FILEICON));

	clickitem->tid = tid;

	clickitem->id = id;

	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	clickitem->pid = p->pid;

	clickitem->color = FOLDERCOLOR;
	clickitem->width = gBigFolderWidth;
	clickitem->height = gBigFolderHeight;
	clickitem->pos.x = x;
	clickitem->pos.y = y;
	clickitem->namecolor = FOLDERFONTCOLOR;
	clickitem->zoomin = 1;
	__strcpy(clickitem->name, name);
	clickitem->frameColor = FOLDERFONTBGCOLOR;
	clickitem->frameSize = GRAPHCHAR_WIDTH;
	clickitem->namebgcolor = DEFAULT_FONT_COLOR;

	int ret = __drawFileIcon(clickitem);
}


