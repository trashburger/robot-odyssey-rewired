#!/usr/bin/env python3

import os, struct
from PIL import Image


def decode_next_image(file):
	# 9B: End of frame
	# 1B: 8-bit count, 8-bit value to repeat 
	# E4: 16-bit LE count, 8-bit value to repeat
	# E6: 16-bit dest address
	# Other bytes are literal

	img = Image.new('RGBA', (320, 200))
	ptr = 0

	while True:
		count = 1
		byte = file.read(1)
		if len(byte) < 1:
			# End of file
			return None
		byte = ord(byte)

		if byte == 0x9B:
			# Successful end of image
			return img

		if ptr >= 0x4000:
			# Ignore anything except 0x9B when past the framebuffer end
			continue

		if byte == 0x1B:
			# 8-bit RLE
			count = struct.unpack('<B', file.read(1))[0]
			byte = ord(file.read(1))

		elif byte == 0xE4:
			# 16-bit RLE
			count = struct.unpack('<H', file.read(2))[0]
			byte = ord(file.read(1))

		elif byte == 0xE6:
			# 16-bit dest pointer
			ptr = struct.unpack('<H', file.read(2))[0]
			continue

		# A run of identical bytes, each with 4 pixels
		while count > 0:
			plot_cga_byte(img, ptr, byte)
			ptr += 1
			count -= 1


def plot_cga_byte(img, ptr, byte):
	palette = [
		0xff000000,
		0xffd8a909,
		0xff317aed,
		0xffffffff,
	]

	if ptr >= 0x2000:
		x = 4 * ((ptr - 0x2000) % 80)
		y = 2 * ((ptr - 0x2000) // 80) + 1
	else:
		x = 4 * (ptr % 80)
		y = 2 * (ptr // 80)

	for bit in range(4):
		img.paste(palette[3 & (byte >> 6)], (x,y,x+1,y+1))
		img.paste(palette[3 & (byte >> 4)], (x+1,y,x+2,y+1))
		img.paste(palette[3 & (byte >> 2)], (x+2,y,x+3,y+1))
		img.paste(palette[3 & (byte     )], (x+3,y,x+4,y+1))


def decode_all_images_from_show(filename):
	number = 0
	with open(filename, 'rb') as file:
		while True:
			img = decode_next_image(file)
			if img:
				number += 1
				name = "tmp/%s-%d.png" % (os.path.basename(filename), number)
				img.save(name)
				print(name)
			else:
				break


if __name__ == '__main__':
	for filename in ('build/fs/show.shw', 'build/fs/show2.shw'):
		decode_all_images_from_show(filename)

