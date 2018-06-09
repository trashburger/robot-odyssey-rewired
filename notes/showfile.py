#!/usr/bin/env python3
# pip3 install Pillow

import os, struct
from PIL import Image


def decode_next_image(file):
    # 9B: End of frame
    # 1B: 8-bit count, 8-bit value to repeat
    # E4: 16-bit LE count, 8-bit value to repeat
    # E6: 16-bit dest address
    # Other bytes are literal

    pixels = bytearray(320*200)
    ptr = 0

    while True:
        count = 1
        byte = file.read(1)
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
            count, byte = struct.unpack('<BB', file.read(2))

        elif byte == 0xE4:
            # 16-bit RLE
            count, byte = struct.unpack('<HB', file.read(3))

        elif byte == 0xE6:
            # 16-bit dest pointer
            ptr = struct.unpack('<H', file.read(2))[0]
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


def decode_all_images_from_show(filename):
    number = 0
    with open(filename, 'rb') as file:
        while True:
            first_offset = file.tell()
            img = decode_next_image(file)
            file_bytes = file.tell() - first_offset
            if img:
                number += 1
                name = "tmp/%s-%d.png" % (os.path.basename(filename), number)
                img.save(name, transparency=0, optimize=1)
                print("%s was %d bytes compressed, %d byte PNG" %
                    (name, file_bytes, os.stat(name).st_size))
            else:
                break


if __name__ == '__main__':
    for filename in ('build/fs/show.shw', 'build/fs/show2.shw'):
        decode_all_images_from_show(filename)

