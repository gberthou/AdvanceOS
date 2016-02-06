#include <sys/types.h>

#include "irq.h"
#include "console.h"
#include "linker.h"
#include "timer.h"
#include "gba.h"
#include "peripherals/peripherals.h"

#define PERIPHERALS_BEGIN 0x04000000
#define PERIPHERALS_END   0x04000803
#define GBA_IF_ADDRESS    0x04000202

#define RESTORE_FAULTING_MODE() __asm__ volatile("stmdb sp, {r0, r1, r2}\n"\
												 "sub r2, sp, #12\n"\
												 "mrs r0, cpsr\n"\
												 "ldr r1, =spsrDataHandler\n"\
												 "ldr r1, [r1]\n"\
												 "bic r0, r0, #0x1F\n"\
												 "and r1, r1, #0x1F\n"\
												 "orr r0, r0, r1\n"\
												 "msr cpsr, r0\n"\
												 "ldmia r2, {r0, r1, r2}"\
												 )

static uint32_t spsrDataHandler;
static uint32_t lrToRestore;

//const uint32_t RESUME_ARM   = 0xef00002b; // svc 0x2b
//const uint32_t RESUME_ARM = 0xe92d8000; // pop {pc}
const uint32_t RESUME_ARM = 0xe12fff1e; // bx lr
//const uint16_t RESUME_THUMB = 0xdf2b;     // svc 0x2b
//const uint16_t RESUME_THUMB = 0xbd00;   // pop {pc}
const uint16_t RESUME_THUMB = 0x4700 | (1 << 6) | ((14-8) << 3); // bx lr

static void __attribute__((naked)) onPeripheralWritten(void)
{
	// IRQs are already disabled at this point because of DataHandler
	// (this function should not be called by any other mechanism)
	
	__asm__ volatile("cps 0x17\n"
					 "push {r0-r12}");
	PeripheralsRefresh();
	PeripheralsResume();

	RESTORE_FAULTING_MODE();
	__asm__ volatile("ldr lr, %0"
					 :: "m"(lrToRestore));
	
	/*
	__asm__ volatile("push {%0}\n"
					 "push {%1}"
					 :: "r"(spsrDataHandler),
						"r"((uint32_t) periphInstructionResumeAddress));
	__asm__ volatile("cps 0x17\n"
					 "pop {r0-r12, lr}");
	RESTORE_FAULTING_MODE();
	__asm__ volatile("rfeia sp!");
	*/

	__asm__ volatile("cps 0x17\n"
					 "msr spsr, %0\n"
					 "pop {r0-r12}\n"
					 "ldr lr, =periphInstructionResumeAddress\n"
					 "ldr lr, [lr]\n"
					 "movs pc, lr"
					 :: "r"(spsrDataHandler)		 
			);
}

/*
static void __attribute__((naked)) shiftStack(register uint32_t oldStack)
{
	__asm__ volatile(
					 "push {r1, r2}\n"
					 "sub r1, sp, #4\n"
	 "shiftStackLoop: ldr r2, [r1, #4]\n"
	 				 "str r2, [r1]\n"
					 "add r1, r1, #4\n"
					 "cmp %0, r1\n"
					 "bge shiftStackLoop\n"
					 "sub sp, sp, #4\n"
					 "pop {r1, r2}\n"
					 "bx lr" :: "r"(oldStack));
}
*/

void __attribute__((naked)) DataHandler(void)
{
	register uint32_t instructionAddress;
	register uint32_t faultAddress;
	
	__asm__ volatile(
					 "push {r0-r12, lr}\n"
					 "sub %0, lr, #8\n" 
					 "mrs %1, spsr\n" // Get CPSR value at the moment when the interrupt was triggered (SPSR_abt)
					 : "=r"(instructionAddress),
					   "=r"(spsrDataHandler));

	// Disable IRQs of the faulting mode
	__asm__ volatile("msr spsr, %0" :: "r"(spsrDataHandler | 0xc0));

	// Read Fault Address Register
	__asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(faultAddress));

	if(faultAddress >= PERIPHERALS_BEGIN && faultAddress <= PERIPHERALS_END)
	{
		if(faultAddress == GBA_IF_ADDRESS)
		{
			if(spsrDataHandler & 0x20) // Thumb mode was enabled
			{
				// TODO
				for(;;);	
				__asm__ volatile("pop {r0-r12, lr}\n"
								 "subs pc, lr, #6"); // 8-2
			}
			else
			{
				uint32_t instructionCode = *(uint32_t*)instructionAddress;
				
				// /!\ Warning: Assumes that the faulting instruction was STR, STRH or STRD
				uint32_t sourceRegister = (instructionCode >> 12) & 0xf;
				uint32_t registerValue;

				// /!\ Warning: Assumes that the register is in the range r0-r12
				__asm__ volatile("ldr %0, [sp, %1]" // Loads the stored version of the register
								 :"=r"(registerValue)
								 :"r"(sourceRegister << 2));
	
				// Clear the corresponding IF bits	
				GBAClearInterruptFlags(registerValue);

				__asm__ volatile("pop {r0-r12, lr}\n"
								 "subs pc, lr, #4"); // 8-4
			}
		}
		else
		{
			// Check thumb bit to compute the instruction following the one which
			// triggered the interrupt

			if(spsrDataHandler & 0x20) // Thumb mode was enabled
			{
				periphInstructionResumeAddress = (void*)(instructionAddress + 2);
				periphInstructionContent = *(uint16_t*)periphInstructionResumeAddress;
				*(uint16_t*) periphInstructionResumeAddress = RESUME_THUMB;
				periphThumb = 1;
			}
			else
			{
				periphInstructionResumeAddress = (void*)(instructionAddress + 4);
				periphInstructionContent = *(uint32_t*)periphInstructionResumeAddress;
				*(uint32_t*)periphInstructionResumeAddress = RESUME_ARM;
				periphThumb = 0;
			}

			lastPeripheralAddress = faultAddress & 0xFF00FFFF;
			PeripheralsSetAccess(READWRITE);

			RESTORE_FAULTING_MODE();
			__asm__ volatile("str lr, %0" :: "m"(lrToRestore));
			__asm__ volatile(
							 "mov lr, %0\n"
							 "cps 0x17\n"
							 "pop {r0-r12, lr}\n"
							 "subs pc, lr, #8"
							 :: "r"(onPeripheralWritten));
		}
	}
	else
	{
		ConsolePrint(31, 1, "DataHandler: ");
		ConsolePrintHex(44, 1, faultAddress);

		ConsolePrint(31, 2, "@ ");
		ConsolePrintHex(33, 2, instructionAddress);

		for(;;);
	}

	__asm__ volatile("pop {r0-r12, lr}\n"
					 "subs pc, lr, #8");
}

