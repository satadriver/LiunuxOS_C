#pragma once

#include "../def.h"


// 定义基础类型 (如果尚未定义)
#ifdef _MSC_VER
typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;
#define __be16 u16
#define __be32 u32
#define __be64 u64
#define __le32 u32
#endif

typedef __be64 xfs_timestamp_t;

#define __le32 unsigned int

#define __be64 unsigned long long
#define __be32 unsigned int
#define __be16 unsigned short
#define __be8 unsigned char

#define __u8 unsigned char
#define __u16 unsigned short
#define __u32 unsigned int

#define u16 unsigned short
#define u8 unsigned char
#define u32 unsigned int

#define uuid_t unsigned int

#define XFSLABEL_MAX 12
#define XFS_AGI_UNLINKED_BUCKETS 64
#define XFS_DIR2_DATA_FD_COUNT 2

#define XFS_DIR3_BLOCK_MAGIC    0x58444233      /* XDB3: single block dirs */
#define XFS_DIR3_DATA_MAGIC     0x58444433      /* XDD3: multiblock dirs */
#define XFS_DIR3_FREE_MAGIC     0x58444633      /* XDF3: free index blocks */


//xfs sb 位于分区第一个扇区
/*
 * Superblock - on disk version.  Must match the in core version above.
 * Must be padded to 64 bit alignment.
 */
typedef struct xfs_dsb {
    __be32          sb_magicnum;    /* magic number == XFS_SB_MAGIC */
    __be32          sb_blocksize;   /* logical block size, bytes */
    __be64          sb_dblocks;     /* number of data blocks */
    __be64          sb_rblocks;     /* number of realtime blocks */
    __be64          sb_rextents;    /* number of realtime extents */
    uuid_t          sb_uuid;        /* user-visible file system unique id */
    __be64          sb_logstart;    /* starting block of log if internal */
    __be64          sb_rootino;     /* root inode number */
    __be64          sb_rbmino;      /* bitmap inode for realtime extents */
    __be64          sb_rsumino;     /* summary inode for rt bitmap */
    __be32          sb_rextsize;    /* realtime extent size, blocks */
    __be32          sb_agblocks;    /* size of an allocation group */
    __be32          sb_agcount;     /* number of allocation groups */
    __be32          sb_rbmblocks;   /* number of rt bitmap blocks */
    __be32          sb_logblocks;   /* number of log blocks */
    __be16          sb_versionnum;  /* header version == XFS_SB_VERSION */
    __be16          sb_sectsize;    /* volume sector size, bytes */
    __be16          sb_inodesize;   /* inode size, bytes */
    __be16          sb_inopblock;   /* inodes per block */
    char            sb_fname[XFSLABEL_MAX]; /* file system name */
    __u8            sb_blocklog;    /* log2 of sb_blocksize */
    __u8            sb_sectlog;     /* log2 of sb_sectsize */
    __u8            sb_inodelog;    /* log2 of sb_inodesize */
    __u8            sb_inopblog;    /* log2 of sb_inopblock */
    __u8            sb_agblklog;    /* log2 of sb_agblocks (rounded up) */
    __u8            sb_rextslog;    /* log2 of sb_rextents */
    __u8            sb_inprogress;  /* mkfs is in progress, don't mount */
    __u8            sb_imax_pct;    /* max % of fs for inode space */
                                    /* statistics */
    /*
     * These fields must remain contiguous.  If you really
     * want to change their layout, make sure you fix the
     * code in xfs_trans_apply_sb_deltas().
     */
    __be64          sb_icount;      /* allocated inodes */
    __be64          sb_ifree;       /* free inodes */
    __be64          sb_fdblocks;    /* free data blocks */
    __be64          sb_frextents;   /* free realtime extents */
    /*
     * End contiguous fields.
     */
    __be64          sb_uquotino;    /* user quota inode */
    __be64          sb_gquotino;    /* group quota inode */
    __be16          sb_qflags;      /* quota flags */
    __u8            sb_flags;       /* misc. flags */
    __u8            sb_shared_vn;   /* shared version number */
    __be32          sb_inoalignmt;  /* inode chunk alignment, fsblocks */
    __be32          sb_unit;        /* stripe or raid unit */
    __be32          sb_width;       /* stripe or raid width */
    __u8            sb_dirblklog;   /* log2 of dir block size (fsbs) */
    __u8            sb_logsectlog;  /* log2 of the log sector size */
    __be16          sb_logsectsize; /* sector size for the log, bytes */
    __be32          sb_logsunit;    /* stripe unit size for the log */
    __be32          sb_features2;   /* additional feature bits */
    /*
     * bad features2 field as a result of failing to pad the sb
     * structure to 64 bits. Some machines will be using this field
     * for features2 bits. Easiest just to mark it bad and not use
     * it for anything else.
     */
    __be32          sb_bad_features2;

    /* version 5 superblock fields start here */

    /* feature masks */
    __be32          sb_features_compat;
    __be32          sb_features_ro_compat;
    __be32          sb_features_incompat;
    __be32          sb_features_log_incompat;

    __le32          sb_crc;         /* superblock crc */
    __be32          sb_spino_align; /* sparse inode chunk alignment */

    __be64          sb_pquotino;    /* project quota inode */
    __be64          sb_lsn;         /* last write sequence */
    uuid_t          sb_meta_uuid;   /* metadata file system unique id */

    /* must be padded to 64 bit alignment */
} xfs_dsb_t;


