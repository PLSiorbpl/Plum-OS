#include "userspace.h"
#include "arch/x86_64/syscall/syscall.h"
#include "std/printf.hpp"
#include "Drivers/Keyboard.hpp"
#include "std/string.h"
#include "std/iostream.hpp"

static OpenPL::Context ctx;

void list_commands(int argc, char** argv) {
    std::printf("&9\tCommands: &9%s", std::Output::std_out, commands[0].name);
    for (u32 i = 1; i < sizeof(commands) / sizeof(Command); ++i) {
        std::printf("&9, %s", std::Output::std_out, commands[i].name);
    }
    std::printf("\n");
}

extern "C" void user_space_main() {
    //OpenPL::Framebuffer framebuffer = ctx.create_framebuffer(0, 0);
    //ctx.bind_framebuffer(framebuffer);
    sys_swap_framebuffer();
    sys_open("/dev/stdout", 0, 0);
    sys_open("/dev/stdin", 0, 0);

    std::printf("\n&aPrintf(%/i %/u %/s %/x %/c %/l %/f) &c%i %u %s %x %c %l %f\n", std::Output::std_out, -6767, 6767, "LOL",
                0x00006677, 'j', 0x7FFFFFFFFFFFFFFF, 3.146767);
    std::printf("&f------------ &bPlum OS 64bit &f------------\n\n");

    list_commands(0, nullptr);

    std::printf("&fPlum-OS> ");

    static char buffer[256];
    static int i = 0;

    while (true) {
        sys_swap_framebuffer();
        const kb::key_code key = std::cin();

        if (key == kb::key_code::KEY_NULL)
            continue;

        if (key == kb::key_code::KEY_BACKSPACE) {
            if (i <= 0) continue;
            if (buffer[i-1] == '\t') {
                std::printf("\b\b\b\b");
            }
            std::printf("\b");
            buffer[i-1] = '\0';
            i--;
        } else if (key == kb::key_code::KEY_ENTER) {
            buffer[i] = '\0';
            std::printf("\n");

            run_program(buffer);

            std::printf("&fPlum-OS> ");
            i = 0;
        } else {
            if (kb::to_char(key) == 0) continue;
            if (i >= 256) continue;
            buffer[i] = kb::to_char(key);
            std::printf("%c", std::Output::std_out, kb::to_char(key));
            i++;
        }
    }
}

void run_program(char* buffer) {
    // Get command name from buffer
    char command_name[256] = {0};
    int k = 0; int j = 0;

    while (buffer[k] != '\0' && buffer[k] == ' ') k++;

    while (buffer[k] != '\0' && buffer[k] != ' ' && j < 255)
        command_name[j++] = buffer[k++];

    command_name[j] = '\0';

    // Get arguments from buffer
    char* args[32]; int argc = 0;
    k = 0;

    while (buffer[k] != '\0') {
        while (buffer[k] == ' ') k++;
        if (buffer[k] == '\0') break;

        args[argc++] = &buffer[k];

        while (buffer[k] != '\0' && buffer[k] != ' ')
            k++;
    }

    k = 0;
    while (buffer[k] != '\0') {
        if (buffer[k] != ' ') {
            k++;
        } else {
            buffer[k] = '\0';
            k++;
            while (buffer[k] == ' ') k++;
        }
    }

    bool found_command = false;
    for (auto &[name, func] : commands) {
        if (std::strcmp(command_name, name) == 0) {
            func(argc, args);
            found_command = true;
            break;
        }
    }

    if (!found_command) {
        std::printf("&7\tUnknown command: &c%s \n", std::Output::std_out, buffer);
        sys_sleep(250);
    }
}
