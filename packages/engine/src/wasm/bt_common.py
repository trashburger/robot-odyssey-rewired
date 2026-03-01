#
# Common binary translation patches and hooks for all Robot Odyssey binaries.
# Micah Elizabeth Scott <micah@scanlime.org>
#

import sbt86

# Default frame rate. This is somewhat faster and more consistent than the
# original experience would have been, but it seems good for gameplay. Much
# faster and it's hard to navigate and the puzzles run too quickly.
FRAME_RATE_DELAY = 1000 // 12

# Slower frame rate for the master computer center map
MAP_FRAME_DELAY = 1000 // 4


def patch(b):
    """Patches that should be applied to all binaries that have the Robot
    Odyssey game engine. (TUT, GAME, LAB)
    """

    # Each separate level has its own main loop, and each main loop needs to be
    # transformed in order to replace the final jump with a function pointer continuation,
    # and the loop body itself must be extracted into a separate function.
    # This pattern matches a common chunk of code that each main loop has, starting with a
    # call to the video_blit_frame utility (e8____) and ending with a jump back to the
    # beginning of that level's main loop (e9____) with a predictable set of room number
    # manipulations in-between.

    for loopJumpAddr in b.findCodeMultiple(
        "e8____ a0ac05a2____ a0____a2____ e8____ a0____a2____ :e9____"
    ):
        loopEntry = b.jumpTarget(loopJumpAddr)
        b.patchAndHook(
            loopJumpAddr,
            "ret",
            ("return rt.result.continueAlways(%s);") % b.symbols.name(loopEntry),
        )
        b.markSubroutine(loopEntry)

    # Don't bother zeroing video memory, we already clear all of RAM.

    b.patch(b.findCode(":9c 33c0 33ff 9d 7206 b90040 f3aa"), "ret")

    # Don't load a font. The game engine no longer needs a copy of the font,
    # it's converted to a webfont at build time.

    b.patch(
        b.findCode(
            ":ba1200 32c0 b43d cd21 72__ 8bd8 b43f b90003 ba____ cd21 b43e cd21"
        ),
        "nop",
        27,
    )

    # Locate the keyboard mapper, and convert ASCII keys to uppercase.
    # Most keyboard commands (tools, load/save, remote) compare the mapped
    # key against an uppercase character constant. Text entry that should
    # support mixed case (chip documentation in the lab) bypasses this mapper.
    #
    # The location in the mapper we're patching is immediately after the call to
    # bios_get_key, with the key still in AX.

    b.hook(
        b.findCode(
            "e8____ :75__ e9____ 80fc8874__ 80fc4d74__ 80fc8674__ 80fc5074__ 80fc8774"
        ),
        "if (rt.reg.al >= 'a' && rt.reg.al <= 'z') rt.reg.al += 'A' - 'a';",
    )

    # Remove the copy protection. (This would never work on an
    # emulated floppy controller anyway...) Not all binaries have the
    # copy protection itself (TUT.EXE is unprotected) but they all
    # have a call site just after the DOS error handler is installed.

    b.patch(b.findCode("b425 b024 cd21 1f1e :9a________ 1ff7d0"), "nop", 5)

    # Stub out video_set_cursor_position.
    #
    # It calls push/pop_ax_bx_cx_dx, which both do strange
    # things with the stack. This function doesn't appear to be
    # necessary anyway (and we don't implement the BIOS routine
    # it calls), so just stub it out.

    b.patch(b.findCode(":e80f00b700b402cd10e81400c3"), "ret")

    # One of the video_draw_playfield wrappers calls through a function
    # pointer. This pointer only ever has one value, so replace with an
    # unconditional jump.

    b.patchDynamicBranch(
        b.findCode("8a1e____8a3e____:ffe3 a0____3c9a7203"),
        [
            b.findCode(":e8c0ffb91e008b2e____8bf9"),
        ],
    )

    # Stub out self-modifying code. There are two instances of a function
    # which patches the text renderer (one for small text, one for large
    # text) with new addresses depending on the current font. It isn't
    # needed, since this game has only one font and it's already the one
    # present at disassembly-time.

    for addr in b.findCodeMultiple(
        ":b000 8a26____ d1e0 f81226____" "8b1e____ f813d8 2e891e____ f883db60", 2
    ):
        b.patch(addr, "ret")

    # This is a function pointer that appears to be some level-specific or
    # game-specific logic. As far as I can tell, it's always set to the
    # same value by the main loop, and never touched thereafter.

    b.patchDynamicBranch(
        b.findCode("75cc8bcf 880e____ :ff16____ a0____ 3cff"),
        [
            b.findCode(":8a16____ 80faff 74ef a0____ 32f6"),
        ],
    )

    # Object intersection tests make use of a callback function that tests
    # whether the object is allowed to be picked up. There are at least
    # three different implementations of this function: a stub, a simple
    # one which tests BX, and a complex one that handles robot grabbing.

    b.patchDynamicBranch(
        b.findCode("a2____ 0ac0 f8 c3 8a1e____ 8a3e____" ":ffe3 b500 8a____"),
        [
            # Required in all binaries
            b.findCode(":8a85____ 3cff 7402 f9c3 f8c3"),  # Normal callback
            b.findCode(":80fafc f5 72__ 80fa__ f5 72__"),  # Robot grabbing
            b.findCode(":f9c3 a1____ a2____ 8826____ e8"),  # Soldering (stub)
        ]
        # In GAME.EXE only?
        + b.findCodeMultiple(":80fa00 740c 80faf0 f5 7308 80faf6"),  # Non-player bots?
    )

    # At 1271 in TUT.EXE, there's a calculated jump which is used to
    # perform a left/right shift by 0 to 7. At this point, SI is the shift
    # value (times 2, because it's a word-wide table index) to apply to
    # AL.  0-7 are right shifts, 9-15 (-7 to 0) are left shifts.  After
    # shifting, the code in the jump table branches back to 1273 (the next
    # instruction).

    b.patchAndHook(
        b.findCode(
            "b500 8a0e____ 8bf9 8b1e____ 8a01 247f"
            "741c 8a1e____ 8a3e____ :ffe3 b500 8a0e____"
        ),
        "nop",
        length=2,
        cCode="""
        if (rt.reg.si < 16)
            rt.reg.al >>= rt.reg.si >> 1;
        else
            rt.reg.al <<= 16 - (rt.reg.si >> 1);
    """,
    )

    # Locate the robot data table, by finding the data itself.
    # The robot data table has two parts: A main table, and a grabber
    # table. We can't treat them all as one chunk, because the grabber
    # table length depends on the number of robots (which could be 3 or 4).

    b.publishAddress(
        "SBTADDR_ROBOT_DATA_MAIN",
        b.findData(
            ":f0 f0 f1 f1 51 52 53 54 55 56 57 58"
            " 00 00 00 00 07 07 07 07 00 00 00 00"
        ).offset,
    )
    b.publishAddress(
        "SBTADDR_ROBOT_DATA_GRABBER",
        b.findData("01 01 01 04 01 02 03" ":00010000 00010000 00010000").offset,
    )

    # Locate the circuit data, by finding a reference to it in the
    # code which draws all wires.

    b.publishAddress(
        "SBTADDR_CIRCUIT_DATA",
        b.peek16(b.findCode("80f9ff 744f 32ed 8bf1 8a84:____ 0ac0 7437 8a84____")),
    )

    # Locate the world data. The world data is originally loaded by following
    # a pointer which is statically initialized with the address of the data block.
    # Find that code, and read the pointer it references.

    worldPtr = b.peek16(b.findCode("8bd8 b43f 8b0e____ 8b16:____ cd21 b43e cd21"))
    b.publishAddress("SBTADDR_WORLD_DATA", b.peek16(worldPtr))


