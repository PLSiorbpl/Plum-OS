#include "DeviceFilesystem.hpp"

using namespace vfs;

namespace fs {
    void DeviceFileSystem::close(const FileObject *file) {
        if (!file) return;
        for (int i = 0; i < devices.size(); i++) {
            if (devices[i]->device == file) {
                devices.erase(i);
                return;
            }
        }
    }

    bool DeviceFileSystem::register_device(const char *name, FileObject *device) {
        if (!name || !device)
            return false;

        for (const auto& entry : devices) {
            if (std::strcmp(entry->name, name) == 0)
                return false;
        }

        auto ptr = std::make_unique<BlockDevice>();
        ptr->name = name;
        ptr->device = device;
        devices.push_back(std::move(ptr));
        return true;
    }

    FileObject *DeviceFileSystem::open(const char *path, int flags) {
        if (!path)
            return nullptr;

        for (const auto& entry : devices) {
            if (std::strcmp(entry->name, path) == 0) {
                return entry->device;
            }
        }

        return nullptr;
    }
}
