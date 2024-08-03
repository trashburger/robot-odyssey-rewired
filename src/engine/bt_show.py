#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of MENU.EXE,
#
# Menus and the intro are in Javascript land now, so we only
# use this binary to run the new-game cutscene from SHOW.SHW.
# As a result, let's rename this to "show.exe" once translated.
#
# Micah Elizabeth Scott <micah@scanlime.org>
#

import os
import sys
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, "menu.exe"))
b.basename = "show.exe"

b.decl("#include <string.h>")

bt_common.patchJoystick(b)
bt_common.patchFramebufferTrace(b)
b.hook(b.entryPoint, "enable_framebuffer_trace = true;")

# Go directly to the new-game cutscene after we get the SHW file reader setup
b.patch("019E:00F8", "jmp 0x1A6")

# Launch the game once this cutscene ends
b.patchAndHook("019E:04DB", "ret", 'g.hw->exec("game.exe");')

# Time everything in this EXE, not just sound subroutines
sbt86.Subroutine.clockEnable = True

# Dynamic branch for cutscene sound effects
b.patchDynamicBranch(
    "019E:0778",
    [
        sbt86.Addr16(str="019E:0788"),
        sbt86.Addr16(str="019E:07D0"),
        sbt86.Addr16(str="019E:07AB"),
    ],
)

# This isn't actually self-modifying code, but some data stored in the code segment
# will trigger some warnings that we can silence by marking the area explicitly as data.
b.patchDynamicLiteral("019E:0517", length=2)
b.patchDynamicLiteral("019E:0519", length=2)

# This binary uses wallclock time for some delays.
# A save function is called before drawing the screen, to store a seconds timestamp.
# Then, one of two fixed-duration delay functions are called to wait 4 seconds or 1 second
# after the timestamp that was saved earlier.

# Stub out the function which saves the wallclock time ref
b.patch(b.findCode(":b42c cd21 ________ c3"), "ret")

# Split up delays.
# Keyboard input will skip the delay, so flush any queued keystrokes after we come back.
for call_site, delay in [
    ("019E:010F", 2000),
    ("019E:0118", 2800),
    ("019E:01BD", 1000),
    ("019E:01DB", 1000),
]:
    call_site = sbt86.Addr16(str=call_site)
    continue_at = call_site.add(3)

    b.patchAndHook(
        call_site,
        "ret",
        "g.hw->output.pushFrameCGA(g.stack, g.proc->memSeg(0xB800));"
        "g.hw->output.pushDelay(%d);"
        "g.proc->continueFrom(r, &sub_%X);" % (delay, continue_at.linear),
    )
    b.markSubroutine(continue_at)
    b.hook(continue_at, "g.hw->input.clear();")

# The cutscenes use a function I'm calling show_sfx_interruptible, which
# polls for keyboard input while playing a buffer of sound effects.
# Wrap each call site with some code that inserts a delay and breaks control flow.
# (Each invocation would include a joystick and DOS input poll, which takes
# some time)
show_sfx_interruptible = sbt86.Addr16(str="019E:0748")
for call_site in [
    "019E:01C6",
    "019E:01CC",
    "019E:01D5",
    "019E:01E1",
    "019E:01E7",
    "019E:01ED",
    "019E:01F9",
    "019E:0205",
    "019E:020B",
    "019E:0211",
    "019E:0217",
    "019E:0223",
    "019E:022F",
    "019E:023D",
    "019E:024B",
    "019E:0259",
    "019E:0267",
    "019E:0275",
    "019E:0286",
    "019E:0294",
    "019E:02A2",
    "019E:02B0",
    "019E:02BB",
    "019E:02C1",
    "019E:02C7",
    "019E:02CD",
    "019E:02D3",
    "019E:02D9",
]:
    call_site = sbt86.Addr16(str=call_site)
    target = b.jumpTarget(call_site)
    assert target.linear == show_sfx_interruptible.linear
    continue_at = call_site.add(3)
    b.markSubroutine(continue_at)
    b.markSubroutine(target)
    b.patchAndHook(
        call_site,
        "ret",
        "g.hw->output.pushFrameCGA(g.stack, g.proc->memSeg(0xB800));"
        "g.hw->output.pushDelay(50);"
        "sub_%X();"
        "g.proc->continueFrom(r, &sub_%X);" % (target.linear, continue_at.linear),
    )

b.writeCodeToFile(os.path.join(basedir, "bt_show.cpp"), "ShowEXE")
