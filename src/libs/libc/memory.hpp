#pragma once
#include "std/types.hpp"

extern "C" void* malloc(uint64_t size);
extern "C" void free(void* ptr);

extern "C" void* memcpy(void* dst, const void* src, uint64_t size);
extern "C" volatile void* memmove(volatile void* dst, volatile const void* src, size_t size);

extern "C" void* memset(void* dst, uint8_t value, size_t size);
volatile uint16_t *memset16(volatile uint16_t *dst, uint16_t value, size_t count);
uint32_t *memset32(uint32_t *dst, uint32_t value, size_t count);

extern "C" bool memcmp(const void* src1, const void* src2, size_t size);