def patchFramebufferTrace(b):
    b.trace(
        "w",
        "return segment == 0xB800;",
        "rt.io->cgaWrite(rt.clock.take(), offset, read16(rt.eng->mem.seg(segment) + offset));",
    )


def patchChips(b):
    """Patches for binaries that support chips. (LAB and GAME, but not TUT)."""

    # There are several places where the chip simulator uses
    # self-modifying code which requires dynamic literals. The chip
    # simulator patches itself with destination addresses while
    # writing the result of simulating one gate.

    b.patchDynamicLiteral(
        b.findCode("e9c000 :8a9d____ b700 d1e3 8b87____" "86e0 260b84fefe 268984fefe"),
        192,
    )
    b.patchDynamicLiteral(b.findCode(":b200 32f6 8bfa 8b1e____ 8a01 3c07"), 896)
    b.patchDynamicLiteral(b.findCode("8a01 :a2____ 8ad0 32f6 8bfa 8a85____"), 3)

    # The chip simulator uses the stack in an interesting way... Any
    # time there is a gate simulation result which can't be written
    # immediately (it affects other gates, not just pins) it's added
    # to the stack. At the end of each nested chip, these results are
    # popped off the stack and written to memory. The results are
    # enqueued by a subroutine, so this subroutine needs to affect the
    # caller's stack, without its own return value getting in the
    # way. It does this by temporarily removing the return value from
    # the stack. We need to patch this code so that our strongly typed
    # stack is okay with this usage.

    b.hook(b.findCode(":58 a3____ 32f68bfa 8b1e"), "rt.stack.preSaveRet(rt.io);")
    b.hook(
        b.findCode("8ac2 e8____ b200 a1____ 50 :c3"), "rt.stack.postRestoreRet(rt.io);"
    )

    b.hook(b.findCode(":58 a3____ a0____ 0ac0 7417"), "rt.stack.preSaveRet(rt.io);")
    b.hook(b.findCode("75e9 a1____ 50 :c3"), "rt.stack.postRestoreRet(rt.io);")

    # These seem to only be in LAB.EXE? Maybe chip burning related?

    for addr in b.findCodeMultiple(":8885efef"):
        b.patchDynamicLiteral(addr, 4)
    for addr in b.findCodeMultiple(":8884fefe"):
        b.patchDynamicLiteral(addr, 4)
    for addr in b.findCodeMultiple(":8a84fefe"):
        b.patchDynamicLiteral(addr, 4)


