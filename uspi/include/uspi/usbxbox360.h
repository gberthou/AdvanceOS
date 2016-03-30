#ifndef _usbxbox360_h
#define _usbxbox360_h

#define XBOX360_BTN_A      0
#define XBOX360_BTN_B      1
#define XBOX360_BTN_X      2
#define XBOX360_BTN_Y      3
#define XBOX360_BTN_LB     4
#define XBOX360_BTN_RB     5
#define XBOX360_BTN_BACK   6
#define XBOX360_BTN_START  7
#define XBOX360_BTN_LSTICK 8
#define XBOX360_BTN_RSTICK 9
#define XBOX360_BTN_XBOX   10

#define XBOX360_BTNMASK(BTN) (1 << (XBOX360_BTN_ ## BTN))
/* Usage example:
 * pState->buttons & XBOX360_BTNMASK(LB)
 * Returns whether or not LB button is pressed
 */

#define XBOX360_AXE_LX 0
#define XBOX360_AXE_LY 1
#define XBOX360_AXE_RX 2
#define XBOX360_AXE_RY 3
#define XBOX360_AXE_LT 4
#define XBOX360_AXE_RT 5

#define XBOX360_HAT 0

#define XBOX360_HAT_UP    0
#define XBOX360_HAT_DOWN  1
#define XBOX360_HAT_LEFT  2
#define XBOX360_HAT_RIGHT 3

#define XBOX360_HATMASK(BTN) (1 << (XBOX360_HAT_ ## BTN))

#endif

