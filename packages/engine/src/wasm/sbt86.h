#pragma once
#include "iobuffer.h"
#include "nostdlib.h"

/*
 * Forward declarations
 */

class Engine;

/*
 * Special filenames
 */

#define SBT_SAVE_FILE_NAME "savefile"
#define SBT_JOYFILE "joyfile.joy"

/*
 * SBTAddressId --
 *
 *    These are addresses (data or code) which can be determined
 *    statically by an SBT86 script, then looked up at runtime via SBTProcess.
 */

enum SBTAddressId {
    SBTADDR_ENTRY_FUNC = 0,
    SBTADDR_SAVE_GAME_FUNC,
    SBTADDR_LOAD_CHIP_FUNC,
    SBTADDR_WORLD_DATA,
    SBTADDR_CIRCUIT_DATA,
    SBTADDR_ROBOT_DATA_MAIN,
    SBTADDR_ROBOT_DATA_GRABBER,
};

#define SBT_INLINE inline __attribute__((always_inline))

static SBT_INLINE uint16_t read16(const uint8_t *ptr) {
    return ptr[0] | (ptr[1] << 8);
}

static SBT_INLINE void write16(uint8_t *ptr, uint16_t x) {
    ptr[0] = (uint8_t)x;
    ptr[1] = x >> 8;
}

/*
 * SBTRegs --
 *
 *    Register state for the virtual 8086 processor.
 */

struct SBTRegs {
    union {
        uint16_t ax;
        struct {
            uint8_t al, ah;
        };
    };
    union {
        uint16_t bx;
        struct {
            uint8_t bl, bh;
        };
    };
    union {
        uint16_t cx;
        struct {
            uint8_t cl, ch;
        };
    };
    union {
        uint16_t dx;
        struct {
            uint8_t dl, dh;
        };
    };
    uint16_t si, di;
    uint16_t cs, ds, es, ss;
    uint16_t bp, sp;

    /*
     * Flags are implemented by storing 32-bit versions of the operation
     * result. This implementation was originally written to reduce code
     * size when compiling to ARM Thumb, but it may be worth rethinking
     * this now that we are targeting wasm.
     */
    uint32_t uresult;
    int32_t sresult;

    /*
     * Inline accessors for manually setting/inspecting flags.
     */

    SBT_INLINE bool getZF() { return (uresult & 0xFFFF) == 0; }
    SBT_INLINE bool getSF() { return (uresult & 0x8000) != 0; }
    SBT_INLINE bool getOF() {
        return (((sresult >> 1) ^ (sresult)) & 0x8000) != 0;
    }
    SBT_INLINE bool getCF() { return (uresult & 0x10000) != 0; }

    SBT_INLINE void setZF() { uresult &= ~0xFFFF; }
    SBT_INLINE void clearZF() { uresult |= 1; }
    SBT_INLINE void setZF(bool f) {
        if (f)
            setZF();
        else
            clearZF();
    }

    SBT_INLINE void setOF() { sresult = 0x8000; }
    SBT_INLINE void clearOF() { sresult = 0; }
    SBT_INLINE void setOF(bool f) {
        if (f)
            setOF();
        else
            clearOF();
    }

    SBT_INLINE void setCF() { uresult |= 0x10000; }
    SBT_INLINE void clearCF() { uresult &= 0xFFFF; }
    SBT_INLINE void setCF(bool f) {
        if (f)
            setCF();
        else
            clearCF();
    }

    SBT_INLINE uint32_t saveCF() { return uresult & 0x10000; }
    SBT_INLINE void restoreCF(uint32_t saved) {
        uresult = (uresult & 0xFFFF) | saved;
    }
};

/*
 * SBTStack --
 *
 *    Translated code uses a tagged stack that exists
 *    on the C++ stack, outside SBT86's emulated memory space.
 */

class SBTStack {
  private:
    enum Tag {
        STACK_TAG_INVALID = 0,
        STACK_TAG_WORD,
        STACK_TAG_FLAGS,
        STACK_TAG_RETADDR,
    };

  public:
    SBTStack() { reset(); }

    SBT_INLINE void reset() { top = 0; };

    void trace(IOBuffer *io) {
        // For a stack trace, copy out only the RETADDR tags
        uint16_t retaddrCount = 0;
        for (unsigned slot = 0; slot < top; slot++) {
            if (tags[slot] == STACK_TAG_RETADDR) {
                retaddrCount++;
            }
        }
        uint8_t *buffer =
            io->write(IOBuffer::OutputTag::STACK, 2 + retaddrCount * 2);
        memcpy(buffer, &retaddrCount, 2);
        buffer += 2;
        for (unsigned slot = 0; slot < top; slot++) {
            if (tags[slot] == STACK_TAG_RETADDR) {
                memcpy(buffer, &words[slot], 2);
                buffer += 2;
            }
        }
    }

