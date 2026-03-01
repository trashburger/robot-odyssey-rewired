#pragma once

#include "filesystem.h"

struct ZContexts {
    struct ZSTD_DCtx_s *decompressPlain;
    struct ZSTD_DCtx_s *decompressTinySave11;
    struct ZSTD_CCtx_s *compressTinySave11;
    static TinySave::Dict11 tinySave11Dict;

    static ZContexts const &get();

  private:
    void init();
};
