#!/usr/bin/env python3

import sys, os, re, zstandard


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
    blob = b""
    for name in sorted(fs.keys()):
        print(f"Adding file {name}, {len(fs[name])} bytes at offset {len(blob)}")
        index.append('\t{ "%s", %d, %d },' % (name, len(blob), len(fs[name])))
        blob += fs[name]
    return (index, blob)


def to_hex(seq):
    return "".join(
        ["0x%02x,%s" % (b, "\n"[: (i & 15) == 15]) for i, b in enumerate(seq)]
    )


def write_header(cpp):
    cpp.write(
        "// Automatically generated filesystem data\n\n"
        "#include <stdint.h>\n"
        '#include "filesystem.h"\n\n'
    )


def write_blob(cpp, blob):
    compressed = zstandard.compress(blob, 22)
    print(
        f"Combined files are {len(blob)} bytes, compressed to {len(compressed)} bytes ( { len(compressed) / len(blob) * 100 }% )"
    )
    cpp.write("\nstatic const uint8_t fsPack[] = {\n%s\n};\n" % (to_hex(compressed),))
    cpp.write("static uint8_t fsUnpacked[%d];\n" % len(blob))
    cpp.write("const uint8_t* const CompressedFileInfo::packed = fsPack;\n")
    cpp.write("uint8_t* const CompressedFileInfo::unpacked = fsUnpacked;\n")
    cpp.write("const uint32_t CompressedFileInfo::packedSize = %d;\n" % len(compressed))
    cpp.write("const uint32_t CompressedFileInfo::unpackedSize = %d;\n" % len(blob))


def write_index(cpp, index):
    print(f"Writing index with {len(index)} files")
    cpp.write(
        "\nstatic const CompressedFileInfo fsIndex[%d] = {\n%s\n\t{nullptr, 0, 0}\n};\n"
        % (
            len(index) + 1,
            "\n".join(index),
        )
    )
    cpp.write("static FileInfo fsUnpackedIndex[%d];\n" % (len(index) + 1,))
    cpp.write("const CompressedFileInfo* const CompressedFileInfo::index = fsIndex;\n")
    cpp.write("FileInfo* const CompressedFileInfo::unpackedIndex = fsUnpackedIndex;\n")


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
