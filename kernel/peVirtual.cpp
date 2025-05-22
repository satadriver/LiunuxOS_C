
#include "video.h"
#include "pe.h"
#include "Utils.h"
#include "peVirtual.h"
#include "task.h"
#include "malloc.h"






DWORD getAddrFromNameV(DWORD module, const char * funname,DWORD vaddr) {
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((DWORD)dos + dos->e_lfanew);
	DWORD exptrva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	//DWORD size = nt->OptionalHeader.DataDirectory[0].Size;

	PIMAGE_EXPORT_DIRECTORY exptable = (PIMAGE_EXPORT_DIRECTORY)(exptrva + module);

	// const char * name = (const char*)(exp->Name + module);

	const char ** funnames = (const char **)(exptable->AddressOfNames + module);
	for (unsigned int i = 0; i < exptable->NumberOfNames; i++)
	{
		const char * functionname = (funnames[i] + module);
		if (__strcmp((char*)funname, (char*)functionname) == 0)
		{
			WORD * ords = (WORD*)(exptable->AddressOfNameOrdinals + module);
			int idx = ords[i];
			DWORD * addrs = (DWORD *)(exptable->AddressOfFunctions + module);
			unsigned int addr = addrs[idx] + vaddr;
			return addr;
		}
	}

	char szout[1024];
	__printf(szout, "getAddrFromName module:%x,name:%s error\n", module, funname);

	return 0;
}

DWORD getAddrFromOrdV(DWORD module, DWORD ord,DWORD vaddr) {
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((DWORD)dos + dos->e_lfanew);
	DWORD rva = nt->OptionalHeader.DataDirectory[0].VirtualAddress;
	DWORD size = nt->OptionalHeader.DataDirectory[0].Size;

	PIMAGE_EXPORT_DIRECTORY exp = (PIMAGE_EXPORT_DIRECTORY)(rva + module);

	unsigned int funidx = ord - exp->Base;
	if (funidx < 0 || funidx >= exp->NumberOfFunctions)
	{
		char szout[1024];
		__printf(szout, "getAddrFromOrd module:%x,ord:%d error\n", module, ord);

		return 0;
	}

	DWORD * addrs = (DWORD *)(exp->AddressOfFunctions + module);
	DWORD addr = addrs[funidx] + vaddr;
	return addr;
}



bool relocTableV(char* chBaseAddress,DWORD vaddr)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)chBaseAddress;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(chBaseAddress + pDos->e_lfanew);
	PIMAGE_BASE_RELOCATION pLoc = (PIMAGE_BASE_RELOCATION)(chBaseAddress +
		pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	if ((char*)pLoc == (char*)pDos)
	{
		return TRUE;
	}

	DWORD dwDelta = (DWORD)vaddr - pNt->OptionalHeader.ImageBase;

	while ((pLoc->VirtualAddress + pLoc->SizeOfBlock) != 0)
	{
		WORD *pLocData = (WORD *)((char*)pLoc + sizeof(IMAGE_BASE_RELOCATION));

		int nNumberOfReloc = (pLoc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

		for (int i = 0; i < nNumberOfReloc; i++)
		{
			if ((DWORD)(pLocData[i] & 0x0000F000) == 0x00003000)
			{
				DWORD* pAddress = (DWORD *)((char*)pDos + pLoc->VirtualAddress + (pLocData[i] & 0x0FFF));

				*pAddress += dwDelta;
			}
		}

		pLoc = (PIMAGE_BASE_RELOCATION)((char*)pLoc + pLoc->SizeOfBlock);
	}

	return TRUE;
}



int setImageBaseV(char* chBaseAddress,DWORD vaddr)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)chBaseAddress;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(chBaseAddress + pDos->e_lfanew);
	pNt->OptionalHeader.ImageBase = (ULONG32)vaddr;

	return TRUE;
}


DWORD memLoadDllV(char* filedata,int size,char* addr,DWORD vaddr,DWORD * cr3) {
	int ret = 0;
	mapFile(filedata, (char*)addr);
	//remap((DWORD)addr,size,cr3);
	ret = importTable((DWORD)addr);
	if (ret == 0) {
		//return 0;
	}
	relocTableV(addr,vaddr);
	setImageBaseV(addr,vaddr);
	return (DWORD)addr;
}













