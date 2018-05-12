#include <emscripten.h>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include "sbt86.h"
#include "filesystem.h"

static const bool verbose_filesystem_info = false;

DOSFilesystem::DOSFilesystem()
{
    reset();
}

void DOSFilesystem::reset()
{
    memset(openFiles, 0, sizeof openFiles);
}

int DOSFilesystem::open(const char *name)
{
    int fd = allocateFD();
    const FileInfo *file;

    if (verbose_filesystem_info) {
        printf("FILE, opening '%s'\n", name);
    }

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {
        /*
         * Save file
         */

        saveFileInfo.data = save.buffer;
        saveFileInfo.data_size = save.size;
        save.writeMode = false;
        file = &saveFileInfo;

    } else if (!strcmp(name, SBT_JOYFILE)) {
        /*
         * Joystick/configuration file
         */

        joyFileInfo.data = (const uint8_t*) &joyfile;
        joyFileInfo.data_size = sizeof joyfile;
        file = &joyFileInfo;

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

int DOSFilesystem::create(const char *name)
{
    int fd = allocateFD();
    const FileInfo *file;

    if (verbose_filesystem_info) {
        printf("FILE, creating '%s'\n", name);
    }

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {

        saveFileInfo.data = save.buffer;
        saveFileInfo.data_size = 0;
        save.size = 0;
        save.writeMode = true;
        file = &saveFileInfo;

    } else {
        fprintf(stderr, "FILE, failed to open '%s' for writing\n", name);
        return -1;
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

void DOSFilesystem::close(uint16_t fd)
{
    assert(fd < MAX_OPEN_FILES && "Closing an invalid file descriptor");
    assert(openFiles[fd] && "Closing a file which is not open");

    if (openFiles[fd] == &saveFileInfo && save.writeMode) {
        EM_ASM_({
            Module.onSaveFileWrite(HEAPU8.subarray($0, $0 + $1));
        }, save.buffer, save.size);
    }

    openFiles[fd] = 0;
}

uint16_t DOSFilesystem::read(uint16_t fd, void *buffer, uint16_t length)
{
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Reading an invalid file descriptor");
    assert(file && "Reading a file which is not open");

    uint32_t offset = fileOffsets[fd];
    uint16_t actual_length = std::min<unsigned>(length, file->data_size - offset);

    if (verbose_filesystem_info) {
        printf("FILE, read %d(%d) bytes at %d\n", length, actual_length, offset);
    }
    memcpy(buffer, file->data + offset, actual_length);

    fileOffsets[fd] += actual_length;
    return actual_length;
}

uint16_t DOSFilesystem::write(uint16_t fd, const void *buffer, uint16_t length)
{
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Writing an invalid file descriptor");
    assert(file && "Writing a file which is not open");
    assert(file == &saveFileInfo && "Writing a file that isn't the saved game file");

    uint32_t offset = fileOffsets[fd];
    uint16_t actual_length = std::min<unsigned>(length, sizeof save.buffer - offset);

    if (verbose_filesystem_info) {
        printf("FILE, write %d(%d) bytes at %d\n", length, actual_length, offset);
    }
    memcpy(save.buffer + offset, buffer, actual_length);

    fileOffsets[fd] += actual_length;
    save.size = fileOffsets[fd];
    return actual_length;
}

uint16_t DOSFilesystem::allocateFD()
{
    uint16_t fd = 0;

    while (openFiles[fd]) {
        fd++;
        if (fd >= MAX_OPEN_FILES) {
            assert(0 && "Too many open files");
        }
    }

    return fd;
}
