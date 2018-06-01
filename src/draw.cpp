#include <stdio.h>
#include <algorithm>
#include "draw.h"
#include "roData.h"

RGBDraw::RGBDraw()
{
    for (unsigned pixel = 0; pixel < SCREEN_WIDTH * SCREEN_HEIGHT; pixel++) {
        backbuffer[pixel] = 0xff000000;
    }
}

void RGBDraw::pixel_160x192(unsigned x, unsigned y, uint8_t color, unsigned anchor_x, unsigned anchor_y)
{
    pixel_320x192(2*x, y, color, 2*anchor_x, anchor_y);
    pixel_320x192(2*x+1, y, color, 2*anchor_x + 1, anchor_y);
}

void RGBDraw::pixel_320x192(unsigned x, unsigned y, uint8_t color, unsigned anchor_x, unsigned anchor_y)
{
    if (x >= 320 || y >= 192) {
        return;
    }

    unsigned screen_x = x * CGAFramebuffer::ZOOM;
    unsigned screen_y = (191 - y) * CGAFramebuffer::ZOOM;

    uint32_t *pattern = patterns + (SCREEN_TILE_SIZE * SCREEN_TILE_SIZE * color);
    unsigned pattern_x = anchor_x * CGAFramebuffer::ZOOM;
    unsigned pattern_y = SCREEN_TILE_SIZE - (1 + anchor_y) * CGAFramebuffer::ZOOM;

    for (unsigned zy = 0; zy < CGAFramebuffer::ZOOM; zy++) {
        for (unsigned zx = 0; zx < CGAFramebuffer::ZOOM; zx++) {
            unsigned px = (pattern_x + zx) % SCREEN_TILE_SIZE;
            unsigned py = (pattern_y + zy) % SCREEN_TILE_SIZE;
            uint32_t rgb = pattern[px + py * SCREEN_TILE_SIZE];
            backbuffer[screen_x + zx + (zy + screen_y) * SCREEN_WIDTH] = rgb;
        }
    }
}

void RGBDraw::sprite(uint8_t *data, uint8_t x, uint8_t y, uint8_t color)
{
    for (unsigned byte_index = 0; byte_index < 16; byte_index++) {
        uint8_t byte = data[byte_index];
        for (unsigned bit_index = 0; bit_index < 7; bit_index++) {
            if ((byte >> bit_index) & 1) {
                unsigned bitx = 6 - bit_index;
                pixel_160x192(x + bitx, y + byte_index, color, bitx, byte_index);
            }
        }
    }
}

void RGBDraw::playfield(uint8_t *data, uint8_t foreground, uint8_t background)
{
    for (unsigned byte_index = 0; byte_index < 30; byte_index++) {
        uint8_t byte = data[byte_index];
        for (unsigned bit_index = 0; bit_index < 8; bit_index++) {
            unsigned pattern_id = ((byte >> bit_index) & 1) ? foreground : background;
            uint32_t *pattern = patterns + (SCREEN_TILE_SIZE * SCREEN_TILE_SIZE * pattern_id);
            unsigned tile_x = (byte_index % 10)*2 + (bit_index >> 2);
            unsigned tile_y = (byte_index / 10)*4 + (bit_index & 3);
            unsigned screen_x = tile_x * SCREEN_TILE_SIZE;
            unsigned screen_y = tile_y * SCREEN_TILE_SIZE;
            uint32_t *dest = backbuffer + screen_x + screen_y * SCREEN_WIDTH;
            for (unsigned y = 0; y < SCREEN_TILE_SIZE; y++) {
                uint32_t *pattern_line = pattern + y*SCREEN_TILE_SIZE;
                uint32_t *dest_line = dest + y*SCREEN_WIDTH;
                for (unsigned x = 0; x < SCREEN_TILE_SIZE; x++) {
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
            // Clip off-screen characters entirely.
            // The original game data has some junk
            // strings that normally aren't visible;
            // see room 0x19 in TUT6.WOR, and room 0x14 in TUT5.WOR
            if (x < 160 && y < 192) {
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
            }

            // Next character cell
            x += 4 * zoom;
        }

        string++;
    }
}

void RGBDraw::vline(uint8_t x, uint8_t y1, uint8_t y2, uint8_t color)
{
    unsigned start = std::min(y1, y2);
    unsigned count = std::max(y1, y2) + 1 - start; 
    for (unsigned i = 0; i < count; i++) {
        pixel_160x192(x, start+i, color, 0, i);
    }
}

void RGBDraw::hline(uint8_t x1, uint8_t x2, uint8_t y, uint8_t color)
{
    unsigned start = std::min(x1, x2);
    unsigned count = std::max(x1, x2) + 1 - start; 
    for (unsigned i = 0; i < count; i++) {
        pixel_160x192(start+i, y, color, i, 0);
    }
}
