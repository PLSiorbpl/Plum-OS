#include "userspace.h"
#include "arch/x86_64/syscall/syscall.h"
#include "Drivers/Keyboard.hpp"
#include "Drivers/Network/Sockets/tcp_socket.hpp"
#include "kernel/system.hpp"
#include "std/iostream.hpp"
#include "std/printf.hpp"
#include "std/string.h"
#include "User_Programs/MyCraft/main.hpp"

Command commands[8] = {
    {
        "http", [](int argc, char** argv) {
            int server_num = tsock::socket();
            auto server = tsock::find_socket(server_num);

            server->bind(8080);
            server->listen();

            std::printf("HTTP server listening on port 8080\n");
            std::printf("Waiting for client...\n");

            while (true) {
                sys_swap_framebuffer();
                tsock::tcp_socket* client = nullptr;

                while (!client) {
                    auto key = std::cin();
                    if (key == kb::key_code::KEY_ESC) { if (client) client->close(); server->close(); return; }

                    client = server->accept();
                }

                std::printf("New HTTP client!\n");

                char request[2048] = {};
                const size_t received = client->recv(request, sizeof(request) - 1);

                if (received > 0) {
                    request[received] = '\0';

                    std::printf("Request:\n%s\n", std::Output::std_out, request);

                    const char response[] =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        R"HTML(
                        <html>
                            <head>
                                <title>Plum-OS 64bit</title>
                                <style>
                                    body {
                                        background:#0b0b10; color:#ddd;
                                        font:16px Arial; text-align:center; padding:50px
                                    }
                                    h1 { color:#b46cff }
                                </style>
                            </head>
                            <body>
                                <h1>Plum-OS</h1>
                                <p>64bit UEFI kernel made in C++</p>
                                <p>HTTP server running!</p>
                            </body>
                        </html>
                        )HTML";
                    client->send(response, sizeof(response) - 1);
                }

                client->close();
            }

            server->close();
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
                    if (std::strcmp(argv[i], "-h") == 0) {
                        std::printf("&7Usage: &fsleep &e[OPTIONS]\n\n");
                        std::printf("&eOption     &fMeaning\n");
                        std::printf("&b-us        &7Micro-seconds mode\n");
                        std::printf("&b-ms        &7Mili-seconds mode\n");
                        std::printf("&b-s         &7Seconds mode\n");
                        std::printf("&b-t TIME    &7Time to wait\n");
                        std::printf("&b-h         &7This text\n");
                        return;
                    } else if (std::strcmp(argv[i], "-us") == 0) {
                        us = true;
                        unit = false;
                    } else if (std::strcmp(argv[i], "-ms") == 0) {
                        unit = false;
                    } else if (std::strcmp(argv[i], "-s") == 0) {
                        unit = true;
                    } else if (std::strcmp(argv[i], "-t") == 0) {
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
                    if (std::strcmp(argv[i], "-s") == 0) {
                        show_all = false;
                    } else if (std::strcmp(argv[i], "-l") == 0) {
                        show_all = true;
                    } else if (std::strcmp(argv[i], "-h") == 0) {
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
