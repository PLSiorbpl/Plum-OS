#!/usr/bin/env bash
set -Eeuo pipefail

IMAGE="../Images/disk.img"
MOUNT_DIR="../Images/mnt"

LOOP=""

cleanup() {
    set +e
    sync
    mountpoint -q "$MOUNT_DIR/fat32" && umount "$MOUNT_DIR/fat32"
    mountpoint -q "$MOUNT_DIR/ext2" && umount "$MOUNT_DIR/ext2"

    if [[ -n "$LOOP" ]]; then
        losetup -d "$LOOP"
    fi
}

trap cleanup EXIT

echo "[*] Creating disk image..."

if [[ -e "$IMAGE" ]]; then
    echo "[!] $IMAGE already exists!"
    exit 1
fi

mkdir -p "$MOUNT_DIR/fat32" "$MOUNT_DIR/ext2"

# 512 MiB image
truncate -s 256M "$IMAGE"

# GPT
parted -s "$IMAGE" mklabel gpt
parted -s "$IMAGE" mkpart FAT32 fat32 1MiB 128MiB
parted -s "$IMAGE" mkpart EXT2 ext2 128MiB 100%

# Loop device with partition scanning
LOOP=$(losetup --find --show --partscan "$IMAGE")
udevadm settle

P1="${LOOP}p1"
P2="${LOOP}p2"

echo "[*] Formatting partitions..."

mkfs.fat -F 32 -n PLUM_BOOT "$P1"
mkfs.ext2 -F -L PLUM_DATA "$P2"

echo "[*] Mounting..."

mount "$P1" "$MOUNT_DIR/fat32"
mount "$P2" "$MOUNT_DIR/ext2"

# FAT32 contents
mkdir -p "$MOUNT_DIR/fat32"/{boot,home,user,etc}

echo "LABEL=PLUM_BOOT /boot vfat defaults 0 2" \
    > "$MOUNT_DIR/fat32/fstab"

echo "Welcome to Plum-OS!" \
    > "$MOUNT_DIR/fat32/README.txt"

echo "FAT32 test file" \
    > "$MOUNT_DIR/fat32/home/user.txt"

mkdir -p "$MOUNT_DIR/fat32/home/user"
echo "Example document" \
    > "$MOUNT_DIR/fat32/home/user/document.txt"

# EXT2 contents (without fstab)
mkdir -p "$MOUNT_DIR/ext2"/{home,user,documents}

echo "Welcome to ext2!" \
    > "$MOUNT_DIR/ext2/README.txt"

echo "EXT2 test file" \
    > "$MOUNT_DIR/ext2/home/test.txt"

mkdir -p "$MOUNT_DIR/ext2/home/user"
echo "Example ext2 document" \
    > "$MOUNT_DIR/ext2/home/user/document.txt"

echo "[+] Disk contents created."

sync

echo "[*] Unmounting and detaching..."
umount "$MOUNT_DIR/ext2"
umount "$MOUNT_DIR/fat32"

losetup -d "$LOOP"
LOOP=""

chown "$(logname):$(logname)" "$IMAGE"

echo "[+] Done! Created $IMAGE"