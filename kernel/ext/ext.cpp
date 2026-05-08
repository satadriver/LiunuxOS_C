

#include "ext.h"

#include "../file.h"
#include "../ata.h"

#include "../Utils.h"
#include "../video.h"
#include "../malloc.h"

char gExt4Dbr[512];
unsigned long long g_ext4_part_offset = 0;

ext2_super_block gExt4SuperBlock;
int gLogBlockSize = 0;
int s_first_data_block = 0;

ext2_group_desc gExt4GroupDesc;
unsigned long long g_inode_offset = 0;

ext2_inode* gExt4Inode = 0;

ext4_dir_entry_2* gExt4RootDir = 0;




int GetNextPath(char path, int a1) {
	return 0;
}


int InitExt4() {
	GetExt4DBR();
	GetSuperBlock();
	GetGroupDesc();
	GetExt4Inode();
	GetExt4RootDir();
	return 0;
}

int GetExt4DBR() {
	int ret = 0;
	unsigned long max = 0;
	unsigned long seq = 0;
	for (int i = 0; i < 4; i++) {
		//if (gMBR.dpt[i].flag & 0x80) 
		if(gMBR.dpt[i].type)
		{
			unsigned long total = gMBR.dpt[i].total;
			if (total > max) {
				max = total;
				seq = gMBR.dpt[i].offset;
			}
		}
	}

	ret = readSector(seq, 0, 1, (char*)&gExt4Dbr);

	g_ext4_part_offset = seq;

	return ret;
}


int GetSuperBlock() {
	int ret = 0;
	char szout[256];

	unsigned long long sector = g_ext4_part_offset + 2;
	DWORD low = sector & 0xffffffff;
	DWORD high = sector >>32;

	int cnt = sizeof(ext2_super_block) / BYTES_PER_SECTOR;
	int mod = sizeof(ext2_super_block) % BYTES_PER_SECTOR;
	if (mod) {
		cnt++;
	}

	char buf[0x1000];
	ret = readSector(low, high, cnt,(char*)buf );
	__memcpy((char*)&gExt4SuperBlock, buf, sizeof(ext2_super_block));

	gLogBlockSize = (1 << gExt4SuperBlock.s_log_block_size) * 1024 ;

	s_first_data_block = gExt4SuperBlock.s_first_data_block;
	if (gExt4SuperBlock.s_magic != 0xef53) {
		ret = 0;
		__printf(szout, "%s %d ext4 magic error:%x\r\n", gExt4SuperBlock.s_magic);
	}

	__printf(szout, "%s %d ext4 magic:%x,s_first_data_block:%x\r\n", gExt4SuperBlock.s_magic, gExt4SuperBlock.s_log_block_size);
	return ret;

}


int GetGroupDesc() {
	int ret = 0;

	unsigned long long sector = g_ext4_part_offset + gLogBlockSize/ BYTES_PER_SECTOR;
	DWORD low = sector & 0xffffffff;
	DWORD high = sector >> 32;

	int cnt = sizeof(ext2_group_desc) / BYTES_PER_SECTOR;
	int mod = sizeof(ext2_group_desc) % BYTES_PER_SECTOR;
	if (mod) {
		cnt++;
	}
	char buf[0x1000];
	ret = readSector(low, high, cnt, (char*)buf);
	__memcpy((char*)&gExt4GroupDesc, buf, sizeof(ext2_group_desc));

	g_inode_offset = gExt4GroupDesc.bg_inode_table;

	char szout[256];
	__printf(szout, "%s %d ext4 gdt bg_inode_table:%x\r\n", gExt4GroupDesc.bg_inode_table);

	return ret;
	
}



int GetExt4Inode() {
	int ret = 0;

	if (gExt4Inode == 0) {
		int size = gLogBlockSize;
		gExt4Inode = (ext2_inode*)__kMalloc(gLogBlockSize);

	}
	unsigned long long sector = g_ext4_part_offset + gExt4GroupDesc.bg_inode_table* gLogBlockSize/ BYTES_PER_SECTOR;
	DWORD low = sector & 0xffffffff;
	DWORD high = sector >> 32;

	int cnt = sizeof(ext2_inode) / BYTES_PER_SECTOR;
	int mod = sizeof(ext2_inode) % BYTES_PER_SECTOR;
	if (mod) {
		cnt++;
	}

	ret = readSector(low, high, cnt, (char*)gExt4Inode);

	char szout[256];
	__printf(szout, "%s %d ext4 i_mode:%x,i_blocks:%x,i_block:%x\r\n", gExt4Inode->i_mode, gExt4Inode->i_blocks, gExt4Inode->i_block[0]);

	return 0;
}


int GetExt4RootDir() {
	int ret = 0;
	
	ext2_inode* node = gExt4Inode;

	int rootDirSize = 0;

	for (int i = 0; i < gExt4GroupDesc.bg_used_dirs_count; i++) {

		if (node->i_mode && node->i_block && node->i_blocks) {

			unsigned int long sector = 0;

			ext4_extent_header* hdr = (ext4_extent_header*)node->i_block;
			if (hdr->eh_magic == 0xf30a) {
				ext4_extent* ext = (ext4_extent*)((char*)hdr+sizeof(ext4_extent_header));
				sector =(ext->ee_start_hi << 32)+ ext->ee_start_lo;
				sector = sector * 0x1000 / BYTES_PER_SECTOR + g_ext4_part_offset;
			}
			else {
				sector = g_ext4_part_offset+ node->i_block[0] * gLogBlockSize / BYTES_PER_SECTOR;
			}

			DWORD low = sector & 0xffffffff;
			DWORD high = sector >> 32;

			rootDirSize = node->i_blocks * gLogBlockSize;

			int seccnt = node->i_blocks* gLogBlockSize/ BYTES_PER_SECTOR;
			if (gExt4RootDir == 0) {
				gExt4RootDir = (ext4_dir_entry_2*)__kMalloc(seccnt * BYTES_PER_SECTOR);
			}
			
			ret = readSector(low, high, seccnt, (char*)gExt4RootDir);
			break;
		}
		node = (ext2_inode * )((char*)node + gExt4SuperBlock. s_inode_size);
	}

	char szout[1024];
	ext4_dir_entry_2* dir = gExt4RootDir;
	while ((char*)dir < (char*) gExt4RootDir + rootDirSize) {
		if (dir->file_type == 0) {
			break;
		}
		__printf(szout, "file:%s,type:%x,inode:%x\r\n", dir->name, dir->file_type, dir->inode);

		dir = (ext4_dir_entry_2 * )((char*)dir + dir->rec_len);
	}

	return 0;
}