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

            img = Image.frombuffer('P', (320, 200), pixels, 'raw', 'P', 0, 1)
            img.putpalette([
                0, 0, 0,
                0, 0, 0,
                0x09, 0xA9, 0xD8,
                0xED, 0x7A, 0x31,
                0xFF, 0xFF, 0xFF,
            ])
            return img

        if ptr >= 0x4000:
            # Ignore anything except 0x9B when past the framebuffer end
            continue

        if byte == 0x1B:
            # 8-bit RLE
            packet = file.read(2)
            compressed.extend(packet)
            count, byte = struct.unpack('<BB', packet)

        elif byte == 0xE4:
            # 16-bit RLE
            packet = file.read(3)
            compressed.extend(packet)
            count, byte = struct.unpack('<HB', packet)

        elif byte == 0xE6:
            # 16-bit dest pointer
            packet = file.read(2)
            compressed.extend(packet)
            ptr = struct.unpack('<H', packet)[0]
            continue

        # A run of identical bytes, each with 4 pixels.
        # Each CGA byte-wide write turns into four 8bpp pixels.

        while count > 0:

            # Convert CGA framebuffer address 'ptr' to 8bpp byte address.
            # CGA is arranged as two 0x2000-byte interleaved planes.
            if ptr >= 0x2000:
                pixaddr = (4 * ((ptr - 0x2000) % 80)) + 320*(2 * ((ptr - 0x2000) // 80) + 1)
            else:
                pixaddr = (4 * (ptr % 80)) + 320*(2 * (ptr // 80))

            if pixaddr <= 63996:
                # Four pixels per byte, MSB first. Palette entry zero is transparent
                # for pixels that aren't written, and the next 4 entries represent CGA colors.
                pixels[pixaddr] = 1 + (3 & (byte >> 6))
                pixels[pixaddr + 1] = 1 + (3 & (byte >> 4))
                pixels[pixaddr + 2] = 1 + (3 & (byte >> 2))
                pixels[pixaddr + 3] = 1 + (3 & byte)

            ptr += 1
            count -= 1


def decode_all_images(showfile):
    for composited in (False, True):
        compressed_images = []
        if composited:
            # Opaque black, before the first frame, then every frame composited together
            pixels = bytearray(b'\x01' * (320*200))

        with open(showfile, 'rb') as file:
            number = 0
            while True:
                if not composited:
                    # Erase to transparent black before each frame, no compositing
                    pixels = bytearray(b'\x00' * (320*200))

                raw = bytearray()
                im = decompress_image(file, pixels, raw)
                if im is None:
                    break
                compressed_images.append(raw)

                name = '%s-%02d%s.png' % (os.path.splitext(showfile)[0], number, ('', '-c')[composited])
                im.save(name, transparency=0, optimize=1)
                print("%s is %d bytes, from the original %d byte RLE data" % (name, os.stat(name).st_size, len(raw)))
                number += 1

    return compressed_images


def repack_show_without_menu(dest_file, first_cutscene_frame, parts):
    end_frame = b'\x9B'

    with open(dest_file, 'wb') as file:
        for i, part in enumerate(parts):
            if i < first_cutscene_frame:
                # Menu and disk prompts. Replace with empty frames
                file.write(end_frame)
            else:
                file.write(part)

        print("%s is %d bytes, original file was %d bytes" %
            (dest_file, file.tell(), sum(len(part) for part in parts)))


def main(build):
    show = decode_all_images(os.path.join(build, 'show/show.shw'))
    show2 = decode_all_images(os.path.join(build, 'show/show2.shw'))
    print("Found %d images in show.shw and %d in show2.shw" % (len(show), len(show2)))

    repack_show_without_menu(os.path.join(build, 'fs/show.shw'), 0x07, show)
    repack_show_without_menu(os.path.join(build, 'fs/show2.shw'), 0x05, show2)

if __name__ == '__main__':
    main(sys.argv[1])
