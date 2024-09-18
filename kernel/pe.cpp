

//#include <windows.h>
#include "video.h"
#include "pe.h"
#include "Utils.h"
#include "fat32/Fat32File.h"
#include "task.h"
#include "malloc.h"

#include "kernel.h"





char * gDllLoadPath = "c:\\liunux\\";



DWORD getAddrFromName(DWORD module, const char * funname) {
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
			unsigned int addr = addrs[idx] + module;
			return addr;
		}
	}

	char szout[1024];
	__printf(szout, "getAddrFromName module:%x,name:%s error\n", module,funname);

	return 0;
}

DWORD getAddrFromOrd(DWORD module, DWORD ord) {
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
	DWORD addr = addrs[funidx] + module;
	return addr;
}



bool relocTable(char* chBaseAddress)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)chBaseAddress;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(chBaseAddress + pDos->e_lfanew);
	PIMAGE_BASE_RELOCATION pLoc = (PIMAGE_BASE_RELOCATION)(chBaseAddress +
		pNt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

	if ((char*)pLoc == (char*)pDos)
	{
		return TRUE;
	}

	DWORD dwDelta = (DWORD)chBaseAddress - pNt->OptionalHeader.ImageBase;

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

bool mapFileWithAttrib(char* pFileBuff, char* chBaseAddress,DWORD * cr3)
{

	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuff;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pFileBuff + pDos->e_lfanew);

	DWORD dwSizeOfHeaders = pNt->OptionalHeader.SizeOfHeaders;
	__memcpy(chBaseAddress, pFileBuff, dwSizeOfHeaders);

	//PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
	PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)((char*)pNt + sizeof(IMAGE_NT_HEADERS));

	int nNumerOfSections = pNt->FileHeader.NumberOfSections;
	for (int i = 0; i < nNumerOfSections; i++, pSection++)
	{
		if ((0 == pSection->VirtualAddress) || (0 == pSection->SizeOfRawData))
		{
			continue;
		}

		char* chDestMem = (char*)((DWORD)chBaseAddress + pSection->VirtualAddress);
		char* chSrcMem = (char*)((DWORD)pFileBuff + pSection->PointerToRawData);
		DWORD dwSizeOfRawData = pSection->SizeOfRawData;
		__memcpy(chDestMem, chSrcMem, dwSizeOfRawData);
	}

	return TRUE;
}

bool mapFile(char* pFileBuff, char* chBaseAddress)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuff;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pFileBuff + pDos->e_lfanew);

	DWORD dwSizeOfHeaders = pNt->OptionalHeader.SizeOfHeaders;
	__memcpy(chBaseAddress, pFileBuff, dwSizeOfHeaders);

	//PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNt);
	PIMAGE_SECTION_HEADER pSection = (PIMAGE_SECTION_HEADER)((char*)pNt + sizeof(IMAGE_NT_HEADERS));

	int nNumerOfSections = pNt->FileHeader.NumberOfSections;
	for (int i = 0; i < nNumerOfSections; i++, pSection++)
	{
		if ((0 == pSection->VirtualAddress) || (0 == pSection->SizeOfRawData))
		{
			continue;
		}

		char* chDestMem = (char*)((DWORD)chBaseAddress + pSection->VirtualAddress);
		char* chSrcMem = (char*)((DWORD)pFileBuff + pSection->PointerToRawData);
		DWORD dwSizeOfRawData = pSection->SizeOfRawData;
		__memcpy(chDestMem, chSrcMem, dwSizeOfRawData);
	}

	return TRUE;
}


DWORD getSizeOfImage(char* pFileBuff)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuff;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pFileBuff + pDos->e_lfanew);
	DWORD dwSizeOfImage = pNt->OptionalHeader.SizeOfImage;

	return dwSizeOfImage;
}


DWORD getEntry(char * pe) {
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pe;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pe + pDos->e_lfanew);
	DWORD entry = pNt->OptionalHeader.AddressOfEntryPoint;

	return entry;
}