def patchLoadSave(b):
    """Patch loading and saving. This removes the game's normal file picking UI.
    Instead, we'll have the game load/save with an arbitrary file name and we'll
    handle loading/storing that ourselves.
    """

    # Find the location of the save filename buffer in the data
    # segment.  To locate the name, we'll find the snippet of code
    # which creates a new file using this name. This buffer is about
    # 20 bytes long.

    saveFilenameAddr = b.peek16(b.findCode("b000 b43c 33c9 ba:____ cd21"))

    # The load filename address is the same as the current world name.
    # It's probably safe to hardcode this...

    loadFilenameAddr = 0x0002

    # Now find the subroutines which ask the user which file to load
    # or save.  There are two separate functions, one for load and one
    # for save. This pattern matches both. The same function is used
    # for chips and for game saves.
    #
    # This function returns bx=0 on failure. On success, it seems to
    # return a pointer to the file name.
    #
    # So, return success, and set a static name of 'savefile'.
    #
    # This is called early-on within the actual save function (below)

    for addr in b.findCodeMultiple(
        ":e8____ f8 e8____ c706________ 803e____00"
        "7406 c706________ e8____ e8____ e8____",
        2,
    ):
        b.patchAndHook(
            addr,
            "ret",
            """
            rt.reg.bx = 0x%04x;
            memcpy((char*)rt.seg.ds + 0x%04x, SBT_SAVE_FILE_NAME, sizeof SBT_SAVE_FILE_NAME);
            memcpy((char*)rt.seg.ds + 0x%04x, SBT_SAVE_FILE_NAME, sizeof SBT_SAVE_FILE_NAME);
        """ % (saveFilenameAddr, saveFilenameAddr, loadFilenameAddr),
        )

    # Find the code for the menu that asks you to press "S" to save.
    # Specifically, we're locating the comparison for that "S" and patching
    # the basic block that leads to the actual save routine. Break the inlining
    # there and turn save into a separate subroutine, and export it.

    jumpToSave = b.findCode(
        "e8____ 3c1b 7409 3c53 7408 3c0d 75f1 c3 e9____ b205 :e9____"
    )
    saveFunc = b.jumpTarget(jumpToSave)
    b.publishSubroutine("SBTADDR_SAVE_GAME_FUNC", saveFunc)
    b.patch(jumpToSave, "call 0x%x" % saveFunc.offset, length=1)
    b.patch(jumpToSave.add(1), "ret")

    # Most file operations can be dealt with asynchronously, but when the game tries
    # to load chip data, this particular invocation of the filename picker functions
    # (patched above) needs to correspond with a synchronous file picker on the
    # JS ui side. To do this, we patch right after the chip ID to load has been
    # determined, issuing a callback, and publishing the address of a continuation.
    #
    # The actual function we're patching is a mov that stores the chip ID from DL
    # in a data segment byte.

    loadChipEntry = b.findCode(
        ":8816____ e8____ a0____ 98 f726____ 0500__ a3____ a0____ 98 d1e0 d1e0 d1e0"
        "05____ a3____ c606____01 e8____ 0bdb 75__"
    )
    loadChipContinued = loadChipEntry.add(3)
    b.patch(
        loadChipContinued,
        "mov byte [0x%x],dl" % b.peek16(loadChipEntry.add(2)),
        length=1,
    )
    b.publishSubroutine("SBTADDR_LOAD_CHIP_FUNC", loadChipContinued)
    b.patchAndHook(loadChipEntry, "ret", "// g.eng->requestLoadChip(r);")


