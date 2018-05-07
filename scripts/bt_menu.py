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

bt_common.patchFramebufferTrace(b)
b.hook(b.entryPoint, 'enable_framebuffer_trace = true;')

# Dynamic branch for cutscene sound effects
b.patchDynamicBranch('019E:0778', [
    sbt86.Addr16(str='019E:0788'), 
    sbt86.Addr16(str='019E:07D0'), 
    sbt86.Addr16(str='019E:07AB')
])

# This isn't actually self-modifying code, but some data stored in the code segment
# will trigger some warnings that we can silence by marking the area explicitly as data.
b.patchDynamicLiteral('019E:0517', length=2)
b.patchDynamicLiteral('019E:0519', length=2)

# The main menu is a subroutine with a single caller;
# inline it to make cutting the control flow easier. Also disable framebuffer traces
# while drawing the main menu, so we can draw it faster.
b.patchAndHook('019E:012A', 'jmp 0x310', 'enable_framebuffer_trace = false;')
b.patchAndHook('019E:034A', 'jmp 0x12d', 'enable_framebuffer_trace = true;')

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
  
    # Redraw the screen and yield on the way out, check input on the way back in.
    # Use an arbitrarily low frame rate here, since we'll rely on skipping the delay
    # when the key event comes in.

    b.patchAndHook(call_site, 'ret',
        'hw->outputFrame(gStack, hw->memSeg(0xB800));'
        'hw->outputDelay(1000);'
        'proc->continueFrom(r, &sub_%X);' % continue_at.linear)
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

# Stub out the function which saves the wallclock time ref
b.patch(b.findCode(':b42c cd21 ________ c3'), 'ret')

# Split up the actual delays.
# Keyboard input will skip the delay, so flush any queued keystrokes after we come back.
for (call_site, delay) in [
    ('019E:010F', 4000),
    ('019E:0118', 4000),
    ('019E:01BD', 1000),
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(3)

    b.patchAndHook(call_site, 'ret',
        'hw->outputDelay(%d);'
        'proc->continueFrom(r, &sub_%X);' % (delay, continue_at.linear))    
    b.exportSub(continue_at)
    b.hook(continue_at, 'hw->clearKeyboardBuffer();')

# Break up more main loop call sites, related to the cutscenes. None of these
# are required, but exiting earlier will reduce cutscene load times and memory
# requirements since we don't have to queue so many frames before displaying them.
for call_site in [
    '019E:011E',
    '019E:0121',
    '019E:0124',
    '019E:0127',
    '019E:0134',
    '019E:013E',
    '019E:0148',
    '019E:014B',
    '019E:016C',
    '019E:016F',
    '019E:0172',
    '019E:0175',
    '019E:018F',
    '019E:0199',
    '019E:01BA',
    '019E:01C0',
    '019E:01C6',
    '019E:01CC',
    '019E:01CF',
    '019E:01D5',
    '019E:01E1',
    '019E:01E7',
    '019E:01ED',
    '019E:01F0',
    '019E:01F9',
    '019E:01FC',
    '019E:0205',
    '019E:020B',
    '019E:0211',
    '019E:0217',
    '019E:021A',
    '019E:0223',
    '019E:0226',
    '019E:0229',
    '019E:022F',
    '019E:0232',
    '019E:023D',
    '019E:0240',
    '019E:024B',
    '019E:0259',
    '019E:025C',
    '019E:0267',
    '019E:026A',
    '019E:0275',
    '019E:0278',
    '019E:027B',
    '019E:0286',
    '019E:0289',
    '019E:0294',
    '019E:0297',
    '019E:02A2',
    '019E:02A5',
    '019E:02B0',
    '019E:02BB',
    '019E:02C1',
    '019E:02C7',
    '019E:02CD',
    '019E:02D3',
    '019E:02D9',
    '019E:02E5',
    '019E:02E8',
    '019E:02EE',
    '019E:02F1',
    '019E:02F4',
]:
    call_site = sbt86.Addr16(str=call_site)
    target = b.jumpTarget(call_site)
    continue_at = call_site.add(1)
    b.patchAndHook(call_site, 'ret', 'proc->continueFrom(r, &sub_%X);' % continue_at.linear)
    b.patch(continue_at, 'call 0x%04x' % target.offset, length=2)
    b.exportSub(continue_at)


b.writeCodeToFile(os.path.join(basedir, 'bt_menu.cpp'), 'MenuEXE')