//位于分区第2个扇区，扇区号从0开始
typedef struct xfs_agi {
    /*
     * Common allocation group header information
     */
    __be32          agi_magicnum;   /* magic number == XFS_AGI_MAGIC */
    __be32          agi_versionnum; /* header version == XFS_AGI_VERSION */
    __be32          agi_seqno;      /* sequence # starting from 0 */
    __be32          agi_length;     /* size in blocks of a.g. */
    /*
     * Inode information
     * Inodes are mapped by interpreting the inode number, so no
     * mapping data is needed here.
     */
    __be32          agi_count;      /* count of allocated inodes */
    __be32          agi_root;       /* root of inode btree */
    __be32          agi_level;      /* levels in inode btree */
    __be32          agi_freecount;  /* number of free inodes */

    __be32          agi_newino;     /* new inode just allocated */
    __be32          agi_dirino;     /* last directory inode chunk */
    /*
     * Hash table of inodes which have been unlinked but are
     * still being referenced.
     */
    __be32          agi_unlinked[XFS_AGI_UNLINKED_BUCKETS];
    /*
     * This marks the end of logging region 1 and start of logging region 2.
     */
    uuid_t          agi_uuid;       /* uuid of filesystem */
    __be32          agi_crc;        /* crc of agi sector */
    __be32          agi_pad32;
    __be64          agi_lsn;        /* last write sequence */

    __be32          agi_free_root; /* root of the free inode btree */
    __be32          agi_free_level;/* levels in free inode btree */

    /* structure must be padded to 64 bit alignment */
} xfs_agi_t;






// 短格式头部定义 (size = 48 bytes)
struct xfs_btree_block_shdr {
    __be32      bb_leftsib;     /* 左兄弟块指针 */
    __be32      bb_rightsib;    /* 右兄弟块指针 */
    __be64      bb_blkno;       /* 当前块的文件系统块号 (FSB) */
    __be64      bb_lsn;         /* 日志序列号 */
    uuid_t      bb_uuid;        /* 文件系统 UUID */
    __be32      bb_owner;       /* 所属分配组 ID (AG Number) */
    __le32      bb_crc;         /* CRC32c 校验和 */
};

struct xfs_btree_block_lhdr {
    __be64  bb_leftsib;   /* 左兄弟块号 */
    __be64  bb_rightsib;  /* 右兄弟块号 */
    __be64  bb_blkno;     /* 当前块块号 */
    __be64  bb_lsn;       /* 日志序列号 */
    uuid_t  bb_uuid;      /* UUID */
    __be64  bb_owner;     /* 所属对象标识 */
    __le32  bb_crc;       /* CRC校验值 */
    __be32  bb_pad;       /* 填充字段，保证64字节对齐 */
};

