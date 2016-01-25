qemu-system-arm -kernel ../kernel.elf -cpu arm1176 -m 4096 -M raspi -no-reboot -serial vc:240x160 -append "" -S -gdb tcp::2222

