#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of PLAY.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import sys
import os
import sbt86

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'play.exe'))

# Break control flow every time we would be exec()'ing another EXE file.
# This is a wrapper that handles changing disks if necessary. BX is a pointer
# to a nul-terminated list of nul-terminated strings, forming the argv[].

b.decl('#include <string.h>')

for call_site in [
    '0070:003E',
    '0070:0044',
    '0070:004E',
    '0070:0057',
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(3)
  
    # Redraw the screen and yield on the way out, check input on the way back in
    b.patchAndHook(call_site, 'ret',
  		'const char *argv = (const char*) (hw->memSeg(r.ds) + r.bx);'
        'hw->exec(argv, argv+strlen(argv)+1);'
        'proc->continue_from(r, &sub_%X);' % continue_at.linear)
    b.exportSub(continue_at)

b.writeCodeToFile(os.path.join(basedir, 'bt_play.cpp'), 'PlayEXE')
