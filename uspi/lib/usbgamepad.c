//
// usbgamepad.c
//
// USPi - An USB driver for Raspberry Pi written in C
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
// Copyright (C) 2014  M. Maccaferri <macca@maccasoft.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <uspi/usbgamepad.h>
#include <uspi/usbhostcontroller.h>
#include <uspi/devicenameservice.h>
#include <uspi/assert.h>
#include <uspi/util.h>
#include <uspios.h>

// HID Report Items from HID 1.11 Section 6.2.2
#define HID_USAGE_PAGE      0x04
#define HID_USAGE           0x08
#define HID_COLLECTION      0xA0
#define HID_END_COLLECTION  0xC0
#define HID_REPORT_COUNT    0x94
#define HID_REPORT_SIZE     0x74
#define HID_USAGE_MIN       0x18
#define HID_USAGE_MAX       0x28
#define HID_LOGICAL_MIN     0x14
#define HID_LOGICAL_MAX     0x24
#define HID_PHYSICAL_MIN    0x34
#define HID_PHYSICAL_MAX    0x44
#define HID_INPUT           0x80
#define HID_REPORT_ID       0x84
#define HID_OUTPUT          0x90

// HID Report Usage Pages from HID Usage Tables 1.12 Section 3, Table 1
#define HID_USAGE_PAGE_GENERIC_DESKTOP 0x01
#define HID_USAGE_PAGE_KEY_CODES       0x07
#define HID_USAGE_PAGE_LEDS            0x08
#define HID_USAGE_PAGE_BUTTONS         0x09

// HID Report Usages from HID Usage Tables 1.12 Section 4, Table 6
#define HID_USAGE_POINTER   0x01
#define HID_USAGE_MOUSE     0x02
#define HID_USAGE_JOYSTICK  0x04
#define HID_USAGE_GAMEPAD   0x05
#define HID_USAGE_KEYBOARD  0x06
#define HID_USAGE_X         0x30
#define HID_USAGE_Y         0x31
#define HID_USAGE_Z         0x32
#define HID_USAGE_RX        0x33
#define HID_USAGE_RY        0x34
#define HID_USAGE_RZ        0x35
#define HID_USAGE_SLIDER    0x36
#define HID_USAGE_WHEEL     0x38
#define HID_USAGE_HATSWITCH 0x39

// HID Report Collection Types from HID 1.12 6.2.2.6
#define HID_COLLECTION_PHYSICAL    0
#define HID_COLLECTION_APPLICATION 1

// HID Input/Output/Feature Item Data (attributes) from HID 1.11 6.2.2.5
#define HID_ITEM_CONSTANT 0x1
#define HID_ITEM_VARIABLE 0x2
#define HID_ITEM_RELATIVE 0x4

static unsigned s_nDeviceNumber;
static u8 ps3writeBuf[48];
static u8 ps3leds[5];

static const char FromUSBPad[] = "usbpad";

static boolean USBGamePadDeviceStartRequest (TUSBGamePadDevice *pThis);
static void USBGamePadDeviceCompletionRoutine (TUSBRequest *pURB, void *pParam, void *pContext);
static void USBGamePadDevicePS3Configure (TUSBGamePadDevice *pThis);

void USBGamePadStaticInit(void)
{
    const u8 writeBuf[] =
    {
            0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0xff, 0x27, 0x10, 0x00, 0x32,
            0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00
    };

    const u8 leds[] =
    {
        0x00, // OFF
        0x01, // LED1
        0x02, // LED2
        0x04, // LED3
        0x08, // LED4
    };

	unsigned int i;

	s_nDeviceNumber = 1;

	for(i = 0; i < 48; ++i)
		ps3writeBuf[i] = writeBuf[i];

	for(i = 0; i < 5; ++i)
		ps3leds[i] = leds[i];
}

void USBGamePadDevice (TUSBGamePadDevice *pThis, TUSBDevice *pDevice)
{
	assert (pThis != 0);

	USBDeviceCopy (&pThis->m_USBDevice, pDevice);
	pThis->m_USBDevice.Configure = USBGamePadDeviceConfigure;

    pThis->m_type = GAMEPAD_HID;
	pThis->m_pEndpointIn = 0;
    pThis->m_pEndpointOut = 0;
    pThis->m_pStatusHandler = 0;
	pThis->m_pURB = 0;
	pThis->m_pHIDReportDescriptor = 0;
	pThis->m_usReportDescriptorLength = 0;
    pThis->m_nReportSize = 0;

    pThis->m_State.naxes = 0;
    for (int i = 0; i < MAX_AXIS; i++) {
        pThis->m_State.axes[i].value = 0;
        pThis->m_State.axes[i].minimum = 0;
        pThis->m_State.axes[i].maximum = 0;
    }

    pThis->m_State.nhats = 0;
    for (int i = 0; i < MAX_HATS; i++)
        pThis->m_State.hats[i] = 0;

    pThis->m_State.nbuttons = 0;
    pThis->m_State.buttons = 0;

	pThis->m_pReportBuffer = malloc (64);
	assert (pThis->m_pReportBuffer != 0);
}

