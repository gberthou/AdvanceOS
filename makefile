ARM=arm-none-eabi
BIN=kernel

OBJDIR=obj
DISASDIR=disas

CFLAGS=-g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -fno-stack-limit -mno-apcs-frame -nostartfiles -ffreestanding -march=armv6z -marm -mthumb-interwork -O2

ASFLAGS=-march=armv6z

CFILES=$(wildcard *.c) $(wildcard peripherals/*.c)
ASFILES=$(wildcard *.s)
LDSCRIPT=ldscript.l

OBJS=$(patsubst %.s,$(OBJDIR)/%.o,$(ASFILES))
OBJS+=$(patsubst %.c,$(OBJDIR)/%.o,$(CFILES))

$(OBJDIR)/%.o : %.s
	$(ARM)-as $(ASFLAGS) $< -o $@

$(OBJDIR)/%.o : %.c
	$(ARM)-gcc $(CFLAGS) -c $< -o $@

default: $(LDSCRIPT) $(OBJS)
	echo $(OBJS)
	$(ARM)-ld $(OBJS) -o $(BIN).elf -T ldscript.l
	$(ARM)-objcopy $(BIN).elf -O binary $(BIN).img
	$(ARM)-objdump -D $(BIN).elf > $(DISASDIR)/$(BIN)
	

build:
	mkdir -p $(OBJDIR) $(DISASDIR) $(OBJDIR)/peripherals

clean:
	rm -f $(OBJS)
