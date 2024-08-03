#
# Experimental DOS 8086 -> C static binary translator.
#
# This isn't a general-purpose static binary translator. That wouldn't
# be possible, since self-modifying code and dynamic branches make it
# impossible to create a static translation of any arbitrary program.
# Instead, this is a toolkit which does *most* of the work in creating
# a static translation of one particular program. You'll still need to
# write some program-specific code in order to fix up any untranslateable
# parts in the binary.
#
# These fixups come in the form of a separate per-program Python file
# where you will create a DOSBinary() class to represent your input
# file, apply patches to that in-memory representation, then generate
# code. The patches can either replace original assembly instructions
# (a 'patch') or add C code which executes just prior to an original
# instruction (a 'hook').
#
# Patches affect the analysis phase, where we discover subroutines and
# figure out which parts of the binary are executable code. So, if you
# stub out a function by patching its first instruction with a 'ret',
# or if you patch out any call sites, no code will be generated for
# that function. If you want to replace an original subroutine with a
# C-language routine, for example, you can patch in a 'ret'
# instruction, then hook the 'ret'.
#
# Requires the ndisasm disassembler.
#
# Copyright (c) 2009-2018 Micah Elizabeth Scott <micah@scanlime.org>
#
#    Permission is hereby granted, free of charge, to any person
#    obtaining a copy of this software and associated documentation
#    files (the "Software"), to deal in the Software without
#    restriction, including without limitation the rights to use,
#    copy, modify, merge, publish, distribute, sublicense, and/or sell
#    copies of the Software, and to permit persons to whom the
#    Software is furnished to do so, subject to the following
#    conditions:
#
#    The above copyright notice and this permission notice shall be
#    included in all copies or substantial portions of the Software.
#
#    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#    OTHER DEALINGS IN THE SOFTWARE.
#

import os
import sys
import struct
import subprocess
import re
import binascii
import tempfile
import io


def log(msg):
    print("SBT86: %s" % msg)


class Signature:
    """A binary signature, which can be used to identify a code fragment
    at an unknown address. The signature consists of a list of
    bytes before and after the matching address, as well as an
    optional list of wildcard bytes to exclude from the search.
    The signature is formatted as a hexadecimal string. A ':'
    represents the search address. Characters prior to the colon
    occur before the search address, and characters afterward occur
    at/after the address. Underscores represent 'blanks', that can
    contain any value.  Underscores must by byte
    aligned. Whitespace is optional. The '#' character marks the
    beginning of a comment, all text until the next newline is
    ignored.
    """

    def __init__(self, text):
        self.longText = text

        # Remove comments
        t = re.sub(r"#.*", "", text)

        # Remove whitespace
        t = t.replace("\n", "").replace("\t", "").replace(" ", "")

        self.shortText = t

        m = re.search(r"[^0-9a-fA-F_:]", t)
        if m:
            raise SignatureFormatError(
                "Invalid character %r in pattern %r" % (m.group(0), self.shortText)
            )

        parts = t.split(":")
        if len(parts) != 2:
            raise SignatureFormatError(
                "Pattern must have exactly one ':' to" " mark the search address."
            )
        pre, post = parts
        self.preLength = len(pre) // 2
        t = pre + post

        if len(t) % 2:
            raise SignatureFormatError(
                "Pattern must have an even " "number of digits: %r" % self.shortText
            )

        regex = b""
        for byte in range(len(t) // 2):
            hexByte = t[byte * 2 : (byte + 1) * 2]
            if "_" in hexByte:
                if hexByte == "__":
                    regex += b"."
                else:
                    SignatureFormatError(
                        "Pattern has blanks which are "
                        "not byte aligned: %r" % self.shortText
                    )
            else:
                regex += b"\\x%02x" % int(hexByte, 16)

        self.re = re.compile(regex)

    def find(self, str):
        """Locate the pattern in a string, with zero or more occurrances.
        Returns a list of offsets, in bytes from the beginning of the string.
        """
        return [m.start(0) + self.preLength for m in self.re.finditer(str)]


class SignatureFormatError(Exception):
    pass


class SignatureMatchError(Exception):
    pass


class Addr16:
    """A 16-bit Real Mode address.
    Requires 2 out of 3 parameters, or 'str' to parse an
    address in hexadecimal string form.
    """

    width = 4

    def __init__(self, segment=None, offset=None, linear=None, str=None):
        if str is not None:
            if ":" in str:
                segment, offset = str.split(":")
                segment = int(segment, 16)
                offset = int(offset, 16)
            else:
                segment = 0
                offset = int(str, 16)

        if linear is None:
            linear = (segment << 4) + offset
        elif segment is None:
            s = linear - offset
            assert (s & 0xF) == 0
            segment = s >> 4
        elif offset is None:
            offset = linear - (segment << 4)

        segment += (offset >> 16) << 12
        offset &= 0xFFFF

        self.segment = segment
        self.offset = offset
        self.linear = linear

    def __lt__(self, other):
        return self.linear < other.linear

    def __eq__(self, other):
        return self.linear == other.linear

    def add(self, x, wrap=False):
        if isinstance(x, Addr16):
            s = self.segment + x.segment
            o = self.offset + x.offset
        else:
            s = self.segment
            o = self.offset + x
        if wrap:
            o = o & 0xFFFF
        return Addr16(s, o)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "%04X:%04X" % (self.segment, self.offset)

    def label(self):
        return "loc_%X" % self.linear


class Literal(int):
    def __new__(self, x, addr=None, width=None, dynamic=False):
        """A literal value. Optionally has a width in bytes,
        and an Addr16 where this literal is located in the
        code segment.

        If 'dynamic' is set, this literal may be modified at
        runtime. We code-generate it as a reference to the
        immeidate data within an instruction in the code
        segment.
        """
        obj = int.__new__(self, x)
        obj.addr = addr
        obj.width = width
        obj.dynamic = dynamic
        return obj

    def __repr__(self):
        return self.codegen()

    def codegen(self):
        if self.dynamic:
            return Indirect(
                Register("cs"), (Literal(self.addr.offset),), self.width
            ).codegen()

        if self < 16:
            return "%d" % self
        elif self < 0x100:
            return "0x%02x" % self
        else:
            return "0x%04x" % self


class Register:
    def __init__(self, name):
        name = name.lower()
        self.name = name

        if name[-1] in "lh":
            self.width = 1
        else:
            self.width = 2

    def __repr__(self):
        return "Reg(%s)" % self.name

    def codegen(self, mode="r"):
        if mode == "w":
            return "r.%s =(" % self.name
        else:
            return "r.%s" % self.name


class Indirect:
    def __init__(self, segment, offsets, width=None):
        self.segment = segment
        self.offsets = offsets
        self.width = width

    def __repr__(self):
        return "[%s:%s w%s]" % (
            self.segment,
            "+".join(map(repr, self.offsets)),
            self.width,
        )

    def genAddr(self):
        """Generate a (segment, offset) tuple, where each item is
        generated C code which calculates the semgent/offset of this
        Indirect.
        """
        return (self.segment.codegen(), " + ".join([o.codegen() for o in self.offsets]))

    def codegen(self, mode="r"):
        # Instead of using the normal code generated for segment, use
        # the segment cache. This means our segment _must_ be a
        # register.

        assert isinstance(self.segment, Register)
        _, offset = self.genAddr()
        mem = "g.s.%s[(uint16_t)(%s)]" % (self.segment.name, offset)

        if self.width == 1:
            if mode == "w":
                return "%s=(" % mem
            else:
                return mem
        elif self.width == 2:
            if mode == "w":
                return "write16(&%s," % mem
            else:
                return "read16(&%s)" % mem
        else:
            raise InternalError("Unsupported memory access width")


def signed(operand):
    """Generate signed code for an operand."""
    if operand.width == 1:
        return "((int8_t)%s)" % operand.codegen()
    else:
        return "((int16_t)%s)" % operand.codegen()


def u32(operand):
    """Generate 32-bit unsigned code for an operand."""
    return "((uint32_t)%s)" % operand.codegen()


def findAll(needle, haystack):
    """Find all occurrances of 'needle' in 'haystack', even overlapping ones.
    Returns a list of zero-based offsets.
    """
    results = []
    offset = 0
    while True:
        search = haystack[offset:]
        if not search:
            break
        x = search.find(needle)
        if x < 0:
            break
        results.append(offset + x)
        offset += x + 1
    return results


class InternalError(Exception):
    pass


class Trace:
    _args = "uint16_t segment, uint16_t offset, uint16_t cs, uint16_t ip, int width"

    def __init__(self, name, mode, probe, fire):
        self.name = name
        self.mode = mode
        self.probe = probe
        self.fire = fire

    def codegen(self):
        return (
            "static SBT_INLINE int %s_probe(%s) {\n"
            "ProcessLocals& g = gProcessLocals\n;"
            "SBTRegs& r = g.r\n;"
            "%s\n}\n"
            "static void %s_fire(%s) {\n"
            "ProcessLocals& g = gProcessLocals\n;"
            "SBTRegs& r = g.r\n;"
            "%s\n}\n"
        ) % (self.name, self._args, self.probe, self.name, self._args, self.fire)


def _genCycleTable():
    """Generate a table of 8086/8086 instruction timings.  This isn't
    quite accurate, as it doesn't bother calculating the number of
    cycles necessary to look up the effective address based on the
    type of Indirect() argument we have. It also doesn't account
    for instructions that take a variable number of clocks
    depending on their arguments, like MUL. In practice I doubt
    this matters, at least for the things we're using our virtual
    clock for.

    This is based on the information from:
       http://umcs.maine.edu/~cmeadow/courses/cos335/80x86-Integer-Instruction-Set-Clocks.pdf
    """

    table = {
        ("mov", Register, Register): 2,
        ("mov", Indirect, Register): 13,
        ("mov", Register, Indirect): 12,
        ("mov", Indirect, Literal): 14,
        ("mov", Register, Literal): 4,
        ("cmp", Register, Register): 3,
        ("cmp", Indirect, Register): 13,
        ("cmp", Register, Indirect): 12,
        ("cmp", Indirect, Literal): 14,
        ("cmp", Register, Literal): 4,
        ("test", Register, Register): 3,
        ("test", Indirect, Register): 13,
        ("test", Register, Indirect): 13,
        ("test", Indirect, Literal): 11,
        ("test", Register, Literal): 5,
        ("xchg", Register, Register): 4,
        ("xchg", Indirect, Register): 25,
        ("xchg", Register, Indirect): 25,
        ("imul", Register): 89,  # Average (8-bit)
        ("imul", Indirect): 95,
        ("mul", Register): 73,  # Average (8-bit)
        ("mul", Indirect): 79,
        ("div", Register): 85,  # Average (8-bit)
        ("div", Indirect): 91,
        ("not", Register): 3,
        ("not", Indirect): 24,
        ("neg", Register): 3,
        ("neg", Indirect): 24,
        ("inc", Register): 3,
        ("inc", Indirect): 23,
        ("dec", Register): 3,
        ("dec", Indirect): 23,
        ("les", Register, Indirect): 24,
        ("jmp", Literal): 15,
        ("loop", Literal): 17,
        ("call", Register): 20,
        ("call", Literal): 23,
        ("ret",): 20,
        ("out", Literal, Register): 14,
        ("out", Register, Register): 12,
        ("in", Register, Literal): 14,
        ("in", Register, Register): 12,
        ("push", Register): 15,
        ("push", Indirect): 24,
        ("pop", Register): 12,
        ("pop", Indirect): 25,
        ("cmc",): 2,
        ("clc",): 2,
        ("stc",): 2,
        ("cbw",): 2,
        # Stubs for instructions that take a long and variable
        # amount of time to execute. No sane programmer would
        # use these in a timing-critical loop.. (fingers crossed)
        ("int", Literal): 0,
        ("rep_stosb",): 0,
        ("rep_stosw",): 0,
        ("rep_movsb",): 0,
        ("rep_movsw",): 0,
    }

    # Conditional jumps (assume jump taken)
    for op in "jz jnz jc jnc js jns ja jnl jl jng jna jcxz".split():
        table[(op, Literal)] = 16

    # All shifts and rotates are the same
    for op in ("shl", "shr", "rcl", "rcr", "sar", "ror"):
        table.update(
            {
                (op, Register, Register): 12,
                (op, Indirect, Register): 32,  # This is why you see so many
                (op, Indirect, Literal): 23,  #    repeated shifts by 1...
                (op, Register, Literal): 2,  # <-- Much cheaper.
            }
        )

    # 2-operand ALU operations are mostly the same.
    for op in ("xor", "and", "or", "add", "sub", "adc", "sbb"):
        table.update(
            {
                (op, Register, Register): 3,
                (op, Indirect, Register): 24,
                (op, Register, Indirect): 13,
                (op, Register, Literal): 4,
                (op, Indirect, Literal): 23,
            }
        )

    return table


class Instruction:
    """A disassembled instruction, in NASM format."""

    _widthNames = {
        "byte": 1,
        "short": 2,
        "word": 2,
        "dword": 4,
    }

    _cycleTable = _genCycleTable()

    # Optional dynamic branch targets, assigned during subroutine
    # analysis.  This is a list of Addr16 instances.
    dynTargets = None

    def __init__(self, line, offset=None, prefix=None, dynLiteralsMap=None):
        self.raw = line
        self._offset = offset

        line = line.replace("far ", "dword ")
        line = line.replace("near ", "word ")

        # Different versions of ndisasm treat prefixes differently...
        # Some of them disassemble prefixes correctly ("rep stosb"),
        # whereas some versions don't disassemble the prefix at all
        # and give a separate 'db' pseudo-op for the prefix.
        #
        # We want to convert either of these to a single combined
        # opcode (like "rep_stosb"). If it's a separate 'db', we'll
        # convert the db into a rep/rene here, then combine that with
        # the next instruction via the isPrefix flag below.
        #
        # If ndisasm gives us a normal prefix, though, we can just
        # do a string replace to combine it with the opcode.

        line = line.replace("db 0xF3", "rep")
        line = line.replace("db 0xF2", "repne")
        line = line.replace("rep ", "rep_")
        line = line.replace("repe ", "rep_")
        line = line.replace("repne ", "repne_")

        # Split up the ndisasm line into useful pieces.

        tokens = line.strip().split(None, 3)
        try:
            addr, encoding, op, args = tokens
            args = args.split(",")
        except ValueError:
            addr, encoding, op = tokens
            args = ()

        # Save most of the attributes we care about, then parse
        # operands. Some operands may have a dependency on the other
        # values.

        self.op = op
        self.addr = Addr16(str=addr).add(offset or 0)
        if encoding and encoding != "-":
            self.encoding = binascii.a2b_hex(encoding)
        else:
            self.encoding = None

        # Figure out whether this instruction has dynamic literals
        # (if it was marked by patchDynamicLiteral)

        self.dynLiteralsMap = dynLiteralsMap or {}
        self.dynamicLiterals = self.addr.offset in self.dynLiteralsMap

        # Normally we get an offset from the disassembler,
        # but if this is a manually entered (patch) instruction,
        # assume jumps are relative to the segment base.

        if offset is None:
            offset = Addr16(self.addr.segment, 0)

        self.args = tuple(map(self._decodeOperand, args))

        # Propagate known widths to operands with unknown width
        for a in self.args:
            if a.width is None:
                a.width = self._findUnknownWidth()

        # If this instruction has been attached to the prefix,
        # represent that in a prefix_opcode notation, and merge this
        # instruction's address and encoding info with the prefix.  If
        # this instruction IS a prefix, let the disassembler know.

        self.isPrefix = op in ("rep", "repe", "repne")
        if prefix:
            assert not prefix.args
            self.op = "%s_%s" % (prefix.op, self.op)
            self.addr = prefix.addr
            self.encoding = prefix.encoding + self.encoding
            self.raw = prefix.raw + self.raw

        # Calculate instruction length *after* merging the prefix.
        # Otherwise, we'll end up decoding the instruction after
        # the prefix twice.
        if self.encoding:
            self.length = len(self.encoding)
        else:
            self.length = None

        # Calculate the next address(es) for this instruction.
        # For most instructions, this is the next instruction.
        # For unconditional jumps, it's the jump target. For
        # conditional jumps, there are two addresses.
        #
        # For subroutine calls, we treat both the return address and
        # the subroutine as 'next' addresses. Return instructions
        # have no next address.
        #
        # If one of the 'next' addresses can't be known statically
        # (it's an Indirect) we ignore it. Function pointers and
        # computed jumps can't be translated automatically.

        if op in ("jmp", "ret", "iret", "retf"):
            self.nextAddrs = ()
        elif self.length is None:
            self.nextAddrs = ()
        else:
            self.nextAddrs = (self.addr.add(self.length),)

        self.labels = ()
        if self.args and (op[0] == "j" or op in ("call", "loop")):
            arg = self.args[0]
            if isinstance(arg, Literal):
                # Near branch
                self.labels = (offset.add(self.args[0]),)
            elif isinstance(arg, Addr16):
                # Far branch
                self.labels = (arg,)
        self.nextAddrs += self.labels

        # If this was a branch instruction, assume near (16-bit) by default.
        # Newer versions of nasm leave the explicit width annotation off these addrs.
        if self.nextAddrs:
            for a in self.args:
                if a.width is None:
                    a.width = 2  # Near ptr

    def _findUnknownWidth(self):
        """Figure out an operand width that wasn't explicitly specified.
        May return None if the width can't be determined.
        """
        w = None
        for a in self.args:
            if a.width is not None:
                if w is None:
                    w = a.width
                elif w != a.width:
                    raise InternalError(
                        "Contradictory widths %s and %s in %s" % (w, a.width, self)
                    )
        return w

    def _decodeOperand(self, text):
        """Decode an instruction operand."""

        # Does the operand begin with a width keyword?
        firstToken = text.split(None, 2)[0]
        if firstToken in self._widthNames:
            width = self._widthNames[firstToken]
            text = text[len(firstToken) :].strip()
        else:
            width = None

        # Is it an immediate value?
        try:
            value = int(text, 0)
        except ValueError:
            pass
        else:
            return self._decodeLiteral(value)

        # Does it look like a register?
        if len(text) == 2:
            return Register(text)

        # Is this in indirect?
        if text[0] == "[" and text[-1] == "]":
            return self._decodeIndirect(text[1:-1], width)

        # Is it an address?
        if ":" in text:
            return Addr16(str=text)

        raise InternalError(
            "Unsupported operand %r in %r at %r" % (text, self.raw, self.addr)
        )

    def _findLiteralAddr(self, value, opcodeLen=1):
        """Try to figure out the address of a string that occurs inside the
        encoded instruction. Returns an (address, width) tuple if
        there is one unique place where the value occurs. If the
        value can't be found or isn't unique, returns (None, None).

        XXX: This is a huge hack. Really we should just be
             decoding the machine code directly, parsing asm
             is turning out to be more trouble than it's worth.
        """
        if self.encoding:
            # First try 16-bit
            offsets = findAll(struct.pack("<H", value), self.encoding[opcodeLen:])
            if len(offsets) == 1:
                return self.addr.add(offsets[0] + opcodeLen), 2

            # Now 8-bit
            if value < 0x100:
                offsets = findAll(struct.pack("<B", value), self.encoding[opcodeLen:])
                if len(offsets) == 1:
                    return self.addr.add(offsets[0] + opcodeLen), 1

        return None, None

    def _decodeLiteral(self, value):
        """Decode a literal operand.
        Optionally converts it to an indirect reference to the code segment,
        in case we expect the literal to be modified at runtime.
        """

        # Ignore dynamic literals for certain instructions.
        # Shifts are encoded with less than one byte, jumps
        # aren't yet supported.
        dynamic = self.dynamicLiterals and not (
            self.op[0] == "j"
            or self.op in ("call", "shl", "shr", "rol", "ror", "sar", "rcl", "rcr")
        )

        # Make it unsigned
        if value < 0:
            value += 0x10000

        addr, width = self._findLiteralAddr(value)
        if dynamic and not addr:
            raise Exception(
                "Can't find address for dynamic literal 0x%x at %s" % (value, self.addr)
            )

        return Literal(value, addr, width, dynamic=dynamic)

    def _decodeIndirect(self, text, width=None):
        """Decode an operand that uses indirect addressing."""
        if ":" in text:
            segment, text = text.split(":", 1)
            segment = self._decodeOperand(segment)
        else:
            segment = Register("ds")

        # Convert signed offsets to unsigned.
        if "-" in text:
            text = text.replace("-", "+-")
            if text[0] == "+":
                text = text[1:]

        if "+" in text:
            offsets = list(map(self._decodeOperand, text.split("+")))
        else:
            offsets = [self._decodeOperand(text)]

        return Indirect(segment, offsets, width)

    def __repr__(self):
        return "%s %s %r" % (self.addr, self.op, self.args)

    def _repeat(self, cnt, contents):
        """Repeat 'contents', 'cnt' times."""
        if cnt == 1:
            return "{ %s }" % contents
        else:
            return "{ int c = %s; while (c--) { %s } }" % (cnt.codegen(), contents)

    def _resultShift(self, dest):
        if dest.width == 1:
            return "r.uresult <<= 8; r.sresult <<= 8;"
        else:
            return ""

    def codegen(self, traces=None, clockEnable=False):
        f = getattr(self, "codegen_%s" % self.op, None)
        if not f:
            raise NotImplementedError("Unsupported opcode in %s" % self)
        self._traces = traces

        code = f(*self.args)

        if clockEnable:
            # Come up with a signature for this instruction, which
            # consists of its opcode name and operand types. This
            # will be used to index into a table of instruction timings.

            sig = (self.op,) + tuple([arg.__class__ for arg in self.args])
            code = "g.clock+=%d;%s" % (self._cycleTable[sig], code)

        return code

    def _genTraces(self, *ops):
        """Generate code to fire any runtime traces. If there are no runtime
        traces, this generates no code (and there is no overhead).

        'ops' is a list of (operand, mode) tuples, which describe
        accesses to operands. Currently we only care about tracing memry,
        so we ignore operands that aren't Indirects. If any traces are in
        place, we call an inline 'probe' function to test whether we should
        call its 'fire' function.

        Besides user-provided traces, this function also implements
        the internal traces we use to update cached segment pointers
        and to detect writes to the code segment.
        """

        code = []
        for operand, mode in ops:

            if (
                isinstance(operand, Register)
                and mode == "w"
                and operand.name in ("cs", "es", "ds", "ss")
            ):

                code.append("g.s.load%s(g.proc, r);" % operand.name.upper())

            if (
                isinstance(operand, Indirect)
                and isinstance(operand.segment, Register)
                and operand.segment.name == "cs"
            ):

                self._warnSelfModifyingCode(operand, mode)

            if isinstance(operand, Indirect):
                for trace in self._traces:
                    if mode not in trace.mode:
                        continue

                    seg, off = operand.genAddr()
                    args = "%s,%s,0x%x,0x%x,%d" % (
                        seg,
                        off,
                        self.addr.segment,
                        self.addr.offset,
                        operand.width,
                    )

                    code.append(
                        "if (%s_probe(%s)) %s_fire(%s);"
                        % (trace.name, args, trace.name, args)
                    )

        return "".join(code)

    def _warnSelfModifyingCode(self, operand, mode):
        """This is a read or write to the code segment.
        The address may be static or dynamic. If any
        of the given offsets are inside the dynLiteralsMap,
        ignore it. Otherwise, print a warning.
        """
        for offset in operand.offsets:
            if isinstance(offset, int) and offset in self.dynLiteralsMap:
                return

        if len(operand.offsets) != 1 or not isinstance(operand.offsets[0], int):
            log(
                "Warning! Access (%s) to code segment "
                "@%s with dynamic offset %r. \nInstruction: %r"
                % (mode, self.addr, operand.offsets, self)
            )

        elif int(operand.offsets[0]) not in self.dynLiteralsMap:
            log(
                "Warning! Access (%s) to code segment "
                "@%s with offset %04x. Mark as dynamic literal. \nInstruction: %r"
                % (mode, self.addr, operand.offsets[0], self)
            )

    def codegen_mov(self, dest, src):
        return "%s%s);%s" % (
            dest.codegen("w"),
            src.codegen(),
            self._genTraces((src, "r"), (dest, "w")),
        )

    def codegen_xor(self, dest, src):
        return "r.uresult = %s ^ %s; %sr.uresult); r.sresult=0; %s%s" % (
            dest.codegen(),
            src.codegen(),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_or(self, dest, src):
        return "r.uresult = %s | %s; %sr.uresult); r.sresult=0; %s%s" % (
            dest.codegen(),
            src.codegen(),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_and(self, dest, src):
        return "r.uresult = %s & %s; %sr.uresult); r.sresult=0; %s%s" % (
            dest.codegen(),
            src.codegen(),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_add(self, dest, src):
        return ("r.sresult = %s + %s;" "r.uresult = %s + %s;" "%sr.uresult); %s%s") % (
            signed(dest),
            signed(src),
            u32(dest),
            u32(src),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_adc(self, dest, src):
        return (
            "r.sresult = %s + %s + r.getCF();"
            "r.uresult = %s + %s + r.getCF();"
            "%sr.uresult); %s%s"
        ) % (
            signed(dest),
            signed(src),
            u32(dest),
            u32(src),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_sub(self, dest, src):
        return ("r.sresult = %s - %s;" "r.uresult = %s - %s;" "%sr.uresult); %s%s") % (
            signed(dest),
            signed(src),
            u32(dest),
            u32(src),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_sbb(self, dest, src):
        return (
            "r.sresult=%s - (%s + r.getCF());"
            "r.uresult=%s - (%s + r.getCF());"
            "%sr.uresult); %s%s"
        ) % (
            signed(dest),
            signed(src),
            u32(dest),
            u32(src),
            dest.codegen("w"),
            self._resultShift(dest),
            self._genTraces((src, "r"), (dest, "r"), (dest, "w")),
        )

    def codegen_das(self):
        return (
            "if ((r.al & 0x0F) > 9) { "
            "r.uresult |= (((uint32_t)r.al) - 6) << 8; "
            "r.al -= 6; "
            "}"
            "if ((r.al > 0x9F) || r.getCF()) { "
            "r.setCF(); "
            "r.al -= 0x60; "
            "}"
        )

    def codegen_shl(self, r, cnt):
        return self._repeat(
            cnt,
            ("r.sresult=0; r.uresult = ((uint32_t)%s) << 1;" "%sr.uresult); %s")
            % (
                r.codegen(),
                r.codegen("w"),
                self._genTraces((cnt, "r"), (r, "r"), (r, "w")),
            ),
        ) + self._resultShift(r)

    def codegen_shr(self, r, cnt):
        return self._repeat(
            cnt,
            ("r.sresult=0; " "r.uresult = (%s & 1) << 16; " "%s%s >> 1); %s")
            % (
                u32(r),
                r.codegen("w"),
                r.codegen(),
                self._genTraces((cnt, "r"), (r, "r"), (r, "w")),
            ),
        )

    def codegen_sar(self, r, cnt):
        return self._repeat(
            cnt,
            ("r.sresult=0; " "r.uresult = (%s & 1) << 16; " "%s((int16_t)%s) >> 1); %s")
            % (
                u32(r),
                r.codegen("w"),
                r.codegen(),
                self._genTraces((cnt, "r"), (r, "r"), (r, "w")),
            ),
        )

    def codegen_ror(self, r, cnt):
        msbShift = r.width * 8 - 1
        return self._repeat(
            cnt,
            (
                "r.sresult=0; "
                "r.uresult = ((uint32_t)(%s) & 1) << 16;"
                "%s((%s) >> 1) + ((%s) << %s)); %s"
            )
            % (
                r.codegen(),
                r.codegen("w"),
                r.codegen(),
                r.codegen(),
                msbShift,
                self._genTraces((cnt, "r"), (r, "r"), (r, "w")),
            ),
        )

    def codegen_rcr(self, r, cnt):
        msbShift = r.width * 8 - 1
        return self._repeat(
            cnt,
            (
                "int cf = r.getCF();"
                "r.sresult=0; "
                "r.uresult = %s << 16;"
                "%s((%s) >> 1) + (cf << %s)); %s"
            )
            % (
                r.codegen(),
                r.codegen("w"),
                r.codegen(),
                msbShift,
                self._genTraces((cnt, "r"), (r, "r"), (r, "w")),
            ),
        )

    def codegen_rcl(self, r, cnt):
        return self._repeat(
            cnt,
            ("r.sresult=0; " "r.uresult = (%s << 1) + r.getCF();" "%sr.uresult); %s%s")
            % (
                u32(r),
                r.codegen("w"),
                self._resultShift(r),
                self._genTraces((cnt, "r"), (r, "r"), (r, "w")),
            ),
        )

    def codegen_xchg(self, a, b):
        return "{ uint16_t t = %s; %s%s); %st); %s}" % (
            a.codegen(),
            a.codegen("w"),
            b.codegen(),
            b.codegen("w"),
            self._genTraces((a, "r"), (b, "r"), (a, "w"), (b, "w")),
        )

    def codegen_imul(self, arg):
        return "r.ax = (int8_t)r.al * %s;" % arg.codegen()

    def codegen_mul(self, arg):
        return "r.ax = r.al * %s;" % arg.codegen()

    def codegen_div(self, arg):
        return "r.al = r.ax / %s; r.ah = r.ax %% %s;" % (arg.codegen(), arg.codegen())

    def codegen_cmc(self):
        return "if (r.getCF()) r.clearCF(); else r.setCF();"

    def codegen_clc(self):
        return "r.clearCF();"

    def codegen_stc(self):
        return "r.setCF();"

    def codegen_cbw(self):
        return "r.ax = (int16_t)(int8_t)r.al;"

    def codegen_cmp(self, a, b):
        return "r.sresult = %s - %s; r.uresult = %s - %s; %s" % (
            signed(a),
            signed(b),
            u32(a),
            u32(b),
            self._resultShift(a),
        )

    def codegen_test(self, a, b):
        return "r.uresult = %s & %s; r.sresult=0; %s" % (
            a.codegen(),
            b.codegen(),
            self._resultShift(a),
        )

    def codegen_inc(self, arg):
        return "{ uint32_t _cf = r.saveCF(); %s; r.restoreCF(_cf); }" % (
            self.codegen_add(arg, Literal(1))
        )

    def codegen_dec(self, arg):
        return "{ uint32_t _cf = r.saveCF(); %s; r.restoreCF(_cf); }" % (
            self.codegen_sub(arg, Literal(1))
        )

    def codegen_not(self, arg):
        return "%s~%s);%s" % (
            arg.codegen("w"),
            arg.codegen(),
            self._genTraces((arg, "r"), (arg, "w")),
        )

    def codegen_neg(self, arg):
        return "%s-%s);%s" % (
            arg.codegen("w"),
            arg.codegen(),
            self._genTraces((arg, "r"), (arg, "w")),
        )

    def codegen_les(self, dest, src):
        assert isinstance(src, Indirect)
        assert src.width == 2
        segPtr = Indirect(src.segment, src.offsets + [Literal(2)], 2)
        es = Register("es")

        return "%s%s); %s%s);%s" % (
            dest.codegen("w"),
            src.codegen(),
            es.codegen("w"),
            segPtr.codegen(),
            self._genTraces((src, "r"), (segPtr, "r"), (dest, "w"), (es, "w")),
        )

    def codegen_rep_stosw(self):
        cx = Register("cx").codegen()
        return "while (%s) { %s %s--;}" % (cx, self.codegen_stosw(), cx)

    def codegen_rep_movsw(self):
        cx = Register("cx").codegen()
        return "while (%s) { %s %s--;}" % (cx, self.codegen_movsw(), cx)

    def codegen_rep_stosb(self):
        cx = Register("cx").codegen()
        return "while (%s) { %s %s--;}" % (cx, self.codegen_stosb(), cx)

    def codegen_rep_scasb(self):
        # (Actually the 'repe' prefix, repeat while equal.)
        cx = Register("cx").codegen()
        return "while (%s) { %s %s--; if (!r.getZF()) break; }" % (
            cx,
            self.codegen_scasb(),
            cx,
        )

    def codegen_movsw(self):
        dest = Indirect(Register("es"), (Register("di"),), 2)
        src = Indirect(Register("ds"), (Register("si"),), 2)
        return "%s%s); %s += 2; %s += 2;%s" % (
            dest.codegen("w"),
            src.codegen(),
            Register("si").codegen(),
            Register("di").codegen(),
            self._genTraces((src, "r"), (dest, "w")),
        )

    def codegen_stosb(self):
        dest = Indirect(Register("es"), (Register("di"),), 1)
        return "%s%s); %s++;%s" % (
            dest.codegen("w"),
            Register("al").codegen(),
            Register("di").codegen(),
            self._genTraces((dest, "w")),
        )

    def codegen_stosw(self):
        dest = Indirect(Register("es"), (Register("di"),), 2)
        return "%s%s); %s += 2;%s" % (
            dest.codegen("w"),
            Register("ax").codegen(),
            Register("di").codegen(),
            self._genTraces((dest, "w")),
        )

    def codegen_scasb(self):
        return "%s; %s++;" % (
            self.codegen_cmp(
                Register("al"), Indirect(Register("es"), (Register("di"),), 1)
            ),
            Register("di").codegen(),
        )

    def codegen_lodsb(self):
        src = Indirect(Register("ds"), (Register("si"),), 1)
        return "%s = %s; %s++;%s" % (
            Register("al").codegen(),
            src.codegen(),
            Register("si").codegen(),
            self._genTraces((src, "r")),
        )

    def codegen_push(self, arg):
        # We currently implement the stack as a totally separate memory
        # area which can hold 16-bit words or native code return addresses.
        return "g.stack->pushw(%s);%s" % (arg.codegen(), self._genTraces((arg, "r")))

    def codegen_pop(self, arg):
        return "%sg.stack->popw());%s" % (arg.codegen("w"), self._genTraces((arg, "w")))

    def codegen_pushaw(self):
        return " ".join(
            (
                self.codegen_push(Register("ax")),
                self.codegen_push(Register("cx")),
                self.codegen_push(Register("dx")),
                self.codegen_push(Register("bx")),
                self.codegen_push(Register("sp")),
                self.codegen_push(Register("bp")),
                self.codegen_push(Register("si")),
                self.codegen_push(Register("di")),
            )
        )

    def codegen_popaw(self):
        return " ".join(
            (
                self.codegen_push(Register("di")),
                self.codegen_pop(Register("si")),
                self.codegen_pop(Register("bp")),
                self.codegen_pop(Register("sp")),
                self.codegen_pop(Register("bx")),
                self.codegen_pop(Register("dx")),
                self.codegen_pop(Register("cx")),
                self.codegen_pop(Register("ax")),
            )
        )

    def codegen_pushfw(self):
        return "g.stack->pushf(r);"

    def codegen_pushf(self):
        return "g.stack->pushf(r);"

    def codegen_popfw(self):
        return "r = g.stack->popf(r);"

    def codegen_popf(self):
        return "r = g.stack->popf(r);"

    def codegen_nop(self):
        return "/* nop */;"

    def codegen_cli(self):
        return "/* cli */;"

    def codegen_sti(self):
        return "/* sti */;"

    def codegen_jz(self, arg):
        return "if (r.getZF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jnz(self, arg):
        return "if (!r.getZF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jc(self, arg):
        return "if (r.getCF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jnc(self, arg):
        return "if (!r.getCF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_js(self, arg):
        return "if (r.getSF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jns(self, arg):
        return "if (!r.getSF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_ja(self, arg):
        return "if (!r.getCF() && !r.getZF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jnl(self, arg):
        return "if (r.getSF() == r.getOF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jl(self, arg):
        return "if (r.getSF() != r.getOF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jng(self, arg):
        return (
            "if (r.getZF() || (r.getSF() != r.getOF())) goto %s;"
            % self.nextAddrs[-1].label()
        )

    def codegen_jna(self, arg):
        return "if (r.getCF() || r.getZF()) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jcxz(self, arg):
        return "if (!r.cx) goto %s;" % self.nextAddrs[-1].label()

    def codegen_loop(self, arg):
        return "if (--r.cx) goto %s;" % self.nextAddrs[-1].label()

    def codegen_jmp(self, arg):
        if isinstance(arg, Literal) or isinstance(arg, Addr16):
            return "goto %s;" % self.nextAddrs[-1].label()

        elif self.dynTargets:
            # Generate a dynamic branch to any of the targets in
            # dynTargets.  This is implemented as a switch statement
            # that maps operand to label. If no target matches, we
            # cause a runtime error.

            # XXX: Only handles near jumps

            return (
                "switch (%s) { %s default: g.proc->failedDynamicBranch(%s,%s,%s); }"
                % (
                    arg.codegen(),
                    "".join(
                        [
                            "case 0x%04x: goto %s;" % (addr.offset, addr.label())
                            for addr in self.dynTargets
                        ]
                    ),
                    self.addr.segment,
                    self.addr.offset,
                    arg.codegen(),
                )
            )

        else:
            raise Exception("Dynamic jmp at %s must be patched." % self.addr)

    def codegen_call(self, arg):
        call_fmt = "sub_%X();"

        if isinstance(arg, Literal) or isinstance(arg, Addr16):
            return call_fmt % self.nextAddrs[-1].linear

        elif self.dynTargets:
            # Generate a dynamic call to any of the targets in
            # dynTargets.  This is implemented as a switch statement
            # that maps operand to a function call. If no target
            # matches, we cause a runtime error.

            # XXX: Only handles near calls

            return (
                "switch (%s) { %s default: g.proc->failedDynamicBranch(%s,%s,%s); }"
                % (
                    arg.codegen(),
                    "".join(
                        [
                            "case 0x%04x: %s break;"
                            % (addr.offset, call_fmt % addr.linear)
                            for addr in self.dynTargets
                        ]
                    ),
                    self.addr.segment,
                    self.addr.offset,
                    arg.codegen(),
                )
            )

        else:
            raise Exception("Dynamic call at %s must be patched." % self.addr)

    def codegen_ret(self):
        return "goto ret;"

    def codegen_retf(self):
        return self.codegen_ret()

    def codegen_int(self, arg):
        return "r = g.hw->interrupt%X(r, g.stack);" % arg

    def codegen_out(self, port, value):
        return "g.hw->out(%s,%s,g.clock);" % (port.codegen(), value.codegen())

    def codegen_in(self, value, port):
        return "%s = g.hw->in(%s,g.clock);" % (value.codegen(), port.codegen())


class BinaryImage:
    def __init__(self, filename=None, offset=0, data=None):
        self.basename = os.path.basename(filename)
        self.filename = filename
        file = open(filename, "rb")

        if data is None:
            file.seek(offset)
            data = file.read()
            offset = 0

        if offset:
            data = data[offset:]

        assert type(data) is bytes
        self._data = data
        self.loadSegment = 0
        self.parseHeader()
        self._iCache = {}
        self.fetchedInstructions = {}
        self.dynLiterals = {}
        self._tempFile = None

    def __del__(self):
        if self._tempFile:
            os.unlink(self._tempFile.name)

    def relocate(self, segment, addrs):
        """Apply a list of relocations to this image. 'addrs' is a list of
        Addr16s which describe 16-bit relocations, 'segment' is the
        amount we'll add to each relocation. This function modifies
        the BinaryImage's data buffer.

        This implementation is horribly inefficient, but given how
        few relocations we have and how slow gcc is at compiling
        our output anyway, I don't care.
        """

        log("Applying relocations at %r" % addrs)
        self.loadSegment = segment

        if self._tempFile:
            raise Exception("Must relocate before starting disassembly!")

        for addr in addrs:
            offset = addr.linear
            pre = self._data[:offset]
            reloc = self._data[offset : offset + 2]
            post = self._data[offset + 2 :]

            reloc = struct.unpack("<H", reloc)[0]
            reloc = (reloc + segment) & 0xFFFF
            reloc = struct.pack("<H", reloc)

            self._data = b"".join((pre, reloc, post))

    def offset(self, offset):
        return BinaryImage(self.filename, offset=offset, data=self._data)

    def parseHeader(self):
        pass

    def read(self, offset, length):
        return self._data[offset : offset + length]

    def unpack(self, offset, fmt):
        return struct.unpack(fmt, self.read(offset, struct.calcsize(fmt)))

    def disasm(self, addr, instructionLimit, bits=16):
        """This is an iterator which disassembles starting at the specified
        memory address. Uses a temporary file to pass the relocated
        binary image to ndisasm.

        If the memory address is different from the address on disk,
        self.loadSegment must be set to the segment where this image
        is loaded.
        """

        if not self._tempFile:
            self._tempFile = tempfile.NamedTemporaryFile(delete=False)
            self._tempFile.write(self._data)
            self._tempFile.flush()
            self._tempFile.close()  # Windows compatibility

        args = [
            "ndisasm",
            "-o",
            str(addr.offset),
            "-b",
            str(bits),
            "-e",
            str(addr.linear - (self.loadSegment << 4)),
            self._tempFile.name,
        ]
        proc = subprocess.Popen(args, stdout=subprocess.PIPE)

        prefix = None
        base = Addr16(addr.segment, 0)

        for line in proc.stdout:
            i = Instruction(line.decode("utf-8"), base, prefix, self.dynLiterals)

            prefix = None
            if i.isPrefix:
                prefix = i
            else:
                yield i

            instructionLimit -= 1
            if instructionLimit <= 0:
                break

        proc.stdout.close()

    def iFetch(self, addr):
        """Fetch the next instruction, via the instruction cache.  If there's
        a cache miss, disassemble a block of code starting at this
        address. On successful disassembly, the instruction is removed from
        memory unless it's needed for dynamic literals later on.
        """
        if addr.linear not in self._iCache:
            self._disasmToCache(addr)

        try:
            inst = self._iCache[addr.linear]
        except KeyError:
            raise InternalError("Failed to disassemble instruction at %s" % addr)

        self.fetchedInstructions[addr.linear] = inst
        return inst

    def _disasmToCache(self, addr, instructionLimit=100):
        for i in self.disasm(addr, instructionLimit):
            if i.addr and i.addr.linear not in self._iCache:
                self._iCache[i.addr.linear] = i


class Subroutine:
    """Information about a single subroutine. We define a subroutine as
    the section of a control flow graph which begins just after a
    'call' and ends at all 'ret's. One code fragment may be used
    by multiple subroutines.
    """

    # Should this subroutine generate clock cycle counting code?
    clockEnable = False

    def __init__(self, image, entryPoint, hooks=None):
        self.image = image
        self.entryPoint = entryPoint
        self.hooks = hooks or {}
        self.name = "sub_%X" % self.entryPoint.linear

    def analyze(self, dynBranches=None, verbose=False):
        """Analyze the code in this subroutine.
        Stores analysis results in member attributes.

        The optional 'dynBranches' parameter is a map of dynamic
        branch targets for each dynamic branch intruction. The
        dynamic branch targets must be included alongside static
        branch targets when we perform the label, branch, and call
        analysis.
        """

        # Address memo: linear -> Addr16
        memo = {}

        # Label flags: linear -> True
        self.labels = {self.entryPoint.linear: True}

        # Routines that are called by this one
        self.callsTo = {}

        # Local jump stack: (referent, level, ptr)
        stack = [(None, self.entryPoint)]

        while stack:
            referent, ptr = stack.pop()
            if ptr.linear in memo:
                if verbose:
                    sys.stderr.write("Address %s already visited\n" % ptr)
                continue

            memo[ptr.linear] = ptr
            i = self.image.iFetch(ptr)
            i.referent = referent

            if verbose:
                sys.stderr.write(
                    "%s R[%-9s] D%-3d %-50s -> %-22s || %s\n"
                    % (self.name, referent, len(stack), i, i.nextAddrs, i.raw.strip())
                )

            # I/O instructions enable cycle counting, so that we can
            # give in() and out() accurate timestamps.

            if i.op in ("in", "out"):
                self.clockEnable = True

            # Remember all label targets, and remember future
            # addresses to analyze. Treat subroutine calls specially.

            for next in i.labels:
                self.labels[next.linear] = True

            if i.op == "call" and len(i.nextAddrs) == 2:
                stack.append((ptr, i.nextAddrs[0]))
                self.callsTo[i.nextAddrs[1].linear] = i.nextAddrs[1]
            else:
                for next in i.nextAddrs:
                    stack.append((ptr, next))

            # If this instruction is in the dynamic branch table,
            # include all branch targets in the label/subroutine
            # analysis.

            if i.addr.linear in dynBranches:
                i.dynTargets = dynBranches[i.addr.linear]
                for target in i.dynTargets:
                    target = Addr16(str=str(target))

                    if i.op == "call":
                        self.callsTo[target.linear] = target
                    elif i.op == "jmp":
                        self.labels[target.linear] = True
                        stack.append((ptr, target))
                    else:
                        raise InternalError(
                            "Unknown dynamic branch origin opcode %r" % i.op
                        )

        # Sort the instruction memo, and store a list
        # of instructions sorted by address.
        addrs = list(memo.values())
        addrs.sort()
        self.instructions = map(self.image.iFetch, addrs)

    def codegen(self, traces=None):
        body = []
        for i in self.instructions:
            if i.addr.linear in self.labels:
                label = i.addr.label() + ": "
            else:
                label = ""
            body.append(
                "/* %-60s R[%-9s] */ %15s %s%s"
                % (
                    i,
                    i.referent,
                    label,
                    self.hooks.get(i.addr.linear, ""),
                    i.codegen(traces=traces, clockEnable=self.clockEnable),
                )
            )
        return """
void
%(name)s(void)
{
  ProcessLocals& g = gProcessLocals;
  SBTRegs& r = g.r;
  g.stack->pushret(0x%(offset)04x);
  goto %(label)s;
%(body)s
ret:
  g.stack->popret(0x%(offset)04x);
  return;
}""" % {
            "name": self.name,
            "addr": self.entryPoint,
            "offset": self.entryPoint.offset,
            "label": self.entryPoint.label(),
            "body": "\n".join(body),
        }


class BinaryData:
    """Represents the data portions of a binary image. These parts are RLE
    compressed and converted to an array in the generated code.
    """

    def __init__(self, data=b"", baseAddr=Addr16(0, 0)):
        self.data = list(data)
        self.baseAddr = baseAddr

    def patch(self, addr, data):
        """Overwrite bytes with 'data', starting at 'addr'."""
        offset = addr.linear - self.baseAddr.linear
        for i, byte in enumerate(data):
            self.data[offset + i] = byte

    def fill(self, addr, bytecount, pattern=0):
        """Zero out a range of bytes"""
        offset = addr.linear - self.baseAddr.linear
        for i in range(bytecount):
            self.data[offset + i] = pattern

    def toHexArray(self):
        """Convert compressed binary data to a list of hexadecimal values
        suitable for including in a C array.
        """
        return "".join(
            [
                "0x%02x,%s" % (b, "\n"[: (i & 15) == 15])
                for i, b in enumerate(self.compressRLE())
            ]
        )

    def compressRLE(self):
        """Compress a byte list using a simple form of RLE which
        is optimized for eliminating long runs of zeroes. This
        is used to automatically avoid storing zeroed portions
        of the data segment.

        In the resulting binary, any run of two consecutive zeroes is
        followed by a 16-bit value (little endian) which indicates how
        many more zeroes have been omitted afterwards.

        Trailing zeroes in the data are ignored.
        """
        output = []
        zeroes = []
        for byte in self.data:
            if byte == 0:
                zeroes.append(byte)
            elif not zeroes:
                output.append(byte)
            elif len(zeroes) == 1:
                output.append(0)
                output.append(byte)
                zeroes = []
            else:
                output.extend(list(b"\0\0" + struct.pack("<H", len(zeroes) - 2)))
                output.append(byte)
                zeroes = []
        return output


class DOSBinary(BinaryImage):
    # Skeleton for output C code
    _skel = """/*
 * Static binary translation by sbt86
 *
 * Filename: %(filename)r
 * EXE size: %(exeSize)r
 * Memory size: %(memorySize)r
 * Entry point: %(entryPoint)s
 *
 * Generated process class %(className)r
 */

#include <stdint.h>
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"

SBT_DECL_PROCESS(%(className)s);

%(className)s::%(className)s(Hardware *hardware)
{
    this->hardware = hardware;
    hardware->registerProcess(this);
}

/*
 * Local cache of registers and process pointer.
 */
struct ProcessLocals {
    SBTRegs r;
    SBTSegmentCache s;
    uint32_t clock;
    SBTProcess *proc;
    SBTStack *stack;
    Hardware *hw;
};
static ProcessLocals gProcessLocals = {};

static uint8_t dataImage[] = {
%(dataImage)s};

%(subDecls)s

%(_decls)s
%(traceDecls)s

%(subCode)s

uint8_t *%(className)s::getData()
{
    return dataImage;
}

uint32_t %(className)s::getDataLen()
{
    return sizeof(dataImage);
}

uint16_t %(className)s::getRelocSegment()
{
    return 0x%(relocSegment)04x;
}

uint16_t %(className)s::getEntryCS()
{
    return 0x%(entryCS)04x;
}

const char *%(className)s::getFilename()
{
    return "%(basename)s";
}

void %(className)s::loadEnvironment(SBTStack *stack, SBTRegs reg)
{
    ProcessLocals& g = gProcessLocals;
    g.stack = stack;
    g.r = reg;
    g.proc = this;
    g.hw = hardware;
    g.s.load(g.proc, g.r);
}

SBTProcess::continue_func_t %(className)s::getFunction(SBTAddressId id)
{
    switch (id) {
%(getFunctionCode)s
    default: return 0;
    }
}

int %(className)s::getAddress(SBTAddressId id)
{
    switch (id) {
%(getAddrCode)s
    default: return -1;
    }
}

"""

    relocSegment = None
    subroutines = None

    def __init__(self, filename=None, offset=0, data=None, relocSegment=0x100):
        self.relocSegment = relocSegment
        BinaryImage.__init__(self, filename=filename, offset=offset, data=data)

    def loadAddress(self, addr):
        assert addr.offset == 0
        self.relocSegment = addr.segment

    def toLinear(self, segmentOffset):
        return ((segmentOffset >> 16) << 4) | (segmentOffset & 0xFFFF)

    def parseHeader(self):
        self._hooks = {}
        self._traces = []
        self._decls = ""
        self._dynBranches = {}
        self._publishedAddresses = {}
        self._publishedFunctions = {}
        self._markedSubs = []

        (
            signature,
            bytesInLastPage,
            numPages,
            numRelocations,
            headerParagraphs,
            minMemParagraphs,
            maxMemParagraphs,
            initSS,
            initSP,
            checksum,
            entryIP,
            entryCS,
            relocTable,
            overlayNumber,
        ) = self.unpack(0, "<2sHHHHHHHHHHHHH")

        if signature != b"MZ":
            raise ValueError("Input file is not a DOS executable")

        self.exeSize = (numPages - 1) * 512 + bytesInLastPage

        self.headerSize = headerParagraphs * 16
        self.memorySize = minMemParagraphs * 16 + self.exeSize
        self.entryPoint = Addr16(self.relocSegment + entryCS, entryIP)
        self.image = self.offset(self.headerSize)

        relocs = []
        for i in range(numRelocations):
            offset, segment = self.unpack(relocTable + i * 4, "<HH")
            relocs.append(Addr16(segment, offset))
        self.image.relocate(self.relocSegment, relocs)

        # Create a binary image for our initial memory contents.
        # This can be edited during patching and analysis. Fetched
        # instructions will be trimmed from this image in analyze()

        self.staticData = BinaryData(self.image._data, Addr16(self.relocSegment, 0))

        # Publish the entry point and start analysis there
        self.publishSubroutine("SBTADDR_ENTRY_FUNC", self.entryPoint)

    def decl(self, code):
        """Add code to the declarations in the generated C file.
        This is useful if patches require global variables or
        shared code.
        """
        self._decls = "%s\n%s\n" % (self._decls, code)

    def publishAddress(self, enum, addr):
        """Provide an address that can be looked up via getAddress() at runtime."""
        if enum in self._publishedAddresses:
            raise Exception("Already published %s" % enum)
        self._publishedAddresses[enum] = addr

    def publishSubroutine(self, enum, addr):
        """Mark a subroutine for analysis and export it via getFunction()"""
        if enum in self._publishedFunctions:
            raise Exception("Already published %s" % enum)
        self.markSubroutine(addr)
        self._publishedFunctions[enum] = addr.linear

    def markSubroutine(self, address):
        """Before analysis, mark an extra address to be considered as a subroutine entry point"""
        if address not in self._markedSubs:
            self._markedSubs.append(address)

    def patch(self, addr, code, length=0, deadCode=True):
        """Manually stick a line of assembly into the iCache,
        in order to replace an instruction we would have loaded
        from the EXE file.

        If the instruction is not a return or an unconditional
        jump, you must specify the instruction's length (in bytes)
        so we know where to find the next instruction. The length
        doesn't have to match the actual length of the assembly
        code you provide, it's just used to locate the next
        instruction to disassemble.
        """
        i = Instruction("%s %s %s" % (addr, ("00" * length) or "-", code))
        if deadCode:
            self.markDeadCode(i.addr)
        self.image._iCache[i.addr.linear] = i

    def hook(self, addr, code):
        """Install a C-language hook, to be executed just before the
        instruction at address 'addr'.  The address and C-code are
        both specified as strings.
        """
        code = "{\n%s\n } " % code
        linear = Addr16(str=str(addr)).linear

        if linear in self._hooks:
            self._hooks[linear] += code
        else:
            self._hooks[linear] = code

    def patchAndHook(self, addr, asmCode, cCode, length=0, deadCode=True):
        """This is a shorthand for patching and hooking the same address."""
        self.patch(addr, asmCode, length, deadCode)
        self.hook(addr, cCode)

    def patchDynamicBranch(self, addr, targets):
        """Patch a dynamic jmp or call, using a static list of possible
        targets.  Each of the possible targets will be analyzed as
        if they were a static target of the function. This means
        that all static jmp targets will be inlined into the
        calling subroutine if they aren't already.

        This will generate code to select a target at runtime. If
        the branch does not target any of the addresses in the
        list, the failedDynamicBranch() function will be invoked.

        If 'addr' is already registered as a dynamic branch site,
        this function will add additional targets. It is an error
        to specify one target twice.
        """
        linear = Addr16(str=str(addr)).linear
        prevTargets = self._dynBranches.get(linear, [])
        for target in targets:
            if target in prevTargets:
                raise Exception(
                    "Duplicate dynamic branch target %s at address %s" % (target, addr)
                )
        self._dynBranches[linear] = prevTargets + targets

    def patchDynamicLiteral(self, addr, length=1):
        """Patch a common flavour of self-modifying code by marking an
        instruction's literal data as dynamic. Instead of translating
        the data as numeric literals, it will be translated as
        indirect references to the code segment. Also, the instruction
        will be preserved in the static data image.

        If 'length' is specified, any instructions in a range of
        addresses will be translated using dynamic literals.
        """
        offset = Addr16(str=str(addr)).offset
        while length > 0:
            self.image.dynLiterals[offset] = True
            offset += 1
            length -= 1

    def trace(self, mode, probe, fire):
        """Define a memory trace.

        'mode' is 'r' for memory reads, 'w' for writes, and 'rw' for either.

        'probe' is the body of a C function which is inlined at every
        memory access of the specified type. In this function, 'segment'
        and 'offset' will be a far pointer to the memory that was modified,
        and 'width' will be the 1 or 2 bytes. 'cs' and 'ip' identify the
        instruction performing the memory operation.

        'fire' is the function to be called when 'probe' returns TRUE.
        The parameters are identical to 'probe'.
        """
        self._traces.append(Trace("trace%d" % len(self._traces), mode, probe, fire))

    def analyze(self, verbose=False):
        """Analyze the whole program. This breaks it up into
        subroutines, and analyzes each routine.
        """

        log("Analyzing %s..." % self.basename)

        self.subroutines = {}
        stack = list(self._markedSubs)

        while stack:
            ptr = stack.pop()
            if ptr.linear in self.subroutines:
                if verbose:
                    sys.stderr.write("Subroutine %s already visited\n" % ptr)
                continue

            sub = Subroutine(self.image, ptr, self._hooks)
            sub.analyze(self._dynBranches, verbose)
            stack.extend(sub.callsTo.values())
            self.subroutines[ptr.linear] = sub

        for inst in self.image.fetchedInstructions.values():
            if inst.length and not inst.dynamicLiterals:
                self.staticData.fill(inst.addr, inst.length)

    def markDeadCode(self, addr):
        """Recursively disassemble from the given address, marking instructions
        as code rather than data but throwing away the resulting subroutine.
        """
        Subroutine(self.image, addr).analyze(self._dynBranches)

    def findCode(self, signature):
        """Find a signature in the binary's code segment.
        Returns an Addr16, using the binary's entry
        point as a segment reference.
        """
        return self.findCodeMultiple(signature, 1)[0]

    def findCodeMultiple(self, signature, expectedCount=None):
        """Like findCode(), but allows the signature to appear zero or more times.
        If expectedCount is specified, we require it to occur exactly that many
        times. Returns a list of Addr16s.
        """
        sig = Signature(signature)
        addrs = [
            self.entryPoint.add(o + (self.relocSegment << 4) - self.entryPoint.linear)
            for o in sig.find(self.image._data)
        ]
        if expectedCount is not None and len(addrs) != expectedCount:
            raise SignatureMatchError(
                "Signature found %d times, expected to "
                "find %d. Matches: %r" % (len(addrs), expectedCount, addrs)
            )
        log(
            "Found patch location %r in %s for: %r"
            % (addrs, self.basename, sig.shortText)
        )
        return addrs

    def findData(self, signature):
        """Find a signature in the binary's data segment. Returns an Addr16,
        using the binary's relocated DS as a segment reference.
        """
        return self.findDataMultiple(signature, 1)[0]

    def findDataMultiple(self, signature, expectedCount=None):
        """Like findData(), but allows the signature to appear zero or more times.
        If expectedCount is specified, we require it to occur exactly that many
        times. Returns a list of Addr16s.
        """
        sig = Signature(signature)
        addrs = [Addr16(self.relocSegment, o) for o in sig.find(self.image._data)]

        if expectedCount is not None and len(addrs) != expectedCount:
            raise SignatureMatchError(
                "Signature found %d times, expected to "
                "find %d. Matches: %r" % (len(addrs), expectedCount, addrs)
            )
        log(
            "Found data address %r in %s for: %r"
            % (addrs, self.basename, sig.shortText)
        )
        return addrs

    def peek(self, addr, fmt):
        if not isinstance(addr, Addr16):
            addr = Addr16(self.relocSegment, addr)
        return self.image.unpack(
            addr.linear - Addr16(self.relocSegment, 0).linear, fmt
        )[0]

    def peek8(self, addr):
        """Read an 8-bit value. 'addr' may be a data segment offset or an Addr16."""
        return self.peek(addr, "<B")

    def peek16(self, addr):
        """Read a 16-bit value. 'addr' may be a data segment offset or an Addr16."""
        return self.peek(addr, "<H")

    def peek16s(self, addr):
        """Read a signed 16-bit value. 'addr' may be a data segment offset or an Addr16."""
        return self.peek(addr, "<h")

    def jumpTarget(self, addr):
        """Determine the target of a jump at 'addr'.
        Currently only handles the specific jumps we care about.
        """
        assert self.peek8(addr) in [0xE9, 0xE8]
        target = addr.add(3 + self.peek16s(addr.add(1)), wrap=True)
        log("Found jump from %r -> %r" % (addr, target))
        return target

    def codegen(self, className):
        vars = dict(self.__class__.__dict__)
        vars.update(self.__dict__)

        log(
            "Code generating %s (%d subroutines)..."
            % (self.basename, len(self.subroutines.values()))
        )

        vars["className"] = className
        vars["subCode"] = "\n".join(
            [s.codegen(traces=self._traces) for s in self.subroutines.values()]
        )
        vars["subDecls"] = "\n".join(
            ["static void %s(void);" % s.name for s in self.subroutines.values()]
        )
        vars["traceDecls"] = "\n".join([t.codegen() for t in self._traces])
        vars["entryLinear"] = self.entryPoint.linear
        vars["entryCS"] = self.entryPoint.segment

        vars["dataImage"] = self.staticData.toHexArray()

        vars["getAddrCode"] = "\n".join(
            ["case %s: return 0x%04x;" % i for i in self._publishedAddresses.items()]
        )

        vars["getFunctionCode"] = "\n".join(
            ["case %s: return &sub_%X;" % i for i in self._publishedFunctions.items()]
        )

        return self._skel % vars

    def writeCodeToFile(self, filename, className, verbose=False):
        """Run analysis if necessary, then generate code to a file."""
        f = open(filename, "w")
        if self.subroutines is None:
            self.analyze(verbose)
        f.write(self.codegen(className))
