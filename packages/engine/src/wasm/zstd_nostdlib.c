#include "nostdlib.h"

#define ZSTD_memcpy(d, s, l) memcpy((d), (s), (l))
#define ZSTD_memmove(d, s, l) memmove((d), (s), (l))
#define ZSTD_memset(p, v, l) memset((p), (v), (l))

// Implement allocation but not free, from a fixed size heap.
#define ZSTD_HEAP_SIZE (4 * 1024 * 1024)

static uint8_t __attribute__((aligned(16))) heap[ZSTD_HEAP_SIZE];
static size_t heap_used = 0;

size_t WASM_EXPORT(ZSTD_heapUsed)() { return heap_used; }

static void *ZSTD_malloc(size_t size) {
    void *result = (void *)&heap[heap_used];
    heap_used += (size + 0xf) & ~0xf;
    if (heap_used > ZSTD_HEAP_SIZE) {
        abort();
    }
    return result;
}

static void ZSTD_free(void *p) {
    (void)p;
    abort();
}

static void *ZSTD_calloc(size_t n, size_t size) {
    if (n > SIZE_MAX / size) {
        abort();
    }
    size_t total_size = n * size;
    void *result = ZSTD_malloc(total_size);
    ZSTD_memset(result, 0, total_size);
    return result;
}

#define ZSTD_DEPS_COMMON
#define ZSTD_DEPS_MALLOC

#define DEBUGLEVEL 0
#define ZSTD_LEGACY_SUPPORT 0
#define ZSTD_TRACE 0
#define ZSTD_DISABLE_ASM 1

#include <common/xxhash.h>
#include <common/zstd_deps.h>

// We don't use checksums but XXH64 can't be fully disabled at compile time
XXH64_hash_t XXH64(const void *input, size_t length, XXH64_hash_t seed) {
    (void)input;
    (void)length;
    (void)seed;
    abort();
}

XXH64_hash_t XXH64_digest(const XXH64_state_t *statePtr) {
    (void)statePtr;
    abort();
}

XXH_errorcode XXH64_update(XXH64_state_t *statePtr, const void *input,
                           size_t length) {
    (void)statePtr;
    (void)input;
    (void)length;
    return XXH_OK;
}

XXH_errorcode XXH64_reset(XXH64_state_t *statePtr, XXH64_hash_t seed) {
    (void)statePtr;
    (void)seed;
    return XXH_OK;
}

#include <common/debug.c>
#include <common/entropy_common.c>
#include <common/error_private.c>
#include <common/fse_decompress.c>
#include <common/pool.c>
#include <common/threading.c>
#include <common/zstd_common.c>

#include <compress/fse_compress.c>
#include <compress/hist.c>
#include <compress/huf_compress.c>
#include <compress/zstd_compress.c>
#include <compress/zstd_compress_literals.c>
#include <compress/zstd_compress_sequences.c>
#include <compress/zstd_compress_superblock.c>
#include <compress/zstd_double_fast.c>
#include <compress/zstd_fast.c>
#include <compress/zstd_lazy.c>
#include <compress/zstd_ldm.c>
#include <compress/zstd_opt.c>
#include <compress/zstd_preSplit.c>

#include <decompress/huf_decompress.c>
#include <decompress/zstd_ddict.c>
#include <decompress/zstd_decompress.c>
#include <decompress/zstd_decompress_block.c>
