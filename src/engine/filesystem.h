#pragma once
#include <stdint.h>
#include "roData.h"

struct FileInfo {
    const char *name;
    const uint8_t *data;
    uint32_t size;

    static const FileInfo* lookup(const char* name);
    static const FileInfo* getAllFiles();
};

struct CompressedFileInfo {
    const char *name;
    uint32_t offset;
    uint32_t size;

    static const FileInfo* lazyDecompress();

private:
    static const uint8_t* const packed;
    static uint8_t* const unpacked;
    static const uint32_t packed_size;
    static const uint32_t unpacked_size;

    static const CompressedFileInfo * const index;
    static FileInfo * const cache;
};

class DOSFilesystem
{
public:
    static const unsigned MAX_OPEN_FILES = 16;
    static const unsigned MAX_FILESIZE = 0x10000;

    DOSFilesystem();
    void reset();

    int open(const char *name);
    int create(const char *name);
    void close(uint16_t fd);
    uint16_t read(uint16_t fd, void *buffer, uint16_t length);
    uint16_t write(uint16_t fd, const void *buffer, uint16_t length);

    struct {
        FileInfo file;
        ROJoyfile joyfile;
    } config;

    struct {
        FileInfo file;
        bool openForWrite;
        uint8_t buffer[MAX_FILESIZE];

        inline bool isChip() {
            return file.size == 1333;
        }

        inline bool isGame() {
            return file.size == sizeof(ROSavedGame);
        }

        inline ROSavedGame& asGame() {
            return *reinterpret_cast<ROSavedGame*>(buffer);
        }
    } save;

private:
    uint16_t allocateFD();

    const FileInfo* openFiles[MAX_OPEN_FILES];
    uint32_t fileOffsets[MAX_OPEN_FILES];
};
