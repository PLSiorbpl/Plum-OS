#!/usr/bin/env bash
set -Eeuo pipefail

echo "======================================"
echo "       Plum-OS Dependency Setup"
echo "======================================"

if [[ $EUID -ne 0 ]]; then
    echo "[!] Run this script with sudo:"
    echo "    sudo ./install_dependencies.sh"
    exit 1
fi

export DEBIAN_FRONTEND=noninteractive

echo "[1/4] Updating package lists..."
apt-get update

echo "[2/4] Installing development tools..."

apt-get install -y \
    build-essential \
    gcc \
    g++ \
    make \
    nasm \
    lld \
    git \
    curl \
    ca-certificates \
    gnupg \
    mtools \
    dosfstools \
    e2fsprogs \
    parted \
    util-linux \
    xorriso \
    qemu-system-x86 \
    qemu-system-gui \
    qemu-utils \
    ovmf \
    iproute2 \
    pkg-config

echo "[3/4] Checking CMake version..."

CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')

if ! dpkg --compare-versions "$CMAKE_VERSION" ge "3.28"; then
    echo "[!] CMake $CMAKE_VERSION is too old."
    echo "[*] Adding official Kitware repository..."

    CODENAME=$(. /etc/os-release && echo "$VERSION_CODENAME")

    curl -fsSL https://apt.kitware.com/keys/kitware-archive-latest.asc \
        | gpg --dearmor --yes \
        -o /usr/share/keyrings/kitware-archive-keyring.gpg

    echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ $CODENAME main" \
        > /etc/apt/sources.list.d/kitware.list

    apt-get update
    apt-get install -y cmake
fi

CMAKE_VERSION=$(cmake --version | head -n1 | awk '{print $3}')

if ! dpkg --compare-versions "$CMAKE_VERSION" ge "3.28"; then
    echo "[ERROR] Could not install CMake >= 3.28."
    exit 1
fi

echo "[4/4] Checking installed tools..."

for tool in gcc g++ nasm ld.lld cmake qemu-system-x86_64 \
            mformat mcopy mmd xorriso mkfs.fat mkfs.ext2 parted; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "[ERROR] Missing tool: $tool"
        exit 1
    fi
done

echo ""
echo "======================================"
echo "     Plum-OS setup completed!"
echo "======================================"
echo ""
echo "CMake: $(cmake --version | head -n1)"
echo "GCC:   $(gcc --version | head -n1)"
echo "G++:   $(g++ --version | head -n1)"
echo "NASM:  $(nasm -v)"
echo "QEMU:  $(qemu-system-x86_64 --version | head -n1)"
echo ""
echo "You can now build Plum-OS."
