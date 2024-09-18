
#include "def.h"
#include "TaskBar.h"
#include "console.h"
#include "video.h"
#include "mouse.h"
#include "keyboard.h"
#include "task.h"
#include "graph.h"
#include "soundBlaster/sbPlay.h"

#include "Utils.h"
#include "menu.h"
#include "windowclass.h"
#include "Pe.h"
#include "window.h"

#include "ata.h"
#include "UserUtils.h"
#include "Kernel.h"
#include "sysregs.h"
#include "WindowClass.h"
#include "Utils.h"
#include "paint.h"
#include "malloc.h"
#include "Thread.h"

int __kTaskBar(unsigned int retaddr, int pid, char * filename, char * funcname, DWORD param) {
	int ret = 0;

	//char szout[1024];
	// 	__printf(szout, "__console task retaddr:%x,pid:%x,name:%s,funcname:%s,param:%x\n",retaddr, pid, filename,funcname,param);
	// 	__drawGraphChars((unsigned char*)szout, 0);

	WINDOWCLASS window;
	initTaskbarWindow(&window, filename, pid);

	while (1)
	{
		unsigned int ck = __kGetKbd(window.id);
		//unsigned int ck = __getchar(window.id);
		unsigned int asc = ck & 0xff;


		MOUSEINFO mouseinfo;
		__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
		ret = __kGetMouse(&mouseinfo, window.id);
		if (mouseinfo.status & 1)	//left click
		{

		}
		else if (mouseinfo.status & 4)	//middle click
		{
// 			menu.pos.x = mouseinfo.x;
// 			menu.pos.y = mouseinfo.y;
// 			menu.action = mouseinfo.status;
		}

		__sleep(0);
	}
	return 0;
}

