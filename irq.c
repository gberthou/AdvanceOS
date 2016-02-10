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
const uint32_t RESUME_ARM = 0xe12fff1e; // bx lr
//const uint16_t RESUME_THUMB = 0xdf2b;     // svc 0x2b
const uint16_t RESUME_THUMB = 0x4700 | (1 << 6) | ((14-8) << 3); // bx lr

/* Performance related variables */
uint32_t irqBeginTicks;
uint32_t irqEndTicks;

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

    __asm__ volatile("cps 0x17\n"
                     "msr spsr, %0\n"
                     :: "r"(spsrDataHandler));
    
    irqEndTicks = TimerGetTicks();

    __asm__ volatile("pop {r0-r12}\n"
                     "ldr lr, =periphInstructionResumeAddress\n"
                     "ldr lr, [lr]\n"
                     "movs pc, lr");
}

void __attribute__((naked)) DataHandler(void)
{
    register uint32_t instructionAddress;
    register uint32_t faultAddress;
    
    __asm__ volatile("push {r0-r12, lr}\n");

    __asm__ volatile("push {lr}");
    uint32_t ticks = TimerGetTicks();
    __asm__ volatile("pop {lr}");

    __asm__ volatile("sub %0, lr, #8\n" 
                     "mrs %1, spsr\n" // Get CPSR value at the moment when the interrupt was triggered (SPSR_abt)
                     : "=r"(instructionAddress),
                       "=r"(spsrDataHandler));

    // Read Fault Address Register
    __asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(faultAddress));

    if((faultAddress >= PERIPHERALS_BEGIN && faultAddress <= PERIPHERALS_END) || faultAddress == 0x3FFFFF8)
    {
        if(faultAddress == GBA_IF_ADDRESS || faultAddress == 0x3FFFFF8)
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

                
                if(faultAddress == GBA_IF_ADDRESS)
                    GBAClearInterruptFlags(registerValue);
                else
                    *(uint16_t*)0x3007FF8 &= ~registerValue;


                __asm__ volatile("pop {r0-r12, lr}\n"
                                 "subs pc, lr, #4"); // 8-4
            }
        }
        else
        {
            // Monitoring is not relevant in the other situations as irq
            // handling goes fast.
            // Here irq handling is heavier and thus needs to be optimized
            irqBeginTicks = ticks;

            // Disable IRQs of the faulting mode
            __asm__ volatile("msr spsr, %0" :: "r"(spsrDataHandler | 0xc0));
            
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
            __asm__ volatile("mov lr, %0\n" :: "r"(onPeripheralWritten));
            __asm__ volatile("cps 0x17\n"
                             "pop {r0-r12, lr}\n"
                             "subs pc, lr, #8");
        }
    }
    else
    {
        ConsolePrint(31, 1, "DataHandler: ");
        ConsolePrintHex(44, 1, faultAddress);

        ConsolePrint(31, 2, "@ ");
        ConsolePrintHex(33, 2, instructionAddress);
        FBCopyDoubleBuffer();

        for(;;);
    }

    __asm__ volatile("pop {r0-r12, lr}\n"
                     "subs pc, lr, #8");
}


