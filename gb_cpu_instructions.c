#include <stdio.h>
#include <stdlib.h>
#include "gb.h"

// cpu instructions 
// https://meganesu.github.io/generate-gb-opcodes/
// https://gbdev.io/pandocs/CPU_Instruction_Set.html



// im mostly doing as im told in this part, theres many instructions that seem useless but are needed
// this is the most tedious part obviously its just implementing each instruction

// & 0xF masks off the top 4 bits so you only look at the low part
// if those 4 bits overflow sum > 0xF there was a carry from bit3 into bit4
// no idea why games care but they do
static int hc_add(unsigned char a, unsigned char b) {
    return ((a & 0xF)+(b & 0xF))>0xF;
}

// borrow happens when lower part of a is smaller than b
static int hc_sub(unsigned char a, unsigned char b) {
    return (a & 0xF)<(b&0xF);
}

// for 16bit adds just do the same thing but with the lower 12 bits (0xFFF mask)
static int hc_add16(unsigned short a, unsigned short b) {
    return ((a & 0xFFF)+(b & 0xFFF))>0xFFF;
}

// this is just so i dont have to spam these 2 lines in switch
static void immediate_byte_to_reg(GB *gb, uint8_t *reg) {
    *reg = gb_read(gb, gb->regs.pc); //stores it in the reg and inc programcounter
    gb->regs.pc++;
}

// same thing but 16 bits
static void immediate_word_to_reg(GB *gb, uint16_t *reg) {
    *reg = gb_read16(gb, gb->regs.pc);
    gb->regs.pc += 2;
}

// add r to a and set all 4 flags
static void do_add(GB *gb, unsigned char r) {
    unsigned char res=gb->regs.a + r;
    SET_FLAG(&gb->regs, FLAG_Z, res==0);
    SET_FLAG(&gb->regs, FLAG_N, 0);
    SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a, r));
    SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a);
    gb->regs.a = res;
}

// sub r from a and set all 4 flags
static void do_sub(GB *gb, unsigned char r) {
    unsigned char res=gb->regs.a - r;
    SET_FLAG(&gb->regs,FLAG_Z,res==0);
    SET_FLAG(&gb->regs,FLAG_N,1);
    SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
    SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
    gb->regs.a=res;
}

// cp is just sub but dont actually write back
static void do_cp(GB *gb, unsigned char r) {
    unsigned char res=gb->regs.a - r;
    SET_FLAG(&gb->regs,FLAG_Z,res==0);
    SET_FLAG(&gb->regs,FLAG_N,1);
    SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
    SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
}

// and a with r
// h is always set to 1 for and ops for some reason
static void do_and(GB *gb, unsigned char r) {
    gb->regs.a &= r;
    SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
    SET_FLAG(&gb->regs,FLAG_N,0);
    SET_FLAG(&gb->regs,FLAG_H,1);
    SET_FLAG(&gb->regs,FLAG_C,0);
}

// or a with r
static void do_or(GB *gb, unsigned char r) {
    gb->regs.a |= r;
    SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
    SET_FLAG(&gb->regs,FLAG_N,0);
    SET_FLAG(&gb->regs,FLAG_H,0);
    SET_FLAG(&gb->regs,FLAG_C,0);
}

// xor a with r
static void do_xor(GB *gb, unsigned char r) {
    gb->regs.a ^= r;
    SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
    SET_FLAG(&gb->regs,FLAG_N,0);
    SET_FLAG(&gb->regs,FLAG_H,0);
    SET_FLAG(&gb->regs,FLAG_C,0);
}

// inc a register pointer, h flag is set if lower nibble wraps
static void do_inc(GB *gb, uint8_t *reg) {
    (*reg)++;
    SET_FLAG(&gb->regs,FLAG_Z,*reg==0);
    SET_FLAG(&gb->regs,FLAG_N,0);
    SET_FLAG(&gb->regs,FLAG_H,(*reg&0xF)==0);
}

// dec, h flag when lower nibble borrows (was 0 so wraps to F)
static void do_dec(GB *gb, uint8_t *reg) {
    (*reg)--;
    SET_FLAG(&gb->regs,FLAG_Z,*reg==0);
    SET_FLAG(&gb->regs,FLAG_N,1);
    SET_FLAG(&gb->regs,FLAG_H,(*reg&0xF)==0xF);
}

// add hl, rr
// doesnt touch the Z flag only N H C
static void do_add_hl(GB *gb, unsigned short rr) {
    unsigned short res=gb->regs.hl + rr;
    SET_FLAG(&gb->regs,FLAG_N,0);
    SET_FLAG(&gb->regs,FLAG_H,hc_add16(gb->regs.hl,rr));
    SET_FLAG(&gb->regs,FLAG_C,res<gb->regs.hl);
    gb->regs.hl=res;
}

// push a 16bit reg onto the stack
static void do_push(GB *gb, unsigned short rr) {
    gb->regs.sp -= 2;
    gb_write16(gb, gb->regs.sp, rr);
}

// pop 16 bits off stack into a reg pointer
static void do_pop(GB *gb, uint16_t *reg) {
    *reg = gb_read16(gb, gb->regs.sp);
    gb->regs.sp += 2;
}

// conditional absolute jump, reads addr regardless then checks condition
// returns 16 if taken, 12 if not
static int do_jp_cond(GB *gb, int cond) {
    unsigned short addr=gb_read16(gb,gb->regs.pc);
    gb->regs.pc+=2;
    if(cond) { gb->regs.pc=addr; return 16; }
    return 12;
}

// relative jump, offset is signed so you can go backwards
// returns 12 if taken, 8 if not
static int do_jr_cond(GB *gb, int cond) {
    signed char off=(signed char)gb_read(gb,gb->regs.pc);
    gb->regs.pc++;
    if(cond) { gb->regs.pc+=off; return 12; }
    return 8;
}

// conditional call
static int do_call_cond(GB *gb, int cond) {
    unsigned short addr=gb_read16(gb,gb->regs.pc);
    gb->regs.pc+=2;
    if(cond) {
        gb->regs.sp-=2;
        gb_write16(gb,gb->regs.sp,gb->regs.pc);
        gb->regs.pc=addr;
        return 24;
    }
    return 12;
}

// conditional ret
static int do_ret_cond(GB *gb, int cond) {
    if(cond) {
        gb->regs.pc=gb_read16(gb,gb->regs.sp);
        gb->regs.sp+=2;
        return 20;
    }
    return 8;
}

// rst hardcoded call to a fixed vector
// games use these as fast syscall like jumps
static void do_rst(GB *gb, unsigned short vec) {
    gb->regs.sp-=2;
    gb_write16(gb,gb->regs.sp,gb->regs.pc);
    gb->regs.pc=vec;
}

// adc add with carry
// same as add but turns on the carry flag too
// used for doing 16bit math in two 8bit chunks
static void do_adc(GB *gb, unsigned char r) {
    unsigned char c=GET_FLAG_C(gb->regs)?1:0;
    unsigned char res=gb->regs.a + r + c;
    SET_FLAG(&gb->regs,FLAG_Z,res==0);
    SET_FLAG(&gb->regs,FLAG_N,0);
    SET_FLAG(&gb->regs,FLAG_H,((gb->regs.a&0xF)+(r&0xF)+c)>0xF);
    SET_FLAG(&gb->regs,FLAG_C,(unsigned short)gb->regs.a+r+c>0xFF);
    gb->regs.a=res;
}

// sbc same as earlier
static void do_sbc(GB *gb, unsigned char r) {
    unsigned char c=GET_FLAG_C(gb->regs)?1:0;
    unsigned char res=gb->regs.a - r - c;
    SET_FLAG(&gb->regs,FLAG_Z,res==0);
    SET_FLAG(&gb->regs,FLAG_N,1);
    SET_FLAG(&gb->regs,FLAG_H,((gb->regs.a&0xF)-(r&0xF)-c)<0);
    SET_FLAG(&gb->regs,FLAG_C,(unsigned short)gb->regs.a<(unsigned short)r+c);
    gb->regs.a=res;
}

int gb_cpu_step(GB *gb) {
    // interrupts here 
    // ill do those later

    unsigned char op;
    op=gb_read(gb, gb->regs.pc);
    gb->regs.pc++;

    switch(op) {
        case 0x00: return 4; // NOP
        case 0x76: // HALT
            // supposed to stop until interrupt 
            // just gonna ignore it for rn
            return 4;

        case 0xF3: // disable interrupts
            gb->ime=0;
            return 4;
        case 0xFB: // enable interrupts
            gb->ime=1;
            return 4;
        
        // write the byte given into a register
        // as in not from a place in memory
        // so basiclly js reads the next byte after the opcode and sticks it in the reg
        case 0x06: immediate_byte_to_reg(gb, &gb->regs.b); return 8; // so annoying that i have to do each manually
        case 0x0E: immediate_byte_to_reg(gb, &gb->regs.c); return 8;
        case 0x16: immediate_byte_to_reg(gb, &gb->regs.d); return 8;
        case 0x1E: immediate_byte_to_reg(gb, &gb->regs.e); return 8;
        case 0x26: immediate_byte_to_reg(gb, &gb->regs.h); return 8;
        case 0x2E: immediate_byte_to_reg(gb, &gb->regs.l); return 8;
        case 0x3E: immediate_byte_to_reg(gb, &gb->regs.a); return 8;

        // reg to reg copies 
        // theres like 49 of these
        // some are pointless like loading b into b but whatever
        // i wont make a helper cuz its just a single line 
        case 0x41: gb->regs.b = gb->regs.c; return 4;
        case 0x42: gb->regs.b = gb->regs.d; return 4;
        case 0x43: gb->regs.b = gb->regs.e; return 4;
        case 0x47: gb->regs.b = gb->regs.a; return 4;
        case 0x4F: gb->regs.c = gb->regs.a; return 4;
        case 0x4A: gb->regs.c = gb->regs.d; return 4;
        case 0x57: gb->regs.d = gb->regs.a; return 4;
        case 0x50: gb->regs.d = gb->regs.b; return 4;
        case 0x5F: gb->regs.e = gb->regs.a; return 4;
        case 0x58: gb->regs.e = gb->regs.b; return 4;
        case 0x67: gb->regs.h = gb->regs.a; return 4;
        case 0x60: gb->regs.h = gb->regs.b; return 4;
        case 0x6F: gb->regs.l = gb->regs.a; return 4;
        case 0x68: gb->regs.l = gb->regs.b; return 4;
        case 0x78: gb->regs.a = gb->regs.b; return 4;
        case 0x79: gb->regs.a = gb->regs.c; return 4;
        case 0x7A: gb->regs.a = gb->regs.d; return 4;
        case 0x7B: gb->regs.a = gb->regs.e; return 4;
        case 0x7C: gb->regs.a = gb->regs.h; return 4;
        case 0x7D: gb->regs.a = gb->regs.l; return 4;

        // load from memory at address stored in hl
        case 0x46: gb->regs.b = gb_read(gb, gb->regs.hl); return 8;
        case 0x4E: gb->regs.c = gb_read(gb, gb->regs.hl); return 8;
        case 0x56: gb->regs.d = gb_read(gb, gb->regs.hl); return 8;
        case 0x5E: gb->regs.e = gb_read(gb, gb->regs.hl); return 8;
        case 0x7E: gb->regs.a = gb_read(gb, gb->regs.hl); return 8;

        // store register into memory at the address of whatever is in hl
        case 0x70: gb_write(gb, gb->regs.hl, gb->regs.b); return 8;
        case 0x71: gb_write(gb, gb->regs.hl, gb->regs.c); return 8;
        case 0x72: gb_write(gb, gb->regs.hl, gb->regs.d); return 8;
        case 0x73: gb_write(gb, gb->regs.hl, gb->regs.e); return 8;
        case 0x77: gb_write(gb, gb->regs.hl, gb->regs.a); return 8;
        case 0x02: gb_write(gb, gb->regs.bc, gb->regs.a); return 8;
        case 0x12: gb_write(gb, gb->regs.de, gb->regs.a); return 8;
        case 0x0A: gb->regs.a = gb_read(gb, gb->regs.bc); return 8;
        case 0x1A: gb->regs.a = gb_read(gb, gb->regs.de); return 8;

        case 0xFA: {
            // ld a , addr 
            // load from a full 16bit addr
            // ill use the helper i wrote earlier
            unsigned short addr = gb_read16(gb, gb->regs.pc);
            gb->regs.pc += 2;
            gb->regs.a = gb_read(gb, addr);
            return 16;
        }
        case 0xEA: { 
            // ld addr, a 
            // load into an address
            unsigned short addr = gb_read16(gb, gb->regs.pc);
            gb->regs.pc += 2;
            gb_write(gb,addr,gb->regs.a); 
            return 16;
        }

        // ldh 
        // quick way to access io hardware regs
        case 0xE0: {
            unsigned char n = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            gb_write(gb, 0xFF00+n, gb->regs.a);
            return 12;
        }
        case 0xF0: {
            unsigned char n = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            gb->regs.a = gb_read(gb, 0xFF00+n);
            return 12;
        }
        case 0xE2: gb_write(gb, 0xFF00 + gb->regs.c, gb->regs.a); return 8;
        case 0xF2: gb->regs.a = gb_read(gb, 0xFF00 + gb->regs.c); return 8;

        //ldi / ldd
        case 0x22: gb_write(gb, gb->regs.hl++, gb->regs.a); return 8;
        case 0x2A: gb->regs.a = gb_read(gb, gb->regs.hl++); return 8;
        case 0x32: gb_write(gb, gb->regs.hl--, gb->regs.a); return 8;
        case 0x3A: gb->regs.a = gb_read(gb, gb->regs.hl--); return 8;

        // 16bit immediate loads
        case 0x01: immediate_word_to_reg(gb, &gb->regs.bc); return 12;
        case 0x11: immediate_word_to_reg(gb, &gb->regs.de); return 12;
        case 0x21: immediate_word_to_reg(gb, &gb->regs.hl); return 12;
        case 0x31: immediate_word_to_reg(gb, &gb->regs.sp); return 12;
        case 0xF9: gb->regs.sp = gb->regs.hl; return 8;

        case 0xF8: {
            // ld hl , sp+n
            // sp+signed offset into hl
            // flags are weird here took me a little time
            signed char n=(signed char)gb_read(gb,gb->regs.pc);
            gb->regs.pc++;
            unsigned short res=gb->regs.sp + n;
            SET_FLAG(&gb->regs,FLAG_Z,0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,((gb->regs.sp^n^res)&0x10)!=0);
            SET_FLAG(&gb->regs,FLAG_C,((gb->regs.sp^n^res)&0x100)!=0);
            gb->regs.hl=res;
            return 12;
        }

        case 0x08: {
            // store sp somewhere in memory
            // games use this to save it before messing with it
            unsigned short addr = gb_read16(gb, gb->regs.pc);
            gb->regs.pc+=2;
            gb_write16(gb, addr, gb->regs.sp);
            return 20;
        }

        // push and pop
        // sp grows downward so sub first then write
        // it feels so cool implementing this lol
        case 0xC5: do_push(gb, gb->regs.bc); return 16;
        case 0xD5: do_push(gb, gb->regs.de); return 16;
        case 0xE5: do_push(gb, gb->regs.hl); return 16;
        case 0xF5: do_push(gb, gb->regs.af); return 16;

        case 0xC1: // POP!
            do_pop(gb, &gb->regs.bc); return 12;
        case 0xD1: 
            do_pop(gb, &gb->regs.de); return 12;
        case 0xE1: 
            do_pop(gb, &gb->regs.hl); return 12;
        case 0xF1: 
            // pop af
            // lower part of f is always 0 for some reason
            do_pop(gb, &gb->regs.af);
            gb->regs.f&=0xF0;
            return 12;


        // EVERYTHING BELOW HERE IS JUST SIMPLE OPERATIONS 
        // like add , sub, inc , bitwise ops 
        // super tedious and repetetive




        // add a, r
        // if result is less than a it wrapped past 255 so carry happened
        case 0x80: do_add(gb, gb->regs.b); return 4;
        case 0x81: do_add(gb, gb->regs.c); return 4;
        case 0x82: do_add(gb, gb->regs.d); return 4;
        case 0x83: do_add(gb, gb->regs.e); return 4;
        case 0x84: do_add(gb, gb->regs.h); return 4;
        case 0x85: do_add(gb, gb->regs.l); return 4;
        case 0x86: do_add(gb, gb_read(gb, gb->regs.hl)); return 8; // add but from memory
        case 0x87: do_add(gb, gb->regs.a); return 4; // add a to itself, doubles it
        case 0xC6: // add immediate byte :) i like thes ones
            do_add(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // adc a r
        case 0x88: do_adc(gb, gb->regs.b); return 4;
        case 0x89: do_adc(gb, gb->regs.c); return 4;
        case 0x8A: do_adc(gb, gb->regs.d); return 4;
        case 0x8B: do_adc(gb, gb->regs.e); return 4;
        case 0x8C: do_adc(gb, gb->regs.h); return 4;
        case 0x8D: do_adc(gb, gb->regs.l); return 4;
        case 0x8E: do_adc(gb, gb_read(gb, gb->regs.hl)); return 8;
        case 0x8F: do_adc(gb, gb->regs.a); return 4;
        case 0xCE: // adc immediate
            do_adc(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // sub r
        case 0x90: do_sub(gb, gb->regs.b); return 4;
        case 0x91: do_sub(gb, gb->regs.c); return 4;
        case 0x92: do_sub(gb, gb->regs.d); return 4;
        case 0x93: do_sub(gb, gb->regs.e); return 4;
        case 0x94: do_sub(gb, gb->regs.h); return 4;
        case 0x95: do_sub(gb, gb->regs.l); return 4;
        case 0x96: do_sub(gb, gb_read(gb, gb->regs.hl)); return 8; // sub from memory
        case 0x97: // sub a always zero
            do_sub(gb, gb->regs.a); return 4;
        case 0xD6: // sub immediate
            do_sub(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // sbc a r
        case 0x98: do_sbc(gb, gb->regs.b); return 4;
        case 0x99: do_sbc(gb, gb->regs.c); return 4;
        case 0x9A: do_sbc(gb, gb->regs.d); return 4;
        case 0x9B: do_sbc(gb, gb->regs.e); return 4;
        case 0x9C: do_sbc(gb, gb->regs.h); return 4;
        case 0x9D: do_sbc(gb, gb->regs.l); return 4;
        case 0x9E: do_sbc(gb, gb_read(gb, gb->regs.hl)); return 8;
        case 0x9F: do_sbc(gb, gb->regs.a); return 4;
        case 0xDE: // sbc immediate
            do_sbc(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // and op
        case 0xA0: do_and(gb, gb->regs.b); return 4;
        case 0xA1: do_and(gb, gb->regs.c); return 4;
        case 0xA2: do_and(gb, gb->regs.d); return 4;
        case 0xA3: do_and(gb, gb->regs.e); return 4;
        case 0xA4: do_and(gb, gb->regs.h); return 4;
        case 0xA5: do_and(gb, gb->regs.l); return 4;
        case 0xA7: // and a
            // kinda pointless but it exists
            do_and(gb, gb->regs.a); return 4;
        case 0xE6: // and immediate
            do_and(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // or
        case 0xB0: do_or(gb, gb->regs.b); return 4;
        case 0xB1: do_or(gb, gb->regs.c); return 4;
        case 0xB2: do_or(gb, gb->regs.d); return 4;
        case 0xB3: do_or(gb, gb->regs.e); return 4;
        case 0xB4: do_or(gb, gb->regs.h); return 4;
        case 0xB5: do_or(gb, gb->regs.l); return 4;
        case 0xB7: do_or(gb, gb->regs.a); return 4; // or a ???????????????????
        case 0xF6: // or immediate
            do_or(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // xor
        case 0xA8: do_xor(gb, gb->regs.b); return 4;
        case 0xA9: do_xor(gb, gb->regs.c); return 4;
        case 0xAA: do_xor(gb, gb->regs.d); return 4;
        case 0xAB: do_xor(gb, gb->regs.e); return 4;
        case 0xAC: do_xor(gb, gb->regs.h); return 4;
        case 0xAD: do_xor(gb, gb->regs.l); return 4;
        case 0xAF: 
            // xor a
            // zero fast
            do_xor(gb, gb->regs.a); return 4;
        case 0xEE: // xor immediate
            do_xor(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // cp
        case 0xB8: do_cp(gb, gb->regs.b); return 4;
        case 0xB9: do_cp(gb, gb->regs.c); return 4;
        case 0xBA: do_cp(gb, gb->regs.d); return 4;
        case 0xBB: do_cp(gb, gb->regs.e); return 4;
        case 0xBC: do_cp(gb, gb->regs.h); return 4;
        case 0xBD: do_cp(gb, gb->regs.l); return 4;
        case 0xBF: // cp a , a bruh WHY! 😭
            do_cp(gb, gb->regs.a); return 4;
        case 0xFE: // cp immediate
            do_cp(gb, gb_read(gb, gb->regs.pc));
            gb->regs.pc++;
            return 8;

        // inc
        case 0x04: do_inc(gb, &gb->regs.b); return 4;
        case 0x0C: do_inc(gb, &gb->regs.c); return 4;
        case 0x14: do_inc(gb, &gb->regs.d); return 4;
        case 0x1C: do_inc(gb, &gb->regs.e); return 4;
        case 0x24: do_inc(gb, &gb->regs.h); return 4;
        case 0x2C: do_inc(gb, &gb->regs.l); return 4;
        case 0x3C: do_inc(gb, &gb->regs.a); return 4;
        case 0x34: { // inc hl
            unsigned char v=gb_read(gb,gb->regs.hl);
            v++;
            gb_write(gb,gb->regs.hl,v);
            SET_FLAG(&gb->regs,FLAG_Z,v==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(v&0xF)==0);
            return 12;
        }

        // dec
        case 0x05: do_dec(gb, &gb->regs.b); return 4;
        case 0x0D: do_dec(gb, &gb->regs.c); return 4;
        case 0x15: do_dec(gb, &gb->regs.d); return 4;
        case 0x1D: do_dec(gb, &gb->regs.e); return 4;
        case 0x25: do_dec(gb, &gb->regs.h); return 4;
        case 0x2D: do_dec(gb, &gb->regs.l); return 4;
        case 0x3D: do_dec(gb, &gb->regs.a); return 4;
        case 0x35: { // dec hl
            unsigned char v=gb_read(gb,gb->regs.hl);
            v--;
            gb_write(gb,gb->regs.hl,v);
            SET_FLAG(&gb->regs,FLAG_Z,v==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(v&0xF)==0xF);
            return 12;
        }

        // 16bit inc abd dec
        case 0x03: gb->regs.bc++; return 8;
        case 0x13: gb->regs.de++; return 8;
        case 0x23: gb->regs.hl++; return 8;
        case 0x33: gb->regs.sp++; return 8;
        case 0x0B: gb->regs.bc--; return 8;
        case 0x1B: gb->regs.de--; return 8;
        case 0x2B: gb->regs.hl--; return 8;
        case 0x3B: gb->regs.sp--; return 8;

        // add hl, rr
        case 0x09: do_add_hl(gb, gb->regs.bc); return 8;
        case 0x19: do_add_hl(gb, gb->regs.de); return 8;
        case 0x29: do_add_hl(gb, gb->regs.hl); return 8; // add hl to itself
        case 0x39: do_add_hl(gb, gb->regs.sp); return 8;

        // jumps
        // jp nn just reads 2 bytes and sets pc
        case 0xC3: {
            gb->regs.pc=gb_read16(gb,gb->regs.pc);
            return 16;
        }

        // conditional jumps
        // always read the address first regardless, then decide if we jump
        // 16 cycles if taken 12 if not
        case 0xC2: return do_jp_cond(gb, !GET_FLAG_Z(gb->regs));
        case 0xCA: return do_jp_cond(gb,  GET_FLAG_Z(gb->regs));
        case 0xD2: return do_jp_cond(gb, !GET_FLAG_C(gb->regs));
        case 0xDA: return do_jp_cond(gb,  GET_FLAG_C(gb->regs));

        case 0xE9: // jp hl
            gb->regs.pc=gb->regs.hl;
            return 4;

        // jr relative jump
        // offset is a signed byte so it can go backwards, took me a sec to get that
        case 0x18: {
            signed char off=(signed char)gb_read(gb,gb->regs.pc);
            gb->regs.pc++;
            gb->regs.pc+=off;
            return 12;
        }
        case 0x20: return do_jr_cond(gb, !GET_FLAG_Z(gb->regs)); // jr nz
        case 0x28: return do_jr_cond(gb,  GET_FLAG_Z(gb->regs)); // jr z
        case 0x30: return do_jr_cond(gb, !GET_FLAG_C(gb->regs)); // jr nc
        case 0x38: return do_jr_cond(gb,  GET_FLAG_C(gb->regs)); // jr c

        // call pushes the return address then jumps
        // ret pops it back, basically just push and pop for pc
        case 0xCD: {
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc);
            gb->regs.pc=addr;
            return 24;
        }

        case 0xC4: return do_call_cond(gb, !GET_FLAG_Z(gb->regs)); // call nz
        case 0xCC: return do_call_cond(gb,  GET_FLAG_Z(gb->regs)); // call z
        case 0xD4: return do_call_cond(gb, !GET_FLAG_C(gb->regs)); // call nc
        case 0xDC: return do_call_cond(gb,  GET_FLAG_C(gb->regs)); // call c

        case 0xC9: // ret
            gb->regs.pc=gb_read16(gb,gb->regs.sp);
            gb->regs.sp+=2;
            return 16;

        case 0xD9: // reti - ret and reenable interrupts
            gb->regs.pc=gb_read16(gb,gb->regs.sp);
            gb->regs.sp+=2;
            gb->ime=1;
            return 16;

        case 0xC0: return do_ret_cond(gb, !GET_FLAG_Z(gb->regs)); // ret nz
        case 0xC8: return do_ret_cond(gb,  GET_FLAG_Z(gb->regs)); // ret z
        case 0xD0: return do_ret_cond(gb, !GET_FLAG_C(gb->regs)); // ret nc
        case 0xD8: return do_ret_cond(gb,  GET_FLAG_C(gb->regs)); // ret c

        // rst, hardcoded call to a fixed vector
        // games use these as fast syscall like jumps
        case 0xC7: do_rst(gb, 0x00); return 16;
        case 0xCF: do_rst(gb, 0x08); return 16;
        case 0xD7: do_rst(gb, 0x10); return 16;
        case 0xDF: do_rst(gb, 0x18); return 16;
        case 0xE7: do_rst(gb, 0x20); return 16;
        case 0xEF: do_rst(gb, 0x28); return 16;
        case 0xF7: do_rst(gb, 0x30); return 16;
        case 0xFF: do_rst(gb, 0x38); return 16;

        // rotates on a
        // rlca: shift left, old bit7 wraps into bit0 AND goes to carry
        // rla: shift left, old bit7 goes to carry, old carry goes into bit0
        // & 0x80 isolates bit7, >> 7 shifts it down to bit0
        case 0x07: {
            unsigned char c=(gb->regs.a&0x80)>>7;
            gb->regs.a=(gb->regs.a<<1)|c;
            SET_FLAG(&gb->regs,FLAG_Z,0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,c);
            return 4;
        }
        case 0x17: {
            unsigned char oldc=GET_FLAG_C(gb->regs)?1:0;
            unsigned char newc=(gb->regs.a&0x80)>>7;
            gb->regs.a=(gb->regs.a<<1)|oldc;
            SET_FLAG(&gb->regs,FLAG_Z,0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,newc);
            return 4;
        }
        case 0x0F: { // rrca
            unsigned char c=gb->regs.a&0x01;
            gb->regs.a=(gb->regs.a>>1)|(c<<7);
            SET_FLAG(&gb->regs,FLAG_Z,0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,c);
            return 4;
        }
        case 0x1F: { // rra
            unsigned char oldc=GET_FLAG_C(gb->regs)?1:0;
            unsigned char newc=gb->regs.a&0x01;
            gb->regs.a=(gb->regs.a>>1)|(oldc<<7);
            SET_FLAG(&gb->regs,FLAG_Z,0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,newc);
            return 4;
        }

        // misc
        case 0x2F: // cpl, ~ flips every bit in a
            gb->regs.a=~gb->regs.a;
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,1);
            return 4;

        case 0x3F: // ccf, flip carry flag
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,!GET_FLAG_C(gb->regs));
            return 4;

        case 0x37: // scf, set carry flag
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,1);
            return 4;

        case 0x27:
            // dont really get this yet
            // ill do if smth breaks
            return 4;

        case 0xCB: {
            unsigned char cb = gb_read(gb, gb->regs.pc++);
            return gb_execute_cb(gb, cb);
        }

        default:
            printf("whoops undefined opcode 0x%02X\n :-(", op);
            return 4;
    }
}

// cb prefixes
// bit operations
int gb_execute_cb(GB *gb, unsigned char op) {
    // lower 3 bits tell which reg 
    // bits 3-7 tell op
    // 0=b 1=c 2=d 3=e 4=h 5=l 6=hp 7=a
    unsigned char *r;
    int is_hl=0;
    unsigned char temp;
    
    switch(op&0x07) {
        case 0: r=&gb->regs.b; break;
        case 1: r=&gb->regs.c; break;
        case 2: r=&gb->regs.d; break;
        case 3: r=&gb->regs.e; break;
        case 4: r=&gb->regs.h; break;
        case 5: r=&gb->regs.l; break;
        case 6:
            is_hl=1;
            temp=gb_read(gb,gb->regs.hl);
            r=&temp;
            break;
        case 7: r=&gb->regs.a; break;
    }

    unsigned char high=(op>>6)&0x03;
    unsigned char bit=(op>>3)&0x07;

    if(high==0) {
        // rotates and shifts 0x00-0x3F
        switch((op>>3)&0x07) {
            case 0: { // rlc
                unsigned char c=(*r&0x80)>>7;
                *r=(*r<<1)|c;
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,c);
                break;
            }
            case 1: { // rrc
                unsigned char c=*r&0x01;
                *r=(*r>>1)|(c<<7);
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,c);
                break;
            }
            case 2: { // rl
                unsigned char oldc=GET_FLAG_C(gb->regs)?1:0;
                unsigned char newc=(*r&0x80)>>7;
                *r=(*r<<1)|oldc;
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,newc);
                break;
            }
            case 3: { // rr
                unsigned char oldc=GET_FLAG_C(gb->regs)?1:0;
                unsigned char newc=*r&0x01;
                *r=(*r>>1)|(oldc<<7);
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,newc);
                break;
            }
            case 4: { // sla
                unsigned char c=(*r&0x80)>>7;
                *r<<=1;
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,c);
                break;
            }
            case 5: { // sra keep sign bit
                unsigned char c=*r&0x01;
                unsigned char sign=*r&0x80;
                *r=(*r>>1)|sign;
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,c);
                break;
            }
            case 6: { // swap bottoms
                *r=((*r&0x0F)<<4)|((*r&0xF0)>>4);
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,0);
                break;
            }
            case 7: { // srl
                unsigned char c=*r&0x01;
                *r>>=1;
                SET_FLAG(&gb->regs,FLAG_Z,*r==0);
                SET_FLAG(&gb->regs,FLAG_N,0);
                SET_FLAG(&gb->regs,FLAG_H,0);
                SET_FLAG(&gb->regs,FLAG_C,c);
                break;
            }
        }
    }
    else if(high==1) {
        // bit test 0x40 to 0x7F
        unsigned char result=(*r)&(1<<bit);
        SET_FLAG(&gb->regs,FLAG_Z,result==0);
        SET_FLAG(&gb->regs,FLAG_N,0);
        SET_FLAG(&gb->regs,FLAG_H,1);
    }
    else if(high==2) {
        // res clear bit 0x80 to 0xBF
        *r&=~(1<<bit);
    }
    else if(high==3) {
        // set bit 0xC0 to 0xFF
        *r|=(1<<bit);
    }

    if(is_hl) {
        gb_write(gb,gb->regs.hl,temp);
        return 16;
    }

    return 8;
}