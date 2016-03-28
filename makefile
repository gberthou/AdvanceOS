ARM=arm-none-eabi
BIN=kernel

OBJDIR=obj
DISASDIR=disas

USPI_LIBFILE=uspi/lib/libuspi.a

# Please set LIBDIR and INCDIR accordingly to your file organization
LIBDIR=-L"uspi/lib" 
INCDIR=-I"uspi/include" 

LIBS_USPI=-luspi

CFLAGS=-g -Wall -Wextra -Werror -pedantic -fomit-frame-pointer -fno-stack-limit -mno-apcs-frame -nostartfiles -ffreestanding -march=armv6z -marm -mthumb-interwork -O3 -mfloat-abi=hard

ASFLAGS=-march=armv6z

LDFLAGS=-nostartfiles -mfloat-abi=hard

DEFINES=

CFILES=$(wildcard *.c) $(wildcard peripherals/*.c)
ASFILES=$(wildcard *.s)

CFILES_USPI=$(wildcard uspienv/*.c)
ASFILES_USPI=$(wildcard uspienv/*.s)

LDSCRIPT=ldscript.l

OBJS=$(patsubst %.s,$(OBJDIR)/%.o,$(ASFILES))
OBJS+=$(patsubst %.c,$(OBJDIR)/%.o,$(CFILES))

OBJS_USPI=$(patsubst %.s,$(OBJDIR)/%.o,$(ASFILES_USPI))
OBJS_USPI+=$(patsubst %.c,$(OBJDIR)/%.o,$(CFILES_USPI))

$(OBJDIR)/%.o : %.s
	$(ARM)-as $(ASFLAGS) $< -o $@

$(OBJDIR)/%.o : %.c
	$(ARM)-gcc $(CFLAGS) $(INCDIR) -c $< -o $@ $(DEFINES)

default: $(LDSCRIPT) $(OBJS) $(OBJS_USPI) $(USPI_LIBFILE)
	$(ARM)-gcc $(LDFLAGS) $(OBJS) $(OBJS_USPI) -o $(BIN).elf $(LIBDIR) $(LIBS_USPI) -T ldscript.l
	$(ARM)-objcopy $(BIN).elf -O binary $(BIN).img
	$(ARM)-objdump -D $(BIN).elf > $(DISASDIR)/$(BIN)
	

qemu-compatible: $(LDSCRIPT) $(OBJS)
	$(ARM)-gcc $(LDFLAGS) $(OBJS) -o $(BIN).elf $(LIBDIR) -T ldscript.l
	$(ARM)-objcopy $(BIN).elf -O binary $(BIN).img
	$(ARM)-objdump -D $(BIN).elf > $(DISASDIR)/$(BIN)

build:
	mkdir -p $(OBJDIR) $(DISASDIR) $(OBJDIR)/peripherals $(OBJDIR)/uspienv

clean:
	rm -f $(OBJS) $(OBJS_USPI) $(USPI_LIBFILE)

$(USPI_LIBFILE):
	cd uspi/lib && make clean && make