def patchJoystick(b, map_active_addr=-1):
    # This game uses cycle timing to poll the joystick; we could emulate that
    # using the global clock, but it's easier and more efficient to replace this
    # with a direct call to a polling hook.

    # Find the poller itself. It returns X and Y cycle counts in bx and cx,
    # but button status gets AND'ed together during polling and stored in
    # a variable that we need to find the address of.

    poller = b.findCode(":8b16____ bb0000 b90000 c606____ff eeec2006")
    button_addr = b.peek16(poller.add(12))
    world_addr = b.getAddress("SBTADDR_WORLD_DATA", default=-1)
    b.patchAndHook(
        poller,
        "ret",
        f"""{{
            bool map_active = {map_active_addr} > 0 ? (rt.seg.ds[(uint16_t){map_active_addr}] != 0) : false;
            const uint8_t *world = {world_addr} > 0 ? (rt.seg.ds + {world_addr}) : 0;
            auto result = rt.eng->js.poll(rt.io, world, map_active);
            rt.reg.bx = result.x;
            rt.reg.cx = result.y;
            rt.seg.ds[{button_addr}] = result.status;
        }}""",
    )


def patchVideoBlit(b, code, map_active_addr=-1):
    # Common code for video blit loop replacement, including timing and enable/disable.

    # Replace the game's blitter. The normal blit loop is really large,
    # and unnecessary for us. Replace it with a call to
    # consoleBlitToScreen(), and read directly from the game's backbuffer.

    # Add the bulk of our frame delay after the frame is published, rather than before.
    # When we are in the master computer center map, set a longer frame delay here.
    # (Delaying in map-specific code before rendering would increase latency.)

    b.decl("static bool noBlit = false;")
    b.patchAndHook(
        b.findCode(
            ":803e____01 7503 e95a05 c43e____" "bb2800 a1____ 8cda 8ed8 be0020 33 c0"
        ),
        "ret",
        f"""
        if (!noBlit) {{
            {code}
            {{
               uint8_t map_active = {map_active_addr} > 0 ? rt.seg.ds[{map_active_addr}] : 0;
               rt.clock.addMillisec(map_active ? {MAP_FRAME_DELAY} : {FRAME_RATE_DELAY});
            }}
        }}
    """,
    )

    # Avoid calling the blitter during initialization. Normally the
    # game clears the screen during initialization, but for us this is
    # inconvenient- it means if we want to capture the screen after
    # loading a game, we can't just wait for the first blit. To ensure
    # that the first blitted frame is a 'real' frame, remove this blit
    # call.
    #
    # Note that we can't just stub out the call, since other parts
    # of the initialization rely on pointers that are set here.

    b.hook(
        b.findCode("c3 :e8____ b500 b100 8bf9 8a85ac05 a2____" "e8____ e8____ c3"),
        "noBlit = true;",
    )
    b.hook(
        b.findCode("c3 e8____: b500 b100 8bf9 8a85ac05 a2____" "e8____ e8____ c3"),
        "noBlit = false;",
    )


