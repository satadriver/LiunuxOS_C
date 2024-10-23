#pragma once

#include "../def.h"
#include "../file.h"



//00000002~FFFFFFEF		已分配的簇
//FFFFFFF0~FFFFFFF6		系统保留
//FFFFFFF7				坏簇
//FFFFFFF8~FFFFFFFF		文件结束簇


#define FAT32_FLAG				0xAA55
#define FAT_MAINNAME_LEN		8
#define FAT_SUFFIXNAME_LEN		3
#define FAT_NAMETOTAL_LEN		(FAT_MAINNAME_LEN + FAT_SUFFIXNAME_LEN)
#define FAT_HEAD_FLAG			"0xf8\xff\xff\x0f\xff\xff\xff\xff"
#define EMPTY_MAINNAME			"\x00\x00\x00\x00\x00\x00\x00\x00"
#define EMPTY_SUFFIXNAME		"\x00\x00\x00"
#define FAT_BAD_CLUSTER			0xFFFFFFF7
#define FAT_END_FLAG			0x0FFFFFFF
#define VALID_MAX_CLUSTER_NO	0XFFFFFFEF
#define FAT_EMPTY_CLUSTER		0
//FAT32 的0号表项值总是“F8FFFF0F”
//1号表项可能被用于记录“脏标志”以说明文件系统没有被正常卸载或者磁盘表面存在错误。
//1号表项值“FFFFFFFF”或“FFFFFF0F”

#define FAT_DIR_UNICODE_FLAG	0x0f

// #define FILE_OPEN_READONLY		0
// #define FILE_OPEN_HIDDENWRITE	1
// #define FILE_OPEN_HIDDENAPPEND  2
// #define FILE_OPEN_WRITE			3
// #define FILE_OPEN_APPEND		4
// #define DIR_OPEN_ALL			8


//fat缺点
//1 fat32的根目录文件不能超过2个簇的大小,为cluster*2/32个文件(如果簇大小为8KB,开始簇号为2,则最多有510个文件，还有.和..2个目录,共512个子文件和目录)
//2 对于中文支持有限，如果目录中有中文，那么需要更多的其他目录名字表示改中文名，这样每个目录中的文件数变得更少了
//3 fat32的格式只识别4g以内的单个文件
//4 需要结合fat表读写，每次最多一簇个扇区，效率大受影响


#pragma pack(1)


typedef struct 
{
	unsigned char BS_jmpBoot[3];		//跳转指令			offset: 0

	unsigned char BS_OEMName[8];		//原始制造商		offset: 3		MSDOS5.0

	unsigned short BPB_BytesPerSec;		//每扇区字节数		offset:11

	unsigned char BPB_SecPerClus; 		//每簇扇区数		offset:13

	unsigned short BPB_RsvdSecCnt; 		//保留扇区数目		offset:14

	unsigned char BPB_NumFATs; 			//此卷中FAT表数		offset:16

	unsigned short BPB_RootEntCnt;		//FAT32为0			offset:17

	unsigned short BPB_TotSec16; 		//FAT32为0			offset:19

	unsigned char BPB_Media;			//存储介质			offset:21

	unsigned short BPB_FATSz16;			//FAT32为 0			offset:22

	unsigned short BPB_SecPerTrk;		//磁道扇区数		offset:24

	unsigned short BPB_NumHeads;		//磁头数			offset:26

	unsigned int BPB_HiddSec; 			//FAT区前隐扇区数	offset:28

	unsigned int BPB_TotSec32;			//该卷总扇区数		offset:32

	unsigned int BPB_FATSz32; 			//一个FAT表扇区数	offset:36

	unsigned short BPB_ExtFlags;		//FAT32特有			offset:40

	unsigned short BPB_FSVer;			//FAT32特有			offset:42

	unsigned int BPB_RootClus;			//根目录簇号		offset:44

	unsigned short FSInfo;				//保留扇区FSINFO扇区数 offset:48

	unsigned short BPB_BkBootSec;		//备份通常为6		offset:50

	unsigned char BPB_Reserved[12];		//扩展用			offset:52

	unsigned char BS_DrvNum;			//offset:64

	unsigned char BS_Reserved1;			//offset:65

	unsigned char BS_BootSig;			//offset:66

	unsigned int BS_VolID;				//offset:67

	unsigned char BS_FilSysType[11];	//offset:71

	unsigned char BS_FilSysType1[8];	//"FAT32 " offset:82

	unsigned char runCode[420];
	unsigned short systemFlag;
}FAT32_DBR,*LPFAT32_DBR;



