// /**
//  * finding_filesystems
//  * CS 341 - Fall 2024
//  */
// #include "minixfs.h"
// #include "minixfs_utils.h"
// #include <errno.h>
// #include <stdio.h>
// #include <string.h>

// /**
//  * Virtual paths:
//  *  Add your new virtual endpoint to minixfs_virtual_path_names
//  */
// char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

// /**
//  * Forward declaring block_info_string so that we can attach unused on it
//  * This prevents a compiler warning if you haven't used it yet.
//  *
//  * This function generates the info string that the virtual endpoint info should
//  * emit when read
//  */
// static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
// static char *block_info_string(ssize_t num_used_blocks) {
//     char *block_string = NULL;
//     ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
//     asprintf(&block_string,
//              "Free blocks: %zd\n"
//              "Used blocks: %zd\n",
//              curr_free_blocks, num_used_blocks);
//     return block_string;
// }

// // Don't modify this line unless you know what you're doing
// int minixfs_virtual_path_count =
//     sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

// int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
//     // Thar she blows!
//     inode *node = get_inode(fs, path);
//     if (!node) {
//         errno = ENOENT;
//         return -1;
//     }
//     uint16_t temp = node->mode >> RWX_BITS_NUMBER;
//     node->mode = new_permissions | (temp << RWX_BITS_NUMBER) ;
//     clock_gettime(CLOCK_REALTIME, &(node->ctim));
//     return 0;
// }

// int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
//     // Land ahoy!
//     inode *node = get_inode(fs, path);
//     if (!node) {
//         errno = ENOENT;
//         return -1;
//     }
//     if (owner != ((uid_t)-1)) {
//         node->uid = owner;
//     }
//     if (group != ((uid_t)-1)) {
//         node->gid = group;
//     }
//     clock_gettime(CLOCK_REALTIME, &(node->ctim));
//     return 0;
// }

// inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
//     // Land ahoy!
//     return NULL;
// }

// ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
//                              size_t count, off_t *off) {
//     if (!strcmp(path, "info")) {
//         // TODO implement the "info" virtual file here
//     }

//     errno = ENOENT;
//     return -1;
// }

// ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
//                       size_t count, off_t *off) {
//     // X marks the spot
//     return -1;
// }

// ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
//                      off_t *off) {
//     const char *virtual_path = is_virtual_path(path);
//     if (virtual_path)
//         return minixfs_virtual_read(fs, virtual_path, buf, count, off);
//     // 'ere be treasure!
//     return -1;
// }

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

// Add these macros to handle MIN and MAX
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info"};

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    inode *node = get_inode(fs, path);
    if (!node) {
        errno = ENOENT;
        return -1;
    }
    uint16_t temp = node->mode >> RWX_BITS_NUMBER;
    node->mode = new_permissions | (temp << RWX_BITS_NUMBER);
    clock_gettime(CLOCK_REALTIME, &(node->ctim));
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    inode *node = get_inode(fs, path);
    if (!node) {
        errno = ENOENT;
        return -1;
    }
    if (owner != (uid_t)-1) {
        node->uid = owner;
    }
    if (group != (uid_t)-1) {
        node->gid = group;
    }
    clock_gettime(CLOCK_REALTIME, &(node->ctim));
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    const char *filename;
    inode *parent = parent_directory(fs, path, &filename);

    if (!parent || !valid_filename(filename)) {
        errno = ENOENT;
        return NULL;
    }

    inode_number new_inode_index = first_unused_inode(fs);
    if (new_inode_index == -1) {
        errno = ENOSPC;
        return NULL;
    }

    inode *new_inode = &(fs->inode_root[new_inode_index]);
    init_inode(parent, new_inode);

    minixfs_dirent new_entry = { .inode_num = new_inode_index };
    strncpy(new_entry.name, filename, FILE_NAME_LENGTH - 1);
    new_entry.name[FILE_NAME_LENGTH - 1] = '\0';

    if (add_data_block_to_inode(fs, parent) == -1) {
        errno = ENOSPC;
        return NULL;
    }
    make_string_from_dirent((char *)(fs->data_root + parent->direct[0]) + parent->size, new_entry);
    parent->size += FILE_NAME_ENTRY;
    clock_gettime(CLOCK_REALTIME, &(parent->mtim));

    return new_inode;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        ssize_t used_blocks = 0;
        for (size_t i = 0; i < DATA_NUMBER; i++) {
            if (get_data_used(fs, i)) {
                used_blocks++;
            }
        }

        char *info_str = block_info_string(used_blocks);
        size_t info_len = strlen(info_str);
        if ((size_t)*off >= info_len) {
            free(info_str);
            return 0;
        }

        size_t to_copy = MIN(count, info_len - (size_t)*off);
        memcpy(buf, info_str + *off, to_copy);
        *off += to_copy;

        free(info_str);
        return to_copy;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);

    inode *node = get_inode(fs, path);
    if (!node || !is_file(node)) {
        errno = ENOENT;
        return -1;
    }

    if ((size_t)*off >= node->size) {
        return 0;
    }

    size_t bytes_left = node->size - (size_t)*off;
    size_t to_read = MIN(count, bytes_left);
    size_t read = 0;

    size_t block_index = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);
    char *block_data;

    while (to_read > 0) {
        if (block_index < NUM_DIRECT_BLOCKS) {
            block_data = (char *)(fs->data_root + node->direct[block_index]);
        } else {
            block_data = (char *)(fs->data_root + ((data_block_number *)fs->data_root)[block_index - NUM_DIRECT_BLOCKS]);
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

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    inode *node = get_inode(fs, path);
    if (!node) {
        node = minixfs_create_inode_for_path(fs, path);
        if (!node) {
            return -1;
        }
    }

    size_t to_write = count;
    size_t written = 0;

    size_t block_index = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);
    char *block_data;

    while (to_write > 0) {
        if (block_index < NUM_DIRECT_BLOCKS) {
            if (node->direct[block_index] == UNASSIGNED_NODE) {
                if (add_data_block_to_inode(fs, node) == -1) {
                    errno = ENOSPC;
                    return -1;
                }
            }
            block_data = (char *)(fs->data_root + node->direct[block_index]);
        } else {
            if (node->indirect == UNASSIGNED_NODE) {
                if (add_single_indirect_block(fs, node) == -1) {
                    errno = ENOSPC;
                    return -1;
                }
            }
            if (((data_block_number *)fs->data_root)[block_index - NUM_DIRECT_BLOCKS] == UNASSIGNED_NODE) {
                if (add_data_block_to_indirect_block(fs, (data_block_number *)fs->data_root + node->indirect) == -1) {
                    errno = ENOSPC;
                    return -1;
                }
            }
            block_data = (char *)(fs->data_root + ((data_block_number *)fs->data_root)[block_index - NUM_DIRECT_BLOCKS]);
        }

        size_t chunk = MIN(to_write, sizeof(data_block) - block_offset);
        memcpy(block_data + block_offset, (char *)buf + written, chunk);

        to_write -= chunk;
        written += chunk;
        *off += chunk;

        block_index++;
        block_offset = 0;
    }

    node->size = MAX(node->size, (uint64_t)*off);
    clock_gettime(CLOCK_REALTIME, &(node->mtim));
    clock_gettime(CLOCK_REALTIME, &(node->ctim));
    return written;
}