def patchVideoHighLevel(b, trace_debug=False, map_active_addr=-1):
    # Replace all rendering, using our own high-depth backbuffer.

    # Help us track down writes directly to the backbuffer
    if trace_debug:
        b.trace(
            "w",
            "return segment == read16(rt.seg.ds + 0x3AD5);",
            'rt.io->error("Unpatched backbuffer write");',
        )

    # Low-level sprite (bitmap) rendering
    video_draw_sprite = b.findCode(":A0____ 3c9a 7203 e9c203 a0____ 3cb1")
    drawing_arg_x = b.peek16(video_draw_sprite.add(1))
    drawing_arg_y = drawing_arg_x - 0x4
    sprite_data_ptr = drawing_arg_x - 0xB
    string_ptr = drawing_arg_x - 0xE
    b.patchAndHook(
        video_draw_sprite,
        "ret",
        f"rt.io->sprite(rt.seg.ds[{drawing_arg_x}], rt.seg.ds[{drawing_arg_y}], rt.reg.cl, rt.seg.ds + read16(rt.seg.ds + {sprite_data_ptr}));",
    )

    # Hook the main text renderer entry point, before branching
    # to renderers separately for normal and large text based on 'style'.
    video_draw_text = b.findCode(":a0____ 3c00 7404 3c02 7404 e8____ c3 e8____ c3")

    # Most text arguments are clustered around 'style' which we have a pointer to from above
    text_arg_style = b.peek16(video_draw_text.add(1))
    text_arg_string = text_arg_style - 2
    text_arg_color = text_arg_style + 1
    text_arg_x = text_arg_style + 2
    text_arg_y = text_arg_style + 3
    text_arg_font = text_arg_style + 4

    b.patchAndHook(
        video_draw_text,
        "ret",
        f"rt.io->text(rt.seg.ds[{text_arg_x}], rt.seg.ds[{text_arg_y}], rt.seg.ds[{text_arg_color}], rt.seg.ds[{text_arg_font}], rt.seg.ds[{text_arg_style}], (const char*)rt.seg.ds + read16(rt.seg.ds + {text_arg_string}));",
    )

    # Get pointer to the 30-byte room data block from the low-level playfield renderer
    playfield_data_ptr = b.peek16(
        b.findCode("e8____ b91e00 8b2e:____ 8bf9 4f d1e7 8bb5")
    )

    # Hook the slightly higher level renderer that gets 8-bit foreground/background colors
    video_draw_playfield_with_colors = b.findCode(
        ":" + ("8a______ a2____" * 8) + "e8____c3"
    )
    b.patchAndHook(
        video_draw_playfield_with_colors,
        "ret",
        f"rt.io->displayListClear();"
        f"rt.io->room(rt.reg.di, rt.reg.si, rt.seg.ds + read16(rt.seg.ds + {playfield_data_ptr}));",
    )

    # Vertical line drawing

    video_draw_vline = b.findCode(
        ":32FF 8a1e____ e8____ 8bf3 8a1e____ e8____ 8bcb 2bce 7904"
    )
    drawing_arg_y1 = b.peek16(video_draw_vline.add(0x04))
    drawing_arg_y2 = b.peek16(video_draw_vline.add(0x0D))
    drawing_arg_line_color = b.peek16(video_draw_vline.add(0x1F))
    drawing_arg_x = b.peek16(video_draw_vline.add(0x29))

    b.patchAndHook(
        video_draw_vline,
        "ret",
        f"rt.io->vline(rt.seg.ds[{drawing_arg_x}], rt.seg.ds[{drawing_arg_y1}], rt.seg.ds[{drawing_arg_y2}], rt.seg.ds[{drawing_arg_line_color}]);",
    )

    # Horizontal line drawing

    video_draw_hline = b.findCode(
        ":32FF 8a1e____ e8____ 8bfb 8a1e____ e8____ 8bcb 2bcf 7904"
    )
    drawing_arg_x1 = b.peek16(video_draw_hline.add(0x04))
    drawing_arg_x2 = b.peek16(video_draw_hline.add(0x0D))
    drawing_arg_line_color = b.peek16(video_draw_hline.add(0x1F))
    drawing_arg_y = b.peek16(video_draw_hline.add(0x2B))

    b.patchAndHook(
        video_draw_hline,
        "ret",
        f"rt.io->hline(rt.seg.ds[{drawing_arg_x1}], rt.seg.ds[{drawing_arg_x2}], rt.seg.ds[{drawing_arg_y}], rt.seg.ds[{drawing_arg_line_color}]);",
    )

    # Replace backbuffer-to-frontbuffer blit with display list presentation
    patchVideoBlit(
        b,
        "rt.io->delay(rt.clock.take());" "rt.io->displayListPresent();",
        map_active_addr=map_active_addr,
    )


