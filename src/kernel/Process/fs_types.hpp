#pragma once
#include "std/types.hpp"

// SEEK_Modes
enum {
    SEEK_SET = 0,
    #define SEEK_SET 0
    SEEK_CUR = 1,
    #define SEEK_CUR 1
    SEEK_END = 2,
    #define SEEK_END 2
    SEEK_DATA = 3,
    #define SEEK_DATA 3
    SEEK_HOLE = 4,
    #define SEEK_HOLE 4
    SEEK_MAX = 5
    #define SEEK_MAX  SEEK_HOLE
};

// Dirent
enum {
    DT_UNKNOWN = 0,
    #define DT_UNKNOWN DT_UNKNOWN
    DT_FIFO = 1,
    #define DT_FIFO DT_FIFO
    DT_CHR = 2,
    #define DT_CHR DT_CHR
    DT_DIR = 4,
    #define DT_DIR DT_DIR
    DT_BLK = 6,
    #define DT_BLK DT_BLK
    DT_REG = 8,
    #define DT_REG DT_REG
    DT_LNK = 10,
    #define DT_LNK DT_LNK
    DT_SOCK = 12,
    #define DT_SOCK DT_SOCK
    DT_WHT = 14,
    #define DT_WHT DT_WHT
};

// Stat
enum {
    S_IFSOCK  = 0140000,
    #define S_IFSOCK S_IFSOCK
    S_IFLNK   = 0120000,
    #define S_IFLNK S_IFLNK
    S_IFREG   = 0100000,
    #define S_IFREG S_IFREG
    S_IFBLK   = 0060000,
    #define S_IFBLK S_IFBLK
    S_IFDIR   = 0040000,
    #define S_IFDIR S_IFDIR
    S_IFCHR   = 0020000,
    #define S_IFCHR S_IFCHR
    S_IFIFO  = 0010000,
    #define S_IFIFO S_IFIFO
};

#define S_IFMT    0170000

struct stat {
    unsigned long   st_dev;
    unsigned long   st_ino;
    unsigned int    st_mode; // File Type + Permission
    unsigned int    st_nlink;
    unsigned int    st_uid;
    unsigned int    st_gid;
    unsigned long   st_rdev;
    unsigned long   __pad1;
    long            st_size;
    int             st_blksize;
    int             __pad2;
    long            st_blocks;
    long            st_atime; // Access
    unsigned long   st_atime_nsec;
    long            st_mtime; // Modification
    unsigned long   st_mtime_nsec;
    long            st_ctime; // Status Change
    unsigned long   st_ctime_nsec;
    unsigned int    __unused4;
    unsigned int    __unused5;
};

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t  d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};