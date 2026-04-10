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
static void immediate_byte_to_reg(GameBoy *gb, uint8_t *reg) {
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
        case 0xFB: // enable interrupts
        
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
            unsigned char n=gb_read(gb, gb->regs.pc++);
            gb_write(gb, 0xFF00+n, gb->regs.a);
            return 12;
        }
        case 0xF0: {
            unsigned char n=gb_read(gb, gb->regs.pc++);
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
            unsigned char r=gb_read(gb, gb->regs.pc++);
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
            unsigned char r=gb_read(gb,gb->regs.pc++);
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

        case 0xE6:
            gb->regs.a&=gb_read(gb,gb->regs.pc++);
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,1);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 8;

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

        case 0xF6:
            gb->regs.a|=gb_read(gb,gb->regs.pc++);
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 8;

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

        case 0xEE:
            gb->regs.a^=gb_read(gb,gb->regs.pc++);
            SET_FLAG(&gb->regs,FLAG_Z,gb->regs.a==0);
            SET_FLAG(&gb->regs,FLAG_N,0);
            SET_FLAG(&gb->regs,FLAG_H,0);
            SET_FLAG(&gb->regs,FLAG_C,0);
            return 8;

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
            unsigned char r=gb_read(gb,gb->regs.pc++);
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

        // 16bit inc/dec
        case 0x03: gb->regs.bc++; return 8;
        case 0x13: gb->regs.de++; return 8;
        case 0x23: gb->regs.hl++; return 8;
        case 0x33: gb->regs.sp++; return 8;
        case 0x0B: gb->regs.bc--; return 8;
        case 0x1B: gb->regs.de--; return 8;
        case 0x2B: gb->regs.hl--; return 8;
        case 0x3B: gb->regs.sp--; return 8;
        case 0x27:
            // dont really get this yet
            // ill do if smth breaks
            return 4;

        case 0xCB: {
            unsigned char cb = gb_read(gb, gb->regs.pc++);
            return gb_execute_cb(gb, cb);
        }

        default:
            printf("whoops undefined opcode 0x%02X\n", op);
            return 4;
    }
}

int gb_execute_cb(GB *gb, unsigned char op) {
    //will implement later :)
    return 8;
}