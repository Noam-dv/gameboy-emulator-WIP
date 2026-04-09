#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gb.h"

// the whole point of this file is to have two functions: gb_read and gb_write
// everything else in the emulator talks to memory through these
// that way i dont have to think about the memory map ever again after this

// reads a rom file into memory
// returns 1 if succeded
int gb_load_rom(GB *gb, const char *path) {
    // REMINDER FOR LATER
    // MAKE SURE TO FREE WHEN PROGRAM EXITS 
    // i just know ill forget this lol
    
    FILE *f=fopen(path,"rb");
    if (!f) {
        printf("couldnt open rom %s", path);
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    rewind(f);

    gb->mem.rom = malloc(size);
    if (!gb->mem.rom) {
        printf("out of memory\n");
        fclose(f);
        return 0;
    }

    fread(gb->mem.rom, 1, size, f);
    gb->mem.rom_size = size;
    fclose(f);

    printf("loaded rom %s (%zu bytes)\n", path, size); //will be important for debugging
    return 1;
}

// memory read
// given any 16bit address figure out which region it belongs to and return the byte
// sum edge cases like echo ram and io registers
// all sections are explained in the header file
unsigned char gb_read(GB *gb, unsigned short addr) {

    // rom bank 0 , always mapped to the first 16kb of the rom file
    if (addr <= ROM_BANK0_END)
        return (addr < gb->mem.rom_size) ? gb->mem.rom[addr] : 0xFF;

    // switchable rom bank
    if (addr <= ROM_BANKN_END)
        return (addr < gb->mem.rom_size) ? gb->mem.rom[addr] : 0xFF; // read only bank 1 i didnt add switching yet

    // vram
    // the ppu blocks access tovram during mode 3 but ill implement taht later on
    if (addr >=VRAM_START && addr <= VRAM_END)
        return gb->mem.vram[addr-VRAM_START];

    // external / cartidge ram
    // from what i understand a lot of games dont use this so itll mostly just sit here doing nothing lol
    if (addr >=EXTRAM_START && addr <= EXTRAM_END)
        return gb->mem.extram[addr-EXTRAM_START];

    // work ram
    if (addr >=WRAM_START && addr <= WRAM_END)
        return gb->mem.wram[addr-WRAM_START];

    // echo ram
    // mirrors wram
    // nintendo actually told devs not to use this region apperantly
    if (addr >=ECHO_START && addr <= ECHO_END)
        return gb->mem.wram[addr-ECHO_START]; // same array w a offset

    // oam
    // sprite attribute table
    // again technically blocked during ppu modes 2 and 3   ignoring that for now
    if (addr >=OAM_START && addr <= OAM_END)
        return gb->mem.oam[addr-OAM_START];

    // non usablle region
    if (addr >=0xFEA0 && addr <= 0xFEFF)
        return 0xFF;

    // io registers
    if (addr >=IO_START && addr <= IO_END)
        return gb_read_io(gb, addr); // separate function

    // tiny fast ram
    if (addr >=HRAM_START && addr <= HRAM_END)
        return gb->mem.hram[addr-HRAM_START];

    // interrupts enabled register
    if (addr == IE_REG)
        return gb->mem.ie;

    // shouldnt really get here
    printf("incorrect read");
    return 0xFF;
}

//io register reads 

// some io registers have special read behavior
// like JOYP needs to look at the joypad state and make the byte dynamically
// most are just a straight read from the io array tho
unsigned char gb_read_io(GB *gb, unsigned short addr) {
    switch (addr) {

        case REG_JOYP: {
            // joypad is a bit weird https://gbdev.io/pandocs/Joypad_Input.html
            // this also explains it really well and how i knew which bits t oaccess and when https://gist.github.com/Miliox/eeb9dffa54b95811c593a25e81dba062
            // the upper bits select which group of buttons to read
            // bit 5 = dpad
            // bit 4 = buttons
            unsigned char joyp=gb->mem.io[addr-IO_START] & 0x30; // keep select bits
            unsigned char pressed=gb->joypad.buttons;

            if (!(joyp & 0x20)) {
                if (pressed & DOWN) joyp &= ~0x08; else joyp |= 0x08;
                if (pressed & UP) joyp &= ~0x04; else joyp |= 0x04;
                if (pressed & LEFT) joyp &= ~0x02; else joyp |= 0x02;
                if (pressed & RIGHT) joyp &= ~0x01; else joyp |= 0x01;
            }
            if (!(joyp & 0x10)) {
                if (pressed & START) joyp &= ~0x08; else joyp |= 0x08;
                if (pressed & SELECT) joyp &= ~0x04; else joyp |= 0x04;
                if (pressed & B) joyp &= ~0x02; else joyp |= 0x02;
                if (pressed & A) joyp &= ~0x01; else joyp |= 0x01;
            }

            return joyp;
        }

        case REG_DIV:
            //div is the high byte of the 16bit timer counter
            return (gb->timer.div_cycles >> 8) & 0xFF;

        case REG_LY:
            //cur scanline
            return gb->ppu.curline;

        default:
            return gb->mem.io[addr-IO_START];
    }
}

// same idea as read but for writes
// some things have special write behavior
void gb_write(GB *gb, unsigned short addr, unsigned char val) {

    // writes to rom space dont do anything yet
    if (addr <= ROM_BANKN_END)
        return;

    if (addr >=VRAM_START && addr <= VRAM_END) {
        gb->mem.vram[addr-VRAM_START] = val;
        return;
    }

    if (addr >=EXTRAM_START && addr <= EXTRAM_END) {
        gb->mem.extram[addr-EXTRAM_START] = val;
        return;
    }

    if (addr >=WRAM_START && addr <= WRAM_END) {
        gb->mem.wram[addr-WRAM_START] = val;
        return;
    }

    if (addr >=ECHO_START && addr <= ECHO_END) {
        gb->mem.wram[addr-ECHO_START] = val;
        return;
    }

    if (addr >=OAM_START && addr <= OAM_END) {
        gb->mem.oam[addr-OAM_START] = val;
        return;
    }

    if (addr >=0xFEA0 && addr <= 0xFEFF)
        return; //unusable

    if (addr >=IO_START && addr <= IO_END) {
        gb_write_io(gb, addr, val);
        return;
    }

    if (addr >=HRAM_START && addr <= HRAM_END) {
        gb->mem.hram[addr-HRAM_START] = val;
        return;
    }

    if (addr == IE_REG) {
        gb->mem.ie = val;
        return;
    }

    printf("write is not correct");
}

// io register writes
// some of these do stuff on write instead of just storing the value
void gb_write_io(GB *gb, unsigned short addr, unsigned char val) {
    switch (addr) {
        case REG_DIV:
            // any write to div resets the whole counter to 0
            gb->timer.div_cycles=0;
            gb->mem.io[addr-IO_START]=0;
            return;
        case REG_DMA: {
            // writing will make a copy from (val<<8) into oam as explained in the header file
            // takes smth like 160 cycles and during that time only hram is usable
            // do it instatnly for now
            unsigned short src = val<<8;
            for (int i=0; i<0xA0; i++)
                gb->mem.oam[i] = gb_read(gb, src+i);
            gb->mem.io[addr-IO_START] = val;
            return;
        }
        case REG_LY:
            //this is read only
            // writes just reset it 
            gb->ppu.curline=0;
            gb->mem.io[REG_LY-IO_START]=0;
            return;
        default:
            gb->mem.io[addr-IO_START]=val;
            return;
    }
}

// readwrite helpers
// i read this will be important for later so ill implement it now
//for  things like pushing to the stack
// gb is little endian
unsigned short gb_read16(GB *gb, unsigned short addr) {
    unsigned char l=gb_read(gb,addr);
    unsigned char h=gb_read(gb,addr+1);
    return (h<<8) | l;
}

void gb_write16(GB *gb, unsigned short addr, unsigned short v) {
    gb_write(gb, addr, v&0xFF);
    gb_write(gb, addr+1, v>>8);
}