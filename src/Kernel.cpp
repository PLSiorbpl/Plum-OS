#include "kernel.h"
#include "arch/x86_64/syscall/syscall.h"
#include "libs/std/types.hpp"
#include "std/printf.hpp"
#include "kernel/system.hpp"
#include "kernel/Sleep.hpp"
#include "kernel/Memory/heap.hpp"
#include "Drivers/Keyboard.hpp"
#include "std/string.h"
#include "kernel/linker_info.hpp"
#include "User_Programs/Chess/main.hpp"
#include "User_Programs/MyCraft/main.hpp"
#include "Drivers/Network/Sockets/socket.hpp"
#include "Drivers/Network/Sockets/tcp_socket.hpp"
#include "Drivers/Network/Common.hpp"

struct Command {
    const char *name;

    void (*func)(int argc, char** argv);
};

inline uint64_t range(void *a, void *b) {
    return reinterpret_cast<uint64_t>(b) - reinterpret_cast<uint64_t>(a);
}

void list_commands(int argc, char** argv);

Command commands[] = {
    {
        "http", [](int argc, char** argv) {
            int server_num = tsock::socket();
            auto server = tsock::find_socket(server_num);

            server->bind(8080);
            server->listen();

            std::printf("HTTP server listening on port 8080\r\n");
            std::printf("Waiting for client...\r\n");

            while (true) {
                tsock::tcp_socket* client = nullptr;

                while (!client)
                    client = server->accept();

                std::printf("New HTTP client!\r\n");

                char request[2048] = {};
                const size_t received = client->recv(request, sizeof(request) - 1);

                if (received > 0) {
                    request[received] = '\0';

                    std::printf("Request:\r\n%s\r\n", std::Output::std_out, request);

                    const char response[] =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n"
                        "Content-Length: 72\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "<html><body><h1>Hello from Plum-OS!</h1></body></html>\r\n";

                    client->send(response, sizeof(response) - 1);
                }

                client->close();
            }

            server->close();
        }
    },
    {
        "tcp", [](int argc, char** argv) {
            int server_num = tsock::socket();
            auto server = tsock::find_socket(server_num);

            server->bind(8080);
            server->listen();

            sys_sleep(1000);

            tsock::tcp_socket *client = nullptr;
            std::printf("Waiting for client...");

            while (!client) {
                client = server->accept();
            }

            std::move_cursor(0, -1);
            std::printf("New client connected!");

            if (client) {
                const char msg[] = "Hello from Plum-OS!\r\n";
                client->send(msg, sizeof(msg) - 1);
                client->send(msg, sizeof(msg) - 1);
            }

            client->close();
            server->close();
        }
    },{
        "udp", [](int argc, char** argv) {
            const int sock = soc::socket();
            if (sock == -1) return;

            soc::bind(sock, 25565);
            //soc::udp_recv_packet rec = {};
            //if (sys_socket(sock, &rec, false, 10000)) {
            //    std::printf("\t&audp: %s\n", std::Output::std_out, rec.data);
            //    heap::free(rec.data);
            //}
            const char message[] = "hello";

            soc::udp_send_packet send {};
            send.data = (uint8_t*)message;
            send.size = sizeof(message)-1;
            send.to_ip = NET::make_ipv4(10, 0, 0, 1);
            send.to_port = 25565;
            sys_socket(sock, &send, true, 0);
        }
    }, {
        "clear", [](int argc, char** argv) {
            systemPL::fb.clear();
        }
    }, {
        "chess", [](int argc, char** argv) {
            Chess::main(argc, argv);
        }
    },{
        "m", [](int argc, char** argv) {
            MyCraft::main(argc, argv);
        }
    },{
        "mycraft", [](int argc, char** argv) {
            MyCraft::main(argc, argv);
        }
    }, {
        "sleep", [](int argc, char** argv) {
            uint64_t wait = 1;
            uint8_t unit = true; // ms, s
            bool us = false;
            if (argc > 1) {
                for (int i = 1; i < argc; i++) {
                    if (std::str_cmp(argv[i], "-h")) {
                        std::printf("&7Usage: &fsleep &e[OPTIONS]\n\n");
                        std::printf("&eOption     &fMeaning\n");
                        std::printf("&b-us        &7Micro-seconds mode\n");
                        std::printf("&b-ms        &7Mili-seconds mode\n");
                        std::printf("&b-s         &7Seconds mode\n");
                        std::printf("&b-t TIME    &7Time to wait\n");
                        std::printf("&b-h         &7This text\n");
                        return;
                    } else if (std::str_cmp(argv[i], "-us")) {
                        us = true;
                        unit = false;
                    } else if (std::str_cmp(argv[i], "-ms")) {
                        unit = false;
                    } else if (std::str_cmp(argv[i], "-s")) {
                        unit = true;
                    } else if (std::str_cmp(argv[i], "-t")) {
                        if (i+1 < argc) {
                            wait = std::str_to_int(argv[i+1]);
                        }
                    }
                }
            }
            std::printf("&a\tSleeping for &f%l ", std::Output::std_out, wait);
            auto unit_str = unit ? "seconds" : "mili-seconds";
            if (us)
                unit_str = "micro-seconds";
            std::printf("&a%s\n", std::Output::std_out, unit_str);
            auto time = unit ? wait * 1000 : wait * 1000; // 1000000
            if (us)
                time = wait;
            sys_sleep(time);
        }
    }, {
        "heap", [](int argc, char** argv) {
            bool show_all = false;
            if (argc > 0) {
                for (int i = 0; i < argc; i++) {
                    if (std::str_cmp(argv[i], "-s")) {
                        show_all = false;
                    } else if (std::str_cmp(argv[i], "-l")) {
                        show_all = true;
                    } else if (std::str_cmp(argv[i], "-h")) {
                        std::printf("&7Usage: &fheap &e[OPTIONS]\n\n");
                        std::printf("&eOption     &fMeaning\n");
                        std::printf("&b-l         &7Show all information about heap\n");
                        std::printf("&b-s         &7Show only summary\n");
                        std::printf("&b-h         &7This text\n");
                        return;
                    }
                }
            }
            sys_heap_dump(show_all);
        }
    }, {
        "pci", [](int argc, char** argv) {
            sys_pci_test();
        }
    }, {
        "size", [](int argc, char** argv) {
            auto kernel_size = range(&Linker::__kernel_start, &Linker::__kernel_end);
            auto text_size = range(&Linker::__kernel_text_start, &Linker::__kernel_text_end);
            auto rodata_size = range(&Linker::__kernel_rodata_start, &Linker::__kernel_rodata_end);
            auto data_size = range(&Linker::__kernel_data_start, &Linker::__kernel_data_end);
            auto bss_size = range(&Linker::__kernel_bss_start, &Linker::__kernel_bss_end);
            auto stack_size = range(&Linker::stack_bottom, &Linker::stack_top);
            auto user_stack_size = range(&Linker::user_stack_bottom, &Linker::user_stack_top);

            auto kernel_code_size = text_size + rodata_size;

            std::printf("&9\t.text &7size: &a%u%s \t&9.rodata &7size: &a%u%s\n", std::Output::std_out, text_size,
                        std::format_size(text_size), rodata_size, std::format_size(rodata_size));
            std::printf("&9\t.data &7size: &a%u%s \t&9.bss &7size: &a%u%s\n", std::Output::std_out, data_size,
                        std::format_size(data_size), bss_size, std::format_size(bss_size));
            std::printf("&9\t.stack &7size: &a%u%s \t&9.user_stack &7size: &a%u%s\n", std::Output::std_out, stack_size,
                        std::format_size(stack_size), user_stack_size, std::format_size(user_stack_size));
            std::printf("&b\tKernel Code &7size: &a%u%s\n\n", std::Output::std_out, kernel_code_size,
                        std::format_size(kernel_code_size));
            std::printf("&e\tTotal kernel &7size: &a%u%s\n", std::Output::std_out, kernel_size,
                        std::format_size(kernel_size));
        }
    }, {
        "usb", [](int argc, char** argv) {
            sys_usb();
        }
    }, {
        "colors", [](int argc, char** argv) {
            std::printf("&0 &&00 &1 &&11 &2 &&22 &3 &&33 &4 &&44 &5 &&55 &6 &&66 &7 &&77 &8 &&88 &9 &&99 &a &&aa &b &&bb &c &&cc &d &&dd &e &&ee &f &&ff\n");
        }
    }, {
        "partitions", [](int argc, char** argv) {
            sys_list_parts();
        }
    },
};

