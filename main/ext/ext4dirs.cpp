

#include "ext4dirs.h"

#include "Utils.h"
#include "../FileBrowser.h"
#include "file.h"

#include "ata.h"
#include "atapi.h"

#include "VM86.h"
#include "malloc.h"
#include "floppy.h"

#include "../../kernel/ext/ext.h"


unsigned long Ext4FileReader(DWORD nodenum, int filesize, char* lpdata, int readsize) {
	int ret = 0;

	char szout[256];
	int cnt = 0;

	char nodebuf[256];
	ret = GetNextInode(nodenum, nodebuf);
	ext2_inode* node = (ext2_inode*)(nodebuf);
	//0x8000 file
	//0x4000 dir
	//0x2000 blocks
	//0x1000 pipe
	if (node->i_mode & 0x8000 == 0) {
		__printf(szout, "%s %d error\r\n", __FUNCTION__, __LINE__);
		return 0;
	}

	unsigned long long blocks = 0;
	unsigned long long sector = 0;
	ext4_extent_header* hdr = (ext4_extent_header*)node->i_block;
	if (hdr->eh_magic == 0xf30a) {
		ext4_extent* ext = (ext4_extent*)((char*)hdr + sizeof(ext4_extent_header));
		sector = ext->ee_start_hi;
		sector = (sector << 32) + ext->ee_start_lo;
		sector = sector * (gLogBlockSize / BYTES_PER_SECTOR) + g_ext4_part_offset;
		blocks = ext->ee_len;
	}
	else {
		sector = g_ext4_part_offset + node->i_block[0] * (gLogBlockSize / BYTES_PER_SECTOR);
	}

	DWORD low = sector & 0xffffffff;
	DWORD high = sector >> 32;

	int seccnt = blocks* gLogBlockSize/ BYTES_PER_SECTOR;

	unsigned char szshow[0x1000];
	__dump((char*)nodebuf, gExt4SuperBlock.s_inode_size, 0, szshow);
	__printf((char*)szshow, (char*)szshow);

	ret = readSector(low, high, seccnt, (char*)lpdata);

	__printf(szout, "node:%x,low:%x,high:%x,seccnt:%x,ret:%x\r\n", nodenum, low, high, seccnt,ret);

	if (ret > 0) {
		return node->i_size;
	}
	return 0;
}


int ReadExt4Dirs(DWORD nodenum, LPFILEBROWSER files) {
	int ret = 0;
	
	char szout[256];
	int cnt = 0;

	char nodebuf[256];
	ret = GetNextInode(nodenum, nodebuf);
	ext2_inode* node = (ext2_inode*)(nodebuf );
	if (node->i_mode & 0x4000 == 0) {
		__printf(szout, "%s %d error\r\n",__FUNCTION__, __LINE__);
		return 0;
	}

	unsigned long long sector = 0;
	ext4_extent_header* hdr = (ext4_extent_header*)node->i_block;
	if (hdr->eh_magic == 0xf30a) {
		ext4_extent* ext = (ext4_extent*)((char*)hdr + sizeof(ext4_extent_header));
		sector = ext->ee_start_hi;
		sector = (sector << 32) + ext->ee_start_lo;
		sector = sector * (gLogBlockSize / BYTES_PER_SECTOR) + g_ext4_part_offset;
	}
	else {
		sector = g_ext4_part_offset + node->i_block[0] * (gLogBlockSize / BYTES_PER_SECTOR);
	}

	DWORD low = sector & 0xffffffff;
	DWORD high = sector >> 32;
	int rootDirSize = node->i_blocks * BYTES_PER_SECTOR;
	int seccnt = node->i_blocks ;
	char * buf = (char*)__kMalloc(seccnt * BYTES_PER_SECTOR);
	//__printf(szout, "node:%x,low:%x,high:%x,seccnt:%x\r\n", nodenum,  low, high, seccnt);

	unsigned char szshow[0x1000];
	__dump((char*)nodebuf, gExt4SuperBlock.s_inode_size, 0, szshow);
	//__printf((char*)szshow, (char*)szshow);
			
	ret = readSector(low, high, seccnt, (char*)buf);
	if (ret > 0) {
		ext4_dir_entry_2* dir = (ext4_dir_entry_2*)buf;
		while ((char*)dir < (char*)buf + rootDirSize) {
			if (dir->file_type == 0 || dir->rec_len == 0 || dir->inode == 0 || dir->name_len == 0) {
				break;
			}
			if (dir->name_len < 256 && dir->name_len > 0) {
				__memcpy(files->pathname, dir->name, dir->name_len);
				files->pathname[dir->name_len] = 0;

				if( dir->file_type & 2)
					files->attrib = FILE_ATTRIBUTE_DIRECTORY ;
				if (dir->file_type & 1)
					files->attrib =  FILE_ATTRIBUTE_ARCHIVE;

				files->secno = dir->inode;
				files->filesize = node->i_size;

				files++;

				cnt++;
			}
			//__printf(szout, "file:%s,type:%x,inode:%x\r\n", dir->name, dir->file_type, dir->inode);

			dir = (ext4_dir_entry_2*)((char*)dir + dir->rec_len);
		}
	}
	else {
		__printf(szout, "%s %d error,inode:%x,s_inode_size:%x,g_ext4_part_offset:%i64x,gLogBlockSize:%x\r\n",
			__FUNCTION__,__LINE__, nodenum, gExt4SuperBlock.s_inode_size, g_ext4_part_offset, gLogBlockSize);

		unsigned char szshow[0x1000];
		__dump((char*)buf, gExt4SuperBlock.s_inode_size, 0, szshow);
		__printf((char*)szshow, (char*)szshow);
	}
	__kFree((DWORD)buf);

	return cnt;
}


int BrowseExt4File(LPFILEBROWSER files) {
	char szout[256];

	int cnt = 0;

	//InitExt4();

	ext4_dir_entry_2* dir = gExt4RootDir;
	while (1) {
		if (dir->file_type == 0 || dir->rec_len == 0 ||dir->inode == 0 || dir->name_len == 0) {
			break;
		}

		__memcpy(files->pathname, dir->name, dir->name_len);
		files->pathname[dir->name_len] = 0;

		if (dir->file_type & 2)
			files->attrib = FILE_ATTRIBUTE_DIRECTORY;
		if (dir->file_type & 1)
			files->attrib = FILE_ATTRIBUTE_ARCHIVE;

		files->secno = dir->inode;
		files->filesize = 0;

		files++;

		cnt++;
		//__printf(szout, "file:%s,type:%x,inode:%x\r\n", dir->name, dir->file_type, dir->inode);

		dir = (ext4_dir_entry_2*)((char*)dir + dir->rec_len);
	}
	return cnt;
}