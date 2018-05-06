#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of MENU.EXE.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import os
import sys
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'menu.exe'))

bt_common.patchMenu(b)

# Dynamic branch for cutscene sound effects
b.patchDynamicBranch('019E:0778', [
    sbt86.Addr16(str='019E:07D0'), 
    sbt86.Addr16(str='019E:07AB')
])

# This isn't actually self-modifying code, but some data stored in the code segment
# will trigger some warnings that we can silence by marking the area explicitly as data.
b.patchDynamicLiteral('019E:0517', length=2)
b.patchDynamicLiteral('019E:0519', length=2)

# The main menu is a subroutine with a single caller; inline it to make cutting the control flow easier
b.patch('019E:012A', 'jmp 0x310')
b.patch('019E:034A', 'jmp 0x12d')

# Now break control flow every time we see a call site for the main input polling function at 019E:0428
input_poll_func = sbt86.Addr16(str='019E:0428')
for call_site in [
    '019E:0141',
    '019E:014e',
    '019E:0178',
    '019E:019c',
    '019E:02f7',
    '019E:0315',
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(1)
    assert b.jumpTarget(call_site).linear == input_poll_func.linear
  
    # Redraw the screen and yield on the way out, check input on the way back in
    b.patchAndHook(call_site, 'ret',
        'hw->outputFrame(proc, hw->memSeg(0xB800));'
        'fprintf(stderr, "continue from %r\\n");'
        'proc->continue_from(r, &sub_%X);' % (continue_at, continue_at.linear))
    b.patch(continue_at, 'call 0x%04x' % input_poll_func.offset, length=2)
    b.exportSub(continue_at)

# This is also an input poll callsite, but it's for skipping cutscenes and there isn't
# a good way to break it up with continuations. Just remove it, the cutscenes don't
# need to be skippable like this anyway, we can include a fast forward mode.

b.patch('019E:074c', 'nop', length=3)

# The menu uses wallclock time for some delays.
# A save function is called before drawing the screen, to store a seconds timestamp.
# Then, one of two fixed-duration delay functions are called to wait 4 seconds or 1 second
# after the timestamp that was saved earlier.

# Stub out the save function
b.patch(b.findCode(':b42c cd21 ________ c3'), 'ret')

for (call_site, delay) in [
    ('019E:010F', 4000),
    ('019E:0118', 4000),
    ('019E:01BD', 1000),
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(3)

    b.patchAndHook(call_site, 'ret',
        'hw->outputDelay(proc, %d);'
        'proc->continue_from(r, &sub_%X);' % (delay, continue_at.linear))    
    b.exportSub(continue_at)


b.writeCodeToFile(os.path.join(basedir, 'bt_menu.cpp'), 'MenuEXE')
