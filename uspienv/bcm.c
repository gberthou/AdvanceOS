#include "../mailbox.h"

#define TAG_SET_POWER_STATE 0x28001
#define TAG_GET_MAC_ADDRESS 0x10003

int SetPowerStateOn(unsigned int deviceId)
{
    uint32_t __attribute__((aligned(16))) sequence[] = {
        0, // size of the whole structure
        0, // request
        
        // TAG 0
        TAG_SET_POWER_STATE,
        8, // size
        8, // request
        deviceId,
        3, // on | wait

        0 // End TAG
    };
    sequence[0] = sizeof(sequence);
    
    // Send the requested values
    MailboxSend(8, ((uint32_t)sequence) + 0x40000000);
    
    // Now get the real values
    if(MailboxReceive(8) == 0 || sequence[1] == 0x80000000) // Ok
    {
        const uint32_t *ptr = sequence + 2;
        if(ptr[0] != TAG_SET_POWER_STATE // Unexpected tag
        || ptr[3] != 3       // Unexpected device id
        || (ptr[4] & 2))       // Device does not exist
            return 0;
        if(ptr[4] & 1)
            return 1;
        return 0; // Power is OFF 
    }
    return 0; // Mailbox failure
}

int GetMACAddress(unsigned char buffer[6])
{
    uint32_t __attribute__((aligned(16))) sequence[] = {
        0, // size of the whole structure
        0, // request
        
        // TAG 0
        TAG_GET_MAC_ADDRESS,
        0, // size
        0, // request

        0, // End TAG
        0,0 // Add 8 bytes to store the 6 bytes long MAC address
    };
    sequence[0] = sizeof(sequence);
    
    // Send the requested values
    MailboxSend(8, ((uint32_t)sequence) + 0x40000000);
    
    // Now get the real values
    if(MailboxReceive(8) == 0 || sequence[1] == 0x80000000) // Ok
    {
        const uint32_t *ptr = sequence + 2;
        const uint8_t *ptr8 = (uint8_t*)(ptr + 3);
        unsigned int i;

        if(ptr[0] != TAG_GET_MAC_ADDRESS) // Unexpected tag
            return 0;
        
        for(i = 0; i < 6; ++i)
            buffer[i] = *ptr8++;
        return 1;
    }
    return 0; // Mailbox failure
}