DWORD getType(DWORD chBaseAddress) {
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)chBaseAddress;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(chBaseAddress + dos->e_lfanew);

	return nt->FileHeader.Characteristics;
}

DWORD getImageBase(char* pFileBuff)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)pFileBuff;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(pFileBuff + pDos->e_lfanew);
	DWORD imagebase = pNt->OptionalHeader.ImageBase;

	return imagebase;
}

//why need to modify imagebase£¿
int setImageBase(char* chBaseAddress)
{
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)chBaseAddress;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)(chBaseAddress + pDos->e_lfanew);
	pNt->OptionalHeader.ImageBase = (ULONG32)chBaseAddress;

	return TRUE;
}

DWORD importTable(DWORD module) {

	char szout[1024];

	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((DWORD)dos + dos->e_lfanew);
	DWORD rva = nt->OptionalHeader.DataDirectory[1].VirtualAddress;
	DWORD size = nt->OptionalHeader.DataDirectory[1].Size;

	PIMAGE_IMPORT_DESCRIPTOR impd = (PIMAGE_IMPORT_DESCRIPTOR)(rva + module);

	while (1)
	{
		if (impd->FirstThunk == 0 && impd->ForwarderChain == 0 && impd->Name == 0 &&
			impd->DUMMYUNIONNAME.OriginalFirstThunk == 0 && impd->TimeDateStamp == 0)
		{
			break;
		}

		const char * dllname = (const char *)(module + impd->Name);

		//__printf(szout, "find lib:%s\r\n", dllname);
		//__drawGraphChars((unsigned char*)szout, 0);

		if (__strcmp((CHAR*)dllname,"KERNEL32.dll") == 0 ||
			__strcmp((CHAR*)dllname, "USER32.dll") == 0 ||
			__strcmp((CHAR*)dllname, "GDI32.dll") == 0)
		{
			impd++;
			continue;
		}
		else if (__strstr((CHAR*)dllname, "api-ms-win-crt-runtime-") || 
			__strstr((char*)dllname, "VCRUNTIME") || 
			__strstr((char*)dllname, "MSVCR"))
		{
			impd++;
			continue;
		}

		//dllname here without path,so you need to set default path
		HMODULE dll = loadLibFile((LPSTR)dllname);
		if (NULL == dll)
		{
			impd++;
			continue;
		}

		PIMAGE_THUNK_DATA org = (PIMAGE_THUNK_DATA)(impd->DUMMYUNIONNAME.OriginalFirstThunk + module);
		PIMAGE_THUNK_DATA first = (PIMAGE_THUNK_DATA)(impd->FirstThunk + module);
		while (1)
		{
			if (org->u1.Ordinal == 0 || first->u1.Ordinal == 0)
			{
				break;
			}

			DWORD addr = 0;
			if (org->u1.Ordinal & 0x80000000)
			{
				int ord = org->u1.Ordinal & 0xffff;
				addr = getAddrFromOrd((DWORD)dll, ord);
				if (addr <= 0)
				{
					__printf(szout, "getAddrFromOrd function no:%d from lib:%s error\r\n", ord, dllname);

					break;
				}
				else{
// 					__printf(szout, "getAddrFromOrd function no:%d address:%x from lib:%s ok\r\n", ord,addr, dllname);
// 					__drawGraphChars((unsigned char*)szout, 0);
				}
			}
			else {
				PIMAGE_IMPORT_BY_NAME impname = (PIMAGE_IMPORT_BY_NAME)(module + org->u1.AddressOfData);
				addr = getAddrFromName((DWORD)dll, (char*)impname->Name);
				if (addr <= 0)
				{
					__printf(szout, "getAddrFromOrd function:%s from lib:%s error\r\n", impname->Name, dllname);

					break;
				}
				else {
// 					__printf(szout, "getAddrFromOrd function:%s address:%x from lib:%s ok\r\n", impname->Name,addr, dllname);
// 					__drawGraphChars((unsigned char*)szout, 0);
				}
			}

			first->u1.Function = addr;

			org++;
			first++;
		}

		impd++;
	}
	return 0;
}