void _CUSBGamePadDevice (TUSBGamePadDevice *pThis)
{
	assert (pThis != 0);

    if (pThis->m_pHIDReportDescriptor != 0)
    {
        free (pThis->m_pHIDReportDescriptor);
        pThis->m_pHIDReportDescriptor = 0;
    }

	if (pThis->m_pReportBuffer != 0)
	{
		free (pThis->m_pReportBuffer);
		pThis->m_pReportBuffer = 0;
	}

	if (pThis->m_pEndpointIn != 0)
	{
		_USBEndpoint (pThis->m_pEndpointIn);
		free (pThis->m_pEndpointIn);
		pThis->m_pEndpointIn = 0;
	}

    if (pThis->m_pEndpointOut != 0)
    {
        _USBEndpoint (pThis->m_pEndpointOut);
        free (pThis->m_pEndpointOut);
        pThis->m_pEndpointOut = 0;
    }

	_USBDevice (&pThis->m_USBDevice);
}

static u32 BitGetUnsigned(void *buffer, u32 offset, u32 length)
{
    u8* bitBuffer;
    u8 mask;
    u32 result;

    bitBuffer = buffer;
    result = 0;
    for (u32 i = offset / 8, j = 0; i < (offset + length + 7) / 8; i++) {
        if (offset / 8 == (offset + length - 1) / 8) {
            mask = (1 << ((offset % 8) + length)) - (1 << (offset % 8));
            result = (bitBuffer[i] & mask) >> (offset % 8);
        } else if (i == offset / 8) {
            mask = 0x100 - (1 << (offset % 8));
            j += 8 - (offset % 8);
            result = ((bitBuffer[i] & mask) >> (offset % 8)) << (length - j);
        } else if (i == (offset + length - 1) / 8) {
            mask = (1 << ((offset % 8) + length)) - 1;
            result |= bitBuffer[i] & mask;
        } else {
            j += 8;
            result |= bitBuffer[i] << (length - j);
        }
    }

    return result;
}

static s32 BitGetSigned(void* buffer, u32 offset, u32 length) {
    u32 result = BitGetUnsigned(buffer, offset, length);

    if (result & (1 << (length - 1)))
        result |= 0xffffffff - ((1 << length) - 1);

    return result;
}

enum {
    None = 0,
    GamePad,
    GamePadButton,
    GamePadAxis,
    GamePadHat,
};

#define UNDEFINED   -123456789

static void USBGamePadDeviceDecodeReportXBox360Wired(TUSBGamePadDevice *pThis)
{
    const u8 *pReportBuffer = pThis->m_pReportBuffer;
    const u16 *pAxesBuffer = (u16*)(pReportBuffer + 6);
    USPiGamePadState *pState = &pThis->m_State;
    unsigned int i;

    if(   (pReportBuffer[0] & 0xfe) == 0 // Message type (0 or 1)
       && pReportBuffer[1] == 20) // Packet size
    {
        pState->naxes    = 6;
        pState->nhats    = 1;
        pState->nbuttons = 11;

        // D-Pad
        pState->hats[0] = (pReportBuffer[2] & 0xf);

        // Buttons
        pState->buttons = (pReportBuffer[3] >> 4) // ABXY mapped onto indices
                                                  // 0-3
                        | ((pReportBuffer[3] & 3) << 4) // LB,RB mapped onto
                                                        // indices 4-5
                        | (((pReportBuffer[2] >> 5) & 1) << 6) // Back
                        | (((pReportBuffer[2] >> 4) & 1) << 7) // Start
                        | ((pReportBuffer[2] >> 6) << 8) // Left stick and
                                                         // Right stick
                        | (((pReportBuffer[3] >> 2) & 1) << 10); // Xbox button

        // Axes
        // Left  stick X, Left  stick Y
        // Right stick X, Right stick Y
        for(i = 0; i < 4; ++i)
        {
            int tmp;

            pState->axes[i].minimum = ~0x7fff;
            pState->axes[i].maximum = 0x7fff;

            tmp = *pAxesBuffer++;
            if(tmp & (1 << 15)) // Negative value
                tmp |= 0xffff0000;
            pState->axes[i].value = tmp;
        }

        // Axes
        // Left trigger, Right trigger
        for(i = 0; i < 2; ++i)
        {
            int tmp;

            pState->axes[i + 4].minimum = 0x00;
            pState->axes[i + 4].maximum = 0xff;

            tmp = pReportBuffer[i + 4]; // Unsigned values
            pState->axes[i + 4].value = tmp;
        }
    }
}

