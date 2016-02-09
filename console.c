#include "console.h"
#include "linker.h"

#define GLYPH_W 8
#define GLYPH_H 8

extern uint8_t GLYPHS[];

static void displayChar(unsigned char c, uint32_t x, uint32_t y)
{
	uint32_t localX;
	uint32_t localY;
	size_t index = c * GLYPH_H;

	for(localY = 0; localY < GLYPH_H; ++localY)
	{
		for(localX = 0; localX < GLYPH_W; ++localX)
		{
			FBPutColor(x + localX, y + localY, ((GLYPHS[index + localY] >> (GLYPH_W - 1 - localX)) & 1) == 0 ? 0xFF000000 : 0xFFFFFFFF);
		}
	}
}

void ConsolePrint(uint32_t px, uint32_t py, const char *str)
{
	uint32_t x = px;
	uint32_t y = py;

	unsigned char c;
	while((c = *str++) != 0)
	{
		displayChar(c, (x++) * GLYPH_W, y * GLYPH_H);
	}
    //FBCopyDoubleBuffer();   
}

void ConsolePrintHex(uint32_t px, uint32_t py, uint32_t x)
{
	static const char figures[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
	char s[11];
	uint32_t i;
	uint32_t tmp;

	s[0] = '0';
	s[1] = 'x';
	s[10] = 0;
	for(i = 0; i < 8; ++i)
	{
		tmp = (x >> ((7-i) << 2)) & 0xF;
		s[2 + i] = figures[tmp];
	}	
	
	ConsolePrint(px, py, s);
}

