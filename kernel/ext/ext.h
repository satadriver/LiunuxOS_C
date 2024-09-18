#pragma once

#include "../def.h"


#define __le32 unsigned int
#define __le16 unsigned short
#define __u8 unsigned char
#define __u16 unsigned short


#define __u32 unsigned char
#define u8 unsigned char
#define u32 unsigned int

#define uid_t unsigned int
#define gid_t unsigned int

#define spinlock_t unsigned int


#pragma pack(1)



typedef struct  _rb_node
{
	unsigned long  __rb_parent_color; //下面详细解析这个变量
	struct _rb_node *rb_right;//右孩子指针域
	struct _rb_node *rb_left;//左孩子指针域
} rb_node;


typedef struct  
{
	struct rb_node *rb_node;
}rb_root;


typedef struct
{
	int i;
}percpu_counter;

typedef struct {
	unsigned int i;
}blockgroup_lock;

typedef struct {
	unsigned int i;
}ext2_reserve_window_node;





#define        EXT2_NDIR_BLOCKS					12									//直接块数目

#define        EXT2_IND_BLOCK					EXT2_NDIR_BLOCKS					//简单间接12

#define        EXT2_DIND_BLOCK					(EXT2_IND_BLOCK+ 1)					//二级间接13

#define        EXT2_TIND_BLOCK					(EXT2_DIND_BLOCK+ 1)				//三级间接14

#define        EXT2_N_BLOCKS					(EXT2_TIND_BLOCK+ 1)				//ext2_inode中i_blocks数组的大小15



struct ext2_super_block {
	__le32  s_inodes_count;			/* Inodes count */
	__le32  s_blocks_count;			/* Blocks count */
	__le32  s_r_blocks_count;		/* Reserved blocks count */
	__le32  s_free_blocks_count;    /* Free blocks count */
	__le32  s_free_inodes_count;    /* Free inodes count */
	__le32  s_first_data_block;		/* First Data Block */
	__le32  s_log_block_size;		/* Block size */
	__le32  s_log_frag_size;		/* Fragment size */
	__le32  s_blocks_per_group;		/* # Blocks per group */
	__le32  s_frags_per_group;		/* # Fragments per group */
	__le32  s_inodes_per_group;		/* # Inodes per group */
	__le32  s_mtime;				/* Mount time */
	__le32  s_wtime;				/* Write time */
	__le16  s_mnt_count;			/* Mount count */
	__le16  s_max_mnt_count;		/* Maximal mount count */
	__le16  s_magic;				/* Magic signature */
	__le16  s_state;				/* File system state */
	__le16  s_errors;				/* Behaviour when detecting errors */
	__le16  s_minor_rev_level;		/* minor revision level */
	__le32  s_lastcheck;			/* time of last check */
	__le32  s_checkinterval;		/* max. time between checks */
	__le32  s_creator_os;			/* OS */
	__le32  s_rev_level;			/* Revision level */
	__le16  s_def_resuid;			/* Default uid for reserved blocks */
	__le16  s_def_resgid;			/* Default gid for reserved blocks */
									/*
								* These fields are for EXT2_DYNAMIC_REV superblocks only.
								*
								* Note: the difference between the compatible feature set and
								* the incompatible feature set is that if there is a bit set
								* in the incompatible feature set that the kernel doesn't
								* know about, it should refuse to mount the filesystem.
								*
								* e2fsck's requirements are more strict; if it doesn't know
								* about a feature in either the compatible or incompatible
								* feature set, it must abort and not try to meddle with
								* things it doesn't understand...
								*/
	__le32  s_first_ino;        /* First non-reserved inode */
	__le16   s_inode_size;      /* size of inode structure */
	__le16  s_block_group_nr;   /* block group # of this superblock */
	__le32  s_feature_compat;   /* compatible feature set */
	__le32  s_feature_incompat;			/* incompatible feature set */
	__le32  s_feature_ro_compat;		/* readonly-compatible feature set */
	__u8    s_uuid[16];					/* 128-bit uuid for volume */
	char    s_volume_name[16];			/* volume name */
	char    s_last_mounted[64];			/* directory where last mounted */
	__le32  s_algorithm_usage_bitmap;	/* For compression */
										/*
										* Performance hints.  Directory preallocation should only
										* happen if the EXT2_COMPAT_PREALLOC flag is on.
										*/
	__u8    s_prealloc_blocks;			/* Nr of blocks to try to preallocate*/
	__u8    s_prealloc_dir_blocks;		/* Nr to preallocate for dirs */
	__u16   s_padding1;
	/*
	* Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
	*/
	__u8    s_journal_uuid[16]; /* uuid of journal superblock */
	__u32   s_journal_inum;     /* inode number of journal file */
	__u32   s_journal_dev;      /* device number of journal file */
	__u32   s_last_orphan;      /* start of list of inodes to delete */
	__u32   s_hash_seed[4];     /* HTREE hash seed */
	__u8    s_def_hash_version; /* Default hash version to use */
	__u8    s_reserved_char_pad;
	__u16   s_reserved_word_pad;
	__le32  s_default_mount_opts;
	__le32  s_first_meta_bg;    /* First metablock block group */
	__u32   s_reserved[190];    /* Padding to the end of the block */
};