// 完整的 B+树块定义
struct xfs_btree_block {
    __be32      bb_magic;       /* 魔数，用于区分短/长格式及树类型 */
    __be16      bb_level;       /* 节点层级，0 表示叶节点 */
    __be16      bb_numrecs;     /* 当前块包含的记录/键值对数量 */
    union {
        struct xfs_btree_block_shdr s; /* 短格式 (用于 AG 内部的 B+树) */
        struct xfs_btree_block_lhdr l; /* 长格式 (用于文件数据的 B+树) */
    } bb_u;
};




/*
 * The on-disk inode record structure has two formats. The original "full"
 * format uses a 4-byte freecount. The "sparse" format uses a 1-byte freecount
 * and replaces the 3 high-order freecount bytes wth the holemask and inode
 * count.
 *
 * The holemask of the sparse record format allows an inode chunk to have holes
 * that refer to blocks not owned by the inode record. This facilitates inode
 * allocation in the event of severe free space fragmentation.
 */
typedef struct xfs_inobt_rec {
    __be32          ir_startino;    /* starting inode number */
    union {
        struct {
            __be32  ir_freecount;   /* count of free inodes */
        } f;
        struct {
            __be16  ir_holemask;/* hole mask for sparse chunks */
            __u8    ir_count;       /* total inode count */
            __u8    ir_freecount;   /* count of free inodes */
        } sp;
    } ir_u;
    __be64          ir_free;        /* free inode mask */
} xfs_inobt_rec_t;



#define __uint64_t unsigned long long

#define __uint32_t unsigned int
#define __int32_t int

typedef __uint32_t	xfs_agino_t;	/* inode # within allocation grp */

typedef __uint64_t xfs_inofree_t;

// In-Core 格式（在内存中使用）
typedef struct xfs_inobt_rec_incore {
    xfs_agino_t ir_startino;    // 起始 inode 编号
    __int32_t   ir_freecount;   // 空闲 inode 数量
    xfs_inofree_t ir_free;      // 空闲 inode 的位图
} xfs_inobt_rec_incore_t;




typedef struct xfs_dir2_data_free {
    __be16                  offset;         /* start of freespace */
    __be16                  length;         /* length of freespace */
} xfs_dir2_data_free_t;

/*
 * define a structure for all the verification fields we are adding to the
 * directory block structures. This will be used in several structures.
 * The magic number must be the first entry to align with all the dir2
 * structures so we determine how to decode them just by the magic number.
 */
struct xfs_dir3_blk_hdr {
    __be32                  magic;  /* magic number */
    __be32                  crc;    /* CRC of block */
    __be64                  blkno;  /* first block of the buffer */
    __be64                  lsn;    /* sequence number of last write */
    uuid_t                  uuid;   /* filesystem we belong to */
    __be64                  owner;  /* inode that owns the block */
};

struct xfs_dir3_data_hdr {
    struct xfs_dir3_blk_hdr hdr;
    xfs_dir2_data_free_t    best_free[XFS_DIR2_DATA_FD_COUNT];
    __be32                  pad;    /* 64 bit alignment */
};




/*
 * Active entry in a data block.
 *
 * Aligned to 8 bytes.  After the variable length name field there is a
 * 2 byte tag field, which can be accessed using xfs_dir3_data_entry_tag_p.
 *
 * For dir3 structures, there is file type field between the name and the tag.
 * This can only be manipulated by helper functions. It is packed hard against
 * the end of the name so any padding for rounding is between the file type and
 * the tag.
 */
typedef struct xfs_dir2_data_entry {
    __be64                  inumber;        /* inode number */
    __u8                    namelen;        /* name length */
    __u8                    name[];         /* name bytes, no null */
 /* __u8                    filetype; */    /* type of inode we point to */
 /* __be16                  tag; */         /* starting offset of us */
} xfs_dir2_data_entry_t;

#define XFS_DIR2_DATA_FREE_TAG  0xffff

/*
 * Unused entry in a data block.
 *
 * Aligned to 8 bytes.  Tag appears as the last 2 bytes and must be accessed
 * using xfs_dir2_data_unused_tag_p.
 */
