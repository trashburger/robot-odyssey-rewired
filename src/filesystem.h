#pragma once
#include "roData.h"


struct FileInfo {
    const char *name;
    const uint8_t *data;
    uint32_t data_size;

    static const FileInfo* lookup(const char* name);
};


class DOSFilesystem
{
public:
    DOSFilesystem();
    void reset();

    int open(const char *name);
    int create(const char *name);
    void close(uint16_t fd);
    uint16_t read(uint16_t fd, void *buffer, uint16_t length);
    uint16_t write(uint16_t fd, const void *buffer, uint16_t length);

    ROJoyfile joyfile;

    struct {
        uint32_t size;
        bool writeMode;
        uint8_t buffer[0x10000];
    } save;

private:
    uint16_t allocateFD();

    static const unsigned MAX_OPEN_FILES = 16;
    const FileInfo* openFiles[MAX_OPEN_FILES];
    uint32_t fileOffsets[MAX_OPEN_FILES];
    FileInfo saveFileInfo;
    FileInfo joyFileInfo;
};