static void USBGamePadDeviceDecodeReport(TUSBGamePadDevice *pThis)
{
    if(pThis->m_type == GAMEPAD_XBOX360_WIRED)
    {
        USBGamePadDeviceDecodeReportXBox360Wired(pThis);
        return;
    }

    s32 item, arg;
    u32 offset = 0, size = 0, count = 0;
    s32 lmax = UNDEFINED, lmin = UNDEFINED, pmin = UNDEFINED, pmax = UNDEFINED;
    s32 naxes = 0, nhats = 0;
    u32 id = 0, state = None;

    u8 *pReportBuffer = pThis->m_pReportBuffer;
    s8 *pHIDReportDescriptor = (s8 *)pThis->m_pHIDReportDescriptor;
    u16 wReportDescriptorLength = pThis->m_usReportDescriptorLength;
    USPiGamePadState *pState = &pThis->m_State;

    while (wReportDescriptorLength > 0) {
        item = *pHIDReportDescriptor++;
        wReportDescriptorLength--;

        switch(item & 0x03) {
            case 0:
                arg = 0;
                break;
            case 1:
                arg = *pHIDReportDescriptor++;
                wReportDescriptorLength--;
                break;
            case 2:
                arg = *pHIDReportDescriptor++ & 0xFF;
                arg = arg | (*pHIDReportDescriptor++ << 8);
                wReportDescriptorLength -= 2;
                break;
            default:
                arg = *pHIDReportDescriptor++;
                arg = arg | (*pHIDReportDescriptor++ << 8);
                arg = arg | (*pHIDReportDescriptor++ << 16);
                arg = arg | (*pHIDReportDescriptor++ << 24);
                wReportDescriptorLength -= 4;
                break;
        }

        if ((item & 0xFC) == HID_REPORT_ID) {
            if (id != 0)
                break;
            id = BitGetUnsigned(pReportBuffer, 0, 8);
            if (id != 0 && id != arg)
                return;
            id = arg;
            offset = 8;
        }

        switch(item & 0xFC) {
            case HID_USAGE_PAGE:
                switch(arg) {
                    case HID_USAGE_PAGE_BUTTONS:
                        if (state == GamePad)
                            state = GamePadButton;
                        break;
                }
                break;
            case HID_USAGE:
                switch(arg) {
                    case HID_USAGE_JOYSTICK:
                    case HID_USAGE_GAMEPAD:
                        state = GamePad;
                        break;
                    case HID_USAGE_X:
                    case HID_USAGE_Y:
                    case HID_USAGE_Z:
                    case HID_USAGE_RX:
                    case HID_USAGE_RY:
                    case HID_USAGE_RZ:
                    case HID_USAGE_SLIDER:
                        if (state == GamePad)
                            state = GamePadAxis;
                        break;
                    case HID_USAGE_HATSWITCH:
                        if (state == GamePad)
                            state = GamePadHat;
                        break;
                }
                break;
            case HID_LOGICAL_MIN:
                lmin = arg;
                break;
            case HID_PHYSICAL_MIN:
                pmin = arg;
                break;
            case HID_LOGICAL_MAX:
                lmax = arg;
                break;
            case HID_PHYSICAL_MAX:
                pmax = arg;
                break;
            case HID_REPORT_SIZE: // REPORT_SIZE
                size = arg;
                break;
            case HID_REPORT_COUNT: // REPORT_COUNT
                count = arg;
                break;
            case HID_INPUT:
                if ((arg & 0x03) == 0x02) {  // INPUT(Data,Var)
                    if (state == GamePadAxis) {
                        for (int i = 0; i < count && i < MAX_AXIS; i++) {
                            pState->axes[naxes].minimum = lmin != UNDEFINED ? lmin : pmin;
                            pState->axes[naxes].maximum = lmax != UNDEFINED ? lmax : pmax;

                            int value = (pState->axes[naxes].minimum < 0) ?
                                    BitGetSigned(pReportBuffer, offset + i * size, size) :
                                    BitGetUnsigned(pReportBuffer, offset + i * size, size);

                            pState->axes[naxes++].value = value;
                        }

                        state = GamePad;
                    }
                    else if (state == GamePadHat) {
                        for (int i = 0; i < count && i < MAX_HATS; i++) {
                            int value = BitGetUnsigned(pReportBuffer, offset + i * size, size);
                            pState->hats[nhats++] = value;
                        }
                        state = GamePad;
                    }
                    else if (state == GamePadButton) {
                        pState->nbuttons = count;
                        pState->buttons = BitGetUnsigned(pReportBuffer, offset, size * count);
                        state = GamePad;
                    }
                }
                offset += count * size;
                break;
            case HID_OUTPUT:
                break;
        }
    }

    pState->naxes = naxes;
    pState->nhats = nhats;

    pThis->m_nReportSize = (offset + 7) / 8;
}

