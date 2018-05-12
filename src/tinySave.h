#pragma once
#include <stdint.h>
#include "filesystem.h"


// Holds a minified saved game file
class TinySave {
public:
    uint8_t buffer[DOSFilesystem::MAX_FILESIZE];
    uint32_t size;

    void compress(const FileInfo& src);
    bool decompress(FileInfo& dest);

private:
    struct {
        uint32_t size;
        union {
            uint8_t buffer[DOSFilesystem::MAX_FILESIZE];
            ROSavedGame game;
        };
    } unpacked;
};
