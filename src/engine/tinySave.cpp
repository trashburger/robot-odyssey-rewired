#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

#include "tinySave.h"

// Versioning the save files, so we can change the compression
// or otherwise break compatibility later.
enum SaveVersion {
    CURRENT_SAVE_VERSION = 0x11,
    // Currently we only generate or support one version.
};

TinySave::TinySave()
    : size(0)
{
    initDictionary();

    cctx = ZSTD_createCCtx();
    ZSTD_CCtx_loadDictionary_advanced(cctx, &dict[0], dict.size(), ZSTD_dlm_byRef, ZSTD_dct_rawContent);

    ZSTD_CCtx_setParameter(cctx, ZSTD_c_contentSizeFlag, 0);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 0);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_dictIDFlag, 0);

    // Compression level is a CPU and memory vs space tradeoff.
    // This can be changed without breaking format compatibility.
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_strategy, ZSTD_btultra2);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 18);

    dctx = ZSTD_createDCtx();
    ZSTD_DCtx_loadDictionary_advanced(dctx, &dict[0], dict.size(), ZSTD_dlm_byRef, ZSTD_dct_rawContent);
}

TinySave::~TinySave()
{
    ZSTD_freeCCtx(cctx);
    ZSTD_freeDCtx(dctx);
}

void TinySave::compress(const FileInfo& src)
{
    buffer[0] = CURRENT_SAVE_VERSION;
    size_t result = ZSTD_compress2(cctx, buffer + 1, sizeof buffer - 1, src.data, src.size);
    size = ZSTD_isError(result) ? 0 : 1 + result;
}

bool TinySave::decompress(FileInfo& dest)
{
    if (size < 1) {
        // No version header
        return false;
    }
    if (buffer[0] != CURRENT_SAVE_VERSION) {
        // No other versions supported
        return false;
    }

    size_t result = ZSTD_decompressDCtx(dctx, (uint8_t*) dest.data, DOSFilesystem::MAX_FILESIZE, buffer + 1, size - 1);

    dest.size = ZSTD_isError(result) ? 0 : result;
    return !ZSTD_isError(result);
}

const std::vector<uint8_t>& TinySave::getCompressionDictionary()
{
    return dict;
}

void TinySave::initDictionary()
{
    // The contents of the dictionary must not change at all,
    // or we break savegame compatibility completely! If we want
    // to use a new dictionary later, we could bump SaveVersion.

    const size_t total_expected_size = 57791;

    static const char *files[] = {
        // Built-in loadable chips
        "4bitcntr.csv",
        "stereo.csv",
        "rsflop.csv",
        "oneshot.csv",
        "countton.csv",
        "adder.csv",
        "clock.csv",
        "delay.csv",
        "bus.csv",
        "wallhug.csv",

        // World overlays for the game
        "street.wld",
        "subway.wld",
        "town.wld",
        "comp.wld",

        // Chips used in initial game world
        "countton.chp",
        "wallhug.chp",
        "countton.pin",
        "wallhug.pin",

        // Initial world for the lab
        "lab.wor",

        // Initial world for the game
        "sewer.wor",
        "sewer.cir",
    };

    assert(dict.empty());
    for (size_t i = 0; i < sizeof files / sizeof files[0]; ++i) {
        const FileInfo *file = FileInfo::lookup(files[i]);
        assert(file);

        // Trim trailing zeroes
        uint32_t size = file->size;
        while (size && file->data[size - 1] == 0) size--;

        // Append to dict
        dict.insert(dict.end(), file->data, file->data + size);
    }
    assert(dict.size() == total_expected_size);
}
