/**
 * finding_filesystems
 * CS 341 - Fall 2024
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode *node = get_inode(fs, path);
    if (!node) {
        errno = ENOENT;
        return -1;
    }
    uint16_t temp = node->mode >> RWX_BITS_NUMBER;
    node->mode = new_permissions | (temp << RWX_BITS_NUMBER) ;
    clock_gettime(CLOCK_REALTIME, &(node->ctim));
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode *node = get_inode(fs, path);
    if (!node) {
        errno = ENOENT;
        return -1;
    }
    if (owner != ((uid_t)-1)) {
        node->uid = owner;
    }
    if (group != ((uid_t)-1)) {
        node->gid = group;
    }
    clock_gettime(CLOCK_REALTIME, &(node->ctim));
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if (get_inode(fs, path) || !valid_filename(path)) {
        return NULL;
    }
    const char *filename = NULL;
    inode *parent = parent_directory(fs, path, &filename);
    inode_number new_inode_number = first_unused_inode(fs);
    inode *new_inode = &(fs->inode_root[new_inode_number]);
    init_inode(parent, new_inode);
    minixfs_dirent new_dirent;
    new_dirent.name = strdup(filename);
    new_dirent.inode_num = new_inode_number;

    int num_blocks = ceil(parent->size / sizeof(data_block));
    int remain_size = parent->size % sizeof(data_block);
    
    if (num_blocks < NUM_DIRECT_BLOCKS) { // direct block are not full
        if (parent->direct[num_blocks] == UNASSIGNED_NODE) {
            if (add_data_block_to_inode(fs, parent) == -1) {
                errno = ENOSPC;
                return NULL;
            }
        }
        make_string_from_dirent((char *)(fs->data_root + parent->direct[num_blocks]) + remain_size, new_dirent);
        // data_block_number data_block_num = parent->direct[num_blocks];
        // make_string_from_dirent(fs->data_root[data_block_num].data + remain_size, new_dirent);
    } else { // direct block are full
        if (parent->indirect == UNASSIGNED_NODE) {
            if (add_single_indirect_block(fs, parent) == -1) {
                errno = ENOSPC;
                return NULL;
            }
        }

        data_block_number *indirect_blocks = (data_block_number *)(fs->data_root + parent->indirect);
        int indirect_index = num_blocks - NUM_DIRECT_BLOCKS;

        if (indirect_blocks[indirect_index] == UNASSIGNED_NODE) {
            if (add_data_block_to_indirect_block(fs, indirect_blocks) == -1) {
                errno = ENOSPC;
                return NULL;
            }
        }

        make_string_from_dirent((char *)(fs->data_root + indirect_blocks[indirect_index]) + remain_size, new_dirent);
        // data_block_number indirect_block_num = parent->indirect;
        // data_block_number *indirect_block = (data_block_number *)fs->data_root[indirect_block_num].data;
        // data_block_number data_block_num = indirect_block[num_blocks - NUM_DIRECT_BLOCKS];
        // make_string_from_dirent(fs->data_root[data_block_num].data + remain_size, new_dirent);
    }
    free(new_dirent.name);
    return fs->inode_root + new_inode_number;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        char* data_map = GET_DATA_MAP(fs->meta);
        ssize_t num_used_block=0;
        for(uint64_t i=0; i<fs->meta->dblock_count;i++){
            if(data_map[i]==1){
                num_used_block++;
            }
        }
        char* str = block_info_string(num_used_block);
        size_t to_read = 0;
        if (strlen(str) - *off < count) {
            to_read = strlen(str) - *off;
        } else {
            to_read = count;
        }
        memcpy(buf, str + *off, to_read);
        *off += to_read;

        return to_read;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    size_t max_file_size = (NUM_DIRECT_BLOCKS + NUM_INDIRECT_BLOCKS) * sizeof(data_block);
    if (*off + count > max_file_size) {
        errno = ENOSPC;
        return -1;
    }

    inode *node = get_inode(fs, path);
    if (!node) {
        node = minixfs_create_inode_for_path(fs, path);
    }

    size_t total_blocks = ceil((*off + count) / sizeof(data_block));
    if (minixfs_min_blockcount(fs, path, total_blocks) == -1) {
        errno = ENOSPC;
        return -1;
    }

    size_t bytes_written = 0; 
    size_t block_index = *off / sizeof(data_block); 
    size_t block_offset = *off % sizeof(data_block); 

    while (bytes_written < count) {
        char *block_data = NULL;

        if (block_index < NUM_DIRECT_BLOCKS) {
            block_data = (char *)(fs->data_root + node->direct[block_index]);
        } else {
            size_t indirect_index = block_index - NUM_DIRECT_BLOCKS;
            data_block_number *indirect_blocks = (data_block_number *)(fs->data_root + node->indirect);
            block_data = (char *)(fs->data_root + indirect_blocks[indirect_index]);
        }

        size_t chunk = 0;
        if (block_offset + count - bytes_written > sizeof(data_block)) {
            chunk = sizeof(data_block) - block_offset;
        } else {
            chunk = count - bytes_written;
        }

        memcpy(block_data + block_offset, (char *)buf + bytes_written, chunk);
        bytes_written += chunk;
        block_offset = 0;
        block_index++;
    }

    node->size = *off + count;
    *off += count;
    clock_gettime(CLOCK_REALTIME, &(node->mtim));
    clock_gettime(CLOCK_REALTIME, &(node->atim));

    return bytes_written;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!
    inode *node = get_inode(fs, path);
    if (!node || !is_file(node)) {
        errno = ENOENT;
        return -1;
    }

    size_t bytes_left = node->size - (size_t)*off;
    size_t to_read = 0;
    if (bytes_left < count) {
        to_read = bytes_left;
    } else {
        to_read = count;
    }
    size_t read = 0;

    size_t block_index = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);

    while (to_read > 0) {
        char *block_data = NULL;

        if (block_index < NUM_DIRECT_BLOCKS) {
            block_data = (char *)(fs->data_root + node->direct[block_index]);
        } else {
            data_block_number *indirect_blocks = (data_block_number *)(fs->data_root + node->indirect);
            size_t indirect_index = block_index - NUM_DIRECT_BLOCKS;
            block_data = (char *)(fs->data_root + indirect_blocks[indirect_index]);
        }

        size_t chunk = MIN(to_read, sizeof(data_block) - block_offset);
        memcpy((char *)buf + read, block_data + block_offset, chunk);

        to_read -= chunk;
        read += chunk;
        *off += chunk;
        block_index++;
        block_offset = 0;
    }

    clock_gettime(CLOCK_REALTIME, &(node->atim));
    return read;
}

