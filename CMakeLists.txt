cmake_minimum_required(VERSION 3.28)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_ASM_NASM_COMPILER nasm)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

project(Plum-OS C CXX)

enable_language(ASM_NASM)

# =========================================================
# Kernel

file(GLOB_RECURSE CPP_SOURCES_KERNEL CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src_boot/*.cpp")
file(GLOB_RECURSE CPP_SOURCES_64 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ASM_SOURCES_64 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.asm")
file(GLOB_RECURSE ASM_SOURCES_BOOT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src_boot/*.asm")

include(vendor/uacpi/uacpi.cmake)

add_executable(Kernel ${CPP_SOURCES_KERNEL} ${CPP_SOURCES_64} ${ASM_SOURCES_64} ${ASM_SOURCES_BOOT} ${UACPI_SOURCES})

target_include_directories(Kernel PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src_boot"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/libs"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Drivers/acpi"
        ${UACPI_INCLUDES}
)

target_compile_options(Kernel PRIVATE
        # ---------------------
        # C++
        $<$<COMPILE_LANGUAGE:CXX>:
        -fno-use-cxa-atexit
        -fno-rtti
        >
        # C++ / C
        $<$<COMPILE_LANGUAGE:C,CXX>:
        -m64
        -ffreestanding
        -fno-exceptions
        -fno-stack-protector
        -fno-omit-frame-pointer
        -fno-strict-aliasing
        -fno-builtin
        -fno-pie
        -fno-pic
        -mcmodel=kernel
        -mno-red-zone
        # Floats
        -msse2
        -mfpmath=sse
        -mno-avx
        >
        # ---------------------
        # NASM
        $<$<COMPILE_LANGUAGE:NASM>:
        -g -F dwarf
        >
        # ---------------------
        # General
        -g
        -Wall
        -O3
        # ---------------------
        # uACPI
        -DUACPI_OVERRIDE_CONFIG
)

target_link_options(Kernel PRIVATE
        -m64
        -nostdlib
        -no-pie
        -fuse-ld=lld
        -T ${CMAKE_SOURCE_DIR}/linker.ld
)

set_target_properties(Kernel PROPERTIES
        OUTPUT_NAME "Kernel"
        SUFFIX ".elf"
)
# ---------------------------------------------------------

# All Images
set(FAT_IMG ${CMAKE_SOURCE_DIR}/Images/fat.img)
set(FAT32_IMG ${CMAKE_SOURCE_DIR}/Images/fat32.img)
#set(USB_IMG ${CMAKE_SOURCE_DIR}/Images/usbfat32.img)
set(LIMINE ${CMAKE_SOURCE_DIR}/vendor/limine)

set(OVMF ${CMAKE_SOURCE_DIR}/src_boot/ovmf_code_4M.fd)

# =========================================================
# Building

add_custom_target(build
        COMMENT "Building disk image"
        COMMAND dd if=/dev/zero of=${FAT_IMG} bs=1M count=64
        COMMAND mformat -i ${FAT_IMG}
        -h 32 -t 32 -n 64   # heads, tracks, sectors
        -F ::

        COMMAND mmd    -i ${FAT_IMG} ::/EFI
        COMMAND mmd    -i ${FAT_IMG} ::/EFI/BOOT
        COMMAND mmd    -i ${FAT_IMG} ::/boot
        COMMAND mmd    -i ${FAT_IMG} ::/boot/limine

        COMMAND mcopy  -i ${FAT_IMG} ${LIMINE}/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI # UEFI
        COMMAND mcopy  -i ${FAT_IMG} ${LIMINE}/limine-uefi-cd.bin  ::/boot/limine/limine-uefi-cd.bin # UEFI
        COMMAND mcopy  -i ${FAT_IMG} ${LIMINE}/limine-bios-cd.bin  ::/boot/limine/limine-bios-cd.bin # BIOS
        COMMAND mcopy  -i ${FAT_IMG} ${LIMINE}/limine-bios.sys  ::/boot/limine/limine-bios.sys # BIOS
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_CURRENT_BINARY_DIR}/Kernel.elf  ::/Kernel.elf
        COMMAND mcopy  -i ${FAT_IMG} ${CMAKE_SOURCE_DIR}/limine.conf  ::/boot/limine/limine.conf
        DEPENDS Kernel
        VERBATIM
)
# ---------------------------------------------------------

# =========================================================
# QEMU

set(QEMU_FLAGS
        # ----- PC -----
        -cpu max
        -m 1G
        -M q35
        -vga std
        -boot order=c
        # ----- Drives -----
        -drive if=pflash,format=raw,readonly=on,file=${OVMF}
        -drive if=none,id=fat,format=raw,file=${FAT_IMG}
        -device virtio-blk-pci,drive=fat
        # ----- AHCI -----
        -device ahci,id=ahci
        -device ide-hd,drive=hd0,bus=ahci.0
        -drive file=${FAT32_IMG},format=raw,if=none,id=hd0
        # ----- NVME -----
        #-device nvme,drive=NVME1,serial=nvme-1
        #-drive file=${FAT32_IMG},format=raw,if=none,id=NVME1
        # ----- USB -----
        -device qemu-xhci,id=xhci
        #-device usb-tablet
		-device usb-mouse
		-device usb-kbd
		#-device usb-storage,bus=xhci.0,drive=pendrive,removable=on
		#-drive id=pendrive,file=${USB_IMG},format=raw,if=none
		#-device virtio-keyboard-pci
        # ----- NET -----
        -netdev tap,id=n0,ifname=tap0,script=no,downscript=no
        -device rtl8139,netdev=n0
        # ----- Debug -----
        -serial stdio
        -no-reboot
        -d int,cpu_reset
        -D ${CMAKE_SOURCE_DIR}/qemu.log

        #-d trace:usb_xhci_*
        #-d usb,guest_errors,int
        #-trace usb_xhci_*
        -s
)

add_custom_target(run
        COMMENT "launching QEMU"
        COMMAND qemu-system-x86_64 ${QEMU_FLAGS}
        DEPENDS Kernel build
        VERBATIM
)

# ---------------------------------------------------------

# =========================================================
# Building ISO Image

set(ISO_IMG "${CMAKE_SOURCE_DIR}/Images/PLumOS.iso")
set(ISO_DIR "${CMAKE_SOURCE_DIR}/Images/iso_dir")

add_custom_target(iso
        COMMENT "Building ISO image"

        COMMAND rm -rf ${ISO_DIR}
        COMMAND mkdir -p ${ISO_DIR}/boot/limine ${ISO_DIR}/EFI/BOOT
        COMMAND cp ${LIMINE}/BOOTX64.EFI                    ${ISO_DIR}/EFI/BOOT/BOOTX64.EFI # UEFI
        COMMAND cp ${LIMINE}/limine-uefi-cd.bin             ${ISO_DIR}/boot/limine/limine-uefi-cd.bin # UEFI
        COMMAND cp ${LIMINE}/limine-bios-cd.bin             ${ISO_DIR}/boot/limine/limine-bios-cd.bin # BIOS
        COMMAND cp ${LIMINE}/limine-bios.sys                ${ISO_DIR}/boot/limine/limine-bios.sys # BIOS
        COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/Kernel.elf   ${ISO_DIR}/Kernel.elf
        COMMAND cp ${CMAKE_SOURCE_DIR}/limine.conf          ${ISO_DIR}/boot/limine/limine.conf

        COMMAND xorriso -as mkisofs
		-b boot/limine/limine-bios-cd.bin
		-no-emul-boot -boot-load-size 4 -boot-info-table
		--efi-boot boot/limine/limine-uefi-cd.bin
		-efi-boot-part --efi-boot-image
		--protective-msdos-label
		${ISO_DIR} -o ${ISO_IMG}
        COMMAND ${LIMINE}/limine bios-install ${ISO_IMG}

        DEPENDS Kernel
        VERBATIM
)

# ---------------------------------------------------------