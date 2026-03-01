#include "filesystem.h"
#include "nostdlib.h"
#include "sbt86.h"
#include "zcontexts.h"
#include <zstd.h>

DOSFilesystem::DOSFilesystem() { reset(); }

void DOSFilesystem::reset() {
    /*
     * This isn't intended to be a complete description, see notes/roData.h
     * The C++ side of the game only needs to know enough to choose defaults.
     */
    struct ROJoyfile {
        uint8_t joystick_enabled;
        uint16_t joystick_io_port;
        uint8_t x_center;
        uint8_t y_center;
        uint8_t xplus_divisor;
        uint8_t yplus_divisor;
        uint8_t xminus_divisor;
        uint8_t yminus_divisor;
        uint8_t cheat_control;
        uint16_t debug_control;
        uint8_t disk_drive_id;
        uint8_t joyfile_D;
        uint8_t joyfile_E;
        uint8_t joyfile_F;
    } __attribute__((packed));

    static_assert(sizeof(ROJoyfile) == sizeof(config.buffer),
                  "config buffer size");

    static const uint16_t DEFAULT_JOYSTICK_PORT = 0x201;
    static const uint8_t DEFAULT_JOYSTICK_CENTER = 0x80;
    static const uint8_t DEFAULT_JOYSTICK_DIVISOR = 0x01;
    static const uint16_t DEBUG_NORMAL = 0x238;
    static const uint8_t DRIVE_A = 0x01;

    static const ROJoyfile default_config = {
        .joystick_enabled = true,
        .joystick_io_port = DEFAULT_JOYSTICK_PORT,
        .x_center = DEFAULT_JOYSTICK_CENTER,
        .y_center = DEFAULT_JOYSTICK_CENTER,
        .xplus_divisor = DEFAULT_JOYSTICK_DIVISOR,
        .yplus_divisor = DEFAULT_JOYSTICK_DIVISOR,
        .xminus_divisor = DEFAULT_JOYSTICK_DIVISOR,
        .yminus_divisor = DEFAULT_JOYSTICK_DIVISOR,
        .cheat_control = 0,
        .debug_control = DEBUG_NORMAL,
        .disk_drive_id = DRIVE_A,
        .joyfile_D = 0,
        .joyfile_E = 0,
        .joyfile_F = 0};

    config.file.data = config.buffer;
    config.file.size = sizeof config.buffer;
    memcpy(config.buffer, &default_config, sizeof config.buffer);

    save.file.data = save.buffer;
    save.file.size = 0;

    memset(openFiles, 0, sizeof openFiles);
}

