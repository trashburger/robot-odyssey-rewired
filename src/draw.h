#pragma once
#include <stdint.h>

struct CGAFramebuffer
{
    static const unsigned WIDTH = 320;
    static const unsigned HEIGHT = 200;
    static const unsigned ZOOM = 2;
    uint8_t bytes[0x4000];
};

struct ColorTable
{
    static const unsigned PLAYFIELD_BLOCK_SIZE = 16;
    static const unsigned SCREEN_TILE_SIZE = PLAYFIELD_BLOCK_SIZE * CGAFramebuffer::ZOOM;
    static const unsigned NUM_PATTERNS = 0x100;

    uint32_t cga[4];
    uint32_t patterns[SCREEN_TILE_SIZE * SCREEN_TILE_SIZE * NUM_PATTERNS];
};

class RGBDraw
{
public:
    RGBDraw(ColorTable &colorTable);

    static const unsigned SCREEN_WIDTH = CGAFramebuffer::WIDTH * CGAFramebuffer::ZOOM;
    static const unsigned SCREEN_HEIGHT = CGAFramebuffer::HEIGHT * CGAFramebuffer::ZOOM;

    uint32_t backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
    ColorTable &colorTable;

    void sprite(uint8_t *data, uint8_t x, uint8_t y, uint8_t color);
    void playfield(uint8_t *data, uint8_t foreground, uint8_t background);
    void text(uint8_t *string, uint8_t *font_data, uint8_t x, uint8_t y, uint8_t color, uint8_t font_id, uint8_t style);
    void vline(uint8_t x, uint8_t y1, uint8_t y2, uint8_t color);
    void hline(uint8_t x1, uint8_t x2, uint8_t y, uint8_t color);

    void pixel_160x192(unsigned x, unsigned y, uint8_t color, unsigned anchor_x, unsigned anchor_y);
    void pixel_320x192(unsigned x, unsigned y, uint8_t color, unsigned anchor_x, unsigned anchor_y);
};