void list_commands(int argc, char** argv) {
    std::printf("&9\tCommands: &9%s", std::Output::std_out, commands[0].name);
    for (u32 i = 1; i < sizeof(commands) / sizeof(Command); ++i) {
        std::printf("&9, %s", std::Output::std_out, commands[i].name);
    }
    std::printf("\n");
}

extern "C" void user_space_main() {
    std::printf("\n&aPrintf(%/i %/u %/s %/x %/c %/l %/f) &c%i %u %s %x %c %l %f\n", std::Output::std_out, -6767, 6767, "LOL",
                0x00006677, 'j', 0x7FFFFFFFFFFFFFFF, 3.146767);
    std::printf("&f------------ &bPlum OS 64bit &f------------\n\n");

    list_commands(0, nullptr);

    std::printf("&fPlum-OS> ");

    static char buffer[256];
    static int i = 0;

    while (true) {
        sys_swap_framebuffer();
        const kb::key_code key = sys_get_key();

        if (key == kb::key_code::KEY_NULL)
            continue;

        if (key == kb::key_code::KEY_BACKSPACE) {
            if (i <= 0) continue;
            if (buffer[i-1] == '\t') {
                std::print("\b\b\b\b");
            }
            std::put_char('\b');
            buffer[i-1] = '\0';
            i--;
        } else if (key == kb::key_code::KEY_ENTER) {
            buffer[i] = '\0';
            std::put_char('\n');

            run_program(buffer);

            std::printf("&fPlum-OS> ");
            i = 0;
        } else {
            if (kb::to_char(key) == 0) continue;
            if (i >= 256) continue;
            buffer[i] = kb::to_char(key);
            std::put_char(kb::to_char(key));
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
        if (std::str_cmp(command_name, name)) {
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
