#include "hardware.h"
#include "rcnt.h"
#include <wx/msgout.h>


namespace PSX {
namespace Hardware {


uint8_t Read8(uint32_t addr)
{
    return psxHu8(addr);
}


uint16_t Read16(uint32_t addr)
{
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
        int index = (addr & 0x00000030) >> 4;
        int ofs = (addr & 0xf) >> 2;
        switch (ofs) {
        case 0:
            return static_cast<uint16_t>(RootCounter::ReadCount(index));
        case 1:
            return static_cast<uint16_t>(RootCounter::counters[index].mode);
        case 2:
            return static_cast<uint16_t>(RootCounter::counters[index].target);
        default:
            wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
            return 0;
        }
    }
    if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
        wxMessageOutputDebug().Printf("TODO: implement SPUreadRegister");
        return 0;
    }
    return psxHu16(addr);
}


uint32_t Read32(uint32_t addr)
{
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
        int index = (addr & 0x00000030) >> 4;
        int ofs = (addr & 0xf) >> 2;
        switch (ofs) {
        case 0:
            return RootCounter::ReadCount(index);
        case 1:
            return RootCounter::counters[index].mode;
        case 2:
            return RootCounter::counters[index].target;
        default:
            wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
            return 0;
        }
    }
    return psxHu32(addr);
}


void Write8(uint32_t addr, uint8_t value)
{
    psxHu8ref(addr) = value;
}


void Write16(uint32_t addr, uint16_t value)
{
    if (addr == 0x1f801070) {
        psxHu16ref(0x1070) &= BFLIP16(psxHu16(0x1074) & value);
        return;
    }
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
        int index = (addr & 0x00000030) >> 4;
        int ofs = (addr & 0xf) >> 2;
        switch (ofs) {
        case 0:
            RootCounter::WriteCount(index, value);
            return;
        case 1:
            RootCounter::WriteMode(index, value);
            return;
        case 2:
            RootCounter::WriteTarget(index, value);
            return;
        default:
            wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
            return;
        }
    }
    if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
        wxMessageOutputDebug().Printf("TODO: implement SPUwriteRegister");
        return;
    }
    psxHu16ref(addr) = BFLIP16(value);
}


void Write32(uint32_t addr, uint32_t value)
{
    if (addr == 0x1f801070) {
        psxHu32ref(0x1070) &= BFLIP32(psxHu32(0x1074) & value);
        return;
    }
    if (addr == 0x1f8010c8) {
        wxMessageOutputDebug().Printf("TODO: implement DMA4 chcr");
        return;
    }
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
        int index = (addr & 0x00000030) >> 4;
        int ofs = (addr & 0xf) >> 2;
        switch (ofs) {
        case 0:
            RootCounter::WriteCount(index, value);
            return;
        case 1:
            RootCounter::WriteMode(index, value);
            return;
        case 2:
            RootCounter::WriteTarget(index, value);
            return;
        default:
            wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
            return;
        }
    }
    if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
        wxMessageOutputDebug().Printf("TODO: implement SPUwriteRegister");
        return;
    }
    psxHu32ref(addr) = BFLIP32(value);
}


}   // namespace Hardware
}   // namespace PSX
