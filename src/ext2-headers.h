#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t i16;
typedef int32_t i32;

#define BLOCK_SIZE 1024
#define BLOCK_OFFSET(i) (i * BLOCK_SIZE)
#define NUM_BLOCKS 1024
#define NUM_INODES 128

/* http://www.nongnu.org/ext2-doc/ext2.html */
/* http://www.science.smith.edu/~nhowe/262/oldlabs/ext2.html */

#define FIRST_DATA_BLOCK 1
#define BLOCKS_PER_GROUP 8192
#define FRAGMENTS_PER_GROUP 8192
#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_DEF_RESUID 0
#define EXT2_DEF_RESGID 0

#define EXT2_VALID_FS 1
#define EXT2_ERROR_FS 2

#define EXT2_ERRORS_CONTINUE 1
#define EXT2_ERRORS_RO 2
#define EXT2_ERRORS_PANIC 3

#define MAX_INTERVAL 1

#define EXT2_OS_LINUX 0
#define EXT2_OS_HURD 1
#define EXT2_OS_MASIX 2
#define EXT2_OS_FREEBSD 3
#define EXT2_OS_LITES 4

#define EXT2_BAD_INO 1
#define EXT2_ROOT_INO 2
#define EXT2_GOOD_OLD_FIRST_INO 11

#define EXT2_GOOD_OLD_REV 0

#define EXT2_S_IFSOCK 0xC000
#define EXT2_S_IFLNK 0xA000
#define EXT2_S_IFREG 0x8000
#define EXT2_S_IFBLK 0x6000
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFCHR 0x2000
#define EXT2_S_IFIFO 0x1000
#define EXT2_S_ISUID 0x0800
#define EXT2_S_ISGID 0x0400
#define EXT2_S_ISVTX 0x0200
#define EXT2_S_IRUSR 0x0100
#define EXT2_S_IWUSR 0x0080
#define EXT2_S_IXUSR 0x0040
#define EXT2_S_IRGRP 0x0020
#define EXT2_S_IWGRP 0x0010
#define EXT2_S_IXGRP 0x0008
#define EXT2_S_IROTH 0x0004
#define EXT2_S_IWOTH 0x0002
#define EXT2_S_IXOTH 0x0001

#define LOST_AND_FOUND_INO 11
#define HELLO_WORLD_INO 12
#define HELLO_INO 13
#define LAST_INO HELLO_INO

#define SUPERBLOCK_BLOCKNO 1
#define BLOCK_GROUP_DESCRIPTOR_BLOCKNO 2
#define BLOCK_BITMAP_BLOCKNO 3
#define INODE_BITMAP_BLOCKNO 4
#define INODE_TABLE_BLOCKNO 5
#define ROOT_DIR_BLOCKNO 21
#define LOST_AND_FOUND_DIR_BLOCKNO 22
#define HELLO_WORLD_FILE_BLOCKNO 23
#define LAST_BLOCK HELLO_WORLD_FILE_BLOCKNO

#define NUM_FREE_BLOCKS (NUM_BLOCKS - LAST_BLOCK - 1)
#define NUM_FREE_INODES (NUM_INODES - LAST_INO)

struct ext2_superblock
{
	u32 s_inodes_count;
	u32 s_blocks_count;
	u32 s_r_blocks_count;
	u32 s_free_blocks_count;
	u32 s_free_inodes_count;
	u32 s_first_data_block;
	u32 s_log_block_size;
	i32 s_log_frag_size;
	u32 s_blocks_per_group;
	u32 s_frags_per_group;
	u32 s_inodes_per_group;
	u32 s_mtime;
	u32 s_wtime;
	u16 s_mnt_count;
	i16 s_max_mnt_count;
	u16 s_magic;
	u16 s_state;
	u16 s_errors;
	u16 s_minor_rev_level;
	u32 s_lastcheck;
	u32 s_checkinterval;
	u32 s_creator_os;
	u32 s_rev_level;
	u16 s_def_resuid;
	u16 s_def_resgid;
	u32 s_first_ino;
	u16 s_inode_size;
	u16 s_block_group_nr;
	u32 s_feature_compat;
	u32 s_feature_incompat;
	u32 s_feature_ro_compat;
	u8 s_uuid[16];
	u8 s_volume_name[16];
	u32 s_reserved[229];
};

struct ext2_block_group_descriptor
{
	u32 bg_block_bitmap;
	u32 bg_inode_bitmap;
	u32 bg_inode_table;
	u16 bg_free_blocks_count;
	u16 bg_free_inodes_count;
	u16 bg_used_dirs_count;
	u16 bg_pad;
	u32 bg_reserved[3];
};

#define EXT2_NDIR_BLOCKS 12
#define EXT2_IND_BLOCK EXT2_NDIR_BLOCKS
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)

struct ext2_inode
{
	u16 i_mode;
	u16 i_uid;
	u32 i_size;
	u32 i_atime;
	u32 i_ctime;
	u32 i_mtime;
	u32 i_dtime;
	u16 i_gid;
	u16 i_links_count;
	u32 i_blocks;
	u32 i_flags;
	u32 i_reserved1;
	u32 i_block[EXT2_N_BLOCKS];
	u32 i_version;
	u32 i_file_acl;
	u32 i_dir_acl;
	u32 i_faddr;
	u8 i_frag;
	u8 i_fsize;
	u16 i_pad1;
	u32 i_reserved2[2];
};

#define EXT2_NAME_LEN 255

struct ext2_dir_entry
{
	u32 inode;				/* Inode number */
	u16 rec_len;			/* Directory entry length */
	u16 name_len;			/* name length	*/
	u8 name[EXT2_NAME_LEN]; /* File name */
};

#define errno_exit(str)  \
	do                   \
	{                    \
		int err = errno; \
		perror(str);     \
		exit(err);       \
	} while (0)

#define dir_entry_set(entry, inode_num, str)  \
	do                                        \
	{                                         \
		char *s = str;                        \
		size_t len = strlen(s);               \
		entry.inode = inode_num;              \
		entry.name_len = len;                 \
		memcpy(&entry.name, s, len);          \
		if ((len % 4) != 0)                   \
		{                                     \
			entry.rec_len = 12 + len / 4 * 4; \
		}                                     \
		else                                  \
		{                                     \
			entry.rec_len = 8 + len;          \
		}                                     \
	} while (0)

#define dir_entry_write(entry, fd)           \
	do                                       \
	{                                        \
		ssize_t size = entry.rec_len;        \
		if (write(fd, &entry, size) != size) \
		{                                    \
			errno_exit("write");             \
		}                                    \
	} while (0)