    SBT_INLINE void pushw(IOBuffer *io, uint16_t word) {
        overflowCheck(io);
        words[top] = word;
        pushTag(STACK_TAG_WORD);
    }

    SBT_INLINE void pushf(IOBuffer *io, SBTRegs reg) {
        overflowCheck(io);
        flags[top].uresult = reg.uresult;
        flags[top].sresult = reg.sresult;
        pushTag(STACK_TAG_FLAGS);
    }

    SBT_INLINE void pushret(IOBuffer *io, uint16_t fn) {
        overflowCheck(io);
        words[top] = fn;
        pushTag(STACK_TAG_RETADDR);
    }

    SBT_INLINE uint16_t popw(IOBuffer *io) {
        lowerChecked(io);
        tagCheck(io, STACK_TAG_WORD);
        return words[top];
    }

    SBT_INLINE SBTRegs popf(IOBuffer *io, SBTRegs reg) {
        lowerChecked(io);
        tagCheck(io, STACK_TAG_FLAGS);
        reg.uresult = flags[top].uresult;
        reg.sresult = flags[top].sresult;
        return reg;
    }

    SBT_INLINE void popret(IOBuffer *io, uint16_t expected_addr) {
        lowerChecked(io);
        tagCheck(io, STACK_TAG_RETADDR);
        uint16_t saved_addr = words[top];
        if (saved_addr != expected_addr) {
            addrError(io, saved_addr, expected_addr);
        }
    }

    /*
     * These are special stack accessors for use in hook routines.
     * Some routines in Robot Odyssey save the return value off
     * the stack, manipulate the caller's stack, then restore
     * the return value.
     *
     * preSaveRet() should be called before the return value is
     * saved at the beginning of such a function. It converts the
     * top of the stack from a RETADDR to a WORD, and stores a
     * verification value in that word.
     *
     * postRestoreRet() should be called after the return value is
     * restored. It verifies the value saved by preSaveRet, and
     * converts the top of stack back to a RETADDR.
     */

    SBT_INLINE void preSaveRet(IOBuffer *io) {
        lowerChecked(io);
        tagCheck(io, STACK_TAG_RETADDR);
        pushTag(STACK_TAG_WORD);
    }

    SBT_INLINE void postRestoreRet(IOBuffer *io) {
        lowerChecked(io);
        tagCheck(io, STACK_TAG_WORD);
        pushTag(STACK_TAG_RETADDR);
    }

  private:
    SBT_INLINE void overflowCheck(IOBuffer *io) {
        if (top >= STACK_SIZE) {
            overflowError(io);
        }
    }

    [[noreturn]] void overflowError(IOBuffer *io) {
        trace(io);
        io->error("Stack overflow");
    }

    SBT_INLINE void lowerChecked(IOBuffer *io) {
        underflowCheck(io);
        top--;
    }

    SBT_INLINE void underflowCheck(IOBuffer *io) {
        if (top == 0) {
            underflowError(io);
        }
    }

    [[noreturn]] void underflowError(IOBuffer *io) {
        trace(io);
        io->error("Stack underflow");
    }

    SBT_INLINE void pushTag(Tag tag) {
        tags[top] = tag;
        top++;
    }

    SBT_INLINE void tagCheck(IOBuffer *io, Tag expected) {
        Tag saved = tags[top];
        if (saved != expected) {
            tagError(io, saved, expected);
        }
    }

    [[noreturn]] void tagError(IOBuffer *io, Tag saved, Tag expected) {
        trace(io);
        io->logBinary(saved);
        io->logBinary(expected);
        io->error("Stack tag mismatch");
    }

    [[noreturn]] void addrError(IOBuffer *io, uint16_t saved,
                                uint16_t expected) {
        trace(io);
        io->logBinary(saved);
        io->logBinary(expected);
        io->error("Stack addr mismatch");
    }

    static constexpr uint32_t STACK_SIZE = 512;
    uint32_t top;

    Tag tags[STACK_SIZE];
    uint16_t words[STACK_SIZE];
    struct {
        uint32_t uresult;
        int32_t sresult;
    } flags[STACK_SIZE];
};

