#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ill start by defining the entire state of the system
// this will be useful cuz it allows me to understand every part of the gameboy 
// and read about it before i start trying to implement
// ill take a couple hours to implement this and learn on the way

// screen is 160x14
#define GB_SCREEN_W 160
#define GB_SCREEN_H 144

// cpu runs at abt 4mhz
#define GB_CPU_HZ 4194304

// https://gbdev.io/pandocs/Memory_Map.html
// memory map
// the gb has a 16bit address space so 64kbs
// each region does something different so we hav eto define all the ranges for later
#define ROM_BANK0_START 0x0000 // first 16kb of the cartridge is always here
#define ROM_BANK0_END 0x3FFF
#define ROM_BANKN_START 0x4000 // switchable rom bank
#define ROM_BANKN_END 0x7FFF
#define VRAM_START 0x8000 // video ram , tile data and tilemaps here
#define VRAM_END 0x9FFF
#define EXTRAM_START 0xA000 // external ram on the car  tridge
#define EXTRAM_END 0xBFFF
#define WRAM_START 0xC000 // workram general purpose
#define WRAM_END 0xDFFF
#define ECHO_START 0xE000 // mirrors wram for some reason (probably havent gotten to why thats important yet lol)
#define ECHO_END 0xFDFF
#define OAM_START 0xFE00 // sprite data positions, flags etc
#define OAM_END 0xFE9F
#define IO_START 0xFF00 // hardware registers
#define IO_END 0xFF7F
#define HRAM_START 0xFF80 // tiny bit of fast ram that games use a lot
#define HRAM_END 0xFFFE
#define IE_REG 0xFFFF // one byte that determines if interupts are enabled

// io register addresses
// these are the hardware control knobs basically
// a lot of these i dont understand quite yet , i guess ill see the use for them in the future (shit like 'timer modulo' wtf)
// https://gbdev.io/pandocs/Hardware_Reg_List.html
#define REG_JOYP 0xFF00 // joypad input
#define REG_SB 0xFF01 // serial data ignoring for now
#define REG_SC 0xFF02 // seri al control
#define REG_DIV 0xFF04 // divider register, counts up constantly
#define REG_TIMA 0xFF05 // timer counter
#define REG_TMA 0xFF06 // timer modulo
#define REG_TAC 0xFF07 // timer control
#define REG_IF 0xFF0F // interrupt flag basically which interrupt are pending
#define REG_LCDC 0xFF40 // lcd control , master switch for display stuff
#define REG_STAT 0xFF41 // lcd status , current ppu mode and interupts enabled
#define REG_SCY 0xFF42 // background scroll y
#define REG_SCX 0xFF43 // background scroll x
#define REG_LY 0xFF44 // cur scanline 0 to 153
#define REG_LYC 0xFF45 // scanline compare, triggers interrupt when LY==LYC
#define REG_DMA 0xFF46 // writing here copies a DMA copy into OAM https://gbdev.gg8.se/wiki/articles/OAM_DMA_tutorial
#define REG_BGP 0xFF47 // background palette
#define REG_OBP0 0xFF48 // sprite palette 0
#define REG_OBP1 0xFF49 // sprite palette 1
#define REG_WY 0xFF4A // window y position
#define REG_WX 0xFF4B // window x position

// interrupt bits
// these go in IF and IE registers
// https://gbdev.io/pandocs/Interrupts.html
#define INT_VBLANK 0x01 // end of frame good time to update graphics
#define INT_STAT 0x02 // ppu mode change
#define INT_TIMER 0x04 // TIMA register overflowed 
#define INT_SERIAL 0x08 // serial transfer done
#define INT_JOYPAD 0x10 // button was pressed

// ppu modes
// the ppu cycles through these every scanline
// https://gbdev.io/pandocs/pixel_fifo.html
#define PPU_MODE_HBLANK 0 // end of scanline, cpu can access vram
#define PPU_MODE_VBLANK 1 // end of frame
#define PPU_MODE_OAM 2 // ppu reading sprite data
#define PPU_MODE_TRANSFER 3 // ppu drawing pixels

