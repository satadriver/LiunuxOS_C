#pragma once
#include "def.h"
#include "file.h"


//Unused by ISO 9660(0-0x8000)
//Volume Descriptor (2,048 B) (Root directory)
//Directories and files

//Volume Descriptor type:
/*
0		Boot Record
1		Primary Volume Descriptor
2		Supplementary Volume Descriptor
3		Volume Partition Descriptor
4-254	Reserved
255		Volume Descriptor Set Terminator
*/

//1 前32768(0x8000)个字节(16 个扇区)被保留 
//2 第16个扇区开始是Volume Descriptor
//0 1 Volume Descriptor type
//1 5 “CD001”
//2 1 version
//如果是Primary Volume Descriptor,扇区偏移0x9c存放根目录,从根目录到每个文件的寻址都是下面结构

/*
0	1			Length of Directory Record.

1	1			Extended Attribute Record length.

2	8			Location of extent (LBA) in both-endian format.

10	8			Data length (size of extent) in both-endian format.

18	7			Recording date and time (see format below).

25	1			File flags (see below).

26	1			File unit size for files recorded in interleaved mode, zero otherwise.

27	1			Interleave gap size for files recorded in interleaved mode, zero otherwise.

28	4			Volume sequence number - the volume that this extent is recorded on, in 16 bit both-endian format.

32	1			Length of file identifier (file name). This terminates with a ';'

character followed by the file ID number in ASCII coded decimal ('1').

33	(variable)	File identifier.

(variable)	1	Padding field - zero if length of file identifier is odd, otherwise,
this field is not present. This means that a directory entry will always start on an even byte number.

(variable)	(variable)	System Use - The remaining bytes up to the maximum record size of 255 may be used for extensions of ISO 9660.
The most common one is the System Use Share Protocol (SUSP) and its application,the Rock Ridge Interchange Protocol (RRIP).
*/


#define ISO9660FS_VOLUME_DESCRIPTOR_NO 16

#define ISO9660FS_LEAST_DIR_SIZE		34

#pragma pack(1)



// typedef struct
// {
// 	char pathname[256];
// 	DWORD secno;
// 	DWORD filesize;
// 	DWORD attrib;
// }FILEBROWSER, *LPFILEBROWSER;


#define ISODCL(from, to) (to - from + 1)  

struct iso_primary_descriptor {

	char type[ISODCL(1, 1)]; /* 711 */

	char id[ISODCL(2, 6)];

	char version[ISODCL(7, 7)]; /* 711 */

	char unused1[ISODCL(8, 8)];

	char system_id[ISODCL(9, 40)]; /* achars */

	char volume_id[ISODCL(41, 72)]; /* dchars */

	char unused2[ISODCL(73, 80)];

	char volume_space_size[ISODCL(81, 88)]; /* 733 */

	char unused3[ISODCL(89, 120)];

	char volume_set_size[ISODCL(121, 124)]; /* 723 */

	char volume_sequence_number[ISODCL(125, 128)]; /* 723 */

	char logical_block_size[ISODCL(129, 132)]; /* 723 */ //我们需要的，逻辑块大小，其实也可以硬性规定为0x800，这里是为了保证通用  

	char path_table_size[ISODCL(133, 140)]; /* 733 */

	char type_l_path_table[ISODCL(141, 144)]; /* 731 */

	char opt_type_l_path_table[ISODCL(145, 148)]; /* 731 */

	char type_m_path_table[ISODCL(149, 152)]; /* 732 */

	char opt_type_m_path_table[ISODCL(153, 156)]; /* 732 */

	char root_directory_record[ISODCL(157, 190)]; /* 9.1 */  //我们需要的，记载了记录根文件相关数据的数据结构  

	char volume_set_id[ISODCL(191, 318)]; /* dchars */

	char publisher_id[ISODCL(319, 446)]; /* achars */

	char preparer_id[ISODCL(447, 574)]; /* achars */

	char application_id[ISODCL(575, 702)]; /* achars */

	char copyright_file_id[ISODCL(703, 739)]; /* 7.5 dchars */

	char abstract_file_id[ISODCL(740, 776)]; /* 7.5 dchars */

	char bibliographic_file_id[ISODCL(777, 813)]; /* 7.5 dchars */

	char creation_date[ISODCL(814, 830)]; /* 8.4.26.1 */

	char modification_date[ISODCL(831, 847)]; /* 8.4.26.1 */

	char expiration_date[ISODCL(848, 864)]; /* 8.4.26.1 */

	char effective_date[ISODCL(865, 881)]; /* 8.4.26.1 */

	char file_structure_version[ISODCL(882, 882)]; /* 711 */

	char unused4[ISODCL(883, 883)];

	char application_data[ISODCL(884, 1395)];

	char unused5[ISODCL(1396, 2048)];

};




typedef struct
{
	unsigned char len;		//0

	unsigned char erl;

	unsigned int lba;			//2
	unsigned int lbabe;			//6
	DWORD datasize;				//10
	DWORD datasizebe;			//14
	unsigned char datatime[7];		//18
	unsigned char flag;				//25
	unsigned char fus;			//26
	unsigned char igs;			//27
	DWORD volumeno;				//28
	unsigned char filenamelen;	//32
	char filename;	//33

}ISO9660FSDIR, *LPISO9660FSDIR;

#pragma pack()



int browseISO9660File(LPFILEBROWSER files);

int readIso9660Dirs(DWORD secno, LPFILEBROWSER files);

int readIso9660File(DWORD secno, DWORD seccnt, char ** buf);