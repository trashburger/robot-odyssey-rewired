#include "zcontexts.h"
#include "filesystem.h"

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>

TinySave::Dict11 ZContexts::tinySave11Dict;

void ZContexts::init() {
    decompressPlain = ZSTD_createDCtx();
    compressTinySave11 = ZSTD_createCCtx();
    decompressTinySave11 = ZSTD_createDCtx();

    CompressedFileInfo::decompress(decompressPlain);
    tinySave11Dict.initFromFilesystem();

    ZSTD_CCtx_loadDictionary_advanced(compressTinySave11, tinySave11Dict.bytes,
                                      sizeof tinySave11Dict.bytes,
                                      ZSTD_dlm_byRef, ZSTD_dct_rawContent);

    ZSTD_CCtx_setParameter(compressTinySave11, ZSTD_c_contentSizeFlag, 0);
    ZSTD_CCtx_setParameter(compressTinySave11, ZSTD_c_checksumFlag, 0);
    ZSTD_CCtx_setParameter(compressTinySave11, ZSTD_c_dictIDFlag, 0);

    // Compression level is a CPU and memory vs space tradeoff.
    // This can be changed without breaking format compatibility.
    ZSTD_CCtx_setParameter(compressTinySave11, ZSTD_c_strategy, ZSTD_btultra2);
    ZSTD_CCtx_setParameter(compressTinySave11, ZSTD_c_compressionLevel, 18);

    ZSTD_DCtx_loadDictionary_advanced(
        decompressTinySave11, tinySave11Dict.bytes, sizeof tinySave11Dict.bytes,
        ZSTD_dlm_byRef, ZSTD_dct_rawContent);
}

ZContexts const &ZContexts::get() {
    static ZContexts instance;
    static bool initialized = false;
    if (!initialized) {
        // Note that get() will be re-entered during init(), and expect to have
        // access to the partially finished object
        initialized = true;
        instance.init();
    }
    return instance;
}
