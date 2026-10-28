#include "DeviceFilesystem.hpp"

using namespace vfs;

namespace fs {
    bool DeviceFileSystem::register_device(const char *name, FileObject *device) {
        if (!name || !device)
            return false;

        for (const auto& entry : devices) {
            if (std::strcmp(entry.name, name) == 0)
                return false;
        }

        devices.push_back({name, device});
        return true;
    }

    FileObject *DeviceFileSystem::open(const char *path, int flags) {
        if (!path)
            return nullptr;

        for (const auto& entry : devices) {
            if (std::strcmp(entry.name, path) == 0) {
                return entry.device;
            }
        }

        return nullptr;
    }
}
