#ifndef MMU_H
#define MMU_H

enum AccessRights
{
    READWRITE, READONLY
};

void MMUInit(void);
void MMUPopulateRange(uint32_t vAddress, uint32_t pAddress, size_t size, enum AccessRights accessRights);

#endif

