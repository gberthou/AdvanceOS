# AdvanceOS
A tiny Operating System that emulates GBA on Raspberry Pi

## Principle
GBA has an ARM7TDMI processor and the processor of Raspberry Pi is ARM1176JZF-S. The ARM7 instruction sets (ARM and thumb) are compatible with the ARM11 ones. Hence no instruction decoding is required to simulate the instructions sets. Special registers such as CPSR have the same bit fields so pre-editing the binaries to execute can be avoided.

## Preview
![alt text](https://github.com/gberthou/AdvanceOS/blob/master/images/ansi_console.png "ansi_console demo")

## How to install and run
### Installation
1. Clone the following repo and build it to get a version of qemu that simulates Raspberry Pi: [Torlus/qemu.git](https://github.com/Torlus/qemu.git)
2. Append path to qemu/arm-softmmu to your PATH environment variable
3. Clone this repo
4. run `make build`

### Compiling and running the kernel
5. run `make`
6. Put the generated kernel.img into your SD card

## Features
1. Use of MMU
  * Memory map to recreate the GBA memory environment
  * Traps on attempts to write to GBA peripherals memory
2. GBA Peripherals
  * LCD (background mode 0 only)
  * Timers
  * DMA
  * IRQ management

## Additional informations
GBA files that are located in the resource folder are compiled from the devkitARM [GBA examples](http://sourceforge.net/projects/devkitpro/files/examples/gba/). If you are interested in GBA development, there is more information about [how to setup devkitARM environment](http://devkitpro.org/wiki/Getting_Started/devkitARM).
