# gameboy-emulator

Im interested in learning how to write an emulator from scratch. this is a research project
i am following a guide and implementing alone in an attempt to learn in a more interesting way

#### commit 1

i started by reading through the gameboy docs (pandocs, links are in the code) just to understand how everything actually works cpu registers, memory map, all that stuff

instead of rushing into coding, i first defined the entire system state (registers, memory, ppu, timer, input, etc) so i actually know what im working with and how everything connects

right now its basically just the full structure of the gameboy, no real logic yet

next step is to start actually implementing things, im probably gonna be starting with memory reads and writes and then the cpu loop
