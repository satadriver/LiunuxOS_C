#include "elf.h"
#include "malloc.h"
#include "video.h"
#include "Utils.h"
#include "file.h"


DWORD alignSegment(DWORD addr,DWORD alignsize) {
	int i = addr % alignsize;
	if (i)
	{
		addr -= i;
	}
	return addr;
}

int mapFile(DWORD file,int filesize, DWORD dst) {

	//__memcpy((char*)dst, (char*)file, filesize);

	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Phdr * ph = (Elf32_Phdr*)((DWORD)elf + elf->e_phoff);
	int phnum = elf->e_phnum;
	int phsize = elf->e_phentsize;
	
	for (int i = 0; i < phnum; i++)
	{
		if (ph->p_type == PT_LOAD)
		{
			__memcpy((char*)dst + ph->p_vaddr, (char*)file + ph->p_offset, ph->p_filesz);
		}
		ph = (Elf32_Phdr *)((DWORD)ph + phsize);
	}

	return phnum;
}




//#define _DEBUG
#ifdef _DEBUG
#include <stdio.h>
#endif

//https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-42444.html#scrolltoc
//https://www.jianshu.com/p/2055bd794e58
//http://www.360doc.com/content/15/1126/20/7377734_516130511.shtml

//Elf32_Rela 中是用r_addend 显式地指出加数;而对 Elf32_Rel来说,加数是隐含在被修改的位置里的
//_dl_runtime_reolve()

int initElf(DWORD file, int filesize, DWORD base) {
	char szout[1024];

	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	Elf32_Shdr * symtab = 0;

	Elf32_Shdr * dynamic = 0;

	Elf32_Shdr * dynsym = 0;

	Elf32_Dyn * dyn = 0;

	int dyncnt = 0;

	for (int i = 0; i < elf->e_shnum; i++)
	{
		Elf32_Shdr * tmphdr = &sh[i];
		int sht = tmphdr->sh_type;
		if (sht)
		{
			if (sht == SHT_DYNAMIC)
			{
				dynamic = tmphdr;
				dyn = (Elf32_Dyn*)(tmphdr->sh_offset + file);
				dyncnt = tmphdr->sh_size / sizeof(Elf32_Dyn);
			}
			else if (sht == SHT_DYNSYM)
			{
				dynsym = tmphdr;
			}
			else if (sht == SHT_SYMTAB)
			{
				symtab = tmphdr;
			}
		}
	}

	Elf32_Rel * rel = 0;
	int relsize = 0;
	int relentsize = 0;

	Elf32_sym * sym = 0;
	char * strsym = 0;
	int strsymsize = 0;
	
	for (int i = 0;i < dyncnt;i ++)
	{
		DWORD tag = dyn[i].d_tag;
		if (tag == DT_REL || tag == DT_RELA)
		{
			rel = (Elf32_Rel*)(dyn[i].d_un.d_ptr + file);
		}else if (tag == DT_RELSZ || tag == DT_RELASZ)
		{
			relsize = dyn[i].d_un.d_val;
		}
		else if (tag == DT_RELENT || tag == DT_RELAENT)
		{
			relentsize = dyn[i].d_un.d_val;
		}
		else if (tag == DT_SYMTAB)
		{
			sym = (Elf32_sym*)(file + dyn[i].d_un.d_val);
		}
		else if (tag == DT_STRTAB)
		{
			strsym = (char*)(file + dyn[i].d_un.d_val);
		}else if (tag == DT_STRSZ)
		{
			strsymsize = dyn[i].d_un.d_val;
		}
	}

	for (int i = 0;i < relsize/relentsize; i++)
	{
		DWORD va = rel[i].r_offset;

		DWORD * lpaddr = (DWORD*)(base + va);

		int reltype = rel[i].r_info & 0xff;
		int funcno = rel[i].r_info >> 8;
		if (reltype == 0)
		{
			__printf(szout,"unsupport relocatoin type:%d,rel address:%x\r\n", reltype,va);

		}else if (reltype == 6)
		{
			DWORD symaddr = sym[funcno].st_value + base;
			*lpaddr = symaddr;
		}else if (reltype == 8)
		{
			*lpaddr = *lpaddr + base;		
		}

		else if (reltype == 1)
		{
			DWORD symaddr = sym[funcno].st_value + base;
			Elf32_Rela * rela = (Elf32_Rela*)rel;
			*lpaddr = rela->r_addend + symaddr ;
		}
		else if (reltype == 2)
		{
			DWORD symaddr = sym[funcno].st_value + base;
			Elf32_Rela * rela = (Elf32_Rela*)rel;
			*lpaddr = rela->r_addend + symaddr - (DWORD)lpaddr + base;
		}
		else {
			continue;
		}

		__printf("function:%s,rel addr:%x,info:%x,new addr:%x\r\n", sym[funcno].st_name + strsym, va, rel[i].r_info, *lpaddr);

	}

	return 0;
}