boolean USBGamePadDeviceConfigure (TUSBDevice *pUSBDevice)
{
	TUSBGamePadDevice *pThis = (TUSBGamePadDevice *) pUSBDevice;
	assert (pThis != 0);
    
    if (   pUSBDevice->m_pDeviceDesc->idVendor == 0x054c
        && pUSBDevice->m_pDeviceDesc->idProduct == 0x0268)
    {
        pThis->m_type = GAMEPAD_PS3;
    }
    else if(   pUSBDevice->m_pDeviceDesc->idVendor == 0x045e
            && pUSBDevice->m_pDeviceDesc->idProduct == 0x028e)
    {
        pThis->m_type = GAMEPAD_XBOX360_WIRED;
    }

	TUSBConfigurationDescriptor *pConfDesc =
		(TUSBConfigurationDescriptor *) USBDeviceGetDescriptor (&pThis->m_USBDevice, DESCRIPTOR_CONFIGURATION);
	if (   pConfDesc == 0
	    || pConfDesc->bNumInterfaces <  1)
	{
		USBDeviceConfigurationError (&pThis->m_USBDevice, FromUSBPad);

		return FALSE;
	}

    //pUSBDevice->m_pConfigDesc = pConfDesc;

    TUSBInterfaceDescriptor *pInterfaceDesc =
        (TUSBInterfaceDescriptor *) USBDeviceGetDescriptor (&pThis->m_USBDevice, DESCRIPTOR_INTERFACE);
    if (   pInterfaceDesc == 0
        || pInterfaceDesc->bNumEndpoints      <  1
        || (pThis->m_type != GAMEPAD_XBOX360_WIRED &&
            (pInterfaceDesc->bInterfaceClass    != 0x03   // HID Class
          || pInterfaceDesc->bInterfaceSubClass != 0x00   // Boot Interface Subclass
          || pInterfaceDesc->bInterfaceProtocol != 0x00)) // GamePad
        || (pThis->m_type == GAMEPAD_XBOX360_WIRED &&
            (pInterfaceDesc->bInterfaceClass != 0xff        // Vendor Class
             || pInterfaceDesc->bInterfaceSubClass != 0x5d
             || pInterfaceDesc->bInterfaceProtocol != 0x01)))  // GamePad
    {
        USBDeviceConfigurationError (&pThis->m_USBDevice, FromUSBPad);

        return FALSE;
    }

    pThis->m_ucInterfaceNumber  = pInterfaceDesc->bInterfaceNumber;
    pThis->m_ucAlternateSetting = pInterfaceDesc->bAlternateSetting;

    TUSBHIDDescriptor *pHIDDesc = 0;
    if(pThis->m_type != GAMEPAD_XBOX360_WIRED)
    {
        pHIDDesc = (TUSBHIDDescriptor *) USBDeviceGetDescriptor (&pThis->m_USBDevice, DESCRIPTOR_HID);
        if (   pHIDDesc == 0
            || pHIDDesc->wReportDescriptorLength == 0)
        {
            USBDeviceConfigurationError (&pThis->m_USBDevice, FromUSBPad);

            return FALSE;
        }
    }

    const TUSBEndpointDescriptor *pEndpointDesc;
    while ((pEndpointDesc = (TUSBEndpointDescriptor *) USBDeviceGetDescriptor (&pThis->m_USBDevice, DESCRIPTOR_ENDPOINT)) != 0)
    {
        if ((pEndpointDesc->bmAttributes & 0x3F) == 0x03)       // Interrupt
        {
            if ((pEndpointDesc->bEndpointAddress & 0x80) == 0x80)   // Input
            {
                if (pThis->m_pEndpointIn != 0)
                {
                    USBDeviceConfigurationError (&pThis->m_USBDevice, FromUSBPad);

                    return FALSE;
                }

                pThis->m_pEndpointIn = (TUSBEndpoint *) malloc (sizeof (TUSBEndpoint));
                assert (pThis->m_pEndpointIn != 0);
                USBEndpoint2 (pThis->m_pEndpointIn, &pThis->m_USBDevice, pEndpointDesc);
            }
            else                            // Output
            {
                if (pThis->m_pEndpointOut != 0)
                {
                    USBDeviceConfigurationError (&pThis->m_USBDevice, FromUSBPad);

                    return FALSE;
                }

                pThis->m_pEndpointOut = (TUSBEndpoint *) malloc (sizeof (TUSBEndpoint));
                assert (pThis->m_pEndpointOut != 0);
                USBEndpoint2 (pThis->m_pEndpointOut, &pThis->m_USBDevice, pEndpointDesc);
            }
        }
    }

	if (pThis->m_pEndpointIn == 0
        || (pThis->m_type == GAMEPAD_XBOX360_WIRED && pThis->m_pEndpointOut == 0
    ))
	{
		USBDeviceConfigurationError (&pThis->m_USBDevice, FromUSBPad);

		return FALSE;
	}

    if(pThis->m_type != GAMEPAD_XBOX360_WIRED)
    {
        pThis->m_usReportDescriptorLength = pHIDDesc->wReportDescriptorLength;
        pThis->m_pHIDReportDescriptor = (unsigned char *) malloc(pHIDDesc->wReportDescriptorLength);
        assert (pThis->m_pHIDReportDescriptor != 0);

        if (DWHCIDeviceGetDescriptor (USBDeviceGetHost (&pThis->m_USBDevice),
                        USBDeviceGetEndpoint0 (&pThis->m_USBDevice),
                        pHIDDesc->bReportDescriptorType, DESCRIPTOR_INDEX_DEFAULT,
                        pThis->m_pHIDReportDescriptor, pHIDDesc->wReportDescriptorLength, REQUEST_IN)
            != pHIDDesc->wReportDescriptorLength)
        {
            LogWrite (FromUSBPad, LOG_ERROR, "Cannot get HID report descriptor");

            return FALSE;
        }
        //DebugHexdump (pThis->m_pHIDReportDescriptor, pHIDDesc->wReportDescriptorLength, "hid");

        pThis->m_pReportBuffer[0] = 0;
        USBGamePadDeviceDecodeReport (pThis);
    }
    else
    {
        pThis->m_nReportSize = 20;
    }

    if (!USBDeviceConfigure (&pThis->m_USBDevice))
    {
        LogWrite (FromUSBPad, LOG_ERROR, "Cannot set configuration");

        return FALSE;
    }

    if (pThis->m_ucAlternateSetting != 0)
    {
        if (DWHCIDeviceControlMessage (USBDeviceGetHost (&pThis->m_USBDevice),
                        USBDeviceGetEndpoint0 (&pThis->m_USBDevice),
                        REQUEST_OUT | REQUEST_TO_INTERFACE, SET_INTERFACE,
                        pThis->m_ucAlternateSetting,
                        pThis->m_ucInterfaceNumber, 0, 0) < 0)
        {
            LogWrite (FromUSBPad, LOG_ERROR, "Cannot set interface");

            return FALSE;
        }
    }

    pThis->m_nDeviceIndex = s_nDeviceNumber++;

    if (pThis->m_type == GAMEPAD_PS3)
    {
        USBGamePadDevicePS3Configure (pThis);
    }

	TString DeviceName;
	String (&DeviceName);
	StringFormat (&DeviceName, "upad%u", pThis->m_nDeviceIndex);
	DeviceNameServiceAddDevice (DeviceNameServiceGet (), StringGet (&DeviceName), pThis, FALSE);

	_String (&DeviceName);

	return USBGamePadDeviceStartRequest (pThis);
}

