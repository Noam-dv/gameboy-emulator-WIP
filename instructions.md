# cpu instructions

#### loads (8bit)

- [X]  LD r, n (immediate into register) - done
- [X]  LD r, r (register to register) - done, only did the ones that actually make sense to use, skipped the pointless ones like LD B,B
- [X]  LD r, (HL) (load from memory at HL) - done
- [X]  LD (HL), r (store to memory at HL) - done
- [X]  LD A, (BC) / (DE) - done
- [X]  LD (BC) / (DE), A - done
- [X]  LD A, (nn) / LD (nn), A (full 16bit address) - done
- [X]  LDH (n), A / LDH A, (n) (io port shortcut) - done
- [X]  LD (C), A / LD A, (C) - done
- [X]  LDI / LDD (load/store with HL increment/decrement) - done
- [ ]  LD (HL), n (immediate to memory) - not done yet

#### loads (16bit)

- [X]  LD rr, nn (immediate into register pair) - done, all 4
- [X]  LD SP, HL - done
- [X]  LD (nn), SP - done
- [ ]  LD HL, SP+n (sp + signed offset into hl) - not done yet

#### stack

- [X]  PUSH BC/DE/HL/AF - done
- [X]  POP BC/DE/HL/AF - done

#### arithmetic (8bit)

- [ ]  ADD A, r - done for B C D E, skipped H L for now
- [X]  ADD A, (HL) - done
- [X]  ADD A, n (immediate) - done
- [ ]  ADC A, r - not done
- [ ]  SUB r - done for B C D, skipped the rest for now, SUB A done as special case
- [X]  SUB n - done
- [ ]  SBC r - not done
- [ ]  AND r - done for B C, AND A done as special case
- [X]  AND n - done
- [ ]  OR r - done for B C
- [X]  OR n - done
- [ ]  XOR r - done for B C, XOR A done as special case (zero fast)
- [X]  XOR n - done
- [ ]  CP r - done for B C, CP A done as special case
- [X]  CP n - done
- [X]  INC r - done all 7
- [X]  DEC r - done all 7
- [ ]  INC (HL) / DEC (HL) - not done yet
- [ ]  DAA - not done, dont really understand it yet
- [X]  CPL (complement A) - done
- [X]  CCF (flip carry) - done
- [X]  SCF (set carry) - done

#### arithmetic (16bit)

- [X]  ADD HL, rr - done all 4
- [ ]  ADD SP, n (signed immediate) - not done
- [X]  INC rr - done all 4
- [X]  DEC rr - done all 4

#### jumps

- [X]  JP nn - done
- [X]  JP cc, nn (conditional) - done, NZ Z NC C
- [X]  JP (HL) - done
- [X]  JR n (relative) - done
- [X]  JR cc, n (conditional relative) - done, NZ Z NC C

#### calls and returns

- [X]  CALL nn - done
- [X]  CALL cc, nn - done, all 4 conditions
- [X]  RET - done
- [X]  RET cc - done, all 4 conditions
- [X]  RETI (return + enable interrupts) - done but ime flag is still a todo
- [X]  RST (hardcoded vectors) - done all 8

#### rotates and shifts

- [X]  RLCA / RLA / RRCA / RRA (rotate A) - done
- [ ]  CB prefix (RLC/RRC/RL/RR/SLA/SRA/SWAP/SRL on all registers) - not done
- [ ]  CB prefix BIT/SET/RES - not done

#### misc

- [X]  NOP - done
- [ ]  HALT - stubbed, just returns 4 cycles for now
- [ ]  DI / EI (interrupts) - stubbed, ime flag not implemented yet

keeping track of whats done and whats not so i dont lose track

#### loads (8bit)

LD r, n (immediate into register) - done
LD r, r (register to register) - done, only did the ones that actually make sense to use, skipped the pointless ones like LD B,B
LD r, (HL) (load from memory at HL) - done
LD (HL), r (store to memory at HL) - done
LD A, (BC) / (DE) - done
LD (BC) / (DE), A - done
LD A, (nn) / LD (nn), A (full 16bit address) - done
LDH (n), A / LDH A, (n) (io port shortcut) - done
LD (C), A / LD A, (C) - done
LDI / LDD (load/store with HL increment/decrement) - done
LD (HL), n (immediate to memory) - not done yet

#### loads (16bit)

LD rr, nn (immediate into register pair) - done, all 4
LD SP, HL - done
LD (nn), SP - done
LD HL, SP+n (sp + signed offset into hl) - not done yet

#### stack

PUSH BC/DE/HL/AF - done
POP BC/DE/HL/AF - done

#### arithmetic (8bit)

ADD A, r - done for B C D E, skipped H L for now
ADD A, (HL) - done
ADD A, n (immediate) - done
ADC A, r (add with carry) - not done
SUB r - done for B C D, skipped the rest for now, SUB A done as special case
SUB n - done
SBC r (sub with carry) - not done
AND r - done for B C, AND A done as special case
AND n - done
OR r - done for B C
OR n - done
XOR r - done for B C, XOR A done as special case (zero fast)
XOR n - done
CP r - done for B C, CP A done as special case
CP n - done
INC r - done all 7
DEC r - done all 7
INC (HL) / DEC (HL) - not done yet
DAA - not done, dont really understand it yet
CPL (complement A) - done
CCF (flip carry) - done
SCF (set carry) - done

#### arithmetic (16bit)

ADD HL, rr - done all 4
ADD SP, n (signed immediate) - not done
INC rr - done all 4
DEC rr - done all 4

#### jumps

JP nn - done
JP cc, nn (conditional) - done, NZ Z NC C
JP (HL) - done
JR n (relative) - done
JR cc, n (conditional relative) - done, NZ Z NC C

#### calls and returns

CALL nn - done
CALL cc, nn - done, all 4 conditions
RET - done
RET cc - done, all 4 conditions
RETI (return + enable interrupts) - done but ime flag is still a todo
RST (hardcoded vectors) - done all 8

#### rotates and shifts

RLCA / RLA / RRCA / RRA (rotate A) - done
CB prefix (RLC/RRC/RL/RR/SLA/SRA/SWAP/SRL on all registers) - not done
CB prefix BIT/SET/RES - not done

#### misc

NOP - done
HALT - not working
DI / EI (interrupts) - ime flag not implemented yet
