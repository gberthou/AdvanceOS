# AdvanceOS
A tiny Operating System that emulates GBA on Raspberry Pi.

## Principle
GBA has an ARM7TDMI processor and the processor of Raspberry Pi is ARM1176JZF-S. The ARM7 instruction sets (ARM and thumb) are compatible with the ARM11 ones. Hence no instruction decoding is required to simulate the instructions sets. Special registers such as CPSR have the same bit fields so pre-editing the binaries to execute can be avoided.

## Preview
![alt text](https://github.com/gberthou/AdvanceOS/blob/master/images/ansi_console.png "ansi_console demo")

## How to install and run
### Installation
1. (Only if you want to debug/emulate the kernel on your computer)  Clone the following repo, checkout rpi branch and build it to get a version of qemu that simulates Raspberry Pi: [gberthou/qemu](https://github.com/gberthou/qemu/tree/rpi). The repo is actually a fork of Torlus/qemu with support for DMA features that are used in this project
2. (Only if you want to debug/emulate the kernel on your computer)  Append path to qemu/arm-softmmu to your PATH environment variable
3. Clone this repo
4. run `make build`

### Compiling the kernel
run `make`

### Running the kernel on Raspberry Pi hardware
Put the generated kernel.img into your SD card

### Debugging the kernel on the computer (requires qemu)
1. Change directory to ./qemu
2. Run ./run-emu.sh in a terminal
3. Run ./run-gdb.sh in another terminal

Currently, qemu and gdb communicate through port 2222. If you have another application that uses this port, you can edit run-emu.sh and .gdbinit to change the remote port.

## Features
1. Use of MMU
  * Memory map to recreate the GBA memory environment
  * Traps on attempts to write to GBA peripherals memory
2. GBA Peripherals
  * LCD (backgrounds and sprites, no rotation/scaling)
  * Timers
  * DMA
  * IRQ management

## Branch overview
This repo contains several branches. Everything on master branch belongs to the current version of the project. Other branches show alternative experimental potential improvements:

1. instructiondecoding: replaces the two-stage data fault handler with a lighter one-stage handler. It relies on assumptions about the faulting instructions (only STRx instructions should be used). System performance is better using this solution, but as it is not fully tested (user code might use hacks with stack pointer and STMx instructions that are not yet supported for example) it is not yet merged into master.
2. fulldma: uses Raspberry Pi DMA to perform memory copies and fills to increase system performance, mainly in framebuffer operations.

## Additional informations
GBA files that are located in the resource folder are compiled from the devkitARM [GBA examples](http://sourceforge.net/projects/devkitpro/files/examples/gba/). If you are interested in GBA development, there is more information about [how to setup devkitARM environment](http://devkitpro.org/wiki/Getting_Started/devkitARM).
