#pragma once
#include "filesystem.h"
#include <stdint.h>
#include <vector>
#include <zstd.h>

// Holds a minified saved game file
class TinySave {
  public:
    TinySave();
    ~TinySave();

    uint8_t buffer[DOSFilesystem::MAX_FILESIZE];
    uint32_t size;

    void compress(const FileInfo &src);
    bool decompress(FileInfo &dest);

    const std::vector<uint8_t> &getCompressionDictionary();

  private:
    ZSTD_CCtx *cctx;
    ZSTD_DCtx *dctx;
    std::vector<uint8_t> dict;

    void initDictionary();
};