typedef struct
{
	unsigned char extFlag[4];		//RRaA

	unsigned char unused[480];

	unsigned char signature[4];		//rrAa

	unsigned int freeClusterCnt;

	unsigned int nextCluster;		//初始化完成后不准确，只有写入文件后才是下一个空簇号

	unsigned char unused2[14];

	unsigned short systemFlag;

}FAT32_FSINFO, *LPFAT32_FSINFO;

//注释1：01111 000010 11100
//1)这里高5位代表小时，由于2^5 = 32，足够表示24小时，这边01111(2进制) = 15(10进制)
//2)次6位代表分钟，同理2 ^ 6 = 64，足够表示60分钟，这边000010(2进制) = 2
//3)低5位表示秒的1 / 2， 计算结果需要加上毫秒位上的进位，这边11100(2进制) = 28(10进制)，所以秒数 = 28 * 2 = 56，
//再加上毫秒上的进位1所以结果为57。

//注释2：0100010 1000 01000
//1)这里高7位代表从1980年开始的年数，笔者计算了下可以到2108年，总之还有90多年可以使用，这边0100010(2进制) = 34，所以年份 = 1980 + 34 = 2014
//2)次4位代表月份，2 ^ 4 = 16，可以表示12个月份，这边 1000(2进制) = 8(10进制)
//3)低5位代表日期，2 ^ 5 = 32，可以表示28~31天，这边 01000(2进制) = 8(10进制)

//当创建一个长文件名文件时，系统会自动加上对应的短文件名，其原则如下：
//(1)取长文件名的前6个字符加上"~1"形成短文件名，扩展名不变。
//(2)如果已存在这个文件名，则符号"~"后的数字递增，直到5。
//(3)如果文件名中”~”后面的数字达到5，则短文件名只使用长文件名的前两个字母。
//通过数学操纵长文件名的剩余字母生成短文件名的后四个字母，然后加后缀”~1”直到最后(如果有必要，或是其他数字以避免重复的文件名)。 
//(4)、如果存在老OS或程序无法读取的字符，换以”_”
//目录项的第12个字节的值，如果为0x0F时则系统认为是长目录项

//3
//一个目录项是否被分配使用由它的第一个字节来描述。
//对于已经分配使用的目录项，它的第一个字节是文件名的第一个字符，
//而文件或目录被删除后，它所对应的目录项的第一个字节将被置为0xE5

//4
//系统在存储长文件名时，总是先按倒序填充长文件名目录项，然后紧跟其对应的短文件名。
//一个长文件名总是和其相应的短文件名一一对应，短文件名没有了长文件名还可以读，
//但长文件名如果没有对应的短文件名，不管什么系统都将忽略其存在。所以短文件名是至关重要的。
//在不支持长文件名的环境中对短文件名中的文件名和扩展名字段作更改(包括删除，因为删除是对首字符改写E5H)，都会使长文件名形同虚设

// 卷标目录项0x0B 字节处的属性值为 0x08

//文件名小写，扩展名小写, 如aaa.aaa        则00011000 = 18h
//文件名大写，扩展名大写, 如AAA.AAA  则00000000 = 00h
//文件名大写，扩展名小写, 如AAA.aaa     则00010000 = 10h
//文件名小写，扩展名大写, 如aaa.AAA     则00001000 = 08h
//长文件名不管是LDE还是与之对应的SDE中的0C字节都是0

