
#include "Utils.h"
#include "Kernel.h"
#include "mouse.h"
#include "keyboard.h"
#include "video.h"
#include "task.h"
#include "Pe.h"
#include "console.h"

#include "FileManager.h"
#include "window.h"
#include "file.h"
#include "fat32/FAT32.h"
#include "fat32/Fat32File.h"
#include "NTFS/ntfs.h"
#include "NTFS/ntfsFile.h"
#include "NTFS/ntfsDirs.h"
#include "fat32/fat32dirs.h"
#include "FileManagerWindow.h"
#include "ios9660/ios9660.h"
#include "UserUtils.h"
#include "graph.h"
#include "soundBlaster/sbPlay.h"
#include "fat12/fat12.h"
#include "malloc.h"
#include "Thread.h"
#include "WindowClass.h"

int gPartitionType = 0;

int preparePartitionInfo() {
	int ret = 0;
	gPartitionType = getMBR();
	if (gPartitionType == FAT32_FILE_SYSTEM)
	{
		ret = getDBR();
		if (ret <= 0)
		{
			return FALSE;
		}

		g_bytesPerSec = gFat32Dbr.BPB_BytesPerSec;
		g_SecsPerCluster = gFat32Dbr.BPB_SecPerClus;
		g_ClusterSize = g_bytesPerSec * gFat32Dbr.BPB_SecPerClus;
		g_FirstClusterNO = gFat32Dbr.BPB_RootClus;
		g_SecsBeforeRootDir = getRootDirOffset();
		gMaxDirsInPath = gFat32Dbr.BPB_SecPerClus * g_bytesPerSec / sizeof(FAT32DIRECTORY);

		ret = getFsinfo();

		fat32Init();
	}
	else if (gPartitionType == NTFS_FILE_SYSTEM)
	{
		ret = getNtfsDBR();

		g_bytesPerSec = gNtfsDbr.bytePerSector;
		g_SecsPerCluster = gNtfsDbr.secPerCluster;
		g_ClusterSize = g_SecsPerCluster * g_bytesPerSec;
		gMsfOffset = gNtfsDbr.hideSectors + gNtfsDbr.MFT * g_SecsPerCluster + MSF_ROOTDIR_OFFSET;

		ret = readMSFRoot();
	}
	else {

	}

	return ret;
}



int readFileDirs(DWORD secno, LPFILEBROWSER files, DWORD ntfslast) {
	if (gPartitionType == 2)
	{
		unsigned __int64 secoff = secno * 2 + gNtfsDbr.hideSectors + gNtfsDbr.MFT * g_SecsPerCluster;
		return getNtfsDirs(secoff, files, ntfslast);
	}
	else if (gPartitionType == 1)
	{
		return getFat32NextDirs(secno, files);
	}
	else if (gPartitionType == 3)
	{
		return readIso9660Dirs(secno, files);
	}
	else if (gPartitionType == 4)
	{
		return readFat12Dirs(secno, files);
	}

	return 0;
}


int readFileData(DWORD secno, int filesize, char* databuf, int readsize) {

	if (gPartitionType == NTFS_FILE_SYSTEM)
	{
		DWORD secoff = secno * 2 + gNtfsDbr.hideSectors + (DWORD)gNtfsDbr.MFT * g_SecsPerCluster;
		return (DWORD)getNtfsFileData(secoff, &databuf);
	}
	else if (gPartitionType == FAT32_FILE_SYSTEM)
	{
		return fat32FileReader(secno, filesize, databuf, readsize);
	}
	else if (gPartitionType == CDROM_FILE_SYSTEM)
	{
		int seccnt = filesize / ATAPI_SECTOR_SIZE;
		int mod = filesize % ATAPI_SECTOR_SIZE;
		if (mod)
		{
			seccnt++;
		}
		return readIso9660File(secno, seccnt, &databuf);
	}
	else if (gPartitionType == FLOPPY_FILE_SYSTEM)
	{
		return fat12FileReader(secno, filesize, databuf, readsize);
	}
	else {
		return FALSE;
	}
}


