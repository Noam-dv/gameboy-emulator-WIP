# cpu instructions

#### loads (8bit)

* [X]  LD r, n (immediate into register) - done
* [X]  LD r, r (register to register) - done, only did the ones that actually make sense to use, skipped the pointless ones like LD B,B
* [X]  LD r, (HL) (load from memory at HL) - done
* [X]  LD (HL), r (store to memory at HL) - done
* [X]  LD A, (BC) / (DE) - done
* [X]  LD (BC) / (DE), A - done
* [X]  LD A, (nn) / LD (nn), A (full 16bit address) - done
* [X]  LDH (n), A / LDH A, (n) (io port shortcut) - done
* [X]  LD (C), A / LD A, (C) - done
* [X]  LDI / LDD (load/store with HL increment/decrement) - done
* [ ]  LD (HL), n (immediate to memory) - not done yet

#### loads (16bit)

* [X]  LD rr, nn (immediate into register pair) - done, all 4
* [X]  LD SP, HL - done
* [X]  LD (nn), SP - done
* [X]  LD HL, SP+n (sp + signed offset into hl) - done

#### stack

* [X]  PUSH BC/DE/HL/AF - done
* [X]  POP BC/DE/HL/AF - done

#### arithmetic (8bit)

* [X]  ADD A, r - done all (B C D E H L A)
* [X]  ADD A, (HL) - done
* [X]  ADD A, n (immediate) - done
* [X]  ADC A, r - done all (B C D E H L (HL) A n)
* [X]  SUB r - done all (B C D E H L A)
* [X]  SUB (HL) - done
* [X]  SUB n - done
* [X]  SBC r - done all (B C D E H L (HL) A n)
* [X]  AND r - done all (B C D E H L A)
* [X]  AND n - done
* [X]  OR r - done all (B C D E H L A)
* [X]  OR n - done
* [X]  XOR r - done all (B C D E H L A)
* [X]  XOR n - done
* [X]  CP r - done all (B C D E H L A)
* [X]  CP n - done
* [X]  INC r - done all 7
* [X]  INC (HL) - done
* [X]  DEC r - done all 7
* [X]  DEC (HL) - done
* [ ]  DAA - not done, dont really understand it yet
* [X]  CPL (complement A) - done
* [X]  CCF (flip carry) - done
* [X]  SCF (set carry) - done

#### arithmetic (16bit)

* [X]  ADD HL, rr - done all 4
* [ ]  ADD SP, n (signed immediate) - not done
* [X]  INC rr - done all 4
* [X]  DEC rr - done all 4

#### jumps

* [X]  JP nn - done
* [X]  JP cc, nn (conditional) - done, NZ Z NC C
* [X]  JP (HL) - done
* [X]  JR n (relative) - done
* [X]  JR cc, n (conditional relative) - done, NZ Z NC C

#### calls and returns

* [X]  CALL nn - done
* [X]  CALL cc, nn - done, all 4 conditions
* [X]  RET - done
* [X]  RET cc - done, all 4 conditions
* [X]  RETI (return + enable interrupts) - done but ime flag is still a todo
* [X]  RST (hardcoded vectors) - done all 8

#### rotates and shifts

* [X]  RLCA / RLA / RRCA / RRA (rotate A) - done
* [ ]  CB prefix (RLC/RRC/RL/RR/SLA/SRA/SWAP/SRL on all registers) - not done
* [ ]  CB prefix BIT/SET/RES - not done

#### misc

* [X]  NOP - done
* [ ]  HALT - stubbed, just returns 4 cycles for now
* [ ]  DI / EI (interrupts) - stubbed, ime flag not implemented yet