typedef struct 
{
	unsigned char mainName[FAT_MAINNAME_LEN];
	unsigned char suffixName[FAT_SUFFIXNAME_LEN];
	unsigned char attr;			//0=read andr write,1=readonly,2=hidden,4=system,8=volume,16=dir,32=archive,15=long file name
	unsigned char lowercase;
	unsigned char mseconds;
	unsigned short creationTime;//14
	unsigned short creationDate;	//16
	unsigned short accessDate;	//18
	unsigned short clusterHigh;	//20
	unsigned short modifyTime;	//22
	unsigned short modifyDate;	//24
	unsigned short clusterLow;		//26
	unsigned int size;			//28
}FAT32DIRECTORY,*LPFAT32DIRECTORY;


typedef struct
{
	unsigned char flag;			//第6为1代表最后一个表项，低5为代表序号，反序由下到上,最大0x1f(1-1f,5f - 1)个表项，最大文件名13*31=403个字符
	unsigned short firstUnicode[5];
	unsigned char attr;
	unsigned char reserved;
	unsigned char checksum;
	unsigned short midUnicode[6];
	unsigned short clusterno;
	unsigned short lastUnicode[2];
}FAT32_UNICODE_DIRECTORY, *LPFAT32_UNICODE_DIRECTORY;

#pragma pack()







#ifdef DLL_EXPORT
extern "C" __declspec(dllexport) int fat32Init();
extern "C" __declspec(dllexport) FAT32_DBR			gFat32Dbr;
extern "C" __declspec(dllexport) FAT32_FSINFO		gFsInfo;
extern "C" __declspec(dllexport) unsigned int *		glpFAT;
extern unsigned int *		glpFAT2;
extern "C" __declspec(dllexport) LPFAT32DIRECTORY	glpRootDir;

extern "C" __declspec(dllexport) int				g_FirstClusterNO;
extern "C" __declspec(dllexport) DWORD				g_SecsBeforeRootDir;
extern "C" __declspec(dllexport) int				gMaxDirsInPath;

extern "C" __declspec(dllexport) int getFat32DBR();
extern "C" __declspec(dllexport) int getFsinfo();
extern "C" __declspec(dllexport) DWORD getRootDirOffset();

extern "C" __declspec(dllexport) int getNextFAT32Cluster(DWORD no);
#else
extern "C" __declspec(dllimport) int fat32Init();
extern "C" __declspec(dllimport) FAT32_DBR gFat32Dbr;
extern "C" __declspec(dllimport) FAT32_FSINFO gFsInfo;
extern "C" __declspec(dllimport) unsigned int * glpFAT;
extern "C" __declspec(dllimport) LPFAT32DIRECTORY glpRootDir;

extern "C" __declspec(dllimport) int g_FirstClusterNO;
extern "C" __declspec(dllimport) int g_SecsBeforeRootDir;
extern "C" __declspec(dllimport) int gMaxDirsInPath;

extern "C" __declspec(dllimport) int getFat32DBR();
extern "C" __declspec(dllimport) int getFsinfo();
extern "C" __declspec(dllimport) DWORD getRootDirOffset();
extern "C" __declspec(dllimport) int getNextFAT32Cluster(DWORD no);
#endif



int checkFlag(unsigned char * sector);

int getFSINFOOffset();

int isFAT32(unsigned char * fat);

int isFAT32DBR(LPFAT32_DBR);

DWORD getFATOffset();

DWORD getFAT2Offset();

int getFAT();

int getFAT2();

int getRootDir();

unsigned int getUnicodeDirectoryChecksum(unsigned char * shortname);

int isValidCluster(unsigned int no);

unsigned int getFirstClusterNo(LPFAT32DIRECTORY);

unsigned int getLastClusterNo(LPFAT32DIRECTORY);

int getFsinfoFreeCluster();

int getNextFAT32EmptyCluster(DWORD no);

unsigned short getClusterNoLow(unsigned int clusterno);
unsigned short getClusterNoHigh(unsigned int clusterno);

int updateFSINFO(int leastclusters, int nextfreecluster);

int updateFAT(int clusterno);

int updateSecsTotal(unsigned int secs);

int getKey(FAT32_DBR dbr, FAT32_FSINFO info, unsigned char * key);