#include <stdio.h>
#include <algorithm>
#include "draw.h"

RGBDraw::RGBDraw()
{
    for (unsigned pixel = 0; pixel < SCREEN_WIDTH * SCREEN_HEIGHT; pixel++) {
        backbuffer[pixel] = 0xff000000;
    }
}

void RGBDraw::pixel_160x192(unsigned x, unsigned y, uint8_t color)
{
    if (x >= 160 || y >= 192) {
        return;
    }

    unsigned screen_x = (2 * x) * CGAFramebuffer::ZOOM;
    unsigned screen_y = (191 - y) * CGAFramebuffer::ZOOM;

    unsigned pattern_x = screen_x % SCREEN_TILE_SIZE;
    unsigned pattern_y = screen_y % SCREEN_TILE_SIZE;

    uint32_t *pattern = patterns + (SCREEN_TILE_SIZE * SCREEN_TILE_SIZE * color);

    for (unsigned zy = 0; zy < CGAFramebuffer::ZOOM; zy++) {
        for (unsigned zx = 0; zx < 2 * CGAFramebuffer::ZOOM; zx++) {
            uint32_t rgb = pattern[zx + pattern_x + (zy + pattern_y) * SCREEN_TILE_SIZE];
            backbuffer[screen_x + zx + (zy + screen_y) * SCREEN_WIDTH] = rgb;
        }
    }
}

void RGBDraw::sprite(uint8_t *data, uint8_t x, uint8_t y, uint8_t color)
{
    printf("sprite %p at %d,%d color=%02x\n", data, x, y, color);

    for (unsigned byte_index = 0; byte_index < 16; byte_index++) {
        uint8_t byte = data[byte_index];
        for (unsigned bit_index = 0; bit_index < 7; bit_index++) {
            if ((byte >> bit_index) & 1) {
                pixel_160x192(x + 6 - bit_index, y + byte_index, color);
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
    // printf("text: '%s', font=%p.%d at %d, %d color=%02x style=%02x\n", string, font_data, font_id, x, y, color, style);
}

void RGBDraw::vline(uint8_t x, uint8_t y1, uint8_t y2, uint8_t color)
{
    for (unsigned y = std::min(y1, y2); y <= std::max(y1, y2); y++) {
        pixel_160x192(x, y, color);
    }
}

void RGBDraw::hline(uint8_t x1, uint8_t x2, uint8_t y, uint8_t color)
{
    for (unsigned x = std::min(x1, x2); x <= std::max(x1, x2); x++) {
        pixel_160x192(x, y, color);
    }
}
