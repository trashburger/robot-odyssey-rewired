#include "filesystem.h"
#include "sbt86.h"
#include <algorithm>
#include <emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <zstd.h>

static const bool verbose_filesystem_info = false;

DOSFilesystem::DOSFilesystem() {
    // ROJoyfile constructor sets up a default configuration
    config.file.data = (uint8_t *)&config.joyfile;
    config.file.size = sizeof config.joyfile;

    memset(&save, 0, sizeof save);
    save.file.data = save.buffer;

    reset();
}

void DOSFilesystem::reset() { memset(openFiles, 0, sizeof openFiles); }

int DOSFilesystem::open(const char *name) {
    int fd = allocateFD();
    const FileInfo *file;

    if (verbose_filesystem_info) {
        printf("FILE, opening '%s'\n", name);
    }

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {
        /*
         * Save file
         */

        save.openForWrite = false;
        file = &save.file;

    } else if (!strcmp(name, SBT_JOYFILE)) {
        /*
         * Joystick/configuration file
         */

        file = &config.file;

    } else {
        /*
         * Game file
         */

        file = FileInfo::lookup(name);
        if (!file) {
            fprintf(stderr, "Failed to open file '%s'\n", name);
            return -1;
        }
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

int DOSFilesystem::create(const char *name) {
    int fd = allocateFD();
    const FileInfo *file;

    if (verbose_filesystem_info) {
        printf("FILE, creating '%s'\n", name);
    }

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {

        save.file.size = 0;
        save.openForWrite = true;
        file = &save.file;

    } else {
        fprintf(stderr, "FILE, failed to open '%s' for writing\n", name);
        return -1;
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

void DOSFilesystem::close(uint16_t fd) {
    assert(fd < MAX_OPEN_FILES && "Closing an invalid file descriptor");
    assert(openFiles[fd] && "Closing a file which is not open");

    if (openFiles[fd] == &save.file && save.openForWrite) {
        EM_ASM(Module.onSaveFileWrite(););
    }

    openFiles[fd] = 0;
}

uint16_t DOSFilesystem::read(uint16_t fd, void *buffer, uint16_t length) {
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Reading an invalid file descriptor");
    assert(file && "Reading a file which is not open");

    uint32_t offset = fileOffsets[fd];
    uint16_t actual_length = std::min<unsigned>(length, file->size - offset);

    if (verbose_filesystem_info) {
        printf("FILE, read %d(%d) bytes at %d\n", length, actual_length,
               offset);
    }
    memcpy(buffer, file->data + offset, actual_length);

    fileOffsets[fd] += actual_length;
    return actual_length;
}

uint16_t DOSFilesystem::write(uint16_t fd, const void *buffer,
                              uint16_t length) {
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Writing an invalid file descriptor");
    assert(file && "Writing a file which is not open");
    assert(file == &save.file &&
           "Writing a file that isn't the saved game file");

    uint32_t offset = std::min<unsigned>(sizeof save.buffer, fileOffsets[fd]);
    uint16_t actual_length =
        std::min<unsigned>(length, sizeof save.buffer - offset);

    if (verbose_filesystem_info) {
        printf("FILE, write %d(%d) bytes at %d\n", length, actual_length,
               offset);
    }
    memcpy(save.buffer + offset, buffer, actual_length);

    fileOffsets[fd] = offset + actual_length;
    save.file.size = fileOffsets[fd];
    return actual_length;
}

uint16_t DOSFilesystem::allocateFD() {
    uint16_t fd = 0;

    while (openFiles[fd]) {
        fd++;
        if (fd >= MAX_OPEN_FILES) {
            assert(0 && "Too many open files");
        }
    }

    return fd;
}

const FileInfo *CompressedFileInfo::lazyDecompress() {
    // Decompress files on first access
    if (!cache->name) {
        size_t result =
            ZSTD_decompress(unpacked, unpacked_size, packed, packed_size);
        assert(result == unpacked_size);

        const CompressedFileInfo *index_item = index;
        FileInfo *cache_item = cache;
        while (index_item->name) {
            cache_item->name = index_item->name;
            cache_item->data = unpacked + index_item->offset;
            cache_item->size = index_item->size;
            cache_item++;
            index_item++;
        }
    }
    return cache;
}

const FileInfo *FileInfo::lookup(const char *name) {
    for (const FileInfo *cached = getAllFiles(); cached->name; cached++) {
        if (!strcasecmp(name, cached->name)) {
            return cached;
        }
    }
    return nullptr;
}

const FileInfo *FileInfo::getAllFiles() {
    return CompressedFileInfo::lazyDecompress();
}