struct ext2_sb_info {
	unsigned long s_frag_size;  /* Size of a fragment in bytes */
	unsigned long s_frags_per_block;/* Number of fragments per block */
	unsigned long s_inodes_per_block;/* Number of inodes per block */
	unsigned long s_frags_per_group;/* Number of fragments in a group */
	unsigned long s_blocks_per_group;/* Number of blocks in a group */
	unsigned long s_inodes_per_group;/* Number of inodes in a group */
	unsigned long s_itb_per_group;  /* Number of inode table blocks per group */
	unsigned long s_gdb_count;  /* Number of group descriptor blocks */
	unsigned long s_desc_per_block; /* Number of group descriptors per block */
	unsigned long s_groups_count;   /* Number of groups in the fs */
	unsigned long s_overhead_last;  /* Last calculated overhead */
	unsigned long s_blocks_last;    /* Last seen block count */
	struct buffer_head * s_sbh; /* Buffer containing the super block */
	struct ext2_super_block * s_es; /* Pointer to the super block in the buffer */
	struct buffer_head ** s_group_desc;
	unsigned long  s_mount_opt;
	unsigned long s_sb_block;
	uid_t s_resuid;
	gid_t s_resgid;
	unsigned short s_mount_state;
	unsigned short s_pad;
	int s_addr_per_block_bits;
	int s_desc_per_block_bits;
	int s_inode_size;
	int s_first_ino;
	spinlock_t s_next_gen_lock;
	u32 s_next_generation;
	unsigned long s_dir_count;
	u8 *s_debts;
	percpu_counter s_freeblocks_counter;
	percpu_counter s_freeinodes_counter;
	percpu_counter s_dirs_counter;
	blockgroup_lock *s_blockgroup_lock;
	/* root of the per fs reservation window tree */
	spinlock_t s_rsv_window_lock;
	rb_root s_rsv_window_root;
	ext2_reserve_window_node s_rsv_window_head;
};

struct ext2_inode {
	__le16    i_mode;        /* File mode */
	__le16    i_uid;        /* Low 16 bits of Owner Uid */
	__le32    i_size;        /* Size in bytes */
	__le32    i_atime;    /* Access time */
	__le32    i_ctime;    /* Creation time */
	__le32    i_mtime;    /* Modification time */
	__le32    i_dtime;    /* Deletion Time */
	__le16    i_gid;        /* Low 16 bits of Group Id */
	__le16    i_links_count;    /* Links count */
	__le32    i_blocks;    /* Blocks count */
	__le32    i_flags;    /* File flags */
	union {
		struct {
			__le32  l_i_reserved1;
		} linux1;
		struct {
			__le32  h_i_translator;
		} hurd1;
		struct {
			__le32  m_i_reserved1;
		} masix1;
	} osd1;                /* OS dependent 1 */
	__le32    i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
	__le32    i_generation;    /* File version (for NFS) */
	__le32    i_file_acl;    /* File ACL */
	__le32    i_dir_acl;    /* Directory ACL */
	__le32    i_faddr;    /* Fragment address */
	union {
		struct {
			__u8    l_i_frag;    /* Fragment number */
			__u8    l_i_fsize;    /* Fragment size */
			__u16    i_pad1;
			__le16    l_i_uid_high;    /* these 2 fields    */
			__le16    l_i_gid_high;    /* were reserved2[0] */
			__u32    l_i_reserved2;
		} linux2;
		struct {
			__u8    h_i_frag;    /* Fragment number */
			__u8    h_i_fsize;    /* Fragment size */
			__le16    h_i_mode_high;
			__le16    h_i_uid_high;
			__le16    h_i_gid_high;
			__le32    h_i_author;
		} hurd2;
		struct {
			__u8    m_i_frag;    /* Fragment number */
			__u8    m_i_fsize;    /* Fragment size */
			__u16    m_pad1;
			__u32    m_i_reserved2[2];
		} masix2;
	} osd2;                /* OS dependent 2 */
};

struct ext2_group_desc
{
	__le32	bg_block_bitmap;		/* Blocks bitmap block */
	__le32	bg_inode_bitmap;		/* Inodes bitmap block */
	__le32	bg_inode_table;		/* Inodes table block */
	__le16	bg_free_blocks_count;	/* Free blocks count */
	__le16	bg_free_inodes_count;	/* Free inodes count */
	__le16	bg_used_dirs_count;	/* Directories count */
	__le16	bg_pad;
	__le32	bg_reserved[3];
};


#pragma pack()