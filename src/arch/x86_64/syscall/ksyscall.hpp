#pragma once
#include "std/types.hpp"

u64 kwrite(unsigned int fd, const char* buf, size_t size);

u64 kread(unsigned int fd, void *buf, size_t size);

u64 kopen(const char *path, int flags, int mode);