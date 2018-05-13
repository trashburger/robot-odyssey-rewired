#include <string.h>
#include <stdint.h>
#include <zstd.h>
#include "tinySave.h"

static void initWorldFromFiles(ROSavedGame *game, FileInfo *wor, FileInfo *cir = 0, FileInfo *wld = 0)
{

}

static void initChipFromFiles(ROSavedGame *game, uint8_t slot, FileInfo *chp, FileInfo *pin)
{
    assert(chp && chp->size <= sizeof game->chipBytecode[slot]);
    assert(pin && pin->size == sizeof game->chipPins[slot]);
    
}

static void initReferenceGameWorld(ROSavedGame *game, FileInfo *wld = 0)
{
    extern FileInfo file_sewer_wor;
    extern FileInfo file_sewer_cir;
    extern FileInfo file_countton_chp;
    extern FileInfo file_countton_pin;
    extern FileInfo file_wallhug_chp;
    extern FileInfo file_wallhug_pin;

    // All games start at the sewer with count-to-n and wallhugger chips
    // loaded, then we build on that. If other chips are loaded instead,
    // we'll have to rely on gzip compressing them or finding them in the
    // pre-shared dictionary. If chips haven't been reloaded, the packed game
    // size will be smallest.

    initWorldFromFiles(game, &file_sewer_wor, &file_sewer_cir, wld);
    initChipFromFiles(game, 0, &file_countton_chp, &file_countton_pin);
    initChipFromFiles(game, 1, &file_wallhug_chp, &file_wallhug_pin);
}

static void initReferenceWorld(ROSavedGame *game, uint8_t worldId)
{
    extern FileInfo file_lab_wor;
    extern FileInfo file_comp_wld;
    extern FileInfo file_street_wld;
    extern FileInfo file_subway_wld;
    extern FileInfo file_town_wld;

    memset(game, 0, sizeof *game);

    switch (worldId) {

        case RO_WORLD_SEWER:
            initReferenceGameWorld(game, 0);
            break;

        case RO_WORLD_SUBWAY:
            initReferenceGameWorld(game, &file_subway_wld);
            break;

        case RO_WORLD_TOWN:
            initReferenceGameWorld(game, &file_town_wld);
            break;

        case RO_WORLD_COMP:
            initReferenceGameWorld(game, &file_comp_wld);
            break;

        case RO_WORLD_STREET:
            initReferenceGameWorld(game, &file_street_wld);
            break;

        case RO_WORLD_LAB:
            initWorldFromFiles(game, &file_lab_wor);
            break;
    }
}


void TinySave::compress(const FileInfo& src)
{
    unpacked.size = src.size;
    memcpy(unpacked.buffer, src.data, src.size);

    size = ZSTD_compress(buffer, sizeof buffer, unpacked.buffer, unpacked.size, 18);
}

bool TinySave::decompress(FileInfo& dest)
{
    return false;
}
