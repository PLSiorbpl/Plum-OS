#pragma once
#include "std/types.hpp"

extern "C" void user_space_main();

void run_program(char* buffer);

struct Command {
    const char *name;

    void (*func)(int argc, char** argv);
};

inline uint64_t range(void *a, void *b) {
    return reinterpret_cast<uint64_t>(b) - reinterpret_cast<uint64_t>(a);
}

void list_commands(int argc, char** argv);

extern Command commands[9];