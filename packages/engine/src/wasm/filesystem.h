#pragma once
#include "iobuffer.h"
#include "nostdlib.h"

struct FileInfo {
    const char *name;
    const uint8_t *data;
    uint32_t size;

    static const FileInfo *lookup(const char *name);
    static const FileInfo *getAllFiles();
};

struct CompressedFileInfo {
    const char *name;
    uint32_t offset;
    uint32_t size;

    static void decompress(struct ZSTD_DCtx_s *ctx);
    static const FileInfo *getUnpackedIndex() { return unpackedIndex; }

  private:
    static const uint8_t *const packed;
    static uint8_t *const unpacked;
    static const uint32_t packedSize;
    static const uint32_t unpackedSize;

    static const CompressedFileInfo *const index;
    static FileInfo *const unpackedIndex;
};

class DOSFilesystem {
  public:
    static const unsigned MAX_OPEN_FILES = 16;
    static const unsigned MAX_FILESIZE = 0x10000;

    DOSFilesystem();
    void reset();

    int open(IOBuffer *io, const char *name);
    int create(IOBuffer *io, const char *name);
    void close(IOBuffer *io, uint16_t fd);
    uint16_t read(IOBuffer *io, uint16_t fd, uint8_t *buffer, uint16_t length);
    uint16_t write(IOBuffer *io, uint16_t fd, const uint8_t *buffer,
                   uint16_t length);

    struct {
        FileInfo file;
        uint8_t buffer[16];
    } config;

    struct {
        FileInfo file;
        bool openForWrite;
        uint8_t buffer[MAX_FILESIZE];
    } save;

  private:
    uint16_t allocateFD(IOBuffer *io);

    const FileInfo *openFiles[MAX_OPEN_FILES];
    uint32_t fileOffsets[MAX_OPEN_FILES];
};

namespace TinySave {

size_t compress(const FileInfo &src, uint8_t *dest);
bool decompress(FileInfo &dest, const uint8_t *src, size_t srcLength);

struct Dict11 {
    uint8_t bytes[57791];
    void initFromFilesystem();
};

}; // namespace TinySave
