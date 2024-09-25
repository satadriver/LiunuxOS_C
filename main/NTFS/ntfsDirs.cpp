
#include "def.h"
#include "ntfs/ntfs.h"
#include "../ntfs/ntfsDirs.h"
#include "ntfs/ntfsFile.h"
#include "Utils.h"
#include "video.h"
#include "ata.h"
#include "file.h"
#include "../FileBrowser.h"
#include "fat32/fileutils.h"
#include "file.h"
#include "malloc.h"



unsigned long getIndexDirs(char* buf, LPFILEBROWSER files) {

	int cnt = 0;

	int ret = 0;
	char szout[1024];

	if (__memcmp(buf, "INDX", 4))
	{
		__printf(szout,( char*)"getIdxNextDir format error\n");
		return cnt;
	}

	LPSTD_INDEX_HEADER hdr = (LPSTD_INDEX_HEADER)buf;
	char* baseaddr = (char*)&hdr->SIH_IndexEntryOffset;
	LPSTD_INDEX_ENTRY idxentry = (LPSTD_INDEX_ENTRY)(baseaddr + hdr->SIH_IndexEntryOffset);

	while (1)
	{
		if ((unsigned int)idxentry - (unsigned int)buf >= hdr->SIH_IndexEntryAllocSize)
		{
			break;
		}

		if (idxentry->SIE_IndexEntrySize >= hdr->SIH_IndexEntryAllocSize || idxentry->SIE_IndexEntrySize < sizeof(STD_INDEX_ENTRY) ||
			idxentry->SIE_MFTReferNumber == 0)
		{
			break;
		}

		int fnlen = idxentry->SIE_FileNameSize;
		char* fn = (char*)((unsigned int)idxentry + sizeof(STD_INDEX_ENTRY) - 1);
		char ascfn[256];
		int asclen = unicode2asc((short*)fn, fnlen, ascfn);

		ret = upper2lower(ascfn, __strlen(ascfn));

		__strcpy(files->pathname, ascfn);
		files->secno = (idxentry->SIE_MFTReferNumber & 0x0000ffffffffffff);
		files->filesize = (DWORD)idxentry->SIE_FileRealSize;
		files->attrib = (DWORD)idxentry->SIE_FileFlag;
		if (files->attrib & 0x10000000)
		{
			files->attrib |= FILE_ATTRIBUTE_DIRECTORY;
		}
		else {
			files->attrib |= FILE_ATTRIBUTE_ARCHIVE;
		}
		files++;
		cnt++;

		//MFT idxentry->SIE_MFTReferNumber & 0x0000ffffffffffff  is 1
		idxentry = (LPSTD_INDEX_ENTRY)((unsigned int)idxentry + idxentry->SIE_IndexEntrySize);
	}

	return cnt;
}



DWORD getDirsIndexRoot(LPCommonAttributeHeader hdr, LPFILEBROWSER files) {
	int ret = 0;


	if (hdr->ATTR_ResFlag)
	{
		return FALSE;
	}

	int cnt = 0;

	LPResidentAttributeHeader rhdr = (LPResidentAttributeHeader)hdr;
	if (rhdr->ATTR_DatSz > 0 && rhdr->ATTR_DatOff > 0)
	{
		LPINDEX_ROOT lproot = (LPINDEX_ROOT)((unsigned int)rhdr + rhdr->ATTR_DatOff);
		LPINDEX_HEADER ih = (LPINDEX_HEADER)&lproot->IH;
		LPINDEX_ENTRY entry = (LPINDEX_ENTRY)(ih->IH_EntryOff + (unsigned int)ih);
		LPINDEX_ENTRY lpentry = entry;
		unsigned int entrysize = ih->IH_TalSzOfEntries - sizeof(INDEX_HEADER);
		while (1)
		{
			if (entry->IE_Size <= sizeof(STD_INDEX_ENTRY))
			{
				break;
			}

			if ((unsigned int)entry - (unsigned int)lpentry >= entrysize)
			{
				break;
			}

			int fnlen = entry->IE_FileNameSize;
			char* fn = (char*)((unsigned int)entry + sizeof(INDEX_ENTRY) - 1);
			char ascfn[256];
			int asclen = unicode2asc((short*)fn, fnlen, ascfn);

			ret = upper2lower(ascfn, __strlen(ascfn));

			__strcpy(files->pathname, ascfn);
			files->secno = (entry->IE_MftReferNumber & 0x0000ffffffffffff);

			files->filesize = (DWORD)entry->IE_FileRealSize;
			files->attrib = (DWORD)entry->IE_FileFlag;
			if (files->attrib & 0x10000000)
			{
				files->attrib |= FILE_ATTRIBUTE_DIRECTORY;
			}
			else {
				files->attrib |= FILE_ATTRIBUTE_ARCHIVE;
			}

			files++;
			cnt++;

			entry = (LPINDEX_ENTRY)((unsigned int)entry + entry->IE_Size);
		}
	}

	return cnt;
}