DWORD memLoadDll(char* filedata, char* addr) {
	mapFile(filedata, addr);
	importTable((DWORD)addr);
	relocTable(addr);
	setImageBase(addr);
	return (DWORD)addr;
}


void initDll() {

	DATALOADERINFO* dl = (DATALOADERINFO*)(gKernelData<<4);

	int ks = dl->_kdllSecCnt * BYTES_PER_SECTOR;
	__memcpy((char*)KERNEL_DLL_SOURCE_BASE, (char*)VSKDLL_LOAD_ADDRESS, ks);

	int ms = dl->_maindllSecCnt * BYTES_PER_SECTOR;
	__memcpy((char*)MAIN_DLL_SOURCE_BASE, (char*)VSMAINDLL_LOAD_ADDRESS, ms);

	__kStoreModule(KERNEL_DLL_MODULE_NAME, KERNEL_DLL_BASE);

	memLoadDll((char*)MAIN_DLL_SOURCE_BASE, (char*)MAIN_DLL_BASE);
}



DWORD loadLibFile(char * dllname) {
	char szout[1024];

	HMODULE dll = (HMODULE)__kGetModule((LPSTR)dllname);	//without path,only has filename
	if (NULL == dll)
	{
		char szdllpath[1024];
		int isfullpath = checkFullPath(dllname);
		if (isfullpath)
		{
			__strcpy(szdllpath, dllname);
		}
		else {
			__strcpy(szdllpath, gDllLoadPath);
			__strcat(szdllpath, dllname);
		}

		char * data = 0;
		int filesize = readFile(szdllpath,&data);
		if (filesize)
		{
			int imagesize = getSizeOfImage((char*)data);
			char * dllptr = (char*)__kMalloc(imagesize);

			mapFile((char*)data, (char*)dllptr);
			setImageBase((char*)dllptr);
			importTable((DWORD)dllptr);
			relocTable((char*)dllptr);

			__kStoreModule(dllname, (DWORD)dllptr);

			__kFree((DWORD)data);

			return (DWORD)dllptr;
		}
		else {
			__printf(szout, "loadlib readFileTo:%s  error\n", szdllpath);

			return FALSE;
		}
	}
	return dll;
}


DWORD loadLibRunFun(char * dllname,char * funcname) {

	DWORD result = 0;

	HMODULE dll = loadLibFile(dllname);
	if (dll)
	{
		DWORD funcaddr = getAddrFromName(dll, funcname);
		if (funcaddr)
		{
			__asm {
				mov eax, funcaddr
				call eax
				mov result, eax
			}
		}
	}

	return result;
}

DWORD loadLibFun(char * dllname, char * funcname) {

	HMODULE dll = loadLibFile(dllname);
	if (dll)
	{
		DWORD addr = getAddrFromName(dll, funcname);
		if (addr)
		{
			return addr;
		}
	}

	return 0;
}



void __kRemoveModule(char * filename) {
	LPDLLMODULEINFO module = (LPDLLMODULEINFO)LIB_INFO_BASE;
	int size = LIB_INFO_SIZE / sizeof(DLLMODULEINFO);
	for (int i = 0; i < size; i++)
	{
		if (module[i].addr == 0 || module[i].name[0] == 0)
		{
			continue;
		}

		if (__strcmp(module[i].name, filename) == 0)
		{
			module[i].addr = 0;
			module[i].name[0] = 0;
		}
	}
}


