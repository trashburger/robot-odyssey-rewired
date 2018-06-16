#pragma once
#include <stdint.h>
#include <zstd.h>
#include <vector>
#include "filesystem.h"


// Holds a minified saved game file
class TinySave {
public:
    TinySave();
    ~TinySave();

    uint8_t buffer[DOSFilesystem::MAX_FILESIZE];
    uint32_t size;

    void compress(const FileInfo& src);
    bool decompress(FileInfo& dest);

    const std::vector<uint8_t>& getCompressionDictionary();

private:
    ZSTD_CCtx *cctx;
    ZSTD_DCtx *dctx;
    ZSTD_CDict *cdict;
    ZSTD_DDict *ddict;
    std::vector<uint8_t> dict;

    void initDictionary();
};
