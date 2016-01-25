#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <sys/types.h>

// Format: ARGB?

struct FBRequest
{
	uint32_t width;
	uint32_t height;
	uint32_t virtualWidth;
	uint32_t virtualHeight;
	uint32_t pitch;
	uint32_t depth;
	uint32_t xOffset;
	uint32_t yOffset;
	uint32_t pointer;
	uint32_t size;
};

struct FBInfo
{
	uint32_t *ptr;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
};

extern struct FBInfo fbInfo;

struct FBInfo *FBInit(uint32_t width, uint32_t height);
void FBPutColor(uint32_t x, uint32_t y, uint32_t color);

// To be called BEFORE calling MMUInit
void FBConvertBufferToVirtualSpace(void);

#endif

