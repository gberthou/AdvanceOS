#include <stdint.h>

#include "irq.h"
#include "console.h"
#include "linker.h"
#include "timer.h"
#include "gba.h"
#include "errlog.h"
#include "peripherals/peripherals.h"

// Loads the stored version of the register
#define SAVED_REGISTER_VALUE(stack, reg) (stack[(reg)]) 
#define PERIPHERAL_OFFSET(x) ((x) - PERIPHERALS_BEGIN)

#define PERIPHERALS_BEGIN 0x04000000
#define PERIPHERALS_END   0x04000803
#define GBA_IF_ADDRESS    0x04000202

/* Performance related variables */
uint32_t irqBeginTicks;
uint32_t irqEndTicks;

inline void checkRegister(uint32_t sourceRegister)
{
    if(sourceRegister > 12)
        ErrorDisplayMessage("DataHandler: Unexpected register", 1);
}

static void executeARMStoreInstruction(uint32_t instructionAddress,
                                         uint32_t targetAddress,
                                         uint32_t targetOffset,
                                         uint32_t *registerStack)
{
    uint32_t instructionCode = *(uint32_t*)instructionAddress;
    uint32_t registerValue;

    targetAddress += targetOffset; 

    // Same assumptions as before
    // Additional assumption: source register belongs to r0-r12
    // (this assumption was true in thumb mode as registers belong
    // to r0-r7, but in ARM mode it might not be the case)
    if((instructionCode & 0x0C000000) == 0x04000000) // STR
    {
        uint32_t reg = (instructionCode >> 12) & 0xF;
        checkRegister(reg);

        // Check for write-back
        if((instructionCode & (1 << 24)) && (instructionCode & (1 << 21)))
            ErrorDisplayMessage("DataHandler: unexpected write-back", 1);

        registerValue = SAVED_REGISTER_VALUE(registerStack, reg);
        *(uint32_t*)targetAddress = registerValue;
    }
    else if((instructionCode & 0x0E000000) == 0x00000000) // STRH, STRD
    {
        uint32_t reg = (instructionCode >> 12) & 0xF;
        checkRegister(reg);
        
        // Check for write-back
        if((instructionCode & (1 << 24)) && (instructionCode & (1 << 21)))
            ErrorDisplayMessage("DataHandler: unexpected write-back", 1);

        registerValue = SAVED_REGISTER_VALUE(registerStack, reg);
        if(instructionCode & (1 << 6)) // STRD
        {
            volatile uint32_t *ptr = (uint32_t*)targetAddress;
            
            ptr[0] = registerValue;
            
            registerValue = SAVED_REGISTER_VALUE(registerStack, reg + 1);
            ptr[1] = registerValue;
        }
        else // STRH
        {
            *(uint16_t*)targetAddress = registerValue;
        }
    }
    else if((instructionCode & 0x0E000000) == 0x08000000) // STM
    {
        // TODO
        for(;;);
    }
}

static void executeThumbStoreInstruction(uint32_t instructionAddress,
                                         uint32_t targetAddress,
                                         uint32_t targetOffset,
                                         uint32_t *registerStack)
{
    uint16_t instructionCode = *(uint16_t*)instructionAddress;
    uint16_t instructionPrefix = instructionCode & 0xF000;
    uint32_t registerValue;

    targetAddress += targetOffset;

    // Simple instruction decoder
    // The following piece of code makes the assumption that the peripherals
    // section is read-only protected, so any LDR instruction would not
    // trigger this interrupt as the whole peripheral section is mapped.
    // Hence all LDR instructions are ignored which makes the code light
    // and fast to execute
    switch(instructionPrefix)
    {
        case 0x5000: // STR, STRB, STRH reg. offset
            registerValue = SAVED_REGISTER_VALUE(registerStack, instructionCode & 0x7);
            if(instructionCode & (1 << 9)) // STRH
            {
                //*PERIPH16(PERIPHERAL_OFFSET(faultAddress)) = registerValue;
                *(uint16_t*)targetAddress = registerValue;
            }
            else if(instructionCode & (1 << 10)) // STRB
            {
                //*PERIPH8(PERIPHERAL_OFFSET(faultAddress)) = registerValue;
                *(uint8_t*)targetAddress = registerValue;
            }
            else // STR
            {
                //*PERIPH32(PERIPHERAL_OFFSET(faultAddress)) = registerValue;
                *(uint32_t*)targetAddress = registerValue;
            }
            break;

        case 0x6000: // STR  imm. offset
        case 0x7000: // STRB imm. offset
            registerValue = SAVED_REGISTER_VALUE(registerStack, instructionCode & 0x7);
            if(instructionCode & (1 << 12)) // STRB
            {
                //*PERIPH8(PERIPHERAL_OFFSET(faultAddress)) = registerValue;
                *(uint8_t*)targetAddress = registerValue;
            }
            else // STR
            {
                //*PERIPH32(PERIPHERAL_OFFSET(faultAddress)) = registerValue;
                *(uint32_t*)targetAddress = registerValue;
            }
            break;

        case 0x8000: // STRH imm. offset
            registerValue = SAVED_REGISTER_VALUE(registerStack, instructionCode & 0x7);
            //*PERIPH16(PERIPHERAL_OFFSET(faultAddress)) = registerValue;
            *(uint16_t*)targetAddress = registerValue;
            
            break;

        case 0xA000: // STMxx
            // TODO

        default:
            for(;;);
            break;
    }
}

