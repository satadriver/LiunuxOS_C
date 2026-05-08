

#include <stdio.h>
#include "writeos.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "fileutils.h"


int OSReaderWriter::writedata(int offset,int size,char * data){

	int result = 0;

	fseek(m_fd,offset*SECTOR_SIZE,0);
	result = fwrite(data,1,size,m_fd);
	return result;
}


int OSReaderWriter::readdata(int offset,int size,char * data){

	int result = 0;

	fseek(m_fd,offset*SECTOR_SIZE,0);
	result = fread(data,1,size,m_fd);
	return result;
}


int OSReaderWriter::osWriter() {
	int ret = 0;
	char szout[1024];

	m_fd = fopen("/dev/sda","rb+");
	if(m_fd <= 0){
		printf("open /dev/sda failed\r\n");
		return FALSE;
	}


	char mbr[SECTOR_SIZE+4] = { 0 };
	LPMBR lpmbr = (LPMBR)mbr;
	ret = OSReaderWriter::readdata(0, SECTOR_SIZE, mbr);
	//fclose(m_fd);
	if (ret <= 0)
	{
		printf("mbr read error\r\n");
		return FALSE;
	}

/*
	m_fd = fopen("/dev/sda1","rb+");
	if(m_fd <=0){
		printf("open /dev/sda1 failed\r\n");
		return FALSE;
	}

	char dbr[LINUX_BOOT_SIZE+4] = { 0 };
	ret = OSReaderWriter::readdata(0,LINUX_BOOT_SIZE, dbr);
	if (ret <= 0)
	{
		printf("dbr read error\r\n");
		return FALSE;
	}


	if (lpmbr->dpt[0].type == FAT32_PARTITION || lpmbr->dpt[0].type == FAT32_PARTITION_2 ||
		lpmbr->dpt[0].type == FAT32_LBA_PARTITION || lpmbr->dpt[0].type == FAT32_HIDDEN)
	{
		LPFAT32_DBR fat32dbr = (LPFAT32_DBR)dbr;
		if (fat32dbr->BPB_HiddSec > EMPTY_SECTOR_NEED)
		{
			startsecoff = fat32dbr->BPB_HiddSec - EMPTY_SECTOR_NEED ;
		}
		else if (fat32dbr->BPB_RsvdSecCnt > EMPTY_SECTOR_NEED)
		{
			startsecoff = fat32dbr->BPB_HiddSec + fat32dbr->BPB_RsvdSecCnt - EMPTY_SECTOR_NEED ;
		}
		else {
			DWORD rootdir = fat32dbr->BPB_HiddSec + fat32dbr->BPB_RsvdSecCnt + (fat32dbr->BPB_FATSz32*fat32dbr->BPB_NumFATs);
			startsecoff = rootdir - EMPTY_SECTOR_NEED ;
		}	
	}
	else if (lpmbr->dpt[0].type == NTFS_PARTITION || lpmbr->dpt[0].type == NTFS_HIDDEN)
	{
		LPNTFSDBR ntfsdbr = (LPNTFSDBR)dbr;
		if (ntfsdbr->hideSectors <= EMPTY_SECTOR_NEED)
		{
			startsecoff = ntfsdbr->hideSectors + ntfsdbr->MFT*ntfsdbr->secPerCluster + MSF_ROOTDIR_OFFSET - 4 * EMPTY_SECTOR_NEED;

			//startsecoff = lpmbr->dpt[0].sectortotal + lpmbr->dpt[0].offset - EMPTY_SECTOR_NEED ;
		}
		else {
			startsecoff = ntfsdbr->hideSectors - EMPTY_SECTOR_NEED ;
		}
	}
	else {
		printf("write liunux os from sector number:%d to sector number:%d error\n", startsecoff, endsecno);
		return FALSE;
	}
*/

	unsigned int endsecno = 0;
	unsigned int startsecoff = 0;
	if(lpmbr->dpt[0].offset > EMPTY_SECTOR_NEED + 16){
		endsecno = lpmbr->dpt[0].offset;
		startsecoff = endsecno - EMPTY_SECTOR_NEED;
		printf( "write liunux os from sector number:%d to sector number:%d\n", startsecoff, endsecno);
	}else{
		printf("not found enongh space to write os!\r\n");
		return FALSE;
	}


	char * buf = new char[0x400000];
	*(int*)buf = 0;

	int flag = 0;

	int freesecno = startsecoff;

	flag = 2;

// 	for (; freesecno < endsecno; freesecno+= EMPTY_SECTOR_NEED)
// 	{
// 		ret = SectorReaderWriter::dataReader(freesecno, SECTOR_SIZE *EMPTY_SECTOR_NEED, buf);
// 		if (ret <= 0)
// 		{
// 			return FALSE;
// 		}
// 		else if (memcmp(buf,"LJG",4) == 0)
// 		{
// 			flag = 1;
// 			break;
// 		}
// 		else {
// 			int i = 0;
// 			for (; i < SECTOR_SIZE *EMPTY_SECTOR_NEED; i ++)
// 			{
// 				if (buf[i] == 0 /*|| buf[i] == 0xff || buf[i] == 0xcd || buf[i] == 0xcc ||buf[i] == 0xdd*/)
// 				{
// 					continue;;
// 				}
// 				else {
// 					break;
// 				}
// 			}
// 
// 			if (i >= SECTOR_SIZE * EMPTY_SECTOR_NEED)
// 			{
// 				flag = 2;
// 				break;
// 			}
// 		}
// 	}

	if (freesecno >= endsecno || flag == 0)
	{
		printf("not found enough space\r\n");
		delete buf;
		return FALSE;
	}


	LPLIUNUX_OS_DATA hdr = (LPLIUNUX_OS_DATA)buf;
	hdr->flag = LIUNUX_FLAG;
	hdr->mbrSecOff = freesecno + 1;
	hdr->mbr2SecOff = freesecno + 2;
	hdr->fontSecCnt = 2;
	hdr->fontSecOff = freesecno + 3;

	char * mymbr = 0;
	int mymbrsize = 0;
	ret = FileOper::fileReader(MBR_FILENAME, &mymbr, &mymbrsize);
	if (ret <= 0)
	{
		printf("not found mbr file\r\n");
		return FALSE;
	}
	else if (mymbrsize > 512 - 2 - 64 - 4)
	{
		printf(szout, "mbr size:%u is larger than:%u\r\n", mymbrsize, 512 - 64 - 2 - 4);

		return FALSE;
	}

	memcpy(mymbr + 0x1ba, &freesecno, 4);
	memcpy(mymbr + 0x1be, mbr + 0x1be, 66);	

	ret = OSReaderWriter::writedata(0, SECTOR_SIZE, mymbr);
	if (ret <= 0)
	{
		printf( "mymbr write error\r\n");
		return FALSE;
	}

	delete mymbr;

	if (flag == 1)
	{

	}
	else {
		ret = OSReaderWriter::writedata(hdr->mbrSecOff, SECTOR_SIZE, mbr);
		if (ret <= 0)
		{
			printf("bakmbr write error\r\n");
			return FALSE;
		}

		ret = OSReaderWriter::writedata(hdr->mbr2SecOff, SECTOR_SIZE, mbr);
		if (ret <= 0)
		{
			printf("bakmbr2 write error\r\n");
			return FALSE;
		}
	}

	char * loaderbuf = 0;
	int loadersize = 0;
	ret = FileOper::fileReader(LOADER_FILENAME, &loaderbuf, &loadersize);
	if (ret <= 0)
	{
		printf("read loader or not found loader file\r\n");
		return FALSE;
	}

	char * kerbuf = 0;
	int kersize = 0;
	ret = FileOper::fileReader(KERNEL_EXE_FILENAME, &kerbuf, &kersize);
	if (ret <= 0)
	{
		printf("read kernel or not found kernel file\r\n");
		return FALSE;
	}

	char * fontbuf = 0;
	int fontsize = 0;
	ret = FileOper::fileReader(FONT_FILENAME, &fontbuf, &fontsize);
	if (ret <= 0)
	{
		printf("not found font file\r\n");
		return FALSE;
	}

	char * kerdllbuf = 0;
	int kerdllsize = 0;
	ret = FileOper::fileReader(KERNEL_DLL_FILENAME, &kerdllbuf, &kerdllsize);
	if (ret <= 0)
	{
		printf("not found kernel dll file\r\n");
		return FALSE;
	}

	char * maindllbuf = 0;
	int maindllsize = 0;
	ret = FileOper::fileReader(MAIN_DLL_FILENAME, &maindllbuf, &maindllsize);
	if (ret <= 0)
	{
		printf("not found MAIN dll file\r\n");
		return FALSE;
	}




	int loaderfsmod = loadersize % SECTOR_SIZE;
	int loaderfsalign = loadersize / SECTOR_SIZE;
	if (loaderfsmod)
	{
		loaderfsalign++;
	}

	hdr->loaderSecCnt = loaderfsalign;
	hdr->loaderSecOff = freesecno + 5;

	int kerfsmod = kersize % SECTOR_SIZE;
	int kerfsalign = kersize / SECTOR_SIZE;
	if (kerfsmod)
	{
		kerfsalign++;
	}
	hdr->kernelSecCnt = kerfsalign;
	hdr->kernelSecOff = hdr->loaderSecOff + hdr->loaderSecCnt;

	int kerdllfsmod = kerdllsize % SECTOR_SIZE;
	int kerdllfsalign = kerdllsize / SECTOR_SIZE;
	if (kerdllfsmod)
	{
		kerdllfsalign++;
	}
	hdr->kerdllSecCnt = kerdllfsalign;
	hdr->kerdllSecOff = hdr->kernelSecOff + hdr->kernelSecCnt;

	int maindllfsmod = maindllsize % SECTOR_SIZE;
	int maindllfsalign = maindllsize / SECTOR_SIZE;
	if (maindllfsmod)
	{
		maindllfsalign++;
	}
	hdr->maindllSecCnt = maindllfsalign;
	hdr->maindllSecOff = hdr->kerdllSecOff + hdr->kerdllSecCnt;


	IMAGE_DOS_HEADER *doshdr = (IMAGE_DOS_HEADER*)kerbuf;
	memcpy(kerbuf + (doshdr->e_cparhdr << 4), (char*)hdr, sizeof(LIUNUX_OS_DATA));
	ret = FileOper::fileWriter(KERNEL_EXE_FILENAME, kerbuf, kersize, TRUE);

	ret = OSReaderWriter::writedata(freesecno, SECTOR_SIZE, (char*)hdr);
	if (ret <= 0)
	{
		printf("info sector write error");
		return FALSE;
	}

	ret = OSReaderWriter::writedata(hdr->fontSecOff, SECTOR_SIZE * 2, (char*)fontbuf);
	if (ret <= 0)
	{
		printf("font sector write error");
		return FALSE;
	}

	ret = OSReaderWriter::writedata(hdr->loaderSecOff, loadersize, (char*)loaderbuf);
	if (ret <= 0)
	{
		printf("loader write error");
		return FALSE;
	}

	ret = OSReaderWriter::writedata(hdr->kernelSecOff, kersize, (char*)kerbuf);
	if (ret <= 0)
	{
		printf("kernel write error");
		return FALSE;
	}

	ret = OSReaderWriter::writedata(hdr->kerdllSecOff, kerdllsize, (char*)kerdllbuf);
	if (ret <= 0)
	{
		printf( "kernel dll write error");
		return FALSE;
	}

	ret = OSReaderWriter::writedata(hdr->maindllSecOff, maindllsize, (char*)maindllbuf);
	if (ret <= 0)
	{
		printf( "main dll write error");
		return FALSE;
	}

	const char *szformat = "write ok,sector info offset:%u,rewrite:%u,mbr size:%u,loader size:%u,kernel size:%u,font size:%u,kerneldll size:%u,maindll size:%u";
	printf( szformat, freesecno, flag, mymbrsize, loadersize, kersize, fontsize, kerdllsize, maindllsize);


	delete fontbuf;
	delete loaderbuf;
	delete kerbuf;

	delete kerdllbuf;
	delete maindllbuf;
	delete buf;

	return TRUE;
}
