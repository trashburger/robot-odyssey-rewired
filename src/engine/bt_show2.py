#!/usr/bin/env python3
#
# Patches and hooks for the binary translation of MENU2.EXE.
# This is an alternate build of the main menu for Disk 2,
# and it's also where the final cutscene is stored.
#
# Menus and the intro are in Javascript land now, so we only
# use this binary to run the final cutscene from SHOW2.SHW.
# As a result, let's rename this to "show2.exe" once translated.
#
# Micah Elizabeth Scott <micah@scanlime.org>
#

import os
import sys
import sbt86
import bt_common

basedir = sys.argv[1]
b = sbt86.DOSBinary(os.path.join(basedir, "menu2.exe"))
b.basename = "show2.exe"

bt_common.patchJoystick(b)
bt_common.patchFramebufferTrace(b)
b.hook(b.entryPoint, "enable_framebuffer_trace = true;")

# Time everything in this EXE, not just sound subroutines
sbt86.Subroutine.clockEnable = True

# Dynamic branch for cutscene sound effects
b.patchDynamicBranch(
    "019E:068B",
    [
        sbt86.Addr16(str="019E:069B"),
        sbt86.Addr16(str="019E:06E3"),
        sbt86.Addr16(str="019E:06BE"),
    ],
)

# This isn't actually self-modifying code, but some data stored in the code segment
# will trigger some warnings that we can silence by marking the area explicitly as data.
b.patchDynamicLiteral("019E:0434", length=2)
b.patchDynamicLiteral("019E:0436", length=2)

# Go directly to the end-game cutscene after we get the SHW file reader setup.
# This also skips over some code for skipping menu and loading images, as we've removed those.
b.patch("019E:00F8", "jmp 0x185")

# At the very end of the final cutscene, we're greeted with an
# infinite loop. Break that loop with a delay.
final_loop = sbt86.Addr16(str="019E:0221")
b.markSubroutine(final_loop)
b.patchAndHook(
    final_loop,
    "ret",
    "g.hw->output.pushFrameCGA(g.clock, g.stack, g.proc->memSeg(0xB800));"
    "g.clock += OutputInterface::msecToClocks(1000);"
    "g.proc->continueFrom(r, &sub_%X);" % final_loop.linear,
)

# Split up keyboard input delays.
bt_common.patchShowKeyboardDelays(
    b,
    [
        ("019E:018B", 500),
        ("019E:0204", 2000),
        ("019E:0218", 2000),
    ],
)

# Patch control flow around show_sfx_interruptible calls
bt_common.patchShowSfxInterruptible(
    b,
    "019E:0665",
    [
        "019E:0196",
        "019E:01A4",
        "019E:01B2",
        "019E:01C3",
        "019E:01D1",
        "019E:01DF",
        "019E:01ED",
        "019E:01FE",
        "019E:0212",
    ],
    extra_delay_msec=100,
)

b.writeCodeToFile(os.path.join(basedir, "bt_show2.cpp"), "Show2EXE")
