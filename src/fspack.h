#pragma once

struct FileInfo {
    const char *name;
    const uint8_t *data;
    uint32_t data_size;

    static const FileInfo* lookup(const char* name);
};