#ifdef OOPS
static uint32_t swiArg;
static void __attribute__((naked)) JumpToBiosSVC(void)
{
#ifdef AAAAAB
	/* This instruction is part of the bios,
	 * hence the pop should not be part of
	 * this function*/
	__asm__ volatile(
					 "ldr lr, =afterJump\n"

		/* --> */	 "push {fp, ip, lr}\n"
					 "push {r0-r4}\n");

	__asm__ volatile("ldr ip, [%0]\n"
					 "pop {r0-r4}\n"
					 "b 0x148\n"
					 "afterJump: bx lr"
					 ::"r"(&swiArg));
#else
	// The following code is mostly inspired of the real GBA bios svc routine
	__asm__ volatile("push {fp, ip, lr}\n"
					 "push {r0-r7}\n");
	__asm__ volatile("ldr r1, [%0]\n"
					 "mov ip, #0x1c8\n" // Address of the GBA svc vector
					 "ldr ip, [ip, r1, lsl #2]\n"
					 "pop {r0-r7}\n"
					 "push {r2, lr}\n"
					 "mov lr, pc\n"
					 "bx ip\n"
					 "pop {r2, lr}\n"
					 "pop {fp, ip, lr}\n"
					 "bx lr"
					 :: "r"(&swiArg));
#endif
}

void __attribute__((naked)) SwiHandler(void)
{
	register uint32_t lr;
	register uint32_t spsr;

	__asm__ volatile("push {r0-r12, lr}\n"
					 "mov %0, lr\n"
					 "mrs %1, spsr\n"
					 : "=r"(lr), "=r"(spsr));

	if(spsr & 0x20) // Thumb mode was enabled
	{
		uint16_t *ptr = (uint16_t*) (lr - 2);
		swiArg = *ptr & 0xFF;
		if(swiArg < 0x2B)
		{
			__asm__ volatile("pop {r0-r12, lr}\n"
							 "push {lr}");
			JumpToBiosSVC();
			__asm__ volatile("pop {lr}");
		}
		else
		{
			PeripheralsRefresh();
			PeripheralsResume();
			__asm__ volatile("pop {r0-r12, lr}");
			__asm__ volatile("sub lr, lr, #2"); // lr -> @ of instruction that triggered interrupt
		}

		__asm__ volatile("orr lr, lr, #1"); // re-enable thumb mode on bx
	}
	else
	{
		uint32_t *ptr = (uint32_t*) (lr - 4);
		swiArg = *ptr & 0xFF;
		if(swiArg < 0x2B)
		{
			__asm__ volatile("pop {r0-r12, lr}\n"
							 "push {lr}");
			JumpToBiosSVC();
			__asm__ volatile("pop {lr}");
		}
		else
		{
			PeripheralsRefresh();
			PeripheralsResume();
			__asm__ volatile("pop {r0-r12, lr}");
			__asm__ volatile("sub lr, lr, #4"); // lr -> @ of the instruction that triggered interrupt
		}

		__asm__ volatile("bic lr, lr, #1");
	}

	__asm__ volatile("srsdb sp!, #0x13\n"
					 "rfeia sp!"
					);
}

#else

void __attribute__((naked)) SwiHandler(void)
{
	__asm__ volatile("b 0x8");
}

#endif

/*
void __attribute__((naked)) PrefetchHandler(void)
{
	uint32_t lr;
	__asm__ volatile("push {r0-r12}");
	__asm__ volatile("mov %0, lr": "=r"(lr));

	if(lr == 0x80000004)
	{
		PeripheralsRefresh();
		PeripheralsResume();

		__asm__ volatile("mov lr, %0\n"
						 :: "r"(periphInstructionResumeAddress));

		if(spsr & 0x20) // Thumb mode was enabled
		{
			__asm__ volatile("orr lr, lr, #1");
		}
		else
		{
			__asm__ volatile("bic lr, lr, #1");
		}

		__asm__ volatile("msr spsr, %0\n"
						 "pop {r0-r12}\n"
						 "srsdb sp!, #0x17\n"
						 "rfeia sp!"
						 :: "r"(spsr)
						 );

	}
	else
	{
		// /!\ If you're planning to return from exception from this point,
		// don't forget to pop {r0-r12} from sp_0x17
		//
		for(;;);
	}
}
*/