// gameboy cpu registers
// the gameboty 8bit registers that can be combined into 16bit pairs.
// the main register pairs are AF, BC, DE, HL
// f is the flags register (lower 4 bits are unused).
// https://gbdev.io/pandocs/CPU_Registers_and_Flags.html

// ill define them with unions to allow having one struct for each register
typedef struct {
    union { // AF
        struct { 
            uint8_t f; // flags register
            uint8_t a;
        };
        uint16_t af; // combined 16bit view
    };

    union { // BC
        struct {
            uint8_t c;
            uint8_t b;
        };
        uint16_t bc;
    };

    union { // DE
        struct {
            uint8_t e;
            uint8_t d;
        };
        uint16_t de;
    };

    union { // HL
        struct {
            uint8_t l;
            uint8_t h;
        };
        uint16_t hl;
    };

    uint16_t sp; // stack pointer (i feel so cool defining a stack pointer lmao)
    uint16_t pc; // program counter
} Registers;

// cpu flags
// live in the upper 4 bits of F
#define FLAG_Z 0x80 // zeroflag
#define FLAG_N 0x40 // last operation was subtrcact
#define FLAG_H 0x20 // halfcarry (????? why)
#define FLAG_C 0x10 // carryflag

// check different flags
static int GET_FLAG_Z(Registers r) {
    return (r.f & FLAG_Z) != 0;
}
static int GET_FLAG_N(Registers r) {
    return (r.f & FLAG_N) != 0;
}
static int GET_FLAG_H(Registers r) {
    return (r.f & FLAG_H) != 0;
}
static int GET_FLAG_C(Registers r) {
    return (r.f & FLAG_C) != 0;
}

// sets or clears a flag depending on val
static void SET_FLAG(Registers *r, uint8_t flag, int val) {
    // bitwise operators to not change the whole register only the flag section
    if (val)
        r->f |= flag; // turn the flag on
    else 
        r->f &= ~flag; // turn the flag off
}
typedef struct GB GB;

// memorhy regions
typedef struct {
    // all these where explained earlier in the top
    uint8_t *rom; // heap allocated and loaded from file
    size_t rom_size;
    uint8_t vram[0x2000]; // 8kb
    uint8_t wram[0x2000]; // 8kb
    uint8_t extram[0x2000]; // cartridge ram, probably wont use for a while
    uint8_t oam[0xA0]; // 40 sprites 4 byteseach
    uint8_t hram[0x7F]; // 127 bytes of fast ram
    uint8_t io[0x80]; // io register file
    uint8_t ie; // interrupt enable
} Memory;

// ppu state
typedef struct {
    uint32_t framebuffer[GB_SCREEN_W * GB_SCREEN_H]; // ARGB, one pixel per entry
    int mode; // cur ppu mode
    int mode_clock; // cycles spent in current mode
    int curline; // current scanline
    bool frame_ready; // true when vblank hits
    // main loop checks this when it wants to update graphics
} PPU;

// timer state
// https://gbdev.io/pandocs/Timer_and_Divider_Registers.html
typedef struct {
    uint16_t div_cycles; // 16bit counter , DIV register is the high byte
    int tima_cycles; // tracks when to tick TIMA
} Timer;

// joypad buttons
typedef enum {
    RIGHT = 0x01,
    LEFT = 0x02,
    UP = 0x04,
    DOWN = 0x08,
    A = 0x10,
    B = 0x20,
    SELECT = 0x40,
    START = 0x80,
} GBButton;


// joypad state
// bitmask of whats pressed 
typedef struct {
    uint8_t buttons;
} Joypad;

// FULL EMULATOR STATE!!!
struct GB {
    Registers regs;
    Memory mem;
    PPU ppu;
    Timer timer;
    Joypad joypad;
    int ime; // interupts
};