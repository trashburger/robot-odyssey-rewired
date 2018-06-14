/*
 * Static binary translation by sbt86
 *
 * Filename: 'PLAY.EXE'
 * EXE size: 1605
 * Memory size: 2645
 * Entry point: 0000:0000
 */

#include <stdint.h>

static uint8_t dataImage[] = {
};

#include "sbt86.h"

static StackItem stack[64];
static int stackPtr;

static Regs sub_0(Regs r);
static Regs sub_BF(Regs r);





Regs
sub_0(Regs reg)
{
  goto loc_0;
/* 0000:0000 mov [Reg(ax), 0x1b]                                R[None     ] */         loc_0:  reg.ax = 0x1b;
/* 0000:0003 mov [Reg(ds), Reg(ax)]                             R[0000:0000] */                 reg.ds = reg.ax;
/* 0000:0005 push [Reg(ds)]                                     R[0000:0003] */                 stack[stackPtr++].word = reg.ds;
/* 0000:0006 push [Reg(cs)]                                     R[0000:0005] */                 stack[stackPtr++].word = reg.cs;
/* 0000:0007 pop [Reg(ds)]                                      R[0000:0006] */                 reg.ds = stack[--stackPtr].word;
/* 0000:0008 mov [Reg(dx), 0x0198]                              R[0000:0007] */                 reg.dx = 0x0198;
/* 0000:000B mov [Reg(ah), 0x25]                                R[0000:0008] */                 reg.ah = 0x25;
/* 0000:000D mov [Reg(al), 0x24]                                R[0000:000B] */                 reg.al = 0x24;
/* 0000:000F int [0x21]                                         R[0000:000D] */                 reg = int21(reg);
/* 0000:0011 pop [Reg(ds)]                                      R[0000:000F] */                 reg.ds = stack[--stackPtr].word;
/* 0000:0012 mov [Reg(bx), 0x85]                                R[0000:0011] */                 reg.bx = 0x85;
/* 0000:0015 mov [Reg(ax), Reg(es)]                             R[0000:0012] */                 reg.ax = reg.es;
/* 0000:0017 sub [Reg(bx), Reg(ax)]                             R[0000:0015] */                 reg.sresult = ((int16_t)reg.bx) - ((int16_t)reg.ax); reg.bx = reg.uresult = ((uint32_t)reg.bx) - ((uint32_t)reg.ax); 
/* 0000:0019 add [Reg(bx), 0x11]                                R[0000:0017] */                 reg.sresult = ((int16_t)reg.bx) + ((int16_t)0x11); reg.bx = reg.uresult = ((uint32_t)reg.bx) + ((uint32_t)0x11); 
/* 0000:001C mov [Reg(ah), 0x4a]                                R[0000:0019] */                 reg.ah = 0x4a;
/* 0000:001E int [0x21]                                         R[0000:001C] */                 reg = int21(reg);
/* 0000:0020 mov [Reg(dx), 0xc7]                                R[0000:001E] */        loc_20:  reg.dx = 0xc7;
/* 0000:0023 mov [Reg(al), 0]                                   R[0000:0020] */                 reg.al = 0;
/* 0000:0025 mov [Reg(ah), 0x3d]                                R[0000:0023] */                 reg.ah = 0x3d;
/* 0000:0027 int [0x21]                                         R[0000:0025] */                 reg = int21(reg);
/* 0000:0029 jc [0x20]                                          R[0000:0027] */                 if (CF) goto loc_20;
/* 0000:002B mov [Reg(bx), Reg(ax)]                             R[0000:0029] */                 reg.bx = reg.ax;
/* 0000:002D mov [Reg(ah), 0x3f]                                R[0000:002B] */                 reg.ah = 0x3f;
/* 0000:002F mov [Reg(cx), 0x10]                                R[0000:002D] */                 reg.cx = 0x10;
/* 0000:0032 mov [Reg(dx), 0]                                   R[0000:002F] */                 reg.dx = 0;
/* 0000:0035 int [0x21]                                         R[0000:0032] */                 reg = int21(reg);
/* 0000:0037 mov [Reg(ah), 0x3e]                                R[0000:0035] */                 reg.ah = 0x3e;
/* 0000:0039 int [0x21]                                         R[0000:0037] */                 reg = int21(reg);
/* 0000:003B mov [Reg(bx), 0x12]                                R[0000:0039] */                 reg.bx = 0x12;
/* 0000:003E call [0xbf]                                        R[0000:003B] */                 reg = sub_BF(reg);
/* 0000:0041 jmp [0x5a]                                         R[0000:003E] */                 goto loc_5A;
/* 0000:0044 call [0xbf]                                        R[0000:0090] */        loc_44:  reg = sub_BF(reg);
/* 0000:0047 cmp [Reg(al), 0xd5]                                R[0000:0044] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)0xd5); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)0xd5); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0049 jnz [0x54]                                         R[0000:0047] */                 if (!ZF) goto loc_54;
/* 0000:004B mov [Reg(bx), 0x35]                                R[0000:0049] */                 reg.bx = 0x35;
/* 0000:004E call [0xbf]                                        R[0000:004B] */                 reg = sub_BF(reg);
/* 0000:0051 jmp [0x5a]                                         R[0000:004E] */                 goto loc_5A;
/* 0000:0054 mov [Reg(bx), 0x1d]                                R[0000:0049] */        loc_54:  reg.bx = 0x1d;
/* 0000:0057 call [0xbf]                                        R[0000:0054] */                 reg = sub_BF(reg);
/* 0000:005A cmp [Reg(al), 0x63]                                R[0000:0041] */        loc_5A:  reg.sresult = ((int8_t)reg.al) - ((int16_t)0x63); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)0x63); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:005C jz [0x8d]                                          R[0000:005A] */                 if (ZF) goto loc_8D;
/* 0000:005E cmp [Reg(al), 0x62]                                R[0000:005C] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)0x62); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)0x62); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0060 jz [0x97]                                          R[0000:005E] */                 if (ZF) goto loc_97;
/* 0000:0062 cmp [Reg(al), 8]                                   R[0000:0060] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)8); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)8); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0064 jz [0xba]                                          R[0000:0062] */                 if (ZF) goto loc_BA;
/* 0000:0066 cmp [Reg(al), 7]                                   R[0000:0064] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)7); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)7); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0068 jz [0xb5]                                          R[0000:0066] */                 if (ZF) goto loc_B5;
/* 0000:006A cmp [Reg(al), 6]                                   R[0000:0068] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)6); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)6); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:006C jz [0xb0]                                          R[0000:006A] */                 if (ZF) goto loc_B0;
/* 0000:006E cmp [Reg(al), 5]                                   R[0000:006C] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)5); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)5); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0070 jz [0xab]                                          R[0000:006E] */                 if (ZF) goto loc_AB;
/* 0000:0072 cmp [Reg(al), 4]                                   R[0000:0070] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)4); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)4); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0074 jz [0xa6]                                          R[0000:0072] */                 if (ZF) goto loc_A6;
/* 0000:0076 cmp [Reg(al), 3]                                   R[0000:0074] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)3); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)3); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0078 jz [0xa1]                                          R[0000:0076] */                 if (ZF) goto loc_A1;
/* 0000:007A cmp [Reg(al), 2]                                   R[0000:0078] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)2); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)2); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:007C jz [0x9c]                                          R[0000:007A] */                 if (ZF) goto loc_9C;
/* 0000:007E cmp [Reg(al), 1]                                   R[0000:007C] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)1); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)1); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0080 jz [0x92]                                          R[0000:007E] */                 if (ZF) goto loc_92;
/* 0000:0082 cmp [Reg(al), 0]                                   R[0000:0080] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)0); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)0); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0084 jz [0x88]                                          R[0000:0082] */                 if (ZF) goto loc_88;
/* 0000:0086 jmp [0x54]                                         R[0000:0084] */                 goto loc_54;
/* 0000:0088 mov [Reg(bx), 0x42]                                R[0000:0084] */        loc_88:  reg.bx = 0x42;
/* 0000:008B jmp [0x44]                                         R[0000:0088] */                 goto loc_44;
/* 0000:008D mov [Reg(bx), 0x4e]                                R[0000:005C] */        loc_8D:  reg.bx = 0x4e;
/* 0000:0090 jmp [0x44]                                         R[0000:008D] */                 goto loc_44;
/* 0000:0092 mov [Reg(bx), 0x5b]                                R[0000:0080] */        loc_92:  reg.bx = 0x5b;
/* 0000:0095 jmp [0x44]                                         R[0000:0092] */                 goto loc_44;
/* 0000:0097 mov [Reg(bx), 0x67]                                R[0000:0060] */        loc_97:  reg.bx = 0x67;
/* 0000:009A jmp [0x44]                                         R[0000:0097] */                 goto loc_44;
/* 0000:009C mov [Reg(bx), 0x73]                                R[0000:007C] */        loc_9C:  reg.bx = 0x73;
/* 0000:009F jmp [0x44]                                         R[0000:009C] */                 goto loc_44;
/* 0000:00A1 mov [Reg(bx), 0x7f]                                R[0000:0078] */        loc_A1:  reg.bx = 0x7f;
/* 0000:00A4 jmp [0x44]                                         R[0000:00A1] */                 goto loc_44;
/* 0000:00A6 mov [Reg(bx), 0x8b]                                R[0000:0074] */        loc_A6:  reg.bx = 0x8b;
/* 0000:00A9 jmp [0x44]                                         R[0000:00A6] */                 goto loc_44;
/* 0000:00AB mov [Reg(bx), 0x97]                                R[0000:0070] */        loc_AB:  reg.bx = 0x97;
/* 0000:00AE jmp [0x44]                                         R[0000:00AB] */                 goto loc_44;
/* 0000:00B0 mov [Reg(bx), 0xa3]                                R[0000:006C] */        loc_B0:  reg.bx = 0xa3;
/* 0000:00B3 jmp [0x44]                                         R[0000:00B0] */                 goto loc_44;
/* 0000:00B5 mov [Reg(bx), 0xaf]                                R[0000:0068] */        loc_B5:  reg.bx = 0xaf;
/* 0000:00B8 jmp [0x44]                                         R[0000:00B5] */                 goto loc_44;
/* 0000:00BA mov [Reg(bx), 0xbb]                                R[0000:0064] */        loc_BA:  reg.bx = 0xbb;
/* 0000:00BD jmp [0x44]                                         R[0000:00BA] */                 goto loc_44;
}

