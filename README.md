# AdvanceOS
A tiny Operating System that emulates GBA on Raspberry Pi.

## Principle
GBA has an ARM7TDMI processor and the processor of Raspberry Pi is ARM1176JZF-S. The ARM7 instruction sets (ARM and thumb) are compatible with the ARM11 ones. Hence no instruction decoding is required to simulate the instructions sets. Special registers such as CPSR have the same bit fields so pre-editing the binaries to execute can be avoided.

## Preview
![Console demo](https://github.com/gberthou/AdvanceOS/blob/master/images/ansi_console.png "ansi_console demo")

![Snake homebrew](https://github.com/gberthou/AdvanceOS/blob/master/images/snake.png "snake homebrew")

## How to install and run
### Installation
1. Clone this repo
2. run `make build`

### Compiling the kernel

#### Command to build a kernel that runs on Raspberry Pi
Simply run `make`

#### Command to build a kernel that is compatible with qemu [deprecated]
The aforementionned version of qemu does not support USB features of BCM2835 so it is mandatory to disable USB.
In order to compile that kernel version, run `make qemu-compatible DEFINES=-DNO_USB`

### Running the kernel on Raspberry Pi hardware
#### Prepare the SD card
If your SD card already contains a valid Raspberry Pi system, you can ignore this step.

First, erase its partitions and create a one that supports vfat, for instance using `fdisk`.
Then format it as vfat using `mkfs.vfat`.
Clone the [officiel Raspberry Pi firmware](https://github.com/raspberrypi/firmware) somewhere into your *computer*, not on the SD card, and copy *only* the contents of the `boot` folder onto your partition.
Finally, remove all `kernel*.img` from your SD card, and don't forget to create a `config.txt` adapted to your display.
There is an example of `config.txt` file in the `configs` subdirectory of this repo.

#### Copy the kernel image
Put the generated kernel.img into your SD card.
If your SD card has several partitions, choose the boot partition alongside Raspberry Pi's firmware files.

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
  * Keypad (user polling only)
  * Timers
  * DMA
  * IRQ management

## Branch overview
This repo contains several branches. Everything on master branch belongs to the current version of the project. Other branches show alternative experimental potential improvements:

1. instructiondecoding: replaces the two-stage data fault handler with a lighter one-stage handler. It relies on assumptions about the faulting instructions (only STRx instructions should be used). System performance is better using this solution, but as it is not fully tested (user code might use hacks with stack pointer and STMx instructions that are not yet supported for example) it is not yet merged into master.
2. fulldma: uses Raspberry Pi DMA to perform memory copies and fills to increase system performance, mainly in framebuffer operations.

## Additional informations
GBA files that are located in the resource folder are compiled from the devkitARM [GBA examples](http://sourceforge.net/projects/devkitpro/files/examples/gba/). If you are interested in GBA development, there is more information about [how to setup devkitARM environment](http://devkitpro.org/wiki/Getting_Started/devkitARM).
