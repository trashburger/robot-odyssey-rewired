#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of MENU2.EXE.
# This is an alternate build of the main menu for Disk 2,
# and it's also where the final cutscene is stored.
#
# Since we don't emulate disk swapping, this file is only used
# for the final cutscene in this port.
#
# Micah Elizabeth Scott <micah@scanlime.org>
#

import os
import sys
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, 'menu2.exe'))

bt_common.patchJoystick(b)
bt_common.patchFramebufferTrace(b)
b.hook(b.entryPoint, 'enable_framebuffer_trace = true;')

# Time everything in this EXE, not just sound subroutines
sbt86.Subroutine.clockEnable = True

# Dynamic branch for cutscene sound effects
b.patchDynamicBranch('019E:068B', [
    sbt86.Addr16(str='019E:069B'),
    sbt86.Addr16(str='019E:06E3'),
    sbt86.Addr16(str='019E:06BE')
])

# This isn't actually self-modifying code, but some data stored in the code segment
# will trigger some warnings that we can silence by marking the area explicitly as data.
b.patchDynamicLiteral('019E:0434', length=2)
b.patchDynamicLiteral('019E:0436', length=2)

# Remove the unneeded splashscreen and main menu
b.patch('019E:0102', 'ret')

# At the very end of the final cutscene, we're greeted with an
# infinite loop. Break that loop with a delay.
final_loop = sbt86.Addr16(str='019E:0221')
b.markSubroutine(final_loop)
b.patchAndHook(final_loop, 'ret',
   	'g.hw->output.pushFrameCGA(g.stack, g.proc->memSeg(0xB800));'
    'g.hw->output.pushDelay(1000);'
    'g.proc->continueFrom(r, &sub_%X);' % final_loop.linear)

# Split up delays.
# Keyboard input will skip the delay, so flush any queued keystrokes after we come back.
for (call_site, delay) in [
    ('019E:018B', 1000),
    ('019E:0204', 1500),
    ('019E:0218', 1500),
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(3)

    b.patchAndHook(call_site, 'ret',
        'g.hw->output.pushFrameCGA(g.stack, g.proc->memSeg(0xB800));'
        'g.hw->output.pushDelay(%d);'
        'g.proc->continueFrom(r, &sub_%X);' % (delay, continue_at.linear))
    b.markSubroutine(continue_at)
    b.hook(continue_at, 'g.hw->input.clear();')

# The cutscenes use a function I'm calling show_sfx_interruptible, which
# polls for keyboard input while playing a buffer of sound effects.
# Wrap each call site with some code that inserts a delay and breaks control flow.
# (Each invocation would include a joystick and DOS input poll, which takes
# some time)
show_sfx_interruptible = sbt86.Addr16(str='019E:0665')
for call_site in [
    '019E:0196',
    '019E:01A4',
    '019E:01B2',
    '019E:01C3',
    '019E:01D1',
    '019E:01DF',
    '019E:01ED',
    '019E:01FE',
    '019E:0212',
]:
    call_site = sbt86.Addr16(str=call_site)
    target = b.jumpTarget(call_site)
    assert target.linear == show_sfx_interruptible.linear
    continue_at = call_site.add(3)
    b.markSubroutine(continue_at)
    b.markSubroutine(target)
    b.patchAndHook(call_site, 'ret',
        'g.hw->output.pushFrameCGA(g.stack, g.proc->memSeg(0xB800));'
        'g.hw->output.pushDelay(50);'
        'sub_%X();'
        'g.proc->continueFrom(r, &sub_%X);' % (
            target.linear, continue_at.linear))

b.writeCodeToFile(os.path.join(basedir, 'bt_menu2.cpp'), 'Menu2EXE')