int doFileAction(LPFILEBROWSER files) {
	int result = 0;
	char szout[1024];
	__printf(szout, "doFileAction readFileData:%s size:%x\n", files->pathname, files->filesize);

	if (files->filesize > 0x10000000)
	{
		__printf(szout, "doFileAction filename:%s size:%x error\n", files->pathname, files->filesize);

		return FALSE;
	}

	char* buffer = (char*)__kMalloc(files->filesize);
	int readsize = readFileData(files->secno, files->filesize, (char*)buffer, files->filesize);
	if (readsize <= 0)
	{
		__printf(szout, "doFileAction readFileData:%s size:%x error\n", files->pathname, files->filesize);

		return FALSE;
	}

	int fnlen = __strlen(files->pathname);

	upper2lower(files->pathname, fnlen);

	TASKCMDPARAMS cmd;
	__memset((char*)&cmd, 0, sizeof(TASKCMDPARAMS));
	cmd.addr = (DWORD)buffer;
	cmd.filesize = files->filesize;
	__strcpy(cmd.filename, files->pathname);
	if (__memcmp(files->pathname + fnlen - 4, ".bmp", 4) == 0)
	{
		cmd.cmd = SHOW_WINDOW_BMP;

		DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
		return __kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kShowWindow_bmp");

		//return __kCreateProcess(VSMAINDLL_LOAD_ADDRESS, 0x100000, "main.dll", "__kShowWindow", 3, (DWORD)&cmd);
		//return __kCreateProcess(MAIN_DLL_BASE, 0x100000, "main.dll", "__kShowWindow", 3, (DWORD)&cmd);
	}
	else if (__memcmp(files->pathname + fnlen - 4, ".wav", 4) == 0)
	{
		result = sbplay((char*)buffer, readsize);

		//return playWavFile(files->pathname);
		__kFree((DWORD)buffer);
	}
	else if (isTxtFile(files->pathname, fnlen))
	{
		cmd.cmd = SHOW_WINDOW_TXT;

		//return __kCreateProcess(VSMAINDLL_LOAD_ADDRESS, 0x100000, "main.dll", "__kShowWindow", 3, (DWORD)&cmd);

		DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
		return __kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kShowWindow_txt");

		//return __kCreateThread((DWORD)__kShowWindow, (DWORD)&cmd, "__kShowWindow_txt");
	}
	else if (__memcmp(files->pathname + fnlen - 4, ".jpg", 4) == 0 || __memcmp(files->pathname + fnlen - 5, ".jpeg", 5) == 0)
	{
		cmd.cmd = SHOW_WINDOW_JPEG;

		//return __kCreateProcess(VSMAINDLL_LOAD_ADDRESS, 0x100000, "main.dll", "__kShowWindow", 3, (DWORD)&cmd);

		DWORD thread = getAddrFromName(MAIN_DLL_BASE, "__kShowWindow");
		return __kCreateThread((DWORD)thread, MAIN_DLL_BASE, (DWORD)&cmd, "__kShowWindow_jpg");
	}
	else if (__memcmp(files->pathname + fnlen - 4, ".exe", 4) == 0 || __memcmp(files->pathname + fnlen - 4, ".com", 4) == 0)
	{
		cmd.filesize = files->filesize;
		//return __kCreateThread((DWORD)__kShowWindow, (DWORD)&cmd, "__kShowWindow");
		return __kCreateProcess((unsigned int)cmd.addr, cmd.filesize, files->pathname, files->pathname, 3, (DWORD)&cmd);
	}
	return 0;
}