int realoc(DWORD file,DWORD base) {
	Elf32_Shdr * sh = getSectionByType(file, SHT_RELA);
	if (sh)
	{
		int cnt = sh->sh_size / sizeof(Elf32_Rela);
		Elf32_Rela * rels = (Elf32_Rela *)(sh->sh_offset + file);
		for (int i = 0; i < cnt; i++)
		{
			DWORD idx = rels[i].r_info >> 8;
			int type = rels[i].r_info & 0xff;
			DWORD addr = rels[i].r_offset;

			DWORD funcaddr = getSymAddrByIdx(file, idx,base);
			if (type == 1)
			{
				*(DWORD*)addr = addr + funcaddr;
			}
			else if (type == 2)
			{
				*(DWORD*)addr = addr + funcaddr - base;
			}
		}
	}

	sh = getSectionByType(file, SHT_REL);
	if (sh)
	{
		int cnt = sh->sh_size / sizeof(Elf32_Rel);
		Elf32_Rel * rels = (Elf32_Rel *)(sh->sh_offset + file);
		for (int i = 0; i < cnt; i++)
		{
			DWORD idx = rels[i].r_info >> 8;
			int type = rels[i].r_info & 0xff;
			DWORD addr = rels[i].r_offset;
			DWORD funcaddr = getSymAddrByIdx(file, idx,base);
			if (type == 1)
			{
				*(DWORD*)addr = addr + funcaddr;
			}else if (type ==2)
			{
				*(DWORD*)addr = addr + funcaddr - base;
			}
		}
	}
	return 0;
}


Elf32_Shdr * getSectionByType(DWORD file, int type) {
	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	int seccnt = elf->e_shnum;
	for (int i = 0; i < seccnt; i++)
	{
		if (sh[i].sh_type == type)
		{
			return &sh[i];
		}
	}

	return 0;
}




Elf32_Shdr * getSectionByName(DWORD file,char * sectionname) {
	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;
	
	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	unsigned short shstrndx = elf->e_shstrndx;
	Elf32_Shdr * shstr = &sh[shstrndx];

	DWORD straddr = file + shstr->sh_offset;

	int seccnt = elf->e_shnum;
	for (int i = 0;i < seccnt; i ++)
	{
		if ( __strcmp( sh[i].sh_name + (char*) straddr,sectionname) == 0)
		{
			return &sh[i];
		}
	}

	return 0;
}

Elf32_Shdr * getStrtabSection(DWORD file) {
	return getSectionByName(file, ".strtab");
	//return getSectionByName(file, ".dynstr");
}

Elf32_Shdr * getShstrtabSection(DWORD file) {
	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	unsigned short shstrndx = elf->e_shstrndx;

	Elf32_Shdr * shstr = &sh[shstrndx];

	DWORD straddr = file + shstr->sh_offset;

	if (__strcmp(".shstrtab", (char*)straddr + shstr->sh_name) == 0)
	{
		return &sh[shstrndx];
	}

	return 0;
}





Elf32_Shdr * getSymtabSection(DWORD file) {
	return getSectionByName(file, ".symtab");
}

//.dynsym-dynstr .symtab-strtab
DWORD getSymAddrByName(DWORD file, char * symname,DWORD base) {
	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	Elf32_Shdr * strtab = 0;

	Elf32_Shdr * symtab = 0;

	for (int i = 0;i < elf->e_shnum;i ++)
	{
		if (sh[i].sh_type == SHT_SYMTAB)
		{
			symtab = &sh[i];
			int strtabidx = sh[i].sh_link;
			strtab = &sh[strtabidx];
			break;
		}else if (sh[i].sh_type == SHT_DYNAMIC || sh[i].sh_type == SHT_DYNSYM)
		{

		}
	}

	if (strtab == 0 || symtab == 0)
	{
		return FALSE;
	}

	char * strs = (char*)(strtab->sh_offset + file);

	Elf32_sym * syms = (Elf32_sym*)(symtab->sh_offset + file);
	int symcnt = symtab->sh_size / sizeof(Elf32_sym);
	for (int i = 0;i < symcnt;i++)
	{
		if (syms[i].st_name)
		{
			char * funcname = strs + syms[i].st_name;
			if (__strcmp(funcname, symname)==0)
			{
				return syms[i].st_value + base;
			}
		}
	}

	return 0;
}


//.dynsym-dynstr .symtab-strtab
DWORD getSymAddrByIdx(DWORD file, int idx, DWORD base) {
	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	Elf32_Shdr * strtab = 0;

	Elf32_Shdr * symtab = 0;

	for (int i = 0; i < elf->e_shnum; i++)
	{
		if (sh[i].sh_type == SHT_SYMTAB)
		{
			symtab = &sh[i];
			int strtabidx = sh[i].sh_link;
			strtab = &sh[strtabidx];
			break;
		}
	}

	if (strtab == 0 || symtab == 0)
	{
		return FALSE;
	}

	char * strs = (char*)(strtab->sh_offset + file);
	Elf32_sym * syms = (Elf32_sym*)(symtab->sh_offset + file);
	int symcnt = symtab->sh_size / sizeof(Elf32_sym);
	return syms[idx].st_value + base;
}

