#!/usr/bin/env python3
#
# The menu, intro animation, new-game cutscene, and end-game cutscene
# are all stored in an RLE-delta compressed format which this script can
# manipulate.
#
# 9B: End of frame
# 1B: 8-bit count, 8-bit value to repeat
# E4: 16-bit LE count, 8-bit value to repeat
# E6: 16-bit dest address
# Other bytes are literal
#
# In this file:
#
#   - Extract all images from the original show files
#   - Re-pack the show files with a subset of their original images,
#     so we can avoid distributing multiple copies of those graphics.
#
# Currently we use the original game code to play the new-game and end-game
# animations, so all frames for those animations are included. The intro, however,
# is implemented in HTML and CSS while the engine itself loads, and the menu is
# re-implemented in Javascript for improved usability.
#

import sys, os, struct, io
from PIL import Image

PALETTE = [
    0,
    0,
    0,
    0x09,
    0xA9,
    0xD8,
    0xED,
    0x7A,
    0x31,
    0xFF,
    0xFF,
    0xFF,
    0,
    0,
    0,  # Transparent
]

CGA_SIZE = (320, 200)
ZOOMED_SIZE = (640, 400)
PADDED_SIZE = (644, 388)
MARGIN = 2


def decompress_image(file, pixels, compressed):
    # Decompress an image, using the provided 8bpp scratch space and
    # returning a PIL image object, adding the compressed data to
    # the provided buffer

    ptr = 0
    while True:
        count = 1
        byte = file.read(1)
        compressed.extend(byte)
        if len(byte) < 1:
            # End of file
            return None
        byte = ord(byte)

        if byte == 0x9B:
            # Successful end of image, wrap the 8bpp buffer in a PIL image.

            img = Image.frombuffer("P", CGA_SIZE, pixels, "raw", "P", 0, 1)
            img.putpalette(PALETTE)
            return img.resize(ZOOMED_SIZE)

        if ptr >= 0x4000:
            # Ignore anything except 0x9B when past the framebuffer end
            continue

        if byte == 0x1B:
            # 8-bit RLE
            packet = file.read(2)
            compressed.extend(packet)
            count, byte = struct.unpack("<BB", packet)

        elif byte == 0xE4:
            # 16-bit RLE
            packet = file.read(3)
            compressed.extend(packet)
            count, byte = struct.unpack("<HB", packet)

        elif byte == 0xE6:
            # 16-bit dest pointer
            packet = file.read(2)
            compressed.extend(packet)
            ptr = struct.unpack("<H", packet)[0]
            continue

        # A run of identical bytes, each with 4 pixels.
        # Each CGA byte-wide write turns into four 8bpp pixels.

        while count > 0:

            # Convert CGA framebuffer address 'ptr' to 8bpp byte address.
            # CGA is arranged as two 0x2000-byte interleaved planes.
            if ptr >= 0x2000:
                pixaddr = (4 * ((ptr - 0x2000) % 80)) + 320 * (
                    2 * ((ptr - 0x2000) // 80) + 1
                )
            else:
                pixaddr = (4 * (ptr % 80)) + 320 * (2 * (ptr // 80))

            if pixaddr <= 63996:
                # Four pixels per byte, MSB first. Palette entry 4 is transparent
                # for pixels that aren't written, and the first 4 entries represent CGA colors.
                pixels[pixaddr] = 3 & (byte >> 6)
                pixels[pixaddr + 1] = 3 & (byte >> 4)
                pixels[pixaddr + 2] = 3 & (byte >> 2)
                pixels[pixaddr + 3] = 3 & byte

            ptr += 1
            count -= 1


def decode_all_images(showfile):
    for composited in (False, True):
        compressed_images = []
        if composited:
            # Opaque black, before the first frame, then every frame composited together
            pixels = bytearray(b"\x00" * (320 * 200))

        with open(showfile, "rb") as file:
            number = 0
            while True:
                if not composited:
                    # Erase to transparent black before each frame, no compositing
                    pixels = bytearray(b"\x04" * (320 * 200))

                raw = bytearray()
                im = decompress_image(file, pixels, raw)
                if im is None:
                    break
                compressed_images.append(raw)

                name = "%s-%02d%s.png" % (
                    os.path.splitext(showfile)[0],
                    number,
                    ("", "-c")[composited],
                )
                im.save(name, transparency=4, optimize=1)
                print(
                    "%s is %d bytes, from the original %d byte RLE data"
                    % (name, os.stat(name).st_size, len(raw))
                )
                number += 1

    return compressed_images


def repack_show_without_menu(dest_file, parts):
    with open(dest_file, "wb") as file:
        for part in parts:
            file.write(part)
        print("%s is %d bytes" % (dest_file, file.tell()))


def image_to_644x388(im):
    # The native SHW images are in CGA (320x200) and we double them to 640x400,
    # but the resolution we actually want is 644x388, with a 2-pixel border on each side.
    bg = im.resize(PADDED_SIZE)
    bg.paste(
        0,
        (
            0,
            0,
        )
        + PADDED_SIZE,
    )
    bg.paste(
        im.crop((0, 0, PADDED_SIZE[0] - MARGIN * 2, PADDED_SIZE[1] - MARGIN * 2)),
        (MARGIN, MARGIN),
    )
    return bg


def icon_border(im, s=2):
    # Incoming image has a transparent/black background at index 0. Process
    # each of these pixels, replacing them with transparent black (4) or leaving
    # them opaque (0) based on whether there are neighboring pixels of another color.

    w, h = im.size
    for y in range(h):
        for x in range(w):
            if im.getpixel((x, y)) == 0:
                region = im.crop((x - s, y - s, x + s * 2 - 1, y + s * 2 - 1)).tobytes()
                if sum(pix & 3 for pix in region) == 0:
                    im.putpixel((x, y), 4)
    return im


def carve_rows(im, prefix, heights):
    y = 0
    w = im.size[0]
    for i, h in enumerate(heights):
        im.crop((0, y, w, y + h)).save(prefix + "-%02d.png" % i, optimize=1)
        y += h
    assert y == im.size[1]
    print(
        "Rows carved, heights in percent: %r"
        % list(h * 100.0 / im.size[1] for h in heights)
    )


def main(build):
    show = decode_all_images(os.path.join(build, "show/show.shw"))
    show2 = decode_all_images(os.path.join(build, "show/show2.shw"))
    print("Found %d images in show.shw and %d in show2.shw" % (len(show), len(show2)))

    # Remove frames that we aren't using. No need to replace them with blanks,
    # since the corresponding code will be removed from the player too.

    repack_show_without_menu(os.path.join(build, "fs/show.shw"), show[7:])
    repack_show_without_menu(os.path.join(build, "fs/show2.shw"), show2[5:])

    # Get composited splashscreen images as PIL images

    splash1 = Image.open(os.path.join(build, "show/show-00-c.png"))
    splash2 = Image.open(os.path.join(build, "show/show-01-c.png"))

    # Paste over the original version number with a black rectangle,
    # since it would be misleading to claim this is that version. We might
    # replace this with a different version, but so far IMO that would detract
    # from the original pixel-art look.

    splash2.paste(0, (518, 156, 518 + 44, 156 + 14))

    # Save the splashscreens for inclusion in the main HTML file

    image_to_644x388(splash1).save(os.path.join(build, "show/splash1.png"), optimize=1)
    image_to_644x388(splash2).save(os.path.join(build, "show/splash2.png"), optimize=1)

    # The show.shw file includes in [2] the entire menu plus cursor,
    # and [3] clears the menu itself leaving only the cursor. We actually want
    # separate PNGs, for the menu background (sliced) and the cursor.

    menu = Image.open(os.path.join(build, "show/show-02-c.png"))
    menu_clear = Image.open(os.path.join(build, "show/show-03.png"))
    menu_opacity_mask = menu_clear.point(lambda color: color == 4, mode="1")
    menu.paste(0, None, menu_opacity_mask)
    menu = image_to_644x388(menu)
    carve_rows(
        menu,
        os.path.join(build, "show/menu"),
        [40, 32, 32, 30, 30, 30, 30, 30, 30, 30, 30, 44],
    )

    cursor_only = image_to_644x388(
        Image.open(os.path.join(build, "show/show-03-c.png"))
    )
    cursor_only.save(
        os.path.join(build, "show/menu-cursor.png"), transparency=0, optimize=1
    )

    # Extract an icon image from the menu cursor

    icon = icon_border(cursor_only.crop((50, 12, 50 + 76, 12 + 68))).resize((512, 512))
    icon.save(os.path.join(build, "show/icon.png"), transparency=4, optimize=1)


if __name__ == "__main__":
    main(sys.argv[1])
