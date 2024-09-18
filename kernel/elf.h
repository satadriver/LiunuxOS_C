#pragma once
#include "def.h"

#define Elf32_Addr    unsigned int
#define Elf32_Half    unsigned short
#define Elf32_Off     unsigned int
#define Elf32_SWord   unsigned int
#define Elf32_Word    unsigned int

#define ET_REL			1	//可重定位文件
#define ET_EXEC			2	//可执行文件
#define ET_DYN			3	//共享目标文件

#define SHT_NULL		0	//非活动的，没有对应的节区。
#define SHT_PROGBITS	1	//程序定义信息，其格式和含义都由程序来解释。
#define SHT_SYMTAB		2	//符号表。用于链接编辑（指 ld 而言）的符号，也可用来实现动态链接。
#define SHT_STRTAB		3	//字符串表。目标文件可能包含多个字符串表节区。
#define SHT_RELA		4	//重定位表项，Elf32_Rela 类型。
#define SHT_HASH		5	//符号哈希表。所有参与动态链接的目标都必须包含一个符号哈希表。一个目标文件只能包含一个哈希表
#define SHT_DYNAMIC		6	//same to PT_DYNAMIC
#define SHT_NOTE		7	//标记信息
#define SHT_NOBITS		8	//不包含任何字节
#define SHT_REL			9	//重定位表，Elf32_rel 类型
#define SHT_SHLIB		10	//保留
#define SHT_DYNSYM		11	//符号表
#define SHT_INITARRAY	14	//init array
#define SHT_FINALARRAY	15	//final array

#define SHF_WRITE		0x1			//进程执行的时候，section内的数据可写。
#define SHF_ALLOC		0x2			//进程执行的时候，section需要占据内存。
#define SHF_EXECINSTR	0x4			//节内包含可以执行的机器指令。
#define SHF_STRINGS		0x20		//包含0结尾的字符串。
#define SHF_MASKOS		0x0ff00000	//这个mask为OS特定的语义保留8位。
#define SHF_MASKPROC	0xf0000000	//这个mask包含的所有位保留（也就是最高字节的高4位），为处理器相关的语义使用。

#define PT_NULL			0
#define PT_LOAD			1
#define PT_DYNAMIC		2		
#define PT_INTERP		3
#define PT_NOTE			4
#define PT_SHLIB		5
#define PT_PHDR			6
#define PT_TLS			7
#define PT_LOOS			0X60000000
#define PT_HIOS			0X6FFFFFFF
#define PT_LOPROC		0X70000000
#define PT_HIPROC		0X7FFFFFFF
#define PT_GNU_STACK	0X6474E551
#define PT_GNU_RELRO	0X6474E552

/*d_tag 的取值*/
/* Dynamic Array Tags - d_tag */
#define DT_NULL		0		/* marks end of _DYNAMIC array */
#define DT_NEEDED	1		/* string table offset of needed lib */
#define DT_PLTRELSZ	2		/* size of relocation entries in PLT */
#define DT_PLTGOT	3		/* address PLT/GOT */
#define DT_HASH		4		/* address of symbol hash table */
#define DT_STRTAB	5		/* address of string table */
#define DT_SYMTAB	6		/* address of symbol table */
#define DT_RELA		7		/* address of relocation table */
#define DT_RELASZ	8		/* size of relocation table */
#define DT_RELAENT	9		/* size of relocation entry */
#define DT_STRSZ	10		/* size of string table */
#define DT_SYMENT	11		/* size of symbol table entry */
#define DT_INIT		12		/* address of initialization func. */
#define DT_FINI		13		/* address of termination function */
#define DT_SONAME	14		/* string table offset of shared obj */
#define DT_RPATH	15		/* string table offset of library
search path */
#define DT_SYMBOLIC	16		/* start sym search in shared obj. */
#define DT_REL		17		/* address of rel. tbl. w addends */
#define DT_RELSZ	18		/* size of DT_REL relocation table */
#define DT_RELENT	19		/* size of DT_REL relocation entry */
#define DT_PLTREL	20		/* PLT referenced relocation entry */
#define DT_DEBUG	21		/* bugger */
#define DT_TEXTREL	22		/* Allow rel. mod. to unwritable seg */
#define DT_JMPREL	23		/* add. of PLT's relocation entries */
#define DT_BIND_NOW	24		/* Bind now regardless of env setting */
#define DT_NUM		25		/* Number used. */
#define DT_LOPROC	0x70000000	/* reserved range for processor */
#define DT_HIPROC	0x7fffffff	/*  specific dynamic array tags */

#define EI_NIDENT		16

#pragma pack(1)

typedef struct elfhdr {
	unsigned char e_ident[EI_NIDENT];		/* ELF Identification */
	Elf32_Half    e_type;					/* object file type */
	Elf32_Half    e_machine;				/* machine */
	Elf32_Word    e_version;				/* object file version */
	Elf32_Addr    e_entry;					//0x18	/* virtual entry point指向的是编译器创建的入口start而不是开发者创建的main函数*/ 
	Elf32_Off	  e_phoff;					//0x1c	/* program header table offset */
	Elf32_Off     e_shoff;					//0x20	/* section header table offset */
	Elf32_Word    e_flags;					/* processor-specific flags */
	Elf32_Half    e_ehsize;					//0x28	/* ELF header size */
	Elf32_Half    e_phentsize;				//0x2a	/* program header entry size */
	Elf32_Half    e_phnum;					//0x2c	/* number of program header entries */
	Elf32_Half    e_shentsize;				//0x2e	/* section header entry size */
	Elf32_Half    e_shnum;					//0x30	/* number of section header entries */
	Elf32_Half    e_shstrndx;				//0x32	/* section header table's "section header string table" entry offset */
} Elf32_Ehdr;