char * checkStrExist(DWORD file, char * str) {

	Elf32_Shdr *sh = getStrtabSection(file);
	if (sh == 0)
	{
		return 0;
	}

	char * strs = sh->sh_offset + (char*)file;
	int len = sh->sh_size;
	char * hdr = strs;
	int no = 0;
	while (hdr < strs + len)
	{
		if (__strcmp(hdr, str) == 0)
		{
			return hdr;
		}
		hdr += __strlen(hdr);
		no++;
	}

	return 0;
}


int runElfFunction(char * filename, char * funcname){

	int filesize = 0;
	char * data = 0;
	int result = 0;

	char szout[1024];

	filesize = readFile(filename, &data);
	if (filesize <= 0)
	{
		return FALSE;
	}

	char * elf = (char*)__kMalloc(filesize);

	result = mapFile((DWORD)data,filesize, (DWORD)elf);

	typedef int(*ptrfunction)(int x, int y);

	initElf((DWORD)data, filesize, (DWORD)elf);

	realoc((DWORD)data, (DWORD)elf);

	DWORD addr = getSymAddrByName((DWORD)data, funcname, (DWORD)elf);
	if (addr )
	{
		ptrfunction lpfunction = (ptrfunction)addr;
		result = lpfunction(6, 7);
		
		__sprintf(szout,"linux function result:%x\r\n", result);
	}

	__kFree((DWORD)elf);
	__kFree((DWORD)data);

	return 0;
}

Elf32_Shdr * getDynamicSection() {
	return 0;
}

Elf32_Shdr * getDynsymSection() {
	return 0;
}

//_term_proc
DWORD getFiniSection(DWORD file, DWORD base) {
	Elf32_Shdr * sh = getSectionByName(file, ".fini");
	return (DWORD)sh->sh_offset + file;
}
//_init_proc
DWORD getInitSection(DWORD file, DWORD base) {
	Elf32_Shdr * sh = getSectionByName(file, ".init");
	return sh->sh_offset + file;
}


DWORD getFiniArraySection(DWORD file, DWORD base) {
	Elf32_Shdr * sh = getSectionByName(file, ".fini_array");

	DWORD offset = sh->sh_offset + file;

	DWORD addr = *(DWORD *)offset;
	return addr + base;
}

DWORD getInitArraySection(DWORD file,DWORD base) {
	Elf32_Shdr * sh = getSectionByName(file, ".init_array");
	DWORD offset = sh->sh_offset + file;

	DWORD addr = *(DWORD *)offset ;
	return addr + base;

}

//.init segment对应导出表中的函数.init函数
DWORD getInit(DWORD file,DWORD base) {
	Elf32_Shdr * sh = getSectionByType(file, SHT_INITARRAY);
	if (sh)
	{
		int shoff = sh->sh_offset;
		DWORD* addr = (DWORD *)(sh->sh_offset + file);
		DWORD func = *addr;
		return func+file;
	}
	return 0;
}

//.fini segment对应导出表中的函数.term函数
DWORD getFini(DWORD file,DWORD base) {
	Elf32_Shdr * sh = getSectionByType(file, SHT_FINALARRAY);
	if (sh)
	{
		DWORD* addr = (DWORD *)(sh->sh_offset + file);
		DWORD func = *addr;
		return func+file;
	}
	return 0;
}

//got:Global Offset Table
//plt:Procedure Linkage Table

//main函数call会调用跳转到.got.plt中保存的printf的地址，如果是第一次，那么保存的地址就是.got.plt中的下一句话，
//这个下一句话就是压入这个符号在.rel.plt中的重定位表的偏移量，然后ld程序就会根据重定位表中的信息加上这个偏移量找到这个地址

//“.got”和".got.plt",前者用来保存全局变量引用的地址，后者用来保存函数引用的地址
//got[0]: 本ELF动态段(.dynamic段）的装载地址
//got[1]：本ELF的link_map数据结构描述符地址
//got[2]：_dl_runtime_resolve函数的地址


void showAllSegs(DWORD file) {
	Elf32_Ehdr * elf = (Elf32_Ehdr*)file;

	Elf32_Shdr * sh = (Elf32_Shdr*)((DWORD)elf + elf->e_shoff);

	unsigned short shstrndx = elf->e_shstrndx;

	Elf32_Shdr * shstr = &sh[shstrndx];

	for (int i = 0; i < elf->e_shnum; i++)
	{
		char * name = (char*)(sh[i].sh_name + shstr->sh_offset + file);
		__printf("section name:%s,type:%x,offset:%x,address:%x,size:%x,entsize:%x\r\n",
			name,sh[i].sh_type,sh[i].sh_offset ,sh[i].sh_addr,sh[i].sh_size,sh[i].sh_entsize);
	}
}