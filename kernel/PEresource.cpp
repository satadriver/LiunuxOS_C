
#include "peResource.h"
#include "Pe.h"
#include "Utils.h"



int resourceParser(DWORD module,DWORD resbase, PIMAGE_RESOURCE_DIRECTORY resdir,int level,DWORD id,DWORD type,DWORD * offset,DWORD * size)
{
	int ret = 0;
	int find = 0;
	char buf[1024];

	if (resdir->NumberOfIdEntries == 0 && resdir->NumberOfNamedEntries == 0)
	{
		return 0;
	}

	int cnt = resdir->NumberOfIdEntries + resdir->NumberOfNamedEntries;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY res_dir_entry = 
		(PIMAGE_RESOURCE_DIRECTORY_ENTRY)((DWORD)resdir + sizeof(IMAGE_RESOURCE_DIRECTORY));
	for (int i = 0; i < cnt ; i ++)
	{
		if (res_dir_entry->DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.DataIsDirectory)
		{
			if (res_dir_entry->DUMMYUNIONNAME.DUMMYSTRUCTNAME.NameIsString)
			{
				PIMAGE_RESOURCE_DIR_STRING_U str = 
					(PIMAGE_RESOURCE_DIR_STRING_U)(resbase + res_dir_entry->DUMMYUNIONNAME.DUMMYSTRUCTNAME.NameOffset);
				ret = unicode2asc((short*)str->NameString, str->Length, buf);
				//__printf("get resource level:%u name:%s\r\n", (char*)level, buf);
			}
			else {
				//__printf("get resource level:%u type:%u:\r\n", (char*)level, name);
			}

			//name高位是0的话，第一层是类型，如上图rt定义
			if (level == 1)
			{
				if (type != res_dir_entry->DUMMYUNIONNAME.Name)
				{
					res_dir_entry++;
					continue;
				}
			}

			//第二层是id号码，第三层是语言标识
			if (level == 2 )
			{
				if (res_dir_entry->DUMMYUNIONNAME.Name != id)
				{
					res_dir_entry++;
					continue;
				}
			}

			int nextlevel = level + 1;

			PIMAGE_RESOURCE_DIRECTORY nextdir = 
				(PIMAGE_RESOURCE_DIRECTORY)(resbase + res_dir_entry->DUMMYUNIONNAME2.DUMMYSTRUCTNAME2.OffsetToDirectory);
			ret = resourceParser(module, resbase, nextdir, nextlevel,id,type,offset,size);
		}
		else {
			if (res_dir_entry->DUMMYUNIONNAME.DUMMYSTRUCTNAME.NameIsString)
			{
				PIMAGE_RESOURCE_DIR_STRING_U str = 
					(PIMAGE_RESOURCE_DIR_STRING_U)(resbase + res_dir_entry->DUMMYUNIONNAME.DUMMYSTRUCTNAME.NameOffset);
				ret = unicode2asc((short*)str->NameString, str->Length, buf);
				//__printf("get item name:%s,size:%u,address:%x\r\n", buf,size,offset);
			}
			else {
				//__printf("get item type:%u,size:%u,address:%x\r\n",(char*) name,size,offset);
			}		

#ifdef _DEBUG
			static int gType = 0;
			static int gId = 0;

			int mytype = gType;
			int myid = gId;
			int mylevel = level;
#endif
			PIMAGE_RESOURCE_DATA_ENTRY res_data_entry =
				(PIMAGE_RESOURCE_DATA_ENTRY)(resbase + res_dir_entry->DUMMYUNIONNAME2.OffsetToData);
			char* resoffset = (char*)res_data_entry->OffsetToData + module;
			DWORD ressize = res_data_entry->Size;

			*offset = res_data_entry->OffsetToData + module;
			*size = res_data_entry->Size;		
		}
		res_dir_entry++;
	}

	return 0;
}



int getResFromID(DWORD module,int id,DWORD type,DWORD * offset,DWORD * size) 
{


	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(module + dos->e_lfanew);
	PIMAGE_FILE_HEADER fh = &nt->FileHeader;

	PIMAGE_RESOURCE_DIRECTORY res = (PIMAGE_RESOURCE_DIRECTORY)(nt->OptionalHeader.DataDirectory[2].VirtualAddress + module);

	*offset = 0;
	*size = 0;
	resourceParser(module,(DWORD)res, res, 1,id,type,offset,size);

	return 0;
}


int getResFromName(DWORD module, const char *name, DWORD type, DWORD * offset,DWORD * size) 
{
	PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)module;
	PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)(module + dos->e_lfanew);
	PIMAGE_FILE_HEADER fh = &nt->FileHeader;

	PIMAGE_RESOURCE_DIRECTORY res = 
		(PIMAGE_RESOURCE_DIRECTORY)(nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress + module);

	*offset = 0;
	*size = 0;
	resourceParser(module, (DWORD)res, res, 1,(DWORD)name,type,offset,size);

	return 0;
}