typedef struct  
{
	unsigned char elfFlag[4];
	unsigned char elfClass;		//1
	unsigned char data;			//1=little endian,2=big endian
	unsigned char version;		//1
	unsigned char os_abi;		//0 = unix
	unsigned char abi_version;
	unsigned char pad[7];
}Elf32_Ident;

typedef struct {
	Elf32_Word    p_type;       /* segment type */
	Elf32_Off     p_offset;		/* segment offset 此数据成员给出本段内容在文件中的位置,即段内容的开始位置相对于文件开头的偏移量。*/
	Elf32_Addr    p_vaddr;		/* virtual address of segment */
	Elf32_Addr    p_paddr;		/* physical address - ignored? */
	Elf32_Word    p_filesz;		/* number of bytes in file for seg. */
	Elf32_Word    p_memsz;		/* number of bytes in mem. for seg. */
	Elf32_Word    p_flags;		/* flags */
	Elf32_Word    p_align;		/* memory alignment */
} Elf32_Phdr;

typedef struct {
	Elf32_Word    sh_name;		/* name - index into section header string table section */
	Elf32_Word    sh_type;		/* type */
	Elf32_Word    sh_flags;		/* flags */
	Elf32_Addr    sh_addr;		/* address */
	Elf32_Off     sh_offset;    /* file offset */
	Elf32_Word    sh_size;		/* section size */
	Elf32_Word    sh_link;		/* section header table index link */
	Elf32_Word    sh_info;		/* extra information */
	Elf32_Word    sh_addralign; /* address alignment */
	Elf32_Word    sh_entsize;   /* section entry size */
} Elf32_Shdr;

#define ELF32_R_SYM(i) ((i)>>8)
#define ELF32_R_TYPE(i) ((unsigned char)(i))
#define ELF32_R_INFO(s, t) (((s)<<8) + (unsigned char)(t))

/* Relocation entry with implicit addend */
typedef struct {
	Elf32_Addr    r_offset;		/* offset of relocation */
	Elf32_Word    r_info;        /* symbol table index and type */
	//.rel.dyn 重定位类型一般为R_386_GLOB_DAT和R_386_COPY
	//.rel.plt为R_386_JUMP_SLOT
} Elf32_Rel;

/* Relocation entry with explicit addend */
typedef struct {
	Elf32_Addr		r_offset;		/* offset of relocation */
	Elf32_Word		r_info;			/* symbol table index and type */
	Elf32_SWord		r_addend;
} Elf32_Rela;



typedef struct {
	Elf32_Word st_name;			//符号表项名称。如果该值非0，则表示符号名的字符串表索引(offset)，否则符号表项没有名称。
	Elf32_Addr st_value;		//对于可执行和so文件st_value是一个虚拟地址。android linker 通过将 st_value 与该文件加载到内存的基址相加，从而得到该符号的定义地址。
	Elf32_Word st_size;         //符号的尺寸大小。例如一个数据对象的大小是对象中包含的字节数。
	unsigned char st_info;		//符号的类型和绑定属性。
	unsigned char st_other;		//该成员当前包含 0，其含义没有定义。
	Elf32_Half st_shndx;        //每个符号表项都以和其他节区的关系的方式给出定义。此成员给出相关的节区头部表索引。
} Elf32_sym;


//.got，类型SHT_PROGBITS
//.plt 过程链接表（Procedure Linkage Table），类型SHT_PROGBITS
//.dynamic 指向Elf32_Dyn
//.dynsym指向Elf32_sym



typedef struct {
	Elf32_SWord d_tag;
	union {
		Elf32_Word d_val;
		Elf32_Addr d_ptr;
	} d_un;
} Elf32_Dyn;



#pragma pack()

int initElf(DWORD file, int filesize, DWORD base);

int mapFile(DWORD file, int filesize,DWORD dst);

int realoc(DWORD elf,DWORD base);

Elf32_Shdr * getShstrtabSection(DWORD file);

Elf32_Shdr * getStrtabSection(DWORD elf);

Elf32_Shdr * getSymtabSection(DWORD file);

Elf32_Shdr * getDynamicSection();

Elf32_Shdr * getDynsymSection();


#ifdef DLL_EXPORT
extern "C"  __declspec(dllexport) int runElfFunction(char * filename,char * funcname);
#else
extern "C"  __declspec(dllimport) int runElfFunction(char * filename, char * funcname);
#endif

Elf32_Shdr * getSectionByType(DWORD file, int type);

Elf32_Shdr * getSectionByName(DWORD file, char * sectionname);

char * checkStrExist(DWORD file, char * sectionname);

DWORD getSymAddrByName(DWORD file, char * symname,DWORD base);
DWORD getSymAddrByIdx(DWORD file,int idx, DWORD base);

DWORD getFiniSection(DWORD file, DWORD base);
DWORD getInitSection(DWORD file, DWORD base);
DWORD getFiniArraySection(DWORD file, DWORD base);
DWORD getInitArraySection(DWORD file, DWORD base);

DWORD getInit(DWORD file, DWORD base);
DWORD getFini(DWORD file, DWORD base);

void showAllSegs(DWORD file);
