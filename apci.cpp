#include "apci.h"
#include "apcilib.h"
#include "eNET-AIO16-16F.h"

extern int apci;

int widthFromOffset(int offset);

__u8 in8(int offset)
{
    __u8 value;
    int status = apci_read8(apci, 0, BAR_REGISTER, offset, &value);
    return status ? -1 : value;
}

__u16 in16(int offset)
{
    __u16 value;
    int status = apci_read16(apci, 0, BAR_REGISTER, offset, &value);
    return status ? -1 : value;
}

__u32 in32(int offset)
{
    __u32 value;
    int status = apci_read32(apci, 0, BAR_REGISTER, offset, &value);
    return status ? -1 : value;
}

__u32 in(int offset)
{
    switch (widthFromOffset(offset))
    {
    case 8:
        return in8(offset);
    case 16:
        return in16(offset);
    case 32:
        return in32(offset);
    default:
        return -1;
        break;
    }
}

TError out(int offset, __u32 value)
{
}

TError out8(int offset, __u8 value)
{
}

TError out16(int offset, __u16 value)
{
}

TError out32(int offset, __u32 value)
{
}