/*
 * SBTSegmentCache --
 *
 *    Cached segment pointers. Translated indirects use these pointers
 *    to avoid segment lookups on every memory access.
 */

struct SBTSegmentCache {
    uint8_t *cs;
    uint8_t *ds;
    uint8_t *es;
    uint8_t *ss;
};

/*
 * SBTClock --
 *
 *    Cycle counter, for the portions of translated code that are timed.
 *    It is advanced using a mixture of explicit delays and simulated
 *    instruction timing. Timestamps are critical for sound emulation,
 *    and they also determine the overall game speed.
 */

struct SBTClock {
    uint32_t cycles;
    static constexpr uint32_t KHZ = 4770;
    static constexpr uint32_t HZ = KHZ * 1000;

    SBT_INLINE SBTClock() : cycles(0) {}

    SBT_INLINE void addCycles(uint32_t delay) { cycles += delay; }

    SBT_INLINE void addMillisec(uint32_t delay) { cycles += delay * KHZ; }

    SBT_INLINE uint32_t take() {
        uint32_t result = cycles;
        cycles = 0;
        return result;
    }
};

/*
 * SBTRuntime --
 *
 *    The runtime state for a SBTProcess. Contains registers and
 *    our tagged stack by value, and points to an external Engine.
 */

struct SBTRuntime {
    typedef bool (*func_t)(SBTRuntime &rt);

    struct Result {
        enum Code {
            FINISHED,
            CONTINUE_ONCE,
            CONTINUE_ALWAYS,
        };

        Code code;
        func_t func;

        SBT_INLINE bool continueOnce(func_t nextFunc) {
            code = CONTINUE_ONCE;
            func = nextFunc;
            return true;
        }

        SBT_INLINE bool continueAlways(func_t nextFunc) {
            code = CONTINUE_ALWAYS;
            func = nextFunc;
            return true;
        }

        Result() : code(FINISHED), func(nullptr) {}
    };

    Engine *eng;
    IOBuffer *io;
    SBTRegs reg;
    SBTSegmentCache seg;
    SBTClock clock;
    SBTStack stack;
    Result result;
};

/*
 * SBTMemory --
 *
 *    Emulated system memory
 */

struct SBTMemory {
    static constexpr uint32_t SIZE = 256 * 1024;
    uint8_t bytes[SIZE];

    SBTMemory() { memset(bytes, 0, SIZE); }

    /*
     * Return a pointer to an emulated memory segment. Only 64K of
     * memory past the returned pointer is guaranteed to be valid.  If
     * the address is out of range, returns NULL.
     */
    uint8_t *seg(uint16_t s) {
        /*
         * The highest normal segment we can support.  Any segments over
         * MAX_SEGMENT get remapped to MAX_SEGMENT. This serves two
         * purposes:
         *
         *   1. Out of range memory accesses are dumped somewhere safe.
         *   2. It puts the CGA framebuffer in range of the emulated memory.
         *
         * Padding is set to 0x20000 to give us room for two full 16-bit
         * additions, allowing 16-bit offsets to buffers with 16-bit sizes
         * without further checks.
         */

        static constexpr uint32_t SEGMENT_PADDING = 0x20000;
        static constexpr uint32_t MAX_SEGMENT = (SIZE - SEGMENT_PADDING) >> 4;

        if (s > MAX_SEGMENT) {
            s = MAX_SEGMENT;
        }
        return bytes + (((uint32_t)s) << 4);
    }

    SBTSegmentCache allSeg(SBTRegs const &reg) {
        return {seg(reg.cs), seg(reg.ds), seg(reg.es), seg(reg.ss)};
    }

    SBT_INLINE uint8_t peek8(uint16_t s, uint16_t off) { return seg(s)[off]; }

    SBT_INLINE void poke8(uint16_t s, uint16_t off, uint8_t value) {
        seg(s)[off] = value;
    }

    SBT_INLINE uint16_t peek16(uint16_t s, uint16_t off) {
        return read16(seg(s) + off);
    }

    SBT_INLINE void poke16(uint16_t s, uint16_t off, uint16_t value) {
        write16(seg(s) + off, value);
    }
};

/*
 * SBTProcess --
 *
 *    Results of statically translating a DOS EXE file using sbt86.
 *    Contains compiled code, exported addresses, and initialized data.
 */

struct SBTProcess {
    const char *filename;

    const uint8_t *data;
    uint32_t dataLength;

    uint16_t relocSegment;
    uint16_t entryCS;

    int (*getAddress)(SBTAddressId id);
    SBTRuntime::func_t (*getFunction)(SBTAddressId id);
};
