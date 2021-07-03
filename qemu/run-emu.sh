qemu-system-arm -kernel ../kernel.elf -cpu arm1176 -m 512M -machine raspi1ap -no-reboot -serial vc:240x160 -S -gdb tcp::2222