void __attribute__((naked)) DataHandler(void)
{
    uint32_t instructionAddress;
    uint32_t faultAddress;
    uint32_t spsr;
    uint32_t ticks;
    uint32_t *registerStack;

    __asm__ volatile("push {r0-r12, lr}\n");
    __asm__ volatile("mov %0, sp" : "=r"(registerStack));
    __asm__ volatile("sub %0, lr, #8"
                     : "=r"(instructionAddress));
    
    // Monitoring purpose only
    ticks = TimerGetTicks();

    __asm__ volatile("mrs %0, spsr" // Get CPSR value at the moment when the interrupt was triggered (SPSR_abt)
                    : "=r"(spsr));

    if((spsr & 0x1f) == 0x12)
    {
        __asm__ volatile("nop"); // Breakpoint;
    }

    // Read Fault Address Register
    __asm__ volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(faultAddress));

    /*|| (faultAddress >= 0x3007000 && faultAddress < 0x3008000))*/
    if(faultAddress == GBA_IF_ADDRESS || faultAddress == 0x3FFFFF8)
    {
        if(spsr & 0x20) // Thumb mode was enabled
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

            checkRegister(sourceRegister);

            // /!\ Warning: Assumes that the register is in the range r0-r12
            registerValue = SAVED_REGISTER_VALUE(registerStack, sourceRegister);
            
            if(faultAddress == GBA_IF_ADDRESS)
            {
                GBAClearInterruptFlags(registerValue);
                //*PERIPH32(GBA_IF_ADDRESS - PERIPHERALS_BEGIN) &= ~registerValue; 
            }
            else
                GBAClearIF(registerValue);

            __asm__ volatile("pop {r0-r12, lr}\n"
                             "subs pc, lr, #4"); // 8-4
        }
    }
    else if(faultAddress >= PERIPHERALS_BEGIN && faultAddress <= PERIPHERALS_END)
    {
        // Monitoring is not relevant in the other situations as irq
        // handling goes fast.
        // Here irq handling is heavier and thus needs to be optimized
        irqBeginTicks = ticks;

        // Check thumb bit to compute the instruction following the one which
        // triggered the interrupt

        if(spsr & 0x20) // Thumb mode was enabled
        {
            executeThumbStoreInstruction(instructionAddress, faultAddress,
                                        PERIPHERAL_OFFSET((uint32_t)periphdata),
                                         registerStack);

            PeripheralsRefresh(faultAddress);
            
            // This code leaks support for store SP-relative (Thumb.11)
            // and push/pop instructions (Thumb.14)
            // SP register value is not supposed to be in peripheral range 
            
            irqEndTicks = TimerGetTicks();
            __asm__ volatile("pop {r0-r12, lr}");
            __asm__ volatile("add lr, lr, #2"); // Next instruction address
            __asm__ volatile("subs pc, lr, #8");
        }
        else
        {
            executeARMStoreInstruction(instructionAddress,
                                       faultAddress,
                                       PERIPHERAL_OFFSET((uint32_t)periphdata),
                                       registerStack);

            PeripheralsRefresh(faultAddress);
            
            irqEndTicks = TimerGetTicks();
            __asm__ volatile("pop {r0-r12, lr}");
            __asm__ volatile("add lr, lr, #4"); // Next instruction address
            __asm__ volatile("subs pc, lr, #8");
        }
    }
#if 0
    else if(faultAddress >= 0x3007000 && faultAddress < 0x3008000)
    {
        if(spsr & 0x20) // Thumb mode was enabled
        {
            executeThumbStoreInstruction(instructionAddress, faultAddress, 0,
                                         registerStack);
        }
        else
        {
            executeARMStoreInstruction(instructionAddress, faultAddress, 0,
                                       registerStack);
        }
    }
#endif
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

