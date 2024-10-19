#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "ext2-headers.h"
#include <math.h>
/* locates beginning of the super block (first group) */
#define FD_DEVICE "ext2_filesystem_reference.img" /* the floppy disk device */
#define EXT2_SUPER_MAGIC 0xEF53
#define SUPERBLOCK_OFFSET 1024
#define FIND_BLOCK_OFFSET(i) (block_size * i)

static unsigned int block_size = 0; /* block size (to be calculated) */

int main(void)
{

	struct ext2_superblock super;

	int fd;

	/* open device */

	if ((fd = open(FD_DEVICE, O_RDONLY)) < 0)
	{
		perror(FD_DEVICE);
		exit(1); /* error while opening the floppy device */
	}

	/* read super-block */

	lseek(fd, SUPERBLOCK_OFFSET, SEEK_SET);
	read(fd, &super, sizeof(super));

	if (super.s_magic != EXT2_SUPER_MAGIC)
	{
		fprintf(stderr, "Not a Ext2 filesystem\n");
		exit(1);
	}

	block_size = 1024 << super.s_log_block_size;

	printf("Reading super-block from device " FD_DEVICE ":\n"
		   "Inodes count            : %u\n"
		   "Blocks count            : %u\n"
		   "Reserved blocks count   : %u\n"
		   "Free blocks count       : %u\n"
		   "Free inodes count       : %u\n"
		   "First data block        : %u\n"
		   "Block size              : %u\n"
		   "Blocks per group        : %u\n"
		   "Inodes per group        : %u\n"
		   "Creator OS              : %u\n"
		   "First non-reserved inode: %hu\n"
		   "Size of inode structure : %hu\n"
		   "Revision number : %hu\n\n\n",
		   super.s_inodes_count,
		   super.s_blocks_count,
		   super.s_r_blocks_count, /* reserved blocks count */
		   super.s_free_blocks_count,
		   super.s_free_inodes_count,
		   super.s_first_data_block,
		   block_size,
		   super.s_blocks_per_group,
		   super.s_inodes_per_group,
		   super.s_creator_os,
		   super.s_first_ino,
		   super.s_inode_size,
		   super.s_rev_level);

	/*
		We dont take the reserved GDT entries into account at least now :)
	*/
	size_t groups = ceil((double)super.s_blocks_count / (double)super.s_blocks_per_group);

	struct ext2_block_group_descriptor group_descriptor_table[groups + 1];
	for (size_t i = 0; i < groups; i++)
	{
		// printf("group : %d\n", i);
		lseek(fd, block_size * (i + 1), SEEK_SET);
		read(fd, &group_descriptor_table[i], sizeof(group_descriptor_table[i]));

		printf("size of %lu\n", sizeof(group_descriptor_table[i]));

		printf(
			"Block Bitmap           : %hu\n"
			"Inode Bitmap           : %hu\n"
			"Inode Table            : %hu\n"
			"Free Blocks            : %u\n"
			"Free Inodes            : %u\n"
			"Used Dirs              : %u\n",
			group_descriptor_table[i].bg_block_bitmap,
			group_descriptor_table[i].bg_inode_bitmap,
			group_descriptor_table[i].bg_inode_table,
			group_descriptor_table[i].bg_free_blocks_count,
			group_descriptor_table[i].bg_free_inodes_count,
			group_descriptor_table[i].bg_used_dirs_count);
	}

	int *block_bitmap = malloc(sizeof(int));
	lseek(fd, FIND_BLOCK_OFFSET(group_descriptor_table[0].bg_block_bitmap), SEEK_SET);

	printf("find %d\n", super.s_blocks_count / (32));
	for (u32 i = 0; i < super.s_blocks_count / (sizeof(int) * 8); i++)
	{

		read(fd, block_bitmap, sizeof(int));
		for (char j = 0; j < 32; j++)
		{
			if ((*block_bitmap) & (1 << j))
				printf("block present : %u\n", i * 32 + j);
		}
	}

	int *inode_bitmap = malloc(sizeof(int));
	lseek(fd, FIND_BLOCK_OFFSET(group_descriptor_table[0].bg_inode_bitmap), SEEK_SET);

	// printf("find %d\n", super.s_inodes_per_group/(32));
	for (u32 i = 0; i < super.s_inodes_per_group / (sizeof(int) * 8); i++)
	{

		read(fd, inode_bitmap, sizeof(int));
		for (char j = 0; j < 32; j++)
		{
			// Inefficient AF
			if ((*inode_bitmap) & (1 << j)) /* Inodes start at 1 lol :D */
				printf("inode present : %u\n", i * 32 + j + 1);
		}
	}

	struct ext2_inode inode_table[super.s_inodes_per_group];

	lseek(fd, FIND_BLOCK_OFFSET((group_descriptor_table[0].bg_inode_table)), SEEK_SET);
	for (size_t i = 0; i < super.s_first_ino + 1; i++)
	{
		read(fd, &inode_table[i], sizeof(inode_table[i]));
	}

	/* print root inode thingies*/
	printf(
		"imode          		: %x\n"
		"uid           : %u\n"
		"size            : %hu\n"
		"gid            : %u\n"
		"link count           : %u\n"
		"blocks             : %hu\n"
		"flags             : %hu\n"
		"address             : %hu\n",
		inode_table[EXT2_ROOT_INO].i_mode,
		inode_table[EXT2_ROOT_INO].i_uid,
		inode_table[EXT2_ROOT_INO].i_size,
		inode_table[EXT2_ROOT_INO].i_gid,
		inode_table[EXT2_ROOT_INO].i_links_count,
		inode_table[EXT2_ROOT_INO].i_blocks,
		inode_table[EXT2_ROOT_INO].i_flags,
		inode_table[EXT2_ROOT_INO].i_faddr);
	struct ext2_dir_entry root[20];
	for (size_t i = 0; i < 16; i++)
	{
		printf("block ---> %d\n", inode_table[EXT2_ROOT_INO].i_block[i]);
		if (inode_table[EXT2_ROOT_INO].i_block[i] != 0)
		{
			if (inode_table[EXT2_ROOT_INO].i_mode & EXT2_S_IFDIR)
			{
				lseek(fd, FIND_BLOCK_OFFSET(inode_table[EXT2_ROOT_INO].i_block[i]), SEEK_SET);
				int j = 0;
				do
				{
					read(fd, &root[j], 8);
					
					read(fd, &root[j].name, root[j].rec_len - 8);
					j++;
				} while (root[j - 1].rec_len > 0 && (lseek(fd, 0, SEEK_CUR) + root[j - 1].rec_len < FIND_BLOCK_OFFSET(inode_table[EXT2_ROOT_INO].i_block[i]) + block_size));
			}
		}
	}

	int i = 0;
	while (1)
	{
		printf(
			"inode          		: %d\n"
			"name          		: %s\n",
			root[i].inode,
			root[i].name);
		if (root[i].rec_len <= 0)
			break;
		i++;
	}

	close(fd);
	exit(0);
} /* main() */