void USBGamePadDeviceRegisterStatusHandler (TUSBGamePadDevice *pThis, TGamePadStatusHandler *pStatusHandler)
{
    assert (pThis != 0);
    assert (pStatusHandler != 0);
    pThis->m_pStatusHandler = pStatusHandler;
}

boolean USBGamePadDeviceStartRequest (TUSBGamePadDevice *pThis)
{
	assert (pThis != 0);

	assert (pThis->m_pEndpointIn != 0);
	assert (pThis->m_pReportBuffer != 0);

	assert (pThis->m_pURB == 0);
	pThis->m_pURB = malloc (sizeof (TUSBRequest));
	assert (pThis->m_pURB != 0);
	USBRequest (pThis->m_pURB, pThis->m_pEndpointIn, pThis->m_pReportBuffer, pThis->m_nReportSize, 0);
	USBRequestSetCompletionRoutine (pThis->m_pURB, USBGamePadDeviceCompletionRoutine, 0, pThis);

	return DWHCIDeviceSubmitAsyncRequest (USBDeviceGetHost (&pThis->m_USBDevice), pThis->m_pURB);
}

void USBGamePadDeviceCompletionRoutine (TUSBRequest *pURB, void *pParam, void *pContext)
{
	TUSBGamePadDevice *pThis = (TUSBGamePadDevice *) pContext;
	assert (pThis != 0);

	assert (pURB != 0);
	assert (pThis->m_pURB == pURB);

    if (   USBRequestGetStatus (pURB) != 0
	    && USBRequestGetResultLength (pURB) > 0)
    {
        //DebugHexdump (pThis->m_pReportBuffer, 16, "report");
        if ((pThis->m_type == GAMEPAD_XBOX360_WIRED
             || pThis->m_pHIDReportDescriptor != 0)
            && pThis->m_pStatusHandler != 0)
        {
            USBGamePadDeviceDecodeReport (pThis);
            (*pThis->m_pStatusHandler) (pThis->m_nDeviceIndex - 1, &pThis->m_State);
        }
	}

	_USBRequest (pThis->m_pURB);
	free (pThis->m_pURB);
	pThis->m_pURB = 0;

	USBGamePadDeviceStartRequest (pThis);
}