int DOSFilesystem::open(IOBuffer *io, const char *name) {
    int fd = allocateFD(io);
    const FileInfo *file;

    if (!strcasecmp(name, SBT_SAVE_FILE_NAME)) {
        save.openForWrite = false;
        file = &save.file;

    } else if (!strcasecmp(name, SBT_JOYFILE)) {
        file = &config.file;

    } else {
        file = FileInfo::lookup(name);
        if (!file) {
            io->log("File not found");
            return -1;
        }
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

int DOSFilesystem::create(IOBuffer *io, const char *name) {
    int fd = allocateFD(io);
    const FileInfo *file;

    if (!strcasecmp(name, SBT_SAVE_FILE_NAME)) {
        save.file.size = 0;
        save.openForWrite = true;
        file = &save.file;

    } else {
        io->log("Can't create file");
        return -1;
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

void DOSFilesystem::close(IOBuffer *io, uint16_t fd) {
    if (fd >= MAX_OPEN_FILES) {
        io->error("Bad file descriptor");
    }

    if (openFiles[fd] == &save.file && save.openForWrite) {
        io->saveFileClosed();
    }

    openFiles[fd] = 0;
}

uint16_t DOSFilesystem::read(IOBuffer *io, uint16_t fd, uint8_t *buffer,
                             uint16_t length) {
    const FileInfo *file = openFiles[fd];

    if (fd >= MAX_OPEN_FILES) {
        io->error("Bad file descriptor");
    }
    if (!file) {
        io->error("File not open");
    }

    const uint32_t offset = fileOffsets[fd];
    const uint32_t remaining = file->size - offset;
    if (remaining > MAX_FILESIZE) {
        io->error("File too large");
    }
    const uint32_t actual_length = length < remaining ? length : remaining;
    memcpy(buffer, file->data + offset, actual_length);

    fileOffsets[fd] = offset + actual_length;
    return (uint16_t)actual_length;
}

uint16_t DOSFilesystem::write(IOBuffer *io, uint16_t fd, const uint8_t *buffer,
                              uint16_t length) {
    const FileInfo *file = openFiles[fd];

    if (fd >= MAX_OPEN_FILES) {
        io->error("Bad file descriptor");
    }
    if (file != &save.file) {
        io->error("File not open");
    }

    const uint32_t offset = fileOffsets[fd];
    const uint32_t remaining = file->size - offset;
    if (remaining > sizeof save.buffer) {
        io->error("File too large");
    }
    uint32_t actual_length = length < remaining ? length : remaining;
    memcpy(save.buffer + offset, buffer, actual_length);

    save.file.size = fileOffsets[fd] = offset + actual_length;
    return (uint16_t)actual_length;
}

uint16_t DOSFilesystem::allocateFD(IOBuffer *io) {
    unsigned fd = 0;

    while (openFiles[fd]) {
        fd++;
        if (fd >= MAX_OPEN_FILES) {
            io->error("Too many open files");
        }
    }

    return (uint16_t)fd;
}

void CompressedFileInfo::decompress(struct ZSTD_DCtx_s *ctx) {
    size_t result =
        ZSTD_decompressDCtx(ctx, unpacked, unpackedSize, packed, packedSize);
    if (result != unpackedSize) {
        abort();
    }

    const CompressedFileInfo *indexItem = index;
    FileInfo *unpackedItem = unpackedIndex;
    while (indexItem->name) {
        unpackedItem->name = indexItem->name;
        unpackedItem->data = unpacked + indexItem->offset;
        unpackedItem->size = indexItem->size;
        unpackedItem++;
        indexItem++;
    }
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
    // Decompress files if necessary, as part of setting up the zstd contexts
    ZContexts::get();

    const FileInfo *unpackedIndex = CompressedFileInfo::getUnpackedIndex();
    if (!unpackedIndex->name) {
        abort();
    }
    return unpackedIndex;
}

namespace TinySave {

// Versioning the save files, so we can change the compression
// or otherwise break compatibility later.
enum class Version : uint8_t {
    CURRENT = 0x11,
    // Currently we only generate or support one version.
};

size_t compress(const FileInfo &src, uint8_t *dest) {
    dest[0] = uint8_t(Version::CURRENT);
    size_t result =
        ZSTD_compress2(ZContexts::get().compressTinySave11, dest + 1,
                       DOSFilesystem::MAX_FILESIZE - 1, src.data, src.size);
    return ZSTD_isError(result) ? 0 : 1 + result;
}

bool decompress(FileInfo &dest, const uint8_t *src, size_t srcLength) {
    if (srcLength < 1) {
        // No version header
        return false;
    }
    if (src[0] != uint8_t(Version::CURRENT)) {
        // No other versions supported
        return false;
    }

    size_t result = ZSTD_decompressDCtx(
        ZContexts::get().decompressTinySave11, (uint8_t *)dest.data,
        DOSFilesystem::MAX_FILESIZE, src + 1, srcLength - 1);

    dest.size = ZSTD_isError(result) ? 0 : result;
    return !ZSTD_isError(result);
}

void Dict11::initFromFilesystem() {
    // The contents of the dictionary must not change at all,
    // or we break savegame compatibility completely! If we want
    // to use a new dictionary later, we could bump SaveVersion.

    constexpr size_t total_expected_size = 57791;
    static_assert(total_expected_size == sizeof bytes,
                  "Dictionary size and contents must not change");
    size_t offset = 0;

    static const char *files[] = {
        // Built-in loadable chips
        "4BITCNTR.CSV",
        "STEREO.CSV",
        "RSFLOP.CSV",
        "ONESHOT.CSV",
        "COUNTTON.CSV",
        "ADDER.CSV",
        "CLOCK.CSV",
        "DELAY.CSV",
        "BUS.CSV",
        "WALLHUG.CSV",

        // World overlays for the game
        "STREET.WLD",
        "SUBWAY.WLD",
        "TOWN.WLD",
        "COMP.WLD",

        // Chips used in initial game world
        "COUNTTON.CHP",
        "WALLHUG.CHP",
        "COUNTTON.PIN",
        "WALLHUG.PIN",

        // Initial world for the lab
        "LAB.WOR",

        // Initial world for the game
        "SEWER.WOR",
        "SEWER.CIR",
    };

    for (size_t i = 0; i < sizeof files / sizeof files[0]; ++i) {
        const FileInfo *file = FileInfo::lookup(files[i]);
        if (!file) {
            abort();
        }

        // Trim trailing zeroes
        uint32_t size = file->size;
        while (size && file->data[size - 1] == 0) {
            size--;
        }

        // Append to dict
        memcpy(bytes + offset, file->data, size);
        offset += size;
    }
    if (offset != total_expected_size) {
        abort();
    }
}
}; // namespace TinySave

size_t WASM_EXPORT(FileInfo_sizeof)() { return sizeof(FileInfo); }

const FileInfo *WASM_EXPORT(FileInfo_getAllFiles)() {
    return FileInfo::getAllFiles();
}

const char *WASM_EXPORT(FileInfo_name)(const FileInfo *fileInfo) {
    return fileInfo->name;
}

const uint8_t *WASM_EXPORT(FileInfo_data)(const FileInfo *fileInfo) {
    return fileInfo->data;
}

size_t WASM_EXPORT(FileInfo_size)(const FileInfo *fileInfo) {
    return fileInfo->size;
}

size_t WASM_EXPORT(TinySave_compress)(const FileInfo *src, uint8_t *dest) {
    return TinySave::compress(*src, dest);
}

bool WASM_EXPORT(TinySave_decompress)(FileInfo *dest, const uint8_t *src,
                                      size_t srcLength) {
    return TinySave::decompress(*dest, src, srcLength);
}
