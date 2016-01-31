# AdvanceOS
A tiny Operating System that emulates GBA on Raspberry Pi

## Principle
GBA has an ARM7TDMI processor and the processor of Raspberry Pi is ARM1176JZF-S. The ARM7 instruction sets (ARM and thumb) are compatible with the ARM11 ones. Hence no instruction decoding is required to simulate the instructions sets. Special registers such as CPSR have the same bit fields so pre-editing the binaries to execute can be avoided.

## Preview

## Features
1. Use of MMU
..* Memory map to recreate the GBA memory environment
..* Traps on attempts to write to GBA peripherals memory
2. GBA Peripherals
..* LCD (background mode 0 only)
..* Timers
..* DMA
..* IRQ management

