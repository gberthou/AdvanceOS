#ifndef LINKER_H
#define LINKER_H

#include <sys/types.h>

#define STACK_BEGIN ((uint32_t)__stack_begin)

#define KERNEL_TEXT_BEGIN ((uint32_t)&__kernel_text_begin)
#define KERNEL_TEXT_END ((uint32_t)&__kernel_text_end)

#define KERNEL_DATA_BEGIN ((uint32_t)&__kernel_data_begin)
#define KERNEL_DATA_END ((uint32_t)&__kernel_data_end)

#define GBABIOS_BEGIN ((uint32_t)&__gbabios_begin)
#define GBABIOS_END ((uint32_t)&__gbabios_end)

#define GBAROM_BEGIN ((uint32_t)&__gbarom_begin)
#define GBAROM_END  ((uint32_t)&__gbarom_end)

#define VECTORTABLE_BEGIN ((uint32_t)&__vectortable_begin)
#define VECTORTABLE_END ((uint32_t)&__vectortable_end)

//#define FRAMEBUFFER_BEGIN ((uint32_t)&__framebuffer_begin)

#define GBA_HEAP_BEGIN ((KERNEL_DATA_END+0xFFF) & 0xFFFFF000)

#define KERNEL_TEXT_SIZE (KERNEL_TEXT_END - KERNEL_TEXT_BEGIN)
#define KERNEL_DATA_SIZE (KERNEL_DATA_END - KERNEL_DATA_BEGIN)
#define GBABIOS_SIZE     (GBABIOS_END - GBABIOS_BEGIN)
#define GBAROM_SIZE      (GBAROM_END - GBABIOS_END)
#define VECTORTABLE_SIZE (VECTORTABLE_END - VECTORTABLE_BEGIN)

extern uint32_t __stack_begin;

extern uint32_t __kernel_text_begin;
extern uint32_t __kernel_text_end;

extern uint32_t __kernel_data_begin;
extern uint32_t __kernel_data_end;

extern uint32_t __gbabios_begin;
extern uint32_t __gbabios_end;

extern uint32_t __gbarom_begin;
extern uint32_t __gbarom_end;

extern uint32_t __vectortable_begin;
extern uint32_t __vectortable_end;

//extern uint32_t __framebuffer_begin;

#endif

