
#include "bochs_mbr.h"
#include "SectorRW.h"
#include "FileOper.h"

/*
bximage -q -hd=128 -mode=create -sectorsize=512 liunuxos.img

dd if=liunux_bochs_mbr.bin of=liunuxos.img bs=446 count=1 conv=notrunc

dd if=liunux_os_data.bin of=liunuxos.img bs=512 seek=10000 count=100 conv=notrunc

dd if=font.db of=liunuxos.img bs=512 seek=10003 count=100 conv=notrunc

dd if=loader.com of=liunuxos.img bs=512 seek=10005 count=2 conv=notrunc

dd if=kernel.exe of=liunuxos.img bs=512 seek=3 count=100 conv=notrunc
dd if=kernel.dll of=liunuxos.img bs=512 seek=3 count=100 conv=notrunc
dd if=main.exe of=liunuxos.img bs=512 seek=3 count=100 conv=notrunc
*/


int MakeBochsMBR() {
	int ret = 0;
	int result = 0;
	int cnt = 0;
	ret = SetCurrentDirectoryA(".\\liunuxos\\");

	char* buf = 0;
	int fs = 0;

	int freesecno = 100000;

	char *old_mbr = new char[SECTOR_SIZE+16];
	int old_mbr_size = SECTOR_SIZE;
	ret = FileOper::fileReader(BOCHS_HARDDISK_FILENAME, &old_mbr, &old_mbr_size);

	LIUNUX_OS_DATA hdr;
	hdr.flag = LIUNUX_FLAG;

	hdr.mbrSecOff = freesecno + 1;
	hdr.mbr2SecOff = freesecno + 2;

	fs = FileOper::getFileSize(FONT_FILENAME);
	hdr.fontSecCnt = fs / SECTOR_SIZE;
	hdr.fontSecOff = freesecno + 3;

	fs = FileOper::getFileSize(LOADER_FILENAME);
	hdr.loaderSecOff = hdr.fontSecOff + hdr.fontSecCnt;
	cnt = fs/SECTOR_SIZE;
	result = fs % SECTOR_SIZE;
	if (result) {
		cnt++;
	}
	hdr.loaderSecCnt = cnt;

	fs = FileOper::getFileSize(KERNEL_EXE_FILENAME);
	hdr.kernelSecOff = hdr.loaderSecOff + hdr.loaderSecCnt;
	cnt = fs / SECTOR_SIZE;
	result = fs % SECTOR_SIZE;
	if (result) {
		cnt++;
	}
	hdr.kernelSecCnt = cnt;

	fs = FileOper::getFileSize(KERNEL_DLL_FILENAME);
	hdr.kerdllSecOff = hdr.kernelSecOff + hdr.kernelSecCnt;
	cnt = fs / SECTOR_SIZE;
	result = fs % SECTOR_SIZE;
	if (result) {
		cnt++;
	}
	hdr.kerdllSecCnt = cnt;

	fs = FileOper::getFileSize(MAIN_DLL_FILENAME);
	hdr.maindllSecOff = hdr.kerdllSecOff + hdr.kerdllSecCnt;
	cnt = fs / SECTOR_SIZE;
	result = fs % SECTOR_SIZE;
	if (result) {
		cnt++;
	}
	hdr.maindllSecCnt = cnt;

	char* mymbr = 0;
	int mymbrsize = 0;
	ret = FileOper::fileReader(MBR_FILENAME, &mymbr, &mymbrsize);
	if (ret <= 0)
	{
		MessageBoxA(0, "not found mbr file", "not found mbr file", MB_OK);
		return FALSE;
	}

	LJGMBR* ljmbr = (LJGMBR*)mymbr;
	ljmbr->secoff = freesecno;

	ret = FileOper::fileWriter(LIUNUX_BOCHS_MBR_FILENAME,(const char*) mymbr, (SECTOR_SIZE) - 64 - 2, 1);

	ret = FileOper::fileWriter(LIUNUX_OS_DATA_FILENAME, (const char*)&hdr, sizeof(LIUNUX_OS_DATA) , 1);

	char cmdbuf[1024];
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)"\r\n", 2, 1);

	wsprintfA(cmdbuf, "bximage -q -hd=128 -func=create -sectsize=512 %s\r\n", BOCHS_HARDDISK_FILENAME);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);

	wsprintfA(cmdbuf, "dd if=%s of=%s bs=446 count=1 conv=notrunc\r\n", 
		LIUNUX_BOCHS_MBR_FILENAME,BOCHS_HARDDISK_FILENAME);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);
	
	wsprintfA(cmdbuf, "dd if=%s of=%s bs=512 seek=%d count=%d conv=notrunc\r\n",
		LIUNUX_OS_DATA_FILENAME, BOCHS_HARDDISK_FILENAME, freesecno,1);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);

	wsprintfA(cmdbuf, "dd if=%s of=%s bs=512 seek=%d count=%d conv=notrunc\r\n",
		FONT_FILENAME, BOCHS_HARDDISK_FILENAME, hdr.fontSecOff, hdr.fontSecCnt);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);

	wsprintfA(cmdbuf, "dd if=%s of=%s bs=512 seek=%d count=%d conv=notrunc\r\n",
		LOADER_FILENAME, BOCHS_HARDDISK_FILENAME, hdr.loaderSecOff, hdr.loaderSecCnt);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);

	wsprintfA(cmdbuf, "dd if=%s of=%s bs=512 seek=%d count=%d conv=notrunc\r\n",
		KERNEL_EXE_FILENAME, BOCHS_HARDDISK_FILENAME, hdr.kernelSecOff, hdr.kernelSecCnt);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);

	wsprintfA(cmdbuf, "dd if=%s of=%s bs=512 seek=%d count=%d conv=notrunc\r\n",
		KERNEL_DLL_FILENAME, BOCHS_HARDDISK_FILENAME, hdr.kerdllSecOff, hdr.kerdllSecCnt);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);


	wsprintfA(cmdbuf, "dd if=%s of=%s bs=512 seek=%d count=%d conv=notrunc\r\n",
		MAIN_DLL_FILENAME, BOCHS_HARDDISK_FILENAME, hdr.maindllSecOff, hdr.maindllSecCnt);
	ret = FileOper::fileWriter(BOCHS_CMD_FILENAME, (const char*)cmdbuf, strlen(cmdbuf), 0);
	return TRUE;
}