Regs
sub_BF(Regs reg)
{
  goto loc_BF;
/* 0000:00BF mov [[Reg(ds):0x0293 w2], Reg(bx)]                 R[None     ] */        loc_BF:  MEM16(reg.ds,0x0293) = reg.bx;
/* 0000:00C3 mov [[Reg(ds):0x0291 w2], Reg(bx)]                 R[0000:00BF] */        loc_C3:  MEM16(reg.ds,0x0291) = reg.bx;
/* 0000:00C7 mov [Reg(si), 0x0236]                              R[0000:00C3] */                 reg.si = 0x0236;
/* 0000:00CA mov [Reg(al), [Reg(ds):Reg(bx) w1]]                R[0000:00C7] */        loc_CA:  reg.al = MEM8(reg.ds,reg.bx);
/* 0000:00CC mov [[Reg(ds):Reg(si) w1], Reg(al)]                R[0000:00CA] */                 MEM8(reg.ds,reg.si) = reg.al;
/* 0000:00CE inc [Reg(si)]                                      R[0000:00CC] */                 reg.sresult = ((int16_t)reg.si) + ((int16_t)1); reg.si = reg.uresult = ((uint32_t)reg.si) + ((uint32_t)1); 
/* 0000:00CF inc [Reg(bx)]                                      R[0000:00CE] */                 reg.sresult = ((int16_t)reg.bx) + ((int16_t)1); reg.bx = reg.uresult = ((uint32_t)reg.bx) + ((uint32_t)1); 
/* 0000:00D0 or [Reg(al), Reg(al)]                              R[0000:00CF] */                 reg.uresult = reg.al |= reg.al; reg.sresult=0; reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00D2 jnz [0xca]                                         R[0000:00D0] */                 if (!ZF) goto loc_CA;
/* 0000:00D4 mov [Reg(si), 0x0251]                              R[0000:00D2] */                 reg.si = 0x0251;
/* 0000:00D7 xor [Reg(cl), Reg(cl)]                             R[0000:00D4] */                 reg.uresult = reg.cl ^= reg.cl; reg.sresult=0; reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00D9 mov [Reg(al), [Reg(ds):Reg(bx) w1]]                R[0000:00D7] */        loc_D9:  reg.al = MEM8(reg.ds,reg.bx);
/* 0000:00DB or [Reg(al), Reg(al)]                              R[0000:00D9] */                 reg.uresult = reg.al |= reg.al; reg.sresult=0; reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00DD jnz [0xe1]                                         R[0000:00DB] */                 if (!ZF) goto loc_E1;
/* 0000:00DF mov [Reg(al), 13]                                  R[0000:00DD] */                 reg.al = 13;
/* 0000:00E1 mov [[Reg(ds):Reg(si) w1], Reg(al)]                R[0000:00DD] */        loc_E1:  MEM8(reg.ds,reg.si) = reg.al;
/* 0000:00E3 inc [Reg(si)]                                      R[0000:00E1] */                 reg.sresult = ((int16_t)reg.si) + ((int16_t)1); reg.si = reg.uresult = ((uint32_t)reg.si) + ((uint32_t)1); 
/* 0000:00E4 inc [Reg(bx)]                                      R[0000:00E3] */                 reg.sresult = ((int16_t)reg.bx) + ((int16_t)1); reg.bx = reg.uresult = ((uint32_t)reg.bx) + ((uint32_t)1); 
/* 0000:00E5 inc [Reg(cl)]                                      R[0000:00E4] */                 reg.sresult = ((int8_t)reg.cl) + ((int16_t)1); reg.cl = reg.uresult = ((uint32_t)reg.cl) + ((uint32_t)1); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00E7 cmp [Reg(al), 13]                                  R[0000:00E5] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)13); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)13); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00E9 jnz [0xd9]                                         R[0000:00E7] */                 if (!ZF) goto loc_D9;
/* 0000:00EB dec [Reg(cl)]                                      R[0000:00E9] */                 reg.sresult = ((int8_t)reg.cl) - ((int16_t)1); reg.cl = reg.uresult = ((uint32_t)reg.cl) - ((uint32_t)1); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00ED mov [[Reg(ds):Reg(si) w1], Reg(cl)]                R[0000:00EB] */                 MEM8(reg.ds,reg.si) = reg.cl;
/* 0000:00EF mov [Reg(al), [Reg(ds):Reg(bx) w1]]                R[0000:00ED] */                 reg.al = MEM8(reg.ds,reg.bx);
/* 0000:00F1 mov [[Reg(ds):0x0120 w1], Reg(al)]                 R[0000:00EF] */                 MEM8(reg.ds,0x0120) = reg.al;
/* 0000:00F4 cmp [[Reg(ds):12 w1], 2]                           R[0000:00F1] */        loc_F4:  reg.sresult = ((int8_t)MEM8(reg.ds,12)) - ((int16_t)2); reg.uresult = ((uint32_t)MEM8(reg.ds,12)) - ((uint32_t)2); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:00F9 jnz [0x0108]                                       R[0000:00F4] */                 if (!ZF) goto loc_108;
/* 0000:00FB mov [Reg(dl), [Reg(ds):0x0120 w1]]                 R[0000:00F9] */                 reg.dl = MEM8(reg.ds,0x0120);
/* 0000:00FF sub [Reg(dl), 0x31]                                R[0000:00FB] */                 reg.sresult = ((int8_t)reg.dl) - ((int16_t)0x31); reg.dl = reg.uresult = ((uint32_t)reg.dl) - ((uint32_t)0x31); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0102 mov [Reg(ah), 14]                                  R[0000:00FF] */                 reg.ah = 14;
/* 0000:0104 nop []                                             R[0000:0102] */                 /* nop */;
/* 0000:0105 nop []                                             R[0000:0104] */                 /* nop */;
/* 0000:0106 nop []                                             R[0000:0105] */                 /* nop */;
/* 0000:0107 nop []                                             R[0000:0106] */                 /* nop */;
/* 0000:0108 xor [Reg(al), Reg(al)]                             R[0000:00F9] */       loc_108:  reg.uresult = reg.al ^= reg.al; reg.sresult=0; reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:010A mov [Reg(dx), 0x0236]                              R[0000:0108] */                 reg.dx = 0x0236;
/* 0000:010D push [Reg(ds)]                                     R[0000:010A] */                 stack[stackPtr++].word = reg.ds;
/* 0000:010E pop [Reg(es)]                                      R[0000:010D] */                 reg.es = stack[--stackPtr].word;
/* 0000:010F mov [Reg(bx), 0x0283]                              R[0000:010E] */                 reg.bx = 0x0283;
/* 0000:0112 mov [Reg(ah), 0x4b]                                R[0000:010F] */                 reg.ah = 0x4b;
/* 0000:0114 mov [Reg(al), 0]                                   R[0000:0112] */                 reg.al = 0;
/* 0000:0116 mov [[Reg(ds):0x10 w2], Reg(sp)]                   R[0000:0114] */                 MEM16(reg.ds,0x10) = reg.sp;
/* 0000:011A int [0x21]                                         R[0000:0116] */                 reg = int21(reg);
/* 0000:011C jc [0x0133]                                        R[0000:011A] */                 if (CF) goto loc_133;
/* 0000:011E mov [Reg(ax), 0x1b]                                R[0000:011C] */                 reg.ax = 0x1b;
/* 0000:0121 mov [Reg(ds), Reg(ax)]                             R[0000:011E] */                 reg.ds = reg.ax;
/* 0000:0123 cli []                                             R[0000:0121] */                 /* cli */;
/* 0000:0124 mov [Reg(ax), 0x45]                                R[0000:0123] */                 reg.ax = 0x45;
/* 0000:0127 mov [Reg(ss), Reg(ax)]                             R[0000:0124] */                 reg.ss = reg.ax;
/* 0000:0129 mov [Reg(sp), [Reg(ds):0x10 w2]]                   R[0000:0127] */                 reg.sp = MEM16(reg.ds,0x10);
/* 0000:012D sti []                                             R[0000:0129] */                 /* sti */;
/* 0000:012E mov [Reg(ah), 0x4d]                                R[0000:012D] */                 reg.ah = 0x4d;
/* 0000:0130 int [0x21]                                         R[0000:012E] */                 reg = int21(reg);
/* 0000:0132 ret []                                             R[0000:0130] */                 return reg;
/* 0000:0133 cmp [[Reg(ds):0x0291 w2], 0x12]                    R[0000:011C] */       loc_133:  reg.sresult = ((int16_t)MEM16(reg.ds,0x0291)) - ((int16_t)0x12); reg.uresult = ((uint32_t)MEM16(reg.ds,0x0291)) - ((uint32_t)0x12); 
/* 0000:0139 jz [0x0143]                                        R[0000:0133] */                 if (ZF) goto loc_143;
/* 0000:013B cmp [[Reg(ds):0x0291 w2], 0x1d]                    R[0000:0139] */                 reg.sresult = ((int16_t)MEM16(reg.ds,0x0291)) - ((int16_t)0x1d); reg.uresult = ((uint32_t)MEM16(reg.ds,0x0291)) - ((uint32_t)0x1d); 
/* 0000:0141 jnz [0x0149]                                       R[0000:013B] */                 if (!ZF) goto loc_149;
/* 0000:0143 mov [Reg(bx), 0x29]                                R[0000:0139] */       loc_143:  reg.bx = 0x29;
/* 0000:0146 jmp [0xc3]                                         R[0000:0143] */                 goto loc_C3;
/* 0000:0149 cmp [Reg(ax), 8]                                   R[0000:0141] */       loc_149:  reg.sresult = ((int16_t)reg.ax) - ((int16_t)8); reg.uresult = ((uint32_t)reg.ax) - ((uint32_t)8); 
/* 0000:014C jnz [0x0157]                                       R[0000:0149] */                 if (!ZF) goto loc_157;
/* 0000:014E mov [Reg(dx), 0x01d8]                              R[0000:014C] */                 reg.dx = 0x01d8;
/* 0000:0151 mov [Reg(ah), 9]                                   R[0000:014E] */                 reg.ah = 9;
/* 0000:0153 int [0x21]                                         R[0000:0151] */                 reg = int21(reg);
/* 0000:0155 jmp [0x0155]                                       R[0000:0153] */       loc_155:  goto loc_155;
/* 0000:0157 cmp [[Reg(ds):0x0293 w2], 0x12]                    R[0000:014C] */       loc_157:  reg.sresult = ((int16_t)MEM16(reg.ds,0x0293)) - ((int16_t)0x12); reg.uresult = ((uint32_t)MEM16(reg.ds,0x0293)) - ((uint32_t)0x12); 
/* 0000:015D jz [0x016a]                                        R[0000:0157] */                 if (ZF) goto loc_16A;
/* 0000:015F cmp [[Reg(ds):0x0293 w2], 0x1d]                    R[0000:015D] */                 reg.sresult = ((int16_t)MEM16(reg.ds,0x0293)) - ((int16_t)0x1d); reg.uresult = ((uint32_t)MEM16(reg.ds,0x0293)) - ((uint32_t)0x1d); 
/* 0000:0165 jz [0x016a]                                        R[0000:015F] */                 if (ZF) goto loc_16A;
/* 0000:0167 jmp [0x0180]                                       R[0000:0165] */                 goto loc_180;
/* 0000:016A mov [Reg(dx), 0x014d]                              R[0000:015D] */       loc_16A:  reg.dx = 0x014d;
/* 0000:016D mov [Reg(ah), 9]                                   R[0000:016A] */                 reg.ah = 9;
/* 0000:016F int [0x21]                                         R[0000:016D] */                 reg = int21(reg);
/* 0000:0171 mov [Reg(ah), 7]                                   R[0000:016F] */       loc_171:  reg.ah = 7;
/* 0000:0173 int [0x21]                                         R[0000:0171] */                 reg = int21(reg);
/* 0000:0175 cmp [Reg(al), 13]                                  R[0000:0173] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)13); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)13); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:0177 jnz [0x0171]                                       R[0000:0175] */                 if (!ZF) goto loc_171;
/* 0000:0179 mov [Reg(bx), [Reg(ds):0x0293 w2]]                 R[0000:0177] */                 reg.bx = MEM16(reg.ds,0x0293);
/* 0000:017D jmp [0xc3]                                         R[0000:0179] */                 goto loc_C3;
/* 0000:0180 mov [Reg(dx), 0xd3]                                R[0000:0167] */       loc_180:  reg.dx = 0xd3;
/* 0000:0183 mov [Reg(ah), 9]                                   R[0000:0180] */                 reg.ah = 9;
/* 0000:0185 int [0x21]                                         R[0000:0183] */                 reg = int21(reg);
/* 0000:0187 mov [Reg(ah), 7]                                   R[0000:0185] */       loc_187:  reg.ah = 7;
/* 0000:0189 int [0x21]                                         R[0000:0187] */                 reg = int21(reg);
/* 0000:018B cmp [Reg(al), 13]                                  R[0000:0189] */                 reg.sresult = ((int8_t)reg.al) - ((int16_t)13); reg.uresult = ((uint32_t)reg.al) - ((uint32_t)13); reg.uresult <<= 8; reg.sresult <<= 8;
/* 0000:018D jnz [0x0187]                                       R[0000:018B] */                 if (!ZF) goto loc_187;
/* 0000:018F jmp [0xf4]                                         R[0000:018D] */                 goto loc_F4;
}

uint8_t
play_main(const char *cmdLine)
{
    Regs reg = {{ 0 }};
    int retval;

    // Set up our DOS Exit handler

    retval = setjmp(dosExitJump);
    if (retval) {
        return (uint8_t)retval;
    }

    memset(mem, 0, 2645);
    memcpy(mem, dataImage, sizeof dataImage);

    // Memory size (32-bit)
    reg.bx = 2645 >> 16;
    reg.cx = (uint16_t) 2645;

    // XXX: Stack is fake.
    stackPtr = 0;
    reg.ss = 0;
    reg.sp = 0xFFFF;

    // Beginning of EXE image (unrelocated)
    reg.ds = 0;

    // Program Segment Prefix. Put it right after
    // the end of the program's requested memory,
    // zero it, and copy in our command line.

    reg.es = (2645 + 16) >> 4;
    memset(mem + SEG(reg.es, 0), 0, 0x100);

    mem[SEG(reg.es, 0x80)] = strlen(cmdLine);
    strcpy(mem + SEG(reg.es, 0x81), cmdLine);

    // Jump to the entry point

    reg.cs = 0x0000;
    sub_0(reg);

    return 0xFF;
}