typedef struct xfs_dir2_data_unused {
    __be16                  freetag;        /* XFS_DIR2_DATA_FREE_TAG */
    __be16                  length;         /* total free length */
                                            /* variable offset */
    __be16                  tag;            /* starting offset of us */
} xfs_dir2_data_unused_t;

#define XFS_DINODE_MAGIC                0x494e  /* 'IN' */
typedef struct xfs_dinode {
    __be16          di_magic;       /* inode magic # = XFS_DINODE_MAGIC */
    __be16          di_mode;        /* mode and type of file */
    __u8            di_version;     /* inode version */
    __u8            di_format;      /* format of di_c data */
    __be16          di_onlink;      /* old number of links to file */
    __be32          di_uid;         /* owner's user id */
    __be32          di_gid;         /* owner's group id */
    __be32          di_nlink;       /* number of links to file */
    __be16          di_projid_lo;   /* lower part of owner's project id */
    __be16          di_projid_hi;   /* higher part owner's project id */
    __u8            di_pad[6];      /* unused, zeroed space */
    __be16          di_flushiter;   /* incremented on flush */
    xfs_timestamp_t di_atime;       /* time last accessed */
    xfs_timestamp_t di_mtime;       /* time last modified */
    xfs_timestamp_t di_ctime;       /* time created/inode modified */
    __be64          di_size;        /* number of bytes in file */
    __be64          di_nblocks;     /* # of direct & btree blocks used */
    __be32          di_extsize;     /* basic/minimum extent size for file */
    __be32          di_nextents;    /* number of extents in data fork */
    __be16          di_anextents;   /* number of extents in attribute fork*/
    __u8            di_forkoff;     /* attr fork offs, <<3 for 64b align */
    __u8            di_aformat;     /* format of attr fork's data */
    __be32          di_dmevmask;    /* DMIG event mask */
    __be16          di_dmstate;     /* DMIG state info */
    __be16          di_flags;       /* random flags, XFS_DIFLAG_... */
    __be32          di_gen;         /* generation number */

    /* di_next_unlinked is the only non-core field in the old dinode */
    __be32          di_next_unlinked;/* agi unlinked list ptr */

    /* start of the extended dinode, writable fields */
    __le32          di_crc;         /* CRC of the inode */
    __be64          di_changecount; /* number of attribute changes */
    __be64          di_lsn;         /* flush sequence */
    __be64          di_flags2;      /* more random flags */
    __be32          di_cowextsize;  /* basic cow extent size for file */
    __u8            di_pad2[12];    /* more padding for future expansion */

    /* fields only written to during inode creation */
    xfs_timestamp_t di_crtime;      /* time created */
    __be64          di_ino;         /* inode number */
    uuid_t          di_uuid;        /* UUID of the filesystem */

    /* structure must be padded to 64 bit alignment */
} xfs_dinode_t;


/*
 * Directory layout when stored internal to an inode.
 *
 * Small directories are packed as tightly as possible so as to fit into the
 * literal area of the inode.  These "shortform" directories consist of a
 * single xfs_dir2_sf_hdr header followed by zero or more xfs_dir2_sf_entry
 * structures.  Due the different inode number storage size and the variable
 * length name field in the xfs_dir2_sf_entry all these structure are
 * variable length, and the accessors in this file should be used to iterate
 * over them.
 */
typedef struct xfs_dir2_sf_hdr {
    uint8_t                 count;          /* count of entries */
    uint8_t                 i8count;        /* count of 8-byte inode #s */
    uint8_t                 parent[8];      /* parent dir inode number */
}  xfs_dir2_sf_hdr_t;

typedef struct xfs_dir2_sf_entry {
    __u8                    namelen;        /* actual name length */
    __u8                    offset[2];      /* saved offset */
    __u8                    name[];         /* name, variable size */
    /*
     * A single byte containing the file type field follows the inode
     * number for version 3 directoinory entries.
     *
     * A 64-bit or 32-bit inode number follows here, at a variable offset
     * after the name.
     */
} xfs_dir2_sf_entry_t;