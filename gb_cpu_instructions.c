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
        case 0x01: gb->regs.bc = gb_read16(gb, gb->regs.pc); gb->regs.pc += 2; return 12;
        case 0x11: gb->regs.de = gb_read16(gb, gb->regs.pc); gb->regs.pc += 2; return 12;
        case 0x21: gb->regs.hl = gb_read16(gb, gb->regs.pc); gb->regs.pc += 2; return 12;
        case 0x31: gb->regs.sp = gb_read16(gb, gb->regs.pc); gb->regs.pc += 2; return 12;
        case 0xF9: gb->regs.sp = gb->regs.hl; return 8;

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
        case 0xC5: 
            gb->regs.sp-=2; 
            gb_write16(gb, gb->regs.sp, gb->regs.bc); 
            return 16;
        case 0xD5: 
            gb->regs.sp-=2; 
            gb_write16(gb, gb->regs.sp, gb->regs.de); 
            return 16;
        case 0xE5: 
            gb->regs.sp-=2; 
            gb_write16(gb, gb->regs.sp, gb->regs.hl); 
            return 16;
        case 0xF5: 
            gb->regs.sp-=2; 
            gb_write16(gb, gb->regs.sp, gb->regs.af); 
            return 16;

        case 0xC1: // POP!
            gb->regs.bc=gb_read16(gb, gb->regs.sp); 
            gb->regs.sp += 2; 
            return 12;
        case 0xD1: 
            gb->regs.de=gb_read16(gb, gb->regs.sp); 
            gb->regs.sp += 2; 
            return 12;
        case 0xE1: 
            gb->regs.hl=gb_read16(gb, gb->regs.sp); 
            gb->regs.sp += 2; 
            return 12;
        case 0xF1: 
            // pop af
            // lower part of f is always 0 for some reason
            gb->regs.af = gb_read16(gb, gb->regs.sp);
            gb->regs.f&=0xF0;
            gb->regs.sp+=2;
            return 12;


        // EVERYTHING BELOW HERE IS JUST SIMPLE OPERATIONS 
        // like add , sub, inc , bitwise ops 
        // super tedious and repetetive




        // add a, r
        // if result is less than a it wrapped past 255 so carry happened
        case 0x80: {
            unsigned char r=gb->regs.b;
            unsigned char res=gb->regs.a + r;
            SET_FLAG(&gb->regs, FLAG_Z, res==0);
            SET_FLAG(&gb->regs, FLAG_N, 0);
            SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a,r));
            SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a);
            gb->regs.a = res;
            return 4;
        }

        case 0x81: {
            unsigned char r=gb->regs.c;
            unsigned char res=gb->regs.a + r;
            SET_FLAG(&gb->regs, FLAG_Z, res==0);
            SET_FLAG(&gb->regs, FLAG_N, 0);
            SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a, r));
            SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a);
            gb->regs.a = res;
            return 4;
        }

        case 0x82: {
            unsigned char r=gb->regs.d;
            unsigned char res=gb->regs.a + r;
            SET_FLAG(&gb->regs, FLAG_Z, res==0);
            SET_FLAG(&gb->regs, FLAG_N, 0);
            SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a, r));
            SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a);
            gb->regs.a = res;
            return 4;
        }

        case 0x83: { 
            unsigned char r=gb->regs.e; 
            unsigned char res=gb->regs.a + r; 
            SET_FLAG(&gb->regs, FLAG_Z, res==0); 
            SET_FLAG(&gb->regs, FLAG_N, 0); 
            SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a, r)); 
            SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a); gb->regs.a = res; return 4; }

        case 0x86: {
            // add but from memory
            unsigned char r=gb_read(gb, gb->regs.hl);
            unsigned char res=gb->regs.a + r;
            SET_FLAG(&gb->regs, FLAG_Z, res==0);
            SET_FLAG(&gb->regs, FLAG_N, 0);
            SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a, r));
            SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a);
            gb->regs.a = res;
            return 8;
        }

        case 0xC6: {
            // add immediate byte :) i like thes ones
            unsigned char r = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            unsigned char res=gb->regs.a + r;
            SET_FLAG(&gb->regs, FLAG_Z, res==0);
            SET_FLAG(&gb->regs, FLAG_N, 0);
            SET_FLAG(&gb->regs, FLAG_H, hc_add(gb->regs.a, r));
            SET_FLAG(&gb->regs, FLAG_C, res<gb->regs.a);
            gb->regs.a = res;
            return 8;
        }

        case 0x90: {
            // sub r
            unsigned char r=gb->regs.b;
            unsigned char res=gb->regs.a-r;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
            SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
            gb->regs.a=res;
            return 4;
        }

        case 0x91: {
            unsigned char r=gb->regs.c;
            unsigned char res=gb->regs.a-r;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
            SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
            gb->regs.a=res;
            return 4;
        }

        case 0x92: {
            unsigned char r=gb->regs.d;
            unsigned char res=gb->regs.a-r;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
            SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
            gb->regs.a=res;
            return 4;
        }

        case 0x97: // sub a always zero
            gb->regs.a=0;
            SET_FLAG(&gb->regs,FLAG_Z,1);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xD6: {
            unsigned char r = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            unsigned char res=gb->regs.a-r;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
            SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
            gb->regs.a=res;
            return 8;
        }

        // and op
        case 0xA0:
            gb->regs.a&=gb->regs.b;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,1);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xA1:
            gb->regs.a&=gb->regs.c;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,1);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xA7: 
            // and a
            // kinda pointless but it exists
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,1);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xE6: {
            unsigned char n = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            gb->regs.a&=n;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,1);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 8;
        }

        // or
        case 0xB0:
            gb->regs.a|=gb->regs.b;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xB1:
            gb->regs.a|=gb->regs.c;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xF6: {
            unsigned char n = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            gb->regs.a|=n;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 8;
        }

        // xor
        case 0xA8:
            gb->regs.a^=gb->regs.b;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xA9:
            gb->regs.a^=gb->regs.c;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xAF: 
            // xor a
            // zero fast
            gb->regs.a=0;
            SET_FLAG(&gb->regs,FLAG_Z,1);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xEE: {
            unsigned char n = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            gb->regs.a^=n;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 8;
        }

        // cp
        case 0xB8: {
            unsigned char res=gb->regs.a-gb->regs.b;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,gb->regs.b));
            SET_FLAG(&gb->regs,FLAG_C,gb->regs.b>gb->regs.a);
            return 4;
        }

        case 0xB9: {
            unsigned char res=gb->regs.a-gb->regs.c;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,gb->regs.c));
            SET_FLAG(&gb->regs,FLAG_C,gb->regs.c>gb->regs.a);
            return 4;
        }

        case 0xBF: 
            // cp a , a always true
            SET_FLAG(&gb->regs,FLAG_Z,1);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 4;

        case 0xFE: {
            unsigned char r = gb_read(gb, gb->regs.pc);
            gb->regs.pc++;
            unsigned char res=gb->regs.a-r;
            SET_FLAG(&gb->regs,FLAG_Z,res==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,hc_sub(gb->regs.a,r));
            SET_FLAG(&gb->regs,FLAG_C,r>gb->regs.a);
            return 8;
        }

        // inc
        case 0x04:
            gb->regs.b++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.b==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.b&0xF)==0);
            return 4;

        case 0x0C:
            gb->regs.c++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.c==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.c&0xF)==0);
            return 4;

        case 0x14:
            gb->regs.d++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.d==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.d&0xF)==0);
            return 4;

        case 0x1C:
            gb->regs.e++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.e==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.e&0xF)==0);
            return 4;

        case 0x24:
            gb->regs.h++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.h==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.h&0xF)==0);
            return 4;

        case 0x2C:
            gb->regs.l++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.l==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.l&0xF)==0);
            return 4;

        case 0x3C:
            gb->regs.a++;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.a&0xF)==0);
            return 4;

        // dec
        case 0x05:
            gb->regs.b--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.b==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.b&0xF)==0xF);
            return 4;

        case 0x0D:
            gb->regs.c--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.c==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.c&0xF)==0xF);
            return 4;

        case 0x15:
            gb->regs.d--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.d==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.d&0xF)==0xF);
            return 4;

        case 0x1D:
            gb->regs.e--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.e==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.e&0xF)==0xF);
            return 4;

        case 0x25:
            gb->regs.h--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.h==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.h&0xF)==0xF);
            return 4;

        case 0x2D:
            gb->regs.l--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.l==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.l&0xF)==0xF);
            return 4;

        case 0x3D:
            gb->regs.a--;
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,1);
            SET_FLAG(&gb->regs,FLAG_H,(gb->regs.a&0xF)==0xF);
            return 4;

        // 16bit inc abd dec
        case 0x03: 
            gb->regs.bc++; return 8;
        case 0x13: 
            gb->regs.de++; return 8;
        case 0x23: 
            gb->regs.hl++; return 8;
        case 0x33: 
            gb->regs.sp++; return 8;
        case 0x0B: 
            gb->regs.bc--; return 8;
        case 0x1B: 
            gb->regs.de--; return 8;
        case 0x2B: 
            gb->regs.hl--; return 8;
        case 0x3B: 
            gb->regs.sp--; return 8;

        // add hl, rr
        // doesnt touch the Z flag only N H C
        case 0x09: {
            unsigned short res=gb->regs.hl+gb->regs.bc;
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,hc_add16(gb->regs.hl,gb->regs.bc));
            SET_FLAG(&gb->regs,FLAG_C,res<gb->regs.hl);
            gb->regs.hl=res;
            return 8;
        }
        case 0x19: {
            unsigned short res=gb->regs.hl+gb->regs.de;
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,hc_add16(gb->regs.hl,gb->regs.de));
            SET_FLAG(&gb->regs,FLAG_C,res<gb->regs.hl);
            gb->regs.hl=res;
            return 8;
        }
        case 0x29: {
            // add hl to itself
            unsigned short res=gb->regs.hl+gb->regs.hl;
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,hc_add16(gb->regs.hl,gb->regs.hl));
            SET_FLAG(&gb->regs,FLAG_C,res<gb->regs.hl);
            gb->regs.hl=res;
            return 8;
        }
        case 0x39: {
            unsigned short res=gb->regs.hl+gb->regs.sp;
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,hc_add16(gb->regs.hl,gb->regs.sp));
            SET_FLAG(&gb->regs,FLAG_C,res<gb->regs.hl);
            gb->regs.hl=res;
            return 8;
        }

        // jumps
        // jp nn just reads 2 bytes and sets pc
        case 0xC3: {
            gb->regs.pc=gb_read16(gb,gb->regs.pc);
            return 16;
        }

        // conditional jumps
        // always read the address first regardless, then decide if we jump
        // 16 cycles if taken 12 if not
        case 0xC2: {
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(!GET_FLAG_Z(gb->regs)) { 
                gb->regs.pc=addr; 
                return 16; 
            }
            return 12;
        }
        case 0xCA: {
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(GET_FLAG_Z(gb->regs)) { 
                gb->regs.pc=addr; 
                return 16; 
            }
            return 12;
        }
        case 0xD2: {
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(!GET_FLAG_C(gb->regs)) { 
                gb->regs.pc=addr;
                return 16;
            }
            return 12;
        }
        case 0xDA: {
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(GET_FLAG_C(gb->regs)) { 
                gb->regs.pc=addr;
                return 16;
            }
            return 12;
        }

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
        case 0x20: { // jr nz
            signed char off=(signed char)gb_read(gb,gb->regs.pc);
            gb->regs.pc++;
            if(!GET_FLAG_Z(gb->regs)) { gb->regs.pc+=off; return 12; }
            return 8;
        }
        case 0x28: { // jr z
            signed char off=(signed char)gb_read(gb,gb->regs.pc);
            gb->regs.pc++;
            if(GET_FLAG_Z(gb->regs)) { gb->regs.pc+=off; return 12; }
            return 8;
        }
        case 0x30: { // jr nc
            signed char off=(signed char)gb_read(gb,gb->regs.pc);
            gb->regs.pc++;
            if(!GET_FLAG_C(gb->regs)) { gb->regs.pc+=off; return 12; }
            return 8;
        }
        case 0x38: { // jr c
            signed char off=(signed char)gb_read(gb,gb->regs.pc);
            gb->regs.pc++;
            if(GET_FLAG_C(gb->regs)) { gb->regs.pc+=off; return 12; }
            return 8;
        }

        // call pushes the return address then jumps
        // ret pops it back, basically just push/pop for pc
        case 0xCD: {
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc);
            gb->regs.pc=addr;
            return 24;
        }

        case 0xC4: { // call nz
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(!GET_FLAG_Z(gb->regs)) {
                gb->regs.sp-=2;
                gb_write16(gb,gb->regs.sp,gb->regs.pc);
                gb->regs.pc=addr;
                return 24;
            }
            return 12;
        }
        case 0xCC: { // call z
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(GET_FLAG_Z(gb->regs)) {
                gb->regs.sp-=2;
                gb_write16(gb,gb->regs.sp,gb->regs.pc);
                gb->regs.pc=addr;
                return 24;
            }
            return 12;
        }
        case 0xD4: { // call nc
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(!GET_FLAG_C(gb->regs)) {
                gb->regs.sp-=2;
                gb_write16(gb,gb->regs.sp,gb->regs.pc);
                gb->regs.pc=addr;
                return 24;
            }
            return 12;
        }
        case 0xDC: { // call c
            unsigned short addr=gb_read16(gb,gb->regs.pc);
            gb->regs.pc+=2;
            if(GET_FLAG_C(gb->regs)) {
                gb->regs.sp-=2;
                gb_write16(gb,gb->regs.sp,gb->regs.pc);
                gb->regs.pc=addr;
                return 24;
            }
            return 12;
        }

        case 0xC9: // ret
            gb->regs.pc=gb_read16(gb,gb->regs.sp);
            gb->regs.sp+=2;
            return 16;

        case 0xD9: // reti - ret and reenable interrupts
            gb->regs.pc=gb_read16(gb,gb->regs.sp);
            gb->regs.sp+=2;
            gb->ime=1;
            return 16;

        case 0xC0: // ret nz
            if(!GET_FLAG_Z(gb->regs)) {
                gb->regs.pc=gb_read16(gb,gb->regs.sp);
                gb->regs.sp+=2;
                return 20;
            }
            return 8;
        case 0xC8: // ret z
            if(GET_FLAG_Z(gb->regs)) {
                gb->regs.pc=gb_read16(gb,gb->regs.sp);
                gb->regs.sp+=2;
                return 20;
            }
            return 8;
        case 0xD0: // ret nc
            if(!GET_FLAG_C(gb->regs)) {
                gb->regs.pc=gb_read16(gb,gb->regs.sp);
                gb->regs.sp+=2;
                return 20;
            }
            return 8;
        case 0xD8: // ret c
            if(GET_FLAG_C(gb->regs)) {
                gb->regs.pc=gb_read16(gb,gb->regs.sp);
                gb->regs.sp+=2;
                return 20;
            }
            return 8;

        // rst, hardcoded call to a fixed vector
        // games use these as fast syscall like jumps
        case 0xC7: 
            gb->regs.sp-=2; 
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x00; 
            return 16;
        case 0xCF: 
            gb->regs.sp-=2; 
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x08; 
            return 16;
        case 0xD7: 
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x10; 
            return 16;
        case 0xDF: 
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x18; 
            return 16;
        case 0xE7: 
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x20; 
            return 16;
        case 0xEF: 
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x28; 
            return 16;
        case 0xF7: 
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc); 
            gb->regs.pc=0x30;
            return 16;
        case 0xFF: 
            gb->regs.sp-=2;
            gb_write16(gb,gb->regs.sp,gb->regs.pc);
            gb->regs.pc=0x38;
            return 16;

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

int gb_execute_cb(GB *gb, unsigned char op) {
    //will implement later :)
    return 8;
}