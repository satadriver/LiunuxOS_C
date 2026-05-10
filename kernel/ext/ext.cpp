

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

ext2_group_desc gExt4GroupDesc;

ext2_inode* gExt4Inode = 0;

ext4_dir_entry_2* gExt4RootDir = 0;



int WriteExt4File(const char* filename, char* buf, int size, int writemode) {
	return 0;
}



int ReadExt4File(const char* filename, char** buf) {
	return 0;
}


int InitExt4() {
	char szout[256];

	GetExt4DBR();
	GetSuperBlock();
	GetGroupDesc();
	GetExt4Inode();
	GetExt4RootDir();

	__printf(szout, "%s %d ext4 volume:%s,uuid:%x\r\n", __FUNCTION__, __LINE__, gExt4SuperBlock.s_volume_name,gExt4SuperBlock.s_uuid);

	readFile = ReadExt4File;
	writeFile = WriteExt4File;

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

	if (gExt4SuperBlock.s_magic != 0xef53) {
		ret = 0;
		__printf(szout, "%s %d ext4 magic error:%x\r\n", __FUNCTION__, __LINE__, gExt4SuperBlock.s_magic);
	}

	__printf(szout, "%s %d ext4 magic:%x,s_first_data_block:%x\r\n", __FUNCTION__, __LINE__, gExt4SuperBlock.s_magic, gExt4SuperBlock.s_log_block_size);
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

	//g_inode_offset = gExt4GroupDesc.bg_inode_table;

	char szout[256];
	__printf(szout, "%s %d ext4 gdt bg_inode_table:%x\r\n",__FUNCTION__,__LINE__, gExt4GroupDesc.bg_inode_table);

	return ret;
	
}



int GetExt4Inode() {
	int ret = 0;

	if (gExt4Inode == 0) {
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

	ext2_inode* node = (ext2_inode*)((char*)gExt4Inode + gExt4SuperBlock.s_inode_size);
	char szout[256];
	__printf(szout, "%s %d ext4 i_mode:%x,i_blocks:%x,i_block:%x\r\n",
		__FUNCTION__, __LINE__, node->i_mode, node->i_blocks, node->i_block[0]);

	return 0;
}


int GetExt4RootDir() {
	int ret = 0;
	char szout[256];

	ext2_inode* node = (ext2_inode*)((char*)gExt4Inode + gExt4SuperBlock.s_inode_size);

	if (node->i_mode  && node->i_blocks) {

		unsigned long long sector = 0;
		DWORD seccnt = 0;

		int flags = node->i_flags;
		if (flags & 0x80000) {
			ext4_extent_header* hdr = (ext4_extent_header*)node->i_block;
			if (hdr->eh_magic == 0xf30a) {
				ext4_extent* ext = (ext4_extent*)((char*)hdr + sizeof(ext4_extent_header));
				sector = ext->ee_start_hi;
				sector = (sector << 32) + ext->ee_start_lo;
				sector = sector * gLogBlockSize / BYTES_PER_SECTOR + g_ext4_part_offset;
				seccnt = ext->ee_len* gLogBlockSize/ BYTES_PER_SECTOR;
			}
			else {
				__printf(szout, "%s %d error flags:%x\r\n", __FUNCTION__, __LINE__, flags);
				return 0;
			}
		}
		else {
			sector = g_ext4_part_offset+ node->i_block[0] * gLogBlockSize / BYTES_PER_SECTOR;
			seccnt = node->i_blocks * gLogBlockSize / BYTES_PER_SECTOR;
		}

		if (gExt4RootDir == 0) {
			gExt4RootDir = (ext4_dir_entry_2*)__kMalloc(seccnt * BYTES_PER_SECTOR);
		}

		DWORD low = sector & 0xffffffff;
		DWORD high = sector >> 32;
		ret = readSector(low, high, seccnt, (char*)gExt4RootDir);
	}

	ext4_dir_entry_2* dir = gExt4RootDir;
	int rootDirSize = node->i_blocks * gLogBlockSize;
	while ((char*)dir < (char*) gExt4RootDir + rootDirSize) {
		if (dir->file_type == 0 || dir->inode == 0 || dir->name_len == 0 || dir->rec_len == 0) {
			break;
		}
		//__printf(szout, "file:%s,type:%x,inode:%x\r\n", dir->name, dir->file_type, dir->inode);

		dir = (ext4_dir_entry_2 * )((char*)dir + dir->rec_len);
	}

	return 0;
}


DWORD GetGDSize() {
	DWORD flag = gExt4SuperBlock.s_feature_incompat;
	if (flag == 0) {
		return 32;
	}
	else if (flag& 0x80) {
		return 64;
	}
	return 64;
}


DWORD GetNextInode(DWORD inode,char * buf) {
	int ret = 0;
	char szout[256];

	DWORD gd_page = (inode - 1) / gExt4SuperBlock.s_inodes_per_group ;
	unsigned long long inode_mod = (inode - 1) % gExt4SuperBlock.s_inodes_per_group;

	int gds = GetGDSize();	// sizeof(ext2_group_desc)

	DWORD gd_sector =	g_ext4_part_offset + (gLogBlockSize + gd_page * gds) / BYTES_PER_SECTOR;
	DWORD gd_off = ( gd_page * gds) % BYTES_PER_SECTOR;

	//__printf(szout, "s_inodes_per_group:%x,read gd_sector:%x,gd_off:%x,inode_mod:%x\r\n",gExt4SuperBlock.s_inodes_per_group, gd_sector, gd_off, inode_mod);

	char gdbuf[1024];
	ret = readSector(gd_sector, 0, 2, (char*)gdbuf);
	ext2_group_desc* gd = (ext2_group_desc*)(gdbuf + gd_off);
	unsigned long long node_table = gd->bg_inode_table;

	unsigned long long inode_sector = (node_table * gLogBlockSize + inode_mod * gExt4SuperBlock.s_inode_size) / BYTES_PER_SECTOR;
	inode_sector += g_ext4_part_offset;
	DWORD inode_off =  (node_table * gLogBlockSize + inode_mod * gExt4SuperBlock.s_inode_size) % BYTES_PER_SECTOR;
	
	//__printf(szout, "s_inodes_per_group:%x,read inode_sector:%x,inode_off:%x,node_table:%x\r\n", gExt4SuperBlock.s_inodes_per_group, inode_sector, inode_off, node_table);

	char nodebuf[1024];
	DWORD low = inode_sector & 0xffffffff;
	DWORD hign = inode_sector >> 32;
	ret = readSector(low, hign, 2, (char*)nodebuf);

	__memcpy(buf, (char*)nodebuf + inode_off, gExt4SuperBlock.s_inode_size);

	return 0;
}