void USBGamePadDeviceGetReport (TUSBGamePadDevice *pThis)
{
    if (DWHCIDeviceControlMessage (USBDeviceGetHost (&pThis->m_USBDevice),
                       USBDeviceGetEndpoint0 (&pThis->m_USBDevice),
                       REQUEST_IN | REQUEST_CLASS | REQUEST_TO_INTERFACE,
                       GET_REPORT, (REPORT_TYPE_INPUT << 8) | 0x00,
                       pThis->m_ucInterfaceNumber,
                       pThis->m_pReportBuffer, pThis->m_nReportSize) > 0)
    {
        USBGamePadDeviceDecodeReport (pThis);
    }
}

void USBGamePadDevicePS3Configure (TUSBGamePadDevice *pThis)
{
    /* Special PS3 Controller enable commands */
    pThis->m_pReportBuffer[0] = 0x42;
    pThis->m_pReportBuffer[1] = 0x0c;
    pThis->m_pReportBuffer[2] = 0x00;
    pThis->m_pReportBuffer[3] = 0x00;
    DWHCIDeviceControlMessage (USBDeviceGetHost (&pThis->m_USBDevice),
                           USBDeviceGetEndpoint0 (&pThis->m_USBDevice),
                           REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
                           SET_REPORT, (REPORT_TYPE_FEATURE << 8) | 0xf4,
                           pThis->m_ucInterfaceNumber,
                           pThis->m_pReportBuffer, 4);

    /* Turn on LED */
    ps3writeBuf[9] |= (u8)(ps3leds[pThis->m_nDeviceIndex] << 1);
    DWHCIDeviceControlMessage (USBDeviceGetHost (&pThis->m_USBDevice),
                           USBDeviceGetEndpoint0 (&pThis->m_USBDevice),
                           REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
                           SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0x01,
                           pThis->m_ucInterfaceNumber,
                           ps3writeBuf, 48);
}

/*
void USBGamePadDeviceXBox360SetLeds (TUSBGamePadDevice *pThis, u8 leds)
{
    u8 buf[3] = {0x01, 0x03, 0x00};
    buf[2] = (leds <= 0x0D ? leds : 0x00);
    
    DWHCIDeviceControlMessage (USBDeviceGetHost (&pThis->m_USBDevice),
                           USBDeviceGetEndpoint0 (&pThis->m_USBDevice),
                           REQUEST_OUT | REQUEST_CLASS | REQUEST_TO_INTERFACE,
                           SET_REPORT, (REPORT_TYPE_OUTPUT << 8) | 0x01,
                           pThis->m_ucInterfaceNumber,
                           buf, sizeof(buf));
}
*/