def patchShowKeyboardDelays(b, call_site_and_delay_list):
    # Split up delays.
    # Keyboard input will skip the delay, so flush any queued keystrokes after we come back.
    for call_site, delay in call_site_and_delay_list:
        call_site = sbt86.Addr16(str=call_site)
        continue_at = call_site.add(3)

        b.patchAndHook(
            call_site,
            "ret",
            ("rt.clock.addMillisec(%d);" "return rt.result.continueAlways(%s);")
            % (delay, b.symbols.name(continue_at)),
        )
        b.markSubroutine(continue_at)


def patchShowSfxInterruptible(
    b, show_sfx_interruptible, call_site_list, extra_delay_msec=0
):
    # The cutscenes use a function I'm calling show_sfx_interruptible, which
    # polls for keyboard input while playing a buffer of sound effects.
    # Wrap each call site with some code that inserts a delay and breaks control flow.
    # (Each invocation would include a joystick and DOS input poll, which takes
    # some time)
    #
    # The corresponding output delays vary.
    # Try to measure it using the CPU clock.

    show_sfx_interruptible = sbt86.Addr16(str=show_sfx_interruptible)
    for call_site in call_site_list:
        call_site = sbt86.Addr16(str=call_site)

        target = b.jumpTarget(call_site)
        assert target.linear == show_sfx_interruptible.linear
        continue_at = call_site.add(3)
        b.markSubroutine(continue_at)
        b.markSubroutine(target)

        b.patchAndHook(
            call_site,
            "ret",
            "if (%s(rt)) return true;"
            "rt.clock.addMillisec(%d);"
            "return rt.result.continueOnce(%s);"
            % (
                b.symbols.name(target),
                extra_delay_msec,
                b.symbols.name(continue_at),
            ),
        )
