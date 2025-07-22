#include <stdio.h>
#include <algorithm>
#include "draw.h"
#include "roData.h"
#include "sbt86.h"

RGBDraw::RGBDraw(ColorTable &colorTable)
    : colorTable(colorTable)
{
    // Cleared to transparent black
    memset(backbuffer, 0, sizeof backbuffer);
}

SBT_INLINE void RGBDraw::pixel_160x192(unsigned x, unsigned y, uint8_t color, unsigned anchor_x, unsigned anchor_y)
{
    pixel_320x192(2*x, y, color, 2*anchor_x, anchor_y);
    pixel_320x192(2*x+1, y, color, 2*anchor_x + 1, anchor_y);
}

SBT_INLINE void RGBDraw::pixel_320x192(unsigned x, unsigned y, uint8_t color, unsigned anchor_x, unsigned anchor_y)
{
    if (x >= 320 || y >= 192) {
        return;
    }

    const unsigned tile_size = ColorTable::SCREEN_TILE_SIZE;
    const unsigned zoom = CGAFramebuffer::ZOOM;
    const unsigned screen_x = x * zoom;
    const unsigned screen_y = (191 - y) * zoom;

    const uint32_t *pattern = colorTable.patterns + (tile_size * tile_size * color);
    const unsigned pattern_x = anchor_x * zoom;
    const unsigned pattern_y = tile_size - (1 + anchor_y) * zoom;

    #pragma unroll
    for (unsigned zy = 0; zy < zoom; zy++) {
        #pragma unroll
        for (unsigned zx = 0; zx < zoom; zx++) {
            const unsigned px = (pattern_x + zx) % tile_size;
            const unsigned py = (pattern_y + zy) % tile_size;
            const uint32_t rgb = pattern[px + py * tile_size];
            backbuffer[screen_x + zx + (zy + screen_y) * SCREEN_WIDTH] = rgb;
        }
    }
}

void RGBDraw::sprite(uint8_t *data, uint8_t x, uint8_t y, uint8_t color)
{
    for (unsigned byte_index = 0; byte_index < 16; byte_index++) {
        const uint8_t byte = data[byte_index];
        for (unsigned bit_index = 0; bit_index < 7; bit_index++) {
            if ((byte >> bit_index) & 1) {
                const unsigned bitx = 6 - bit_index;
                pixel_160x192(x + bitx, y + byte_index, color, bitx, byte_index);
            }
        }
    }
}

void RGBDraw::playfield(uint8_t *data, uint8_t foreground, uint8_t background)
{
    const unsigned tile_size = ColorTable::SCREEN_TILE_SIZE;

    for (unsigned byte_index = 0; byte_index < 30; byte_index++) {
        const uint8_t byte = data[byte_index];
        for (unsigned bit_index = 0; bit_index < 8; bit_index++) {
            const unsigned pattern_id = ((byte >> bit_index) & 1) ? foreground : background;
            const uint32_t *pattern = colorTable.patterns + (tile_size * tile_size * pattern_id);
            const unsigned tile_x = (byte_index % 10)*2 + (bit_index >> 2);
            const unsigned tile_y = (byte_index / 10)*4 + (bit_index & 3);
            const unsigned screen_x = tile_x * tile_size;
            const unsigned screen_y = tile_y * tile_size;
            uint32_t *dest = backbuffer + screen_x + screen_y * SCREEN_WIDTH;
            for (unsigned y = 0; y < tile_size; y++) {
                const uint32_t *pattern_line = pattern + y*tile_size;
                uint32_t *dest_line = dest + y*SCREEN_WIDTH;
                #pragma unroll
                for (unsigned x = 0; x < tile_size; x++) {
                    dest_line[x] = pattern_line[x];
                }
            }
        }
    }
}

void RGBDraw::text(uint8_t *string, uint8_t *font_data, uint8_t x, uint8_t y,
    uint8_t color, uint8_t font_id, uint8_t style)
{
    // The game engine calculates font_data to be the offset from character zero,
    // which isn't actually present in the font (-0x20), and from the last line
    // (+7 * 0x60), so the actual pointer we get is to 0x280 past the beginning of
    // the 0x300-byte block of data.
    uint8_t *font_start = font_data - 0x280;

    unsigned zoom = (style == RO_TEXT_BIG) ? 2 : 1;
    uint8_t newline_x = x;

    while (*string) {
        uint8_t c = *string;
        if (c == 0) {
            break;

        } else if (c == 0x0D) {
            // Newline
            x = newline_x;
            y -= 9 * zoom;

        } else if (c >= 0x20 && c < 0x80) {
            // Clip off-screen characters entirely, and stop advancing
            // the cursor so we avoid wrap-around.
            // The original game data has some junk strings that normally aren't visible;
            // see room 0x19 in TUT6.WOR, and room 0x14 in TUT5.WOR

            if (x < 160 && y < 192-8) {
                uint8_t *font = font_start + (c - 0x20);
                for (unsigned line = 0; line < 8; line++) {
                    uint8_t byte = font[line * 0x60];
                    for (unsigned bit = 0; bit < 8; bit++) {
                        if ((byte << bit) & 0x80) {
                            if (style == RO_TEXT_BIG) {
                                // 2x zoomed text with color
                                pixel_160x192(x + bit - 1, y - line*2 + 14, color, bit, line*2);
                                pixel_160x192(x + bit - 1, y - line*2 + 15, color, bit, line*2+1);
                            } else {
                                // Small monochrome text, byte aligned
                                const uint8_t text_color = RO_COLOR_WIRE_COLD;
                                pixel_320x192(2*(x&~1) + bit, y - line + 7, text_color, bit, line);
                            }
                        }
                    }
                }

                // Next character cell
                x += 4 * zoom;
            }
        }

        string++;
    }
}

void RGBDraw::vline(uint8_t x, uint8_t y1, uint8_t y2, uint8_t color)
{
    unsigned start = std::min(y1, y2);
    unsigned end = std::max(y1, y2);
    if (end < 192) {
        unsigned count = end + 1 - start;
        for (unsigned i = 0; i < count; i++) {
            pixel_160x192(x, start+i, color, 0, i);
        }
    }
}

void RGBDraw::hline(uint8_t x1, uint8_t x2, uint8_t y, uint8_t color)
{
    unsigned start = std::min(x1, x2);
    unsigned end = std::max(x1, x2);
    if (end < 160) {
        unsigned count = end + 1 - start;
        for (unsigned i = 0; i < count; i++) {
            pixel_160x192(start+i, y, color, i, 0);
        }
    }
}
