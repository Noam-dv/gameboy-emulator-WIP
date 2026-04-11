# gameboy-emulator

Im interested in learning how to write an emulator from scratch. this is a research project
i am following a guide and implementing alone in an attempt to learn in a more interesting way

#### progress 1

i started by reading through the gameboy docs (pandocs, links are in the code) just to understand how everything actually works cpu registers, memory map, all that stuff

instead of rushing into coding, i first defined the entire system state (registers, memory, ppu, timer, input, etc) so i actually know what im working with and how everything connects

right now its basically just the full structure of the gameboy, no real logic yet

next step is to start actually implementing things, im probably gonna be starting with memory reads and writes and then the cpu loop

#### progress 2

got the memory system working. basically just gb\_read and gb\_write every part of the emulator talks to memory through these 2functions so i wanted to get this done before touching anything else

the memory map has a bunch of different regions (rom, vram, wram, oam, io) and each one needs to be handled separately, some of them have weird stuff like echo ram mirroring wram for no reason (from what i understand) and io registers that do stuff on write instead of just storing the value . the joypad register was a bit confusing, its active low so 0 means pressed which is backwards from what you'd expect lol.

some stuff ill need to fix later: the bank switching (it js reads bank 1 for now) and the 160 cycles for dma transfer

#### progress 3

started on the cpu instruction decoding. its basically one giant switch statement where each case is an opcode, i raed the pandocs and that opcode table i linked in the code to figure out what each one does the most annoying part is theres like 500 instructions and a lot of them are just the same thing repeated for every register (like bruh wtf is ld r,r).

wrote a small helper so i dont have to repeat the immediate load pattern everywhere got through the main ones: loads, arithemtic stuff (add/sub/and/or/xor and a couple more), jumps, calls, returns, stack push/pop, and some more shit.

also learned what the half carry flag actually does, its a carry out of bit3 into bit4, no idea why so many games check it but they do so its gotta be right

still missing quite a bit. check instructions.md (ai generated table to help me keep track of whats left for me to implement)

#### progress 4

finished off the remaining gaps in the arithmetic section. the hl ones cant use the do_inc/do\_dec helpers since those take a pointer and you cant really point into memory the same way

the 8bit arithmetic section is almost entirely done now which is nice to see!!