int getNtfsDirs(unsigned long long secoff, LPFILEBROWSER files, DWORD father) {
	int ret = 0;

	int cnt = 0;

	//if (father)
	{
		__strcpy(files->pathname, "..");
		files->secno = father;
		files->attrib = FILE_ATTRIBUTE_DIRECTORY;
		files->filesize = 0;
		files++;
		cnt++;
	}

	char szout[1024];

	DWORD low = secoff & 0xffffffff;
	DWORD high = ( secoff >> 32 )&0xffff;
	char msfinfo[MFTEntrySize * 2];
	ret = readSector(low, high, 2, (char*)msfinfo);
	if (ret <= 0)
	{
		__printf(szout, "getNtfsDirs readSector:%I64x error\n", secoff);
		return cnt;
	}

	char* buffer = (char*)__kMalloc(g_ClusterSize << 1);

	FILE_RECORD_HEADER* frh = (FILE_RECORD_HEADER*)msfinfo;
	LPCommonAttributeHeader attr = (LPCommonAttributeHeader)(msfinfo + sizeof(FILE_RECORD_HEADER));
	while (1)
	{
		if (attr->ATTR_Type == MSF_INDEXALLOC_FLAG)
		{
			if (attr->ATTR_ResFlag)
			{

				LPNonResidentAttributeHeader res = (LPNonResidentAttributeHeader)attr;
				unsigned char* data = (unsigned char*)attr + res->ATTR_DatOff;

				unsigned long long clsno = 0;

				int i = 0;

				while (*(data + i))
				{
					int clscntbytes = (*(data + i) & 0xf);
					int clsnobytes = ((*(data + i) & 0xf0) >> 4);
					if (clscntbytes == 0 || clscntbytes > 4 || clsnobytes == 0 || clsnobytes > 4)
					{
						return FALSE;
					}

					i++;

					unsigned __int64 clscnt = 0;
					char* lpclscnt = (char*)&clscnt;
					for (int j = 0; j < clscntbytes; j++)
					{
						lpclscnt[j] = data[i + j];
					}
					i += clscntbytes;


					unsigned __int64 nextclsno = 0;
					char* lpclsno = (char*)&nextclsno;
					for (int k = 0; k < clsnobytes; k++)
					{
						lpclsno[k] = data[i + k];
					}
					i += clsnobytes;

					if (clscnt == 2 && (nextclsno & 0x8000))
					{
						nextclsno = nextclsno | 0xffffffffffff0000;
					}
					else if (clscnt == 3 && (nextclsno & 0x800000))
					{
						nextclsno = nextclsno | 0xffffffffff000000;
					}
					else if (clscnt == 4 && (nextclsno & 0x80000000))
					{
						nextclsno = nextclsno | 0xffffffff00000000;
					}

					clsno += nextclsno;

					unsigned long long idxsecoff = gNtfsDbr.hideSectors + clsno * g_SecsPerCluster;
					DWORD low = idxsecoff & 0xffffffff;
					DWORD high = (idxsecoff >> 32)&0xffff;

					ret = readSector(low, high, (DWORD)(g_SecsPerCluster * clscnt), (char*)buffer);
					if (ret <= 0)
					{
						__printf(szout, "getNtfsDirs readSector at sector:%I64x,count:%I64x error in 0xA0\n",
							idxsecoff, g_SecsPerCluster * clscnt);
						break;
					}
					else {
						__printf(szout, "ntfs path:%s,sector:%I64x,cluster number:%I64x,cluster total:%I64x\n",
							files->pathname, idxsecoff, clsno, clscnt);

						char* dirdata = (char*)buffer;
						for (int i = 0; i < clscnt; i++)
						{
							cnt += getIndexDirs((char*)dirdata, files);
							files += cnt;
							dirdata += g_ClusterSize;
						}

						__kFree((DWORD)buffer);
						return cnt;
					}
				}
			}
		}
		else if (attr->ATTR_Type == MSF_INDEXROOT_FLAG)
		{
			cnt += getDirsIndexRoot(attr, files);
			files += cnt;
		}
		else if (attr->ATTR_Type == 0xffffffff)	//end with flag == 0xffffffff and len == 0
		{
			break;
		}

		attr = (LPCommonAttributeHeader)((unsigned int)attr + attr->ATTR_Size);
		if ((char*)attr - msfinfo >= frh->BytesInUse || (char*)attr - msfinfo >= MFTEntrySize || (char*)attr - msfinfo <= 0)
		{
			break;
		}
	}

	__kFree((DWORD)buffer);
	return cnt;
}