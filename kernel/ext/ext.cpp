

#include "ext.h"

#include "../file.h"
#include "../ata.h"

#include "../Utils.h"
#include "../video.h"
#include "../malloc.h"

ext2_super_block gExt4SuperBlock;

ext2_sb_info gExt4SbInfo;

ext2_group_desc gExt4GroupDesc;

char gExt4Dbr[512];

int gLogBlockSize = 0;

int s_first_data_block = 0;

unsigned long long g_inode_offset = 0;

unsigned long long g_ext4_part_offset = 0;

ext2_inode* gExt4Inode = 0;

ext4_dir_entry_2* gExt4RootDir = 0;


int get1stInode() {
	return 0;
}



int GetNextPath(char path, int a1) {
	return 0;
}

int GetExt4DBR() {
	int ret = 0;
	unsigned long max = 0;
	unsigned long seq = 0;
	for (int i = 0; i < 4; i++) {
		if (gMBR.dpt[i].flag & 0x80) {
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
		__printf(szout, "%s %d ext4 magic error:%x\r\n", gExt4SuperBlock.s_magic);
	}

	return ret;

}


int GetGroupDesc() {
	int ret = 0;

	unsigned long long sector = g_ext4_part_offset + 2 + gLogBlockSize/ BYTES_PER_SECTOR;
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

	return 0;
}


int GetExt4RootDir() {
	int ret = 0;
	if (gExt4RootDir == 0) {
		int size = gLogBlockSize;
		gExt4RootDir = (ext4_dir_entry_2 * )__kMalloc(gLogBlockSize);
	}
	
	for (int i = 0; i < gExt4GroupDesc.bg_used_dirs_count; i++) {
		if (gExt4Inode->i_mode && gExt4Inode->i_block && gExt4Inode->i_blocks) {

			unsigned long long sector = g_ext4_part_offset + gExt4Inode->i_block[0] * gLogBlockSize / BYTES_PER_SECTOR;
			DWORD low = sector & 0xffffffff;
			DWORD high = sector >> 32;

			int cnt = gExt4Inode->i_blocks;

			ret = readSector(low, high, cnt, (char*)gExt4RootDir);
		}

	}

	return 0;
}