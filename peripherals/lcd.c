#include "lcd.h"
#include "../console.h"
#include "../gba.h"
#include "../timer.h"
#include "../irq.h"
#include "../gbaConstants.h"
#include "../errlog.h"
#include "peripherals.h"

#define GBA_BG_PALETTE  0x05000000
#define GBA_OBJ_PALETTE 0x05000200
#define GBA_VRAM_BEGIN  0x06000000
#define GBA_TILES_OBJ_BEGIN  (GBA_VRAM_BEGIN + 0x10000)
#define GBA_OBJS_BEGIN  0x07000000

#define GBA_IE (*PERIPH16(0x200))
#define GBA_IME (*PERIPH32(0x208))

#define LCD_VCOUNT (*PERIPH16(6))

#define GBA_LCD_MODE5_WIDTH  160
#define GBA_LCD_MODE5_HEIGHT 128

#define GBA_OBJ_COUNT 128


struct ObjAttributes
{
    uint16_t attr0;
    uint16_t attr1;
    uint16_t attr2;
    uint16_t padding;
};

static uint32_t lcdclock;


void LCDInitClock(uint32_t clock)
{
    lcdclock = clock;
}

void LCDRefresh(void)
{
    register uint32_t spsr;
    ConsolePrint(31, 6, "mode:");
    ConsolePrintHex(37, 6, *PERIPH32(0));

    ConsolePrint(31, 7, "BG0CNT:");
    ConsolePrintHex(39, 7, *PERIPH16(8));

    ConsolePrintHex(31, 16, irqEndTicks - irqBeginTicks);

    LCDUpdateScreen();
    FBCopyDoubleBuffer();

    __asm__ volatile("mrs %0, spsr" : "=r"(spsr));

    if(!(spsr & (1 << 7)) && (GBA_IME & GBA_IE & 1)) // VBLANK
    {
        GBASetInterruptFlags(1);
        GBACallIRQ();
    }
}

void LCDOnTick(uint32_t clock)
{
    uint16_t previousVcount = LCD_VCOUNT;
    uint16_t vcount;
    uint32_t ellapsedTime = clock - lcdclock;
    uint32_t ncycles = ellapsedTime / CLOCK_LCD;

    vcount = (ncycles % 57) << 2; // (1) Here vcount is a multiple of 4
                                  // It enables the LCD clock to be 4 times
                                  // slower
    
    // Hack: some binaries test (VCOUNT == 0x9f) which is not possible
    // due to the fact our vcount is multiple of 4, cf. (1)
    if(vcount == 0x9c)
        vcount = 0x9f;
    
    LCD_VCOUNT = vcount;

    if(vcount == 0
    || vcount < previousVcount) // Overflow: value has been reached between
                                // two calls
    {
        LCDRefresh();
        lcdclock = clock;
    }
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
    if(x < GBA_LCD_WIDTH && y < GBA_LCD_HEIGHT)
        FBPutColor(x, y, color);
}

static void fillWithBackdropColor(void)
{
    uint32_t color = palette2screen(*(uint16_t*)GBA_BG_PALETTE);
    uint32_t x;
    uint32_t y;
    for(y = 0; y < GBA_LCD_HEIGHT; ++y)
        for(x = 0; x < GBA_LCD_WIDTH; ++x)
            FBPutColor(x, y, color);
}

static uint8_t getColorIndex4(const uint8_t *tileData, uint8_t x, uint8_t y)
{
    // Structure of the data:
    // RRRRLLLL
    // R = pixel on the right
    // L = pixel on the left
    
    uint8_t tmp = tileData[(x >> 1) + (y << 2)];
    if(x & 1) // Pixel on the right
        return (tmp >> 4);
    return (tmp & 0xF);
}

static uint8_t getColorIndex8(const uint8_t *tileData, uint8_t x, uint8_t y)
{
    return tileData[x + (y << 3)];
}

uint8_t (*const GET_COLOR_INDEX_FUNCTIONS[2])(const uint8_t*, uint8_t, uint8_t) = {
    getColorIndex4,
    getColorIndex8
};

static void renderBg(uint16_t dispcnt, uint8_t mode, unsigned int bg)
{
    if(mode == 0 || mode == 1)
    {
        // TODO: Manage flip bits
        // TODO 8bit depth support
        
        uint16_t bgcnt = *PERIPH16(8 + bg * 2);
        uint32_t offsetTileData = 0x4000 * ((bgcnt >> 2) & 0x3);
        uint32_t offsetMapData = 0x800 * ((bgcnt >> 8) & 0x1F);
        uint16_t *mapData = (uint16_t*) (GBA_VRAM_BEGIN + offsetMapData);
        uint32_t currentTile;
        uint16_t bgScrollX = (*PERIPH16(0x10 + bg * 4)) & 0x1FF;
        uint16_t bgScrollY = (*PERIPH16(0x12 + bg * 4)) & 0x1FF;
        
        uint8_t (*getColorIndex)(const uint8_t*, uint8_t, uint8_t);
        uint8_t tileIncrement;

        // Loop invariant
        if(bgcnt & (1 << 7)) // 8bit depth
        {
            getColorIndex = GET_COLOR_INDEX_FUNCTIONS[1];
            tileIncrement = 64;
        }
        else // 4bit depth
        {
            getColorIndex = GET_COLOR_INDEX_FUNCTIONS[0];
            tileIncrement = 32;
        }

        for(currentTile = 0; currentTile < 32 * 32; ++currentTile)
        {
            uint8_t *tileData = (uint8_t*) (GBA_VRAM_BEGIN + offsetTileData + ((*mapData) & 0x3FF) * 32);
            uint16_t *palette = (uint16_t*) (GBA_BG_PALETTE + ((*mapData) >> 12) * 32);
            
            uint32_t x;
            uint32_t y;

            for(y = 0; y < 8; ++y)
            {
                for(x = 0; x < 8; ++x)
                {
                    uint8_t colorIndex = getColorIndex(tileData, x, y);
                    uint16_t color = palette[colorIndex];

                    if(color) // Color == 0 -> always transparent
                        clipAndPutPixel(x - bgScrollX + (currentTile & 0x1F) * 8,
                                        y - bgScrollY + (currentTile >> 5) * 8,
                                        palette2screen(color));
                }
            }
            tileData += tileIncrement;
            ++mapData;
        }
    }
    else if(mode == 3)
    {
        uint16_t *ptr = (uint16_t*)GBA_VRAM_BEGIN;
        uint32_t x;
        uint32_t y;

        for(y = 0; y < GBA_LCD_HEIGHT;  ++y)
            for(x = 0; x < GBA_LCD_WIDTH; ++x)
                FBPutColor(x, y, palette2screen(*ptr++));
    }
    else if(mode == 4)
    {
        // Uses the whole BG Palette memory as a 256-colors palette
       
        // Bit 4 of DISPCNT selects frame 
        uint8_t *ptr = (uint8_t*)((dispcnt & (1 << 4)) ?
                    GBA_VRAM_BEGIN + 0x0000A000
                    : GBA_VRAM_BEGIN);
        uint16_t *palette = (uint16_t*) GBA_BG_PALETTE;
        uint32_t x;
        uint32_t y;

        for(y = 0; y < GBA_LCD_HEIGHT; ++y)
            for(x = 0; x < GBA_LCD_WIDTH; ++x)
            {
                uint16_t color = palette[*ptr++];
                if(color)
                    FBPutColor(x, y, palette2screen(color));
            }
    }
    else if(mode == 5)
    {
        // Bit 4 of DISPCNT selects frame 
        uint16_t *ptr = (uint16_t*)((dispcnt & (1 << 4)) ?
                GBA_VRAM_BEGIN + 0x0000A000
                : GBA_VRAM_BEGIN);
        uint32_t x;
        uint32_t y;

        for(y = 0; y < GBA_LCD_MODE5_HEIGHT;  ++y)
            for(x = 0; x < GBA_LCD_MODE5_WIDTH; ++x)
                FBPutColor(x, y, palette2screen(*ptr++));
    }
}

static void renderObj(uint16_t dispcnt, uint8_t id)
{
    const struct ObjAttributes *obj = ((const struct ObjAttributes*) GBA_OBJS_BEGIN) + id;
    
    // Sprite sizes (to be combined with sprite shapes). Unit: tile
    const uint8_t sizes0[] = {2, 4, 4, 8};
    const uint8_t sizes1[] = {1, 1, 2, 4};

    unsigned int rotationScaling = obj->attr0 & (1 << 8);

    if(rotationScaling || !(obj->attr0 & (1 << 9))) // Check disable bit
    {
        // TODO: Rotation/scaling support
        // TODO: OBJ mode support
        // TODO: OBJ Mosaic support
        // TODO: Priority management

        uint16_t objX = (obj->attr1 & 0x1FF);
        uint16_t objY = (obj->attr0 & 0xFF);
        uint8_t objW; // Unit: 8pixel-wide tile
        uint8_t objH; // Same unit
        uint8_t objShape = (obj->attr0 >> 14);
        uint8_t objSize = (obj->attr1 >> 14);

        // Bit 13 of attr0 controls whether the sprite has 256 palette
        // colors (1) or 16 (0)
        uint16_t *palette = (uint16_t*)
                                     ((obj->attr0 & (1 << 13)) ? GBA_OBJ_PALETTE
                                     : GBA_OBJ_PALETTE + ((obj->attr2 >> 12) << 5));
        
        uint8_t *tileData = (uint8_t*) (GBA_TILES_OBJ_BEGIN + ((obj->attr2 & 0x3FF) << 5));
        
        uint8_t (*getColorIndex)(const uint8_t*, uint8_t, uint8_t);
        uint8_t tileIncrement;
        
        uint8_t tx;
        uint8_t ty;
        uint32_t x;
        uint32_t y;
        
        if(obj->attr0 & (1 << 13)) // 8bit depth
        {
            getColorIndex = GET_COLOR_INDEX_FUNCTIONS[1];
            tileIncrement = 64;
        }
        else // 4bit depth
        {
            getColorIndex = GET_COLOR_INDEX_FUNCTIONS[0];
            tileIncrement = 32;
        }
        
        // Sprite shape management
        switch(objShape)
        {
            case 1: // Horizontal
                objW = sizes0[objSize];
                objH = sizes1[objSize];
                break;
            case 2: // Vertical
                objW = sizes1[objSize];
                objH = sizes0[objSize];
                break;
            default: // Square or prohibited
                objW = (1 << objSize);
                objH = objW;
                break;
        }

        for(ty = 0; ty < objH; ++ty)
        {
            uint8_t *prevTileData = tileData;
            
            for(tx = 0; tx < objW; ++tx)
            {
                for(y = 0; y < 8; ++y)
                {
                    for(x = 0; x < 8; ++x)
                    {
                        // Takes flip attributes into account
                        uint8_t colorIndex = getColorIndex(tileData,
                                (obj->attr1 & (1 << 12)) ? 7-x:x,
                                (obj->attr1 & (1 << 13)) ? 7-y:y);
                        uint16_t color = palette[colorIndex];

                        if(color) // Color == 0 -> always transparent
                            clipAndPutPixel(x + objX + (tx << 3),
                                            y + objY + (ty << 3),
                                            palette2screen(color));
                    }
                }
                tileData += tileIncrement;
            }

            if(!(dispcnt & (1 << 6))) // Two dimensional character mapping
            {
                tileData = prevTileData + (tileIncrement << 5);
            }
        }
    }
}

void LCDUpdateScreen(void)
{
    unsigned int i;
    uint16_t dispcnt = *PERIPH16(0);
    uint8_t mode = dispcnt & 0x7;
    
    fillWithBackdropColor();
    
    for(i = 0; i < 4; ++i)
    {
        if(dispcnt & (1 << (i + 8))) // bg enabled
        {
            renderBg(dispcnt, mode, i);
        }
    }

    for(i = 0; i < GBA_OBJ_COUNT; ++i)
    {
        renderObj(dispcnt, i);
    }
}

