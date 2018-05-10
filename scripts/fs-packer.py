#!/usr/bin/env python3

import sys
import os
import re

def collect_files(in_dir):
    fs = {}
    for entry in os.scandir(in_dir):
        dos_name = entry.name.upper()
        if entry.is_file() and re.match(r'[A-Z0-9]{1,8}\.[A-Z]{3}', dos_name):
            data = open(entry.path, 'rb').read()
            fs[dos_name] = data
    return fs

def varname_for_file(name):
    return 'file_%s' % name.lower().replace('.', '_')

def to_hex(seq):
    return ''.join(["0x%02x,%s" % (b, "\n"[:(i&15)==15])
                    for i, b in enumerate(seq)])

def write_header(cpp):
    cpp.write('// Automatically generated filesystem data\n\n'
              '#include <string.h>\n'
              '#include <stdint.h>\n'
              '#include "filesystem.h"\n\n')


def write_file_data(cpp, name, data):
    cpp.write('\nstatic const uint8_t %s_bytes[] = {\n%s\n};\n' % (
        varname_for_file(name), to_hex(data)));
    cpp.write('\nstatic struct FileInfo %s = {\n\t"%s", %s_bytes, %d\n};\n' % (
        varname_for_file(name), name, varname_for_file(name), len(data)));


def write_index(cpp, names):
    cpp.write('\nstatic const FileInfo* file_index[] = {\n')
    for name in names:
        cpp.write('\t&%s,\n' % varname_for_file(name))
    cpp.write('\t0,\n'
              '};\n')

def write_lookup(cpp):
    cpp.write('\nconst FileInfo* FileInfo::lookup(const char* name) {\n'
              '\tfor (unsigned i = 0; file_index[i]; i++) {\n'
              '\t\tif (!strcasecmp(name, file_index[i]->name)) {\n'
              '\t\t\treturn file_index[i];\n'
              '\t\t}\n'
              '\t}\n'
              '\treturn 0;\n'
              '}\n')


def main(in_dir, out_file):
    fs = collect_files(in_dir)
    cpp = open(out_file, 'w')
    write_header(cpp)
    for name, data in fs.items():
        write_file_data(cpp, name, data)
    write_index(cpp, fs.keys())
    write_lookup(cpp)
    cpp.close()


if __name__ == '__main__':
    if len(sys.argv) == 3:
        main(sys.argv[1], sys.argv[2])
    else:
        sys.stderr.write('usage: %s <input dir> <output file>' % sys.argv[0])

