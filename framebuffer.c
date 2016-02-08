#include "framebuffer.h"
#include "mailbox.h"
#include "mmu.h"
#include "linker.h"
#include "mem.h"

#define VIDEOBUS_OFFSET 0x40000000

static struct FBInfo doubleFb;
static struct FBInfo physicalFb;

/* Please see
 * https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface
 */
struct FBInfo *FBInit(uint32_t width, uint32_t height)
{
	uint32_t __attribute__((aligned(16))) sequence[] = {
		0, // size of the whole structure
		0, // request
		
		// TAG 0
		0x48003, // Set width/height
		8, // size
		8, // request
		width,
		height,

		// TAG 1
		0x48004, // Set virtual width/height (??)
		8, // size
		8, // request
		width,
		height,

		// TAG 2
		0x48005, // Set depth
		4, // size
		4, // request
		32, // depth

		// TAG 3
		0x40008, // Get pitch
		4, // size
		4, //request
		0,

		// TAG 4
		0x40001, // Allocate framebuffer (cross your fingers)
		8, // size
		8, // request
		16, // framebuffer alignment
		0,

		0 // End TAG
	};
	sequence[0] = sizeof(sequence);
	
	// Send the requested values
	MailboxSend(8, ((uint32_t)sequence) + VIDEOBUS_OFFSET);
	
	// Now get the real values
	if(MailboxReceive(8) == 0 || sequence[1] == 0x80000000) // Ok
	{
		const uint32_t *ptr = sequence + 2;
		while(*ptr)
		{
			switch(*ptr++)
			{
				case 0x40008:
					physicalFb.pitch = ptr[2];
					break;

				case 0x48003:
					physicalFb.width = ptr[2];
					physicalFb.height = ptr[3];
					break;

				case 0x40001:
					physicalFb.ptr = (uint32_t*) ptr[2];
					break;

				case 0x48004:
				case 0x48005:
					break;

				default: // Panic
					return 0;
			}
			ptr += *ptr / 4 + 2;
		}
		return &physicalFb;
	}
	return 0;
}

void FBPutColor(uint32_t x, uint32_t y, uint32_t color)
{
	doubleFb.ptr[x + y * (doubleFb.pitch >> 2)] = color;
}

void FBConvertBufferToVirtualSpace(void)
{
    size_t bufferSize = physicalFb.height * physicalFb.pitch;
	uint32_t *newPtr = Memalloc(bufferSize, 0x1000);

	MMUPopulateRange((uint32_t) newPtr, (uint32_t) physicalFb.ptr, physicalFb.height * physicalFb.pitch, READWRITE);
	physicalFb.ptr = newPtr;
}

struct FBInfo *FBCreateDoubleBuffer(void)
{
    size_t bufferSize = physicalFb.height * physicalFb.pitch;
    uint32_t *doubleBuffer = Memalloc(bufferSize, 0x1000);

	MMUPopulateRange((uint32_t) doubleBuffer, (uint32_t) doubleBuffer, physicalFb.height * physicalFb.pitch, READWRITE);
    
    doubleFb.ptr    = doubleBuffer;
    doubleFb.width  = physicalFb.width;
    doubleFb.height = physicalFb.height;
    doubleFb.pitch  = physicalFb.pitch;
    return &doubleFb;
}

void FBCopyDoubleBuffer(void)
{
    // TODO: Use DMA
    size_t bufferSize = (physicalFb.height * physicalFb.pitch) >> 2; // Unit: 32bit words
    size_t i = bufferSize;
    while(i--)
    {
        physicalFb.ptr[i] = doubleFb.ptr[i];
    }
}