DWORD __kGetModule(char * filename) {
	LPDLLMODULEINFO module = (LPDLLMODULEINFO)LIB_INFO_BASE;
	int size = LIB_INFO_SIZE / sizeof(DLLMODULEINFO);
	for (int i = 0; i < size; i++)
	{
		if (module[i].addr == 0 ||  module[i].name[0] == 0)
		{
			continue;
		}

		if (__strcmp(module[i].name, filename) == 0)
		{
			return module[i].addr;
		}
	}
	return 0;
}


void __kStoreModule(char * filename, DWORD addr) {
	LPDLLMODULEINFO module = (LPDLLMODULEINFO)LIB_INFO_BASE;
	int size = LIB_INFO_SIZE / sizeof(DLLMODULEINFO);
	for (int i = 0; i < size; i++)
	{
		if ( (module[i].addr == addr) && (module[i].name[0]) && (__strcmp(module[i].name,filename)==0) )
		{
			return;
		}
	}

	for (int i = 0; i < size; i++)
	{
		if (module[i].addr == 0 || module[i].name[0] == 0)
		{
			module[i].addr = addr;
			__strcpy(module[i].name, filename);
		}
	}

	char szout[1024];
	__printf(szout, "__kKeepModule filename:%s,address:%x\n", filename, addr);

	return;
}

DWORD rvaInFile(DWORD module, DWORD rva) {
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((DWORD)dos + dos->e_lfanew);
	int optsize = nt->FileHeader.SizeOfOptionalHeader;
	PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)((DWORD)module +
		dos->e_lfanew + sizeof(nt->Signature) + sizeof(IMAGE_FILE_HEADER) + optsize);

	int seccnt = nt->FileHeader.NumberOfSections;
	for (int i = 0; i < seccnt; i++)
	{
		DWORD end = sections[i].Misc.VirtualSize + sections[i].VirtualAddress;
		DWORD start = sections[i].VirtualAddress;
		if (rva >= start && rva <= end)
		{
			DWORD offset = rva - start;
			DWORD fileoffset = sections[i].PointerToRawData + offset;
			return fileoffset;
		}
	}

	return -1;
}


unsigned char * getRvaSectionName(DWORD module, DWORD rva) {
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((DWORD)dos + dos->e_lfanew);
	int optsize = nt->FileHeader.SizeOfOptionalHeader;
	PIMAGE_SECTION_HEADER sections = (PIMAGE_SECTION_HEADER)((DWORD)module +
		dos->e_lfanew + sizeof(nt->Signature) + sizeof(IMAGE_FILE_HEADER) + optsize);

	int seccnt = nt->FileHeader.NumberOfSections;
	for (int i = 0; i < seccnt; i++)
	{
		DWORD end = sections[i].Misc.VirtualSize + sections[i].VirtualAddress;
		DWORD start = sections[i].VirtualAddress;
		if (rva >= start && rva <= end)
		{
			return sections[i].Name;
		}
	}

	return 0;
}


DWORD getBaseFromEip(DWORD eip) {

	DWORD ip = eip & PAGE_MASK;

	while (1)
	{
		if (getPeType(ip) == 2)
		{
			return ip;
		}

		ip -= PAGE_SIZE;
		if (ip <= 0)
		{
			return 0;
		}
	}
}

int getPeType(DWORD data) {
	if (__memcmp((char*)data, "MZ", 2) == 0)
	{
		int offset = *(DWORD*)(data + 0x3c);
		if (offset < 0x1000 && offset > 0)
		{
			if (__memcmp((char*)data + offset, "PE\x00\x00", 4) == 0)
			{
				return 2;
			}
			else {
				return 1;
			}
		}
		else {
			return 1;
		}
	}
	else if (__memcmp((char*)data, "\x7f\x45\x4c\x46", 4) == 0)
	{
		return 3;
	}
	else {
		return 0;
	}
}

int checkFullPath(char * path) {
	if (__memcmp(path, "c:\\", 3) == 0 || __memcmp(path, "c://", 3) == 0 ||
		__memcmp(path, "C:\\", 3) == 0 || __memcmp(path, "C://", 3) == 0)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}