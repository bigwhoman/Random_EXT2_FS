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
#include "ext2-headers.h"


u32 get_current_time()
{
	time_t t = time(NULL);
	if (t == ((time_t)-1))
	{
		errno_exit("time");
	}
	return t;
}

void write_superblock(int fd)
{
	off_t off = lseek(fd, BLOCK_OFFSET(SUPERBLOCK_BLOCKNO), SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}

	u32 current_time = get_current_time();

	struct ext2_superblock superblock = {0};

	int standard_size = 1024;
	int log_block_size = 0;
	while (standard_size < BLOCK_SIZE)
	{
		log_block_size++;
	}

	int standard_fragment_size = 1024;
	int log_fragment_size = 0;
	while (standard_fragment_size < BLOCK_SIZE)
	{
		log_fragment_size++;
	}

	/* These are intentionally incorrectly set as 0, you should set them
	   correctly and delete this comment */
	superblock.s_inodes_count = NUM_INODES;
	superblock.s_blocks_count = NUM_BLOCKS;
	superblock.s_r_blocks_count = 5 / 100 * NUM_BLOCKS;
	superblock.s_free_blocks_count = NUM_FREE_BLOCKS;
	superblock.s_free_inodes_count = NUM_FREE_INODES;
	superblock.s_first_data_block = FIRST_DATA_BLOCK; /* First Data Block */
	superblock.s_log_block_size = log_block_size;	  /* 1024 */
	superblock.s_log_frag_size = log_fragment_size;	  /* 1024 */
	superblock.s_blocks_per_group = BLOCKS_PER_GROUP;
	superblock.s_frags_per_group = FRAGMENTS_PER_GROUP;
	superblock.s_inodes_per_group = NUM_INODES / (1 + NUM_BLOCKS / BLOCKS_PER_GROUP);
	superblock.s_mtime = current_time;			/* Mount time */
	superblock.s_wtime = current_time;			/* Write time */
	superblock.s_mnt_count = 0;					/* Number of times mounted so far */
	superblock.s_max_mnt_count = -1;		/* Make this unlimited */
	superblock.s_magic = EXT2_SUPER_MAGIC;		/* ext2 Signature */
	superblock.s_state = EXT2_VALID_FS;			/* File system is clean */
	superblock.s_errors = EXT2_ERRORS_CONTINUE; /* Ignore the error (continue on) */
	superblock.s_minor_rev_level = 0;			/* Leave this as 0 */
	superblock.s_lastcheck = current_time;		/* Last check time */
	superblock.s_checkinterval = MAX_INTERVAL;	/* Force checks by making them every 1 second */
	superblock.s_creator_os = EXT2_OS_LINUX;	/* Linux */
	superblock.s_rev_level = EXT2_GOOD_OLD_REV; /* Leave this as 0 */
	superblock.s_def_resuid = EXT2_DEF_RESUID;	/* root */
	superblock.s_def_resgid = EXT2_DEF_RESGID;	/* root */

	/* You can leave everything below this line the same, delete this
	   comment when you're done the lab */
	superblock.s_uuid[0] = 0x5A;
	superblock.s_uuid[1] = 0x1E;
	superblock.s_uuid[2] = 0xAB;
	superblock.s_uuid[3] = 0x1E;
	superblock.s_uuid[4] = 0x13;
	superblock.s_uuid[5] = 0x37;
	superblock.s_uuid[6] = 0x13;
	superblock.s_uuid[7] = 0x37;
	superblock.s_uuid[8] = 0x13;
	superblock.s_uuid[9] = 0x37;
	superblock.s_uuid[10] = 0xC0;
	superblock.s_uuid[11] = 0xFF;
	superblock.s_uuid[12] = 0xEE;
	superblock.s_uuid[13] = 0xC0;
	superblock.s_uuid[14] = 0xFF;
	superblock.s_uuid[15] = 0xEE;

	memcpy(&superblock.s_volume_name, "hello", 5);

	ssize_t size = sizeof(superblock);
	if (write(fd, &superblock, size) != size)
	{
		errno_exit("write");
	}
}

void write_block_group_descriptor_table(int fd)
{
	off_t off = lseek(fd, BLOCK_OFFSET(BLOCK_GROUP_DESCRIPTOR_BLOCKNO), SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}

	struct ext2_block_group_descriptor block_group_descriptor = {0};

	/* These are intentionally incorrectly set as 0, you should set them
	   correctly and delete this comment */
	block_group_descriptor.bg_block_bitmap = BLOCK_BITMAP_BLOCKNO;
	block_group_descriptor.bg_inode_bitmap = INODE_BITMAP_BLOCKNO;
	block_group_descriptor.bg_inode_table = INODE_TABLE_BLOCKNO;
	block_group_descriptor.bg_free_blocks_count = NUM_FREE_BLOCKS;
	block_group_descriptor.bg_free_inodes_count = NUM_FREE_INODES;
	block_group_descriptor.bg_used_dirs_count = 2;
	// block_group_descriptor. = 2;

	ssize_t size = sizeof(block_group_descriptor);
	if (write(fd, &block_group_descriptor, size) != size)
	{
		errno_exit("write");
	}
}

void set_bitmap(char * bitmap, uint16_t bit){
	uint16_t bit_group = bit / 8;
	uint16_t bit_offset = bit % 8;
	bitmap[bit_group] = bitmap[bit_group] | (1 << bit_offset);
}

void write_block_bitmap(int fd)
{
	off_t off = lseek(fd, BLOCK_OFFSET(BLOCK_BITMAP_BLOCKNO), SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}
	char initials[BLOCK_SIZE] ;
	memset(initials, 0, BLOCK_SIZE);
	for (size_t i = 0; i < LAST_BLOCK; i++)
	{
		set_bitmap(initials, i);
	}
	memset((initials + NUM_BLOCKS/8 - 1), 0x80, 1);
	memset((initials + NUM_BLOCKS/8), 0xff, BLOCK_SIZE - NUM_BLOCKS/8);
	if ((write(fd, initials, BLOCK_SIZE)) != BLOCK_SIZE)
	{
		errno_exit("write");
	}
	
}



void write_inode_bitmap(int fd)
{
	off_t off = lseek(fd, BLOCK_OFFSET(INODE_BITMAP_BLOCKNO), SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}
	char initials[BLOCK_SIZE] = {0};
	for (size_t i = 0; i < 11; i++)
	{
		set_bitmap(initials, i);
	}
	set_bitmap(initials, LOST_AND_FOUND_INO);
	set_bitmap(initials, HELLO_WORLD_INO);
	// set_bitmap(initials, HELLO_INO);
	memset((initials + NUM_INODES/8), 0xff, BLOCK_SIZE - NUM_INODES/8);
	/* The first 11 bits in the bitmap should be considered as used in revision 0 of EXT2*/
	if ((write(fd, initials , BLOCK_SIZE)) != BLOCK_SIZE)
	{
		errno_exit("write");
	}
}

void write_inode(int fd, u32 index, struct ext2_inode *inode)
{
	off_t off = BLOCK_OFFSET(INODE_TABLE_BLOCKNO) + (index - 1) * sizeof(struct ext2_inode);
	off = lseek(fd, off, SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}

	ssize_t size = sizeof(struct ext2_inode);
	if (write(fd, inode, size) != size)
	{
		errno_exit("write");
	}
}

void write_inode_table(int fd)
{
	u32 current_time = get_current_time();

	struct ext2_inode lost_and_found_inode = {0};
	lost_and_found_inode.i_mode = EXT2_S_IFDIR | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR | EXT2_S_IRGRP | EXT2_S_IXGRP | EXT2_S_IROTH | EXT2_S_IXOTH;
	lost_and_found_inode.i_uid = 0;
	lost_and_found_inode.i_size = 1024;
	lost_and_found_inode.i_atime = current_time;
	lost_and_found_inode.i_ctime = current_time;
	lost_and_found_inode.i_mtime = current_time;
	lost_and_found_inode.i_dtime = 0;
	lost_and_found_inode.i_gid = 0;
	lost_and_found_inode.i_links_count = 2;
	lost_and_found_inode.i_blocks = 2; /* These are oddly 512 blocks */
	lost_and_found_inode.i_block[0] = LOST_AND_FOUND_DIR_BLOCKNO;
	write_inode(fd, LOST_AND_FOUND_INO, &lost_and_found_inode);

	/* You should add your 3 other inodes in this function and delete this
	   comment */

	struct ext2_inode root_inode = {0};
	root_inode.i_mode = EXT2_S_IFDIR | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IXUSR | EXT2_S_IRGRP | EXT2_S_IXGRP | EXT2_S_IROTH | EXT2_S_IXOTH;
	root_inode.i_uid = 0;
	root_inode.i_size = 1024;
	root_inode.i_atime = current_time;
	root_inode.i_ctime = current_time;
	root_inode.i_mtime = current_time;
	root_inode.i_dtime = 0;
	root_inode.i_gid = 0;
	root_inode.i_links_count = 3;
	root_inode.i_blocks = 2;
	root_inode.i_block[0] = ROOT_DIR_BLOCKNO;
	write_inode(fd, EXT2_ROOT_INO, &root_inode);


	struct ext2_inode hello_world_inode = {0};
	hello_world_inode.i_mode = EXT2_S_IFREG | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IRGRP |  EXT2_S_IROTH ;
	hello_world_inode.i_uid = 1000;
	hello_world_inode.i_size = 12;
	hello_world_inode.i_atime = current_time;
	hello_world_inode.i_ctime = current_time;
	hello_world_inode.i_mtime = current_time;
	hello_world_inode.i_dtime = 0;
	hello_world_inode.i_gid = 1000;
	hello_world_inode.i_links_count = 1;
	hello_world_inode.i_blocks = 2;
	hello_world_inode.i_block[0] = HELLO_WORLD_FILE_BLOCKNO;
	write_inode(fd, HELLO_WORLD_INO, &hello_world_inode);

	struct ext2_inode hello_inode = {0};
	hello_inode.i_mode = EXT2_S_IFLNK | EXT2_S_IRUSR | EXT2_S_IWUSR | EXT2_S_IRGRP |  EXT2_S_IROTH ;
	hello_inode.i_uid = 1000;
	char file_name[] = "hello-world";
	hello_inode.i_size = strlen(file_name);
	hello_inode.i_atime = current_time;
	hello_inode.i_ctime = current_time;
	hello_inode.i_mtime = current_time;
	hello_inode.i_dtime = 0;
	hello_inode.i_gid = 1000;
	hello_inode.i_links_count = 1;
	hello_inode.i_blocks = 0;
	memcpy(hello_inode.i_block, file_name, strlen(file_name));
	write_inode(fd, HELLO_INO, &hello_inode);
}

void write_root_dir_block(int fd)
{
	/* This is all you */
	off_t off = BLOCK_OFFSET(ROOT_DIR_BLOCKNO);
	off = lseek(fd, off, SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}
	ssize_t bytes_remaining = BLOCK_SIZE;

	struct ext2_dir_entry current_entry = {0};
	dir_entry_set(current_entry, EXT2_ROOT_INO, ".");
	dir_entry_write(current_entry, fd);

	bytes_remaining -= current_entry.rec_len;

	struct ext2_dir_entry parent_entry = {0};
	dir_entry_set(parent_entry, EXT2_ROOT_INO, "..");
	dir_entry_write(parent_entry, fd);

	bytes_remaining -= parent_entry.rec_len;

	struct ext2_dir_entry lost_found = {0};
	dir_entry_set(lost_found, LOST_AND_FOUND_INO, "lost+found");
	dir_entry_write(lost_found, fd);

	bytes_remaining -= lost_found.rec_len;

	struct ext2_dir_entry hello_entry = {0};
	dir_entry_set(hello_entry, HELLO_INO, "hello");
	dir_entry_write(hello_entry, fd);

	bytes_remaining -= hello_entry.rec_len;


	struct ext2_dir_entry hello_world_entry = {0};
	dir_entry_set(hello_world_entry, HELLO_WORLD_INO, "hello-world");
	dir_entry_write(hello_world_entry, fd);

	bytes_remaining -= hello_world_entry.rec_len;

	struct ext2_dir_entry fill_entry = {0};
	fill_entry.rec_len = bytes_remaining;
	dir_entry_write(fill_entry, fd);
}

void write_lost_and_found_dir_block(int fd)
{
	off_t off = BLOCK_OFFSET(LOST_AND_FOUND_DIR_BLOCKNO);
	off = lseek(fd, off, SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}

	ssize_t bytes_remaining = BLOCK_SIZE;

	struct ext2_dir_entry current_entry = {0};
	dir_entry_set(current_entry, LOST_AND_FOUND_INO, ".");
	dir_entry_write(current_entry, fd);

	bytes_remaining -= current_entry.rec_len;

	struct ext2_dir_entry parent_entry = {0};
	dir_entry_set(parent_entry, EXT2_ROOT_INO, "..");
	dir_entry_write(parent_entry, fd);

	bytes_remaining -= parent_entry.rec_len;

	struct ext2_dir_entry fill_entry = {0};
	fill_entry.rec_len = bytes_remaining;
	dir_entry_write(fill_entry, fd);
}

void write_hello_world_file_block(int fd)
{
	off_t off = BLOCK_OFFSET(HELLO_WORLD_FILE_BLOCKNO);
	off = lseek(fd, off, SEEK_SET);
	if (off == -1)
	{
		errno_exit("lseek");
	}
	// ssize_t bytes_remaining = BLOCK_SIZE;
	char hello_world[] = "Hello world\n";
	write(fd, hello_world,sizeof(hello_world));

}

int main()
{
	int fd = open("hello.img", O_CREAT | O_WRONLY, 0666);
	if (fd == -1)
	{
		errno_exit("open");
	}

	if (ftruncate(fd, 0))
	{
		errno_exit("ftruncate");
	}
	if (ftruncate(fd, NUM_BLOCKS * BLOCK_SIZE))
	{
		errno_exit("ftruncate");
	}

	write_superblock(fd);
	write_block_group_descriptor_table(fd);
	write_block_bitmap(fd);
	write_inode_bitmap(fd);
	write_inode_table(fd);
	write_root_dir_block(fd);
	write_lost_and_found_dir_block(fd);
	write_hello_world_file_block(fd);

	if (close(fd))
	{
		errno_exit("close");
	}
	return 0;
}
