#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

#include "tinySave.h"

// Compression level is a CPU and memory vs space tradeoff.
// This can be changed without breaking format compatibility.
static const int compress_level = 20;

TinySave::TinySave()
    : size(0), cctx(0), dctx(0), cdict(0), ddict(0)
{}

TinySave::~TinySave()
{
    if (cdict) {
        ZSTD_freeCDict(cdict);
    }
    if (ddict) {
        ZSTD_freeDDict(ddict);
    }
    if (cctx) {
        ZSTD_freeCCtx(cctx);
    }
    if (dctx) {
        ZSTD_freeDCtx(dctx);
    }
}

void TinySave::compress(const FileInfo& src)
{
    if (dict.empty()) {
        initDictionary();
    }
    if (!cctx) {
        cctx = ZSTD_createCCtx();
    }
    if (!cdict) {
        cdict = ZSTD_createCDict(&dict[0], dict.size(), compress_level);
    }

    ZSTD_frameParameters fParams = {};
    fParams.contentSizeFlag = 0;
    fParams.checksumFlag = 1;
    fParams.noDictIDFlag = 1;

    size_t result = ZSTD_compress_usingCDict_advanced(cctx, buffer, sizeof buffer,
            src.data, src.size, cdict, fParams);

    size = ZSTD_isError(result) ? 0 : result;
}

bool TinySave::decompress(FileInfo& dest)
{
    if (dict.empty()) {
        initDictionary();
    }
    if (!dctx) {
        dctx = ZSTD_createDCtx();
    }
    if (!ddict) {
        ddict = ZSTD_createDDict(&dict[0], dict.size());
    }

    size_t result = ZSTD_decompress_usingDDict(dctx, (uint8_t*) dest.data,
        DOSFilesystem::MAX_FILESIZE, buffer, size, ddict);

    dest.size = ZSTD_isError(result) ? 0 : result;
    return !ZSTD_isError(result);
}

const std::vector<uint8_t>& TinySave::getCompressionDictionary()
{
    if (dict.empty()) {
        initDictionary();
    }
    return dict;
}

static void addFileToDict(std::vector<uint8_t> &dict, const FileInfo &file)
{
    // Trim trailing zeroes
    uint32_t size = file.size;
    while (size && file.data[size - 1] == 0) size--;

    // Append to dict
    dict.insert(dict.end(), file.data, file.data + size);
}

void TinySave::initDictionary()
{
    // The contents of the dictionary must not change at all,
    // or we break savegame compatibility completely!

    dict.clear();

    // Built-in loadable chips
    extern FileInfo file_4bitcntr_csv;  addFileToDict(dict, file_4bitcntr_csv);
    extern FileInfo file_stereo_csv;    addFileToDict(dict, file_stereo_csv);
    extern FileInfo file_rsflop_csv;    addFileToDict(dict, file_rsflop_csv);
    extern FileInfo file_oneshot_csv;   addFileToDict(dict, file_oneshot_csv);
    extern FileInfo file_countton_csv;  addFileToDict(dict, file_countton_csv);
    extern FileInfo file_adder_csv;     addFileToDict(dict, file_adder_csv);
    extern FileInfo file_clock_csv;     addFileToDict(dict, file_clock_csv);
    extern FileInfo file_delay_csv;     addFileToDict(dict, file_delay_csv);
    extern FileInfo file_bus_csv;       addFileToDict(dict, file_bus_csv);
    extern FileInfo file_wallhug_csv;   addFileToDict(dict, file_wallhug_csv);

    // World overlays for the game
    extern FileInfo file_street_wld;    addFileToDict(dict, file_street_wld);
    extern FileInfo file_subway_wld;    addFileToDict(dict, file_subway_wld);
    extern FileInfo file_town_wld;      addFileToDict(dict, file_town_wld);
    extern FileInfo file_comp_wld;      addFileToDict(dict, file_comp_wld);

    // Chips used in initial game world
    extern FileInfo file_countton_chp;  addFileToDict(dict, file_countton_chp);
    extern FileInfo file_wallhug_chp;   addFileToDict(dict, file_wallhug_chp);
    extern FileInfo file_countton_pin;  addFileToDict(dict, file_countton_pin);
    extern FileInfo file_wallhug_pin;   addFileToDict(dict, file_wallhug_pin);

    // Initial world for the lab
    extern FileInfo file_lab_wor;       addFileToDict(dict, file_lab_wor);

    // Initial world for the game
    extern FileInfo file_sewer_wor;     addFileToDict(dict, file_sewer_wor);
    extern FileInfo file_sewer_cir;     addFileToDict(dict, file_sewer_cir);
}
