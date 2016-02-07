#include "lcd.h"
#include "../console.h"
#include "../gba.h"
#include "peripherals.h"

#define GBA_WIDTH 240
#define GBA_HEIGHT 160

#define GBA_VRAM_BEGIN  0x06000000
#define GBA_BG_PALETTE  0x05000000
#define GBA_OBJ_PALETTE 0x05000200

#define GBA_IE (*PERIPH16(0x200))
#define GBA_IME (*PERIPH32(0x208))

#define LCD_VCOUNT (*PERIPH16(6))

void LCDRefresh(void)
{
	ConsolePrint(31, 6, "mode:");
	ConsolePrintHex(37, 6, *PERIPH32(0));

	ConsolePrint(31, 7, "BG0CNT:");
	ConsolePrintHex(39, 7, *PERIPH16(8));

	LCDUpdateScreen();

	if(GBA_IME & 1)
	{
		if(GBA_IE & 1) // VBLANK
		{
			GBASetInterruptFlags(1);
			GBACallIRQ();
		}
	}
}

void LCDOnTick(void)
{
	register uint16_t vcount = LCD_VCOUNT;
	// Increment VCOUNT
	vcount = (vcount + 1) & 0xFF;
	
	if(vcount == 227)
	{
		LCD_VCOUNT = vcount;
		LCDRefresh();
	}
	else if(vcount > 227)
		LCD_VCOUNT = 0;
	else
		LCD_VCOUNT = vcount;
}

static uint32_t palette2screen(uint16_t paletteColor)
{
	// There is an additionnal left shift of 3 in order to
	// scale the input color in range 0-31 to 0-248
	return 0x00000000
		 | ((paletteColor & 0x1F) << 3)         // R
		 | (((paletteColor >> 5) & 0x1F) << 11)  // G
		 | (((paletteColor >> 10) & 0x1F) << 19); // B
;
}

static void clipAndPutPixel(uint32_t x, uint32_t y, uint32_t color)
{
	if(x < GBA_WIDTH && y < GBA_HEIGHT)
		FBPutColor(x, y, color);
}

static void fillWithBackdropColor(void)
{
	uint32_t color = palette2screen(*(volatile uint16_t*)GBA_BG_PALETTE);
	uint32_t x;
	uint32_t y;
	for(y = 0; y < GBA_HEIGHT; ++y)
		for(x = 0; x < GBA_WIDTH; ++x)
			FBPutColor(x, y, color);
}

static void renderBg(unsigned int mode, unsigned int bg)
{
	if(mode == 0 || mode == 1)
	{
        uint16_t bgcnt = *PERIPH16(8 + bg * 2);
		uint32_t offsetTileData = 0x4000 * ((bgcnt >> 2) & 0x3);
		uint32_t offsetMapData = 0x800 * ((bgcnt >> 8) & 0x1F);
		volatile uint16_t *mapData = (volatile uint16_t*) (GBA_VRAM_BEGIN + offsetMapData);
		uint32_t currentTile;
		uint16_t bgScrollX = (*PERIPH16(0x10 + bg * 4)) & 0x1FF;
		uint16_t bgScrollY = (*PERIPH16(0x12 + bg * 4)) & 0x1FF;

		//*PERIPH16(0x10+4*bg) += 4;

		ConsolePrintHex(31, 10, bgScrollX);

		for(currentTile = 0; currentTile < 32 * 32; ++currentTile)
		{
			volatile uint8_t *tileData = (volatile uint8_t*) (GBA_VRAM_BEGIN + offsetTileData + ((*mapData) & 0x3FF) * 32);
			volatile uint16_t *palette = (volatile uint16_t*) (GBA_BG_PALETTE + ((*mapData) >> 12) * 32);
			
			uint32_t x;
			uint32_t y;

			for(y = 0; y < 8; ++y)
			{
				for(x = 0; x < 8; ++x)
				{
					// TODO: Manage flip bits
					uint8_t colorIndex = *tileData;
					uint16_t color;

					// Structure of the data:
					// RRRRLLLL
					// R = pixel on the right
					// L = pixel on the left

					if(x & 1) // Pixel on the right
					{
						colorIndex >>=4;
						++tileData; // Increase tile pointer only when the
									// two corresponding pixels have been
									// drawn (<=> when x is odd)
					}
					else // Pixel on the left
						colorIndex &= 0xF;
					color = palette[colorIndex];

					if(color) // Color == 0 -> always transparent
						clipAndPutPixel(x - bgScrollX + (currentTile & 0x1F) * 8,
										y - bgScrollY + (currentTile >> 5) * 8,
										palette2screen(color));
				}
			}
			++mapData;
		}
	}
}

void LCDUpdateScreen(void)
{
	unsigned int bg;
	uint16_t dispcnt = *PERIPH16(0);
	unsigned int mode = dispcnt & 0x7;

	fillWithBackdropColor();
	for(bg = 0; bg < 4; ++bg)
	{
		if(dispcnt & (1 << (bg + 8))) // bg enabled
		{
			renderBg(mode, bg);
		}
	}
}

