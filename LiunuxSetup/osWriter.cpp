#include "osWriter.h"

#define MSF_ROOTDIR_OFFSET 10




int osWriter() {
	int ret = 0;
	char szout[1024];

	char mbr[1024] = { 0 };
	LPMBR lpmbr = (LPMBR)mbr;
	ret = SectorReaderWriter::dataReader(0, SECTOR_SIZE, mbr);
	if (ret <= 0)
	{
		MessageBoxA(0, "mbr read error", "mbr read error", MB_OK);
		return FALSE;
	}

	char dbr[1024] = { 0 };
	ret = SectorReaderWriter::dataReader(lpmbr->dpt[0].offset, SECTOR_SIZE, dbr);
	if (ret <= 0)
	{
		MessageBoxA(0, "dbr read error", "dbr read error", MB_OK);
		return FALSE;
	}

	unsigned __int64 endsecno = 0;
	unsigned __int64 startsecoff = 0;

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
			//startsecoff = lpmbr->dpt[0].offset + lpmbr->dpt[0].sectortotal - EMPTY_SECTOR_NEED ;

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
		wsprintfA(szout, "write liunux os from sector number:%I64d to sector number:%I64d error\n", startsecoff, endsecno);
		MessageBoxA(0, szout, szout, MB_OK);
		return FALSE;
	}

	endsecno = startsecoff + EMPTY_SECTOR_NEED;

	wsprintfA(szout, "write liunux os from sector number:%I64d to sector number:%I64d\n", startsecoff, endsecno);
	MessageBoxA(0, szout, szout, MB_OK);

	char * buf = new char[0x1000000];
	*(int*)buf = 0;

	int flag = 0;

	DWORD freesecno = startsecoff;

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
		MessageBoxA(0, "not found enough space", "not found enough space", MB_OK);
		delete buf;
		return FALSE;
	}

#ifdef _DEBUG
	ret = SectorReaderWriter::dataReader(freesecno, SECTOR_SIZE, buf);
	strcpy(buf, "hello,how are you?fine thank you,and you?i'm fine too");
	ret = SectorReaderWriter::dataWriter(freesecno, SECTOR_SIZE, buf);
	memset(buf, 0, SECTOR_SIZE);
	ret = SectorReaderWriter::dataReader(freesecno, SECTOR_SIZE, buf);
	memset(buf, 0, SECTOR_SIZE);
#endif

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
		MessageBoxA(0, "not found mbr file", "not found mbr file", MB_OK);
		return FALSE;
	}
	else if (mymbrsize > 512 - 2 - 64 - 4)
	{
		wsprintfA(szout, "mbr size:%u is larger than:%u", mymbrsize, 512 - 64 - 2 - 4);
		MessageBoxA(0, szout, szout, MB_OK);
		return FALSE;
	}

	memcpy(mymbr + 0x1ba, &freesecno, 4);
	memcpy(mymbr + 0x1be, mbr + 0x1be, 66);	//include 0x55aa in end

	ret = SectorReaderWriter::dataWriter(0, SECTOR_SIZE, mymbr);
	if (ret <= 0)
	{
		MessageBoxA(0, "mymbr write error", "mymbr write error", MB_OK);
		return FALSE;
	}

	delete mymbr;

	if (flag == 1)
	{

	}
	else {
		ret = SectorReaderWriter::dataWriter(hdr->mbrSecOff, SECTOR_SIZE, mbr);
		if (ret <= 0)
		{
			MessageBoxA(0, "bakmbr write error", "bakmbr write error", MB_OK);
			return FALSE;
		}

		ret = SectorReaderWriter::dataWriter(hdr->mbr2SecOff, SECTOR_SIZE, mbr);
		if (ret <= 0)
		{
			MessageBoxA(0, "bakmbr2 write error", "bakmbr2 write error", MB_OK);
			return FALSE;
		}
	}

	char * loaderbuf = 0;
	int loadersize = 0;
	ret = FileOper::fileReader(LOADER_FILENAME, &loaderbuf, &loadersize);
	if (ret <= 0)
	{
		MessageBoxA(0, "not found loader file", "not found loader file", MB_OK);
		return FALSE;
	}

	char * kerbuf = 0;
	int kersize = 0;
	ret = FileOper::fileReader(KERNEL_EXE_FILENAME, &kerbuf, &kersize);
	if (ret <= 0)
	{
		MessageBoxA(0, "not found kernel file", "not found kernel file", MB_OK);
		return FALSE;
	}

	char * fontbuf = 0;
	int fontsize = 0;
	ret = FileOper::fileReader(FONT_FILENAME, &fontbuf, &fontsize);
	if (ret <= 0)
	{
		MessageBoxA(0, "not found font file", "not found font file", MB_OK);
		return FALSE;
	}

	char * kerdllbuf = 0;
	int kerdllsize = 0;
	ret = FileOper::fileReader(KERNEL_DLL_FILENAME, &kerdllbuf, &kerdllsize);
	if (ret <= 0)
	{
		MessageBoxA(0, "not found kernel dll file", "not found kernel dll file", MB_OK);
		return FALSE;
	}

	char * maindllbuf = 0;
	int maindllsize = 0;
	ret = FileOper::fileReader(MAIN_DLL_FILENAME, &maindllbuf, &maindllsize);
	if (ret <= 0)
	{
		MessageBoxA(0, "not found MAIN dll file", "not found main dll file", MB_OK);
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

	ret = SectorReaderWriter::dataWriter(freesecno, SECTOR_SIZE, (char*)hdr);
	if (ret <= 0)
	{
		MessageBoxA(0, "info sector write error", "info sector write error", MB_OK);
		return FALSE;
	}

	ret = SectorReaderWriter::dataWriter(freesecno + 3, SECTOR_SIZE * 2, (char*)fontbuf);
	if (ret <= 0)
	{
		MessageBoxA(0, "font sector write error", "font sector write error", MB_OK);
		return FALSE;
	}

	ret = SectorReaderWriter::dataWriter(hdr->loaderSecOff, loadersize, (char*)loaderbuf);
	if (ret <= 0)
	{
		MessageBoxA(0, "loader write error", "loader write error", MB_OK);
		return FALSE;
	}

	ret = SectorReaderWriter::dataWriter(hdr->kernelSecOff, kersize, (char*)kerbuf);
	if (ret <= 0)
	{
		MessageBoxA(0, "kernel write error", "kernel write error", MB_OK);
		return FALSE;
	}

	ret = SectorReaderWriter::dataWriter(hdr->kerdllSecOff, kerdllsize, (char*)kerdllbuf);
	if (ret <= 0)
	{
		MessageBoxA(0, "kernel dll write error", "kernel dll write error", MB_OK);
		return FALSE;
	}

	ret = SectorReaderWriter::dataWriter(hdr->maindllSecOff, maindllsize, (char*)maindllbuf);
	if (ret <= 0)
	{
		MessageBoxA(0, "main dll write error", "main dll write error", MB_OK);
		return FALSE;
	}

	char szformat[] = "write ok,sector info offset:%u,rewrite:%u,mbr size:%u,loader size:%u,kernel size:%u,font size:%u,kerneldll size:%u,maindll size:%u";
	wsprintfA(szout, szformat, freesecno, flag, mymbrsize, loadersize, kersize, fontsize, kerdllsize, maindllsize);
	MessageBoxA(0, szout, szout, MB_OK);

	delete fontbuf;
	delete loaderbuf;
	delete kerbuf;

	delete kerdllbuf;
	delete maindllbuf;
	delete buf;

	return TRUE;
}
