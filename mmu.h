#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h>

enum AccessRights
{
    READWRITE, READONLY
};

void MMUInit(void);
void MMUEnable(void);
void MMUPopulateRange(uint32_t vAddress, uint32_t pAddress, size_t size, enum AccessRights accessRights);

#endif