//文件大小，属性必须统一
//文件夹大小统一为0，文件夹属性都是0x10,文件属性都是0x20
int __kFileManager(unsigned int retaddr, int tid, char* filename, char* funcname, DWORD param) {
	int ret = 0;
	char szout[1024];

	LPTASKCMDPARAMS cmd = (LPTASKCMDPARAMS)param;
	__printf(szout, "__kFileManager task tid:%x,name:%s,cmd:%d\n", tid, filename, cmd->cmd);

	if (cmd->cmd == UNKNOWN_FILE_SYSTEM)
	{
		ret = preparePartitionInfo();
		if (ret <= 0)
		{
			return 0;
		}
	}
	else {
		gPartitionType = cmd->cmd;
	}

	char fullpath[MAX_PATH_SIZE];
	__memset(fullpath, 0, MAX_PATH_SIZE);

	DWORD ntfsprevs[MAX_PATH_SIZE];
	int ntfsseq = 0;

	int filetotal = 0;
	LPFILEBROWSER files = (LPFILEBROWSER)__kMalloc(g_ClusterSize * 2);
	if (gPartitionType == NTFS_FILE_SYSTEM)
	{
		unsigned __int64 ntfssecno = gNtfsDbr.hideSectors + gNtfsDbr.MFT * g_SecsPerCluster;
		ntfsprevs[ntfsseq] = MSF_ROOTDIR_OFFSET / 2;
		filetotal = getNtfsDirs(ntfssecno + MSF_ROOTDIR_OFFSET, files, 0);
	}
	else if (gPartitionType == FAT32_FILE_SYSTEM) {

		filetotal = getFat32RootDirs((LPFAT32DIRECTORY)glpRootDir, files);
	}
	else if (gPartitionType == CDROM_FILE_SYSTEM)
	{
		filetotal = browseISO9660File(files);
	}
	else if (gPartitionType == FLOPPY_FILE_SYSTEM)
	{
		filetotal = browseFat12File(files);
	}

	if (filetotal <= 0)
	{
		__kFree((DWORD)files);
		return FALSE;
	}

	FMWINDOW window;
	window.window.tid = tid;
	LPPROCESS_INFO p = (LPPROCESS_INFO)CURRENT_TASK_TSS_BASE;
	window.window.pid = p->pid;
	__strcpy(window.window.caption, cmd->filename);

	drawFileManager(&window);

	int rowlimit = gVideoHeight / window.fsheight;

	int fpagecnt = 0;
	if (filetotal < rowlimit)
	{
		fpagecnt = filetotal;
	}
	else {
		fpagecnt = rowlimit;
	}

	int pagecnt = filetotal / rowlimit;
	int pagemod = filetotal % rowlimit;
	if (pagemod)
	{
		pagecnt++;
	}
	pagecnt--;
	if (pagecnt < 0)
	{
		pagecnt = 0;
	}

	int number = 0;

	while (TRUE)
	{
 		//__printf(szout, "filetotal:%x,first:%s sector:%x size:%x,second:%s sector:%x size:%x,third:%s sector:%x size:%x,fourth:%s sector:%x size:%x\n", 
 		//	filetotal,files[0].pathname,files[0].secno,files[0].filesize, files[1].pathname, files[1].secno, files[1].filesize, 
 		//	files[2].pathname, files[2].secno, files[2].filesize, files[3].pathname, files[3].secno, files[3].filesize);
 		//__drawGraphChars((unsigned char*)szout, 0);

		POINT p;
		p.x = 0;
		p.y = 0;
		__drawRectangle(&p, gVideoWidth, gVideoHeight, window.window.color, 0);

		for (int j = 0; j < fpagecnt; j++)
		{
			//calc positon of char
			int y = (((number + 1) % rowlimit) * window.cpl - 1 - (window.cpl / 2)) * GRAPHCHAR_HEIGHT;
			DWORD pos = __getpos(0, y);

			char szinfo[4096];
			if (files[number].attrib & FILE_ATTRIBUTE_DIRECTORY)
			{
				int len = __sprintf(szinfo, "%s        DIR(%x)        %d(bytes)",
					files[number].pathname, files[number].attrib, files[number].filesize);
				*(szinfo + len) = 0;
				__drawGraphChar((unsigned char*)szinfo, FILE_DIR_FONT_COLOR, pos, window.window.fontcolor);
			}
			else if (files[number].attrib & FILE_ATTRIBUTE_ARCHIVE)
			{
				int len = __sprintf(szinfo, "%s        FILE(%x)       %d(bytes)",
					files[number].pathname, files[number].attrib, files[number].filesize);
				*(szinfo + len) = 0;
				__drawGraphChar((unsigned char*)szinfo, FILE_FILE_FONT_COLOR, pos, window.window.fontcolor);
			}
			else {
				int len = __sprintf(szinfo, "%s        UNKNOWN(%x)    %d(bytes)",
					files[number].pathname, files[number].attrib, files[number].filesize);
				*(szinfo + len) = 0;
				__drawGraphChar((unsigned char*)szinfo, FILE_UNKNOWN_FONT_COLOR, pos, window.window.fontcolor);
			}
			number++;
		}

		//number is last dir
		if (number)
		{
			number--;
		}

		if (number < 0)
		{
			number = 0;
		}

		//number是最后一个目录的序号
		while (TRUE)
		{
			unsigned int ck = __kGetKbd(window.window.id);
			unsigned int asc = ck & 0xff;
			if (asc)
			{
				// 				__printf(szout, "__getchar:%s", &asc);
				if (asc == VK_NEXT || asc == VK_DOWN || asc == VK_RIGHT || asc == VK_END || asc == 's' || asc == 'd')
				{
					//check if is last page
					if ((number / rowlimit) < pagecnt)
					{
						//to be first of next page
						number = ((number + rowlimit) / rowlimit) * rowlimit;

						if (filetotal - number >= rowlimit)
						{
							fpagecnt = rowlimit;
						}
						else {
							fpagecnt = filetotal - number;
						}

						break;
					}
				}
				else if (asc == VK_UP || asc == VK_PRIOR || asc == VK_LEFT || asc == VK_HOME || asc == 'w' || asc == 'a')
				{
					//check if first page
					if ((number / rowlimit) > 0)
					{
						//to first of previous page
						number = ((number - rowlimit) / rowlimit) * rowlimit;

						if (filetotal - number >= rowlimit)
						{
							fpagecnt = rowlimit;
						}
						else {
							fpagecnt = filetotal - number;
						}

						break;
					}
				}
				else if (asc == 0x1b)
				{
					__kFree((DWORD)files);

					restoreFileManager(&window);
					return 0;
				}
			}

			MOUSEINFO mouseinfo;
			__memset((char*)&mouseinfo, 0, sizeof(MOUSEINFO));
			ret = getmouse(&mouseinfo, window.window.id);
			if (mouseinfo.status & 1)
			{
				//y positon is page
				int y = mouseinfo.y / window.fsheight;

				//number positon in page
				int targetno = (number / rowlimit) * rowlimit + y;
				//ntfs dir is 0x10000000

				__printf(szout, "__kFileManager filename:%s\n", files[targetno].pathname);

				if (targetno < filetotal /*&& files[targetno].filesize == 0*/ && files[targetno].attrib & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (__strcmp(files[targetno].pathname, ".") == 0)
					{

					}
					else if (__strcmp(files[targetno].pathname, "..") == 0)
					{
						int len = getPrevPath(fullpath);
						ntfsseq--;
						if (ntfsseq <= 1)
						{
							ntfsseq = 1;
						}
					}
					else {
						__strcat(fullpath, files[targetno].pathname);
						__strcat(fullpath, "/");
						ntfsseq++;
						if (ntfsseq >= 1024 / sizeof(DWORD))
						{
							ntfsseq = 1;
						}
						ntfsprevs[ntfsseq] = files[targetno].secno;
					}


					//sub root dir has directory ".." but without cluster number in ".."
					if (gPartitionType == 1 && __memcmp(files[targetno].pathname, "..", 2) == 0 && files[targetno].secno < g_FirstClusterNO)
					{
						files[targetno].secno = g_FirstClusterNO;
					}

					filetotal = readFileDirs(files[targetno].secno, (LPFILEBROWSER)files, ntfsprevs[ntfsseq - 1]);
					if (filetotal > 0)
					{
						number = 0;

						pagecnt = filetotal / rowlimit;
						if (filetotal % rowlimit)
						{
							pagecnt++;
						}
						pagecnt--;
						if (pagecnt <= 0)
						{
							pagecnt = 0;
						}

						if (rowlimit > filetotal)
						{
							fpagecnt = filetotal;
						}
						else {
							fpagecnt = rowlimit;
						}

						break;
					}
				}
				else if (targetno < filetotal && files[targetno].attrib & FILE_ATTRIBUTE_ARCHIVE) {
					doFileAction(&files[targetno]);
				}
			}

			__sleep(0);
		}
	}

	__kFree((DWORD)files);
	return 0;
}






int __restoreRectangleFrame(LPPOINT p, int width, int height, int framesize, unsigned char* backup) {
	int startpos = p->y * gBytesPerLine + p->x * gBytesPerPixel + gGraphBase;
	unsigned char* ptr = (unsigned char*)startpos;
	unsigned char* keep = ptr;

	for (int i = 0; i < height + framesize; i++)
	{
		for (int j = 0; j < width + framesize; j++)
		{
			for (int k = 0; k < gBytesPerPixel; k++)
			{
				*ptr = *backup;
				ptr++;
				backup++;
			}
		}

		keep += gBytesPerLine;
		ptr = (unsigned char*)keep;
	}

	return (int)ptr - gGraphBase;
}

