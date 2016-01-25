/*
MEMORY
{
	vector  : ORIGIN = 0xFFFF0000, LENGTH = 0x40
	gbabios : ORIGIN = 0x4000, LENGTH = 0x4000
	kernel  : ORIGIN = 0x8000, LENGTH = 0x430000-0x8000
}
*/

SECTIONS
{
	.text 0x00008000: {
		__kernel_text_begin = .;
		obj/handlers.o(.text)
		*(EXCLUDE_FILE(obj/vector.o obj/bios.o obj/rom.o) .text .text.*)
	
		. = ALIGN(0x1000);
		__gbabios_begin = .;
		obj/bios.o(.text)
		__gbabios_end = .;

		. = ALIGN(0x1000);
		__gbarom_begin = .;
		obj/rom.o(.text)
		__gbarom_end = .;
	
		. = ALIGN(0x1000);	
		__vectortable_begin = .;
		obj/vector.o(.text)
		__vectortable_end = .;
		
		__kernel_text_end = .;
		. = ALIGN(0x1000);
		__kernel_data_begin = .;
	}

	.rodata __kernel_data_begin : {
		*(.rodata .rodata.*)
	}

	.data : {
		*(.data)
	}

	.bss : {
		*(.bss)
	}

	__kernel_data_end = .;
	__heap_begin = __kernel_data_end;
}

