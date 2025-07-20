#!/usr/bin/env python3

import sys, os, re, zstd


def collect_files(in_dir):
    fs = {}
    for entry in os.scandir(in_dir):
        dos_name = entry.name.upper()
        if entry.is_file() and re.match(r"[A-Z0-9]{1,8}\.[A-Z]{3}", dos_name):
            data = open(entry.path, "rb").read()
            fs[dos_name] = data
    return fs


def varname_for_file(name):
    return "file_%s" % name.lower().replace(".", "_")


def combine_files(fs):
    index = []
    blob = b''
    for name in sorted(fs.keys()):
        index.append('\t{ .name = "%s", .offset = %d, .size = %d },' % (name, len(blob), len(fs[name])))
        blob += fs[name]
    return (index, blob)


def to_hex(seq):
    return "".join(
        ["0x%02x,%s" % (b, "\n"[: (i & 15) == 15]) for i, b in enumerate(seq)]
    )


def write_header(cpp):
    cpp.write(
        "// Automatically generated filesystem data\n\n"
        "#include <string.h>\n"
        "#include <stdint.h>\n"
        '#include "filesystem.h"\n\n'
    )


def write_blob(cpp, blob):
    compressed = zstd.compress(blob, 22)
    cpp.write("\nstatic const uint8_t fs_pack[] = {\n%s\n};\n" % (to_hex(compressed),))
    cpp.write("static uint8_t fs_unpacked[%d];\n" % len(blob))
    cpp.write("const uint8_t* const CompressedFileInfo::packed = fs_pack;\n")
    cpp.write("uint8_t* const CompressedFileInfo::unpacked = fs_unpacked;\n")
    cpp.write("const uint32_t CompressedFileInfo::packed_size = %d;\n" % len(compressed))
    cpp.write("const uint32_t CompressedFileInfo::unpacked_size = %d;\n" % len(blob))


def write_index(cpp, index):
    cpp.write("\nstatic const CompressedFileInfo fs_index[%d] = {\n%s\n\t{0}\n};\n" % (len(index) + 1, '\n'.join(index),))
    cpp.write("static FileInfo fs_cache[%d];\n" % (len(index) + 1,))
    cpp.write("const CompressedFileInfo* const CompressedFileInfo::index = fs_index;\n")
    cpp.write("FileInfo* const CompressedFileInfo::cache = fs_cache;\n")


def main(in_dir, out_file):
    cpp = open(out_file, "w")
    write_header(cpp)
    index, blob = combine_files(collect_files(in_dir))
    write_blob(cpp, blob)
    write_index(cpp, index)
    cpp.close()


if __name__ == "__main__":
    if len(sys.argv) == 3:
        main(sys.argv[1], sys.argv[2])
    else:
        sys.stderr.write("usage: %s <input dir> <output file>" % sys.argv[0])
