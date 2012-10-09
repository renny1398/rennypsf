#include "PSXHardware.h"
#include "PSXCounters.h"
#include <wx/msgout.h>

uint8_t PSXHardware::Read8(uint32_t addr)
{
    return psxHu8(addr);
}


uint16_t PSXHardware::Read16(uint32_t addr)
{
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
        int index = (addr & 0x00000030) >> 4;
        int ofs = (addr & 0xf) >> 2;
        switch (ofs) {
        case 0:
            return static_cast<uint16_t>(PSXRootCounter::ReadCount(index));
        case 1:
            return static_cast<uint16_t>(PSXRootCounter::counters[index].mode);
        case 2:
            return static_cast<uint16_t>(PSXRootCounter::counters[index].target);
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


uint32_t PSXHardware::Read32(uint32_t addr)
{
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
        int index = (addr & 0x00000030) >> 4;
        int ofs = (addr & 0xf) >> 2;
        switch (ofs) {
        case 0:
            return PSXRootCounter::ReadCount(index);
        case 1:
            return PSXRootCounter::counters[index].mode;
        case 2:
            return PSXRootCounter::counters[index].target;
        default:
            wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
            return 0;
        }
    }
    return psxHu32(addr);
}


void PSXHardware::Write8(uint32_t addr, uint8_t value)
{
    psxHu8ref(addr) = value;
}


void PSXHardware::Write16(uint32_t addr, uint16_t value)
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
            PSXRootCounter::WriteCount(index, value);
            return;
        case 1:
            PSXRootCounter::WriteMode(index, value);
            return;
        case 2:
            PSXRootCounter::WriteTarget(index, value);
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


void PSXHardware::Write32(uint32_t addr, uint32_t value)
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
            PSXRootCounter::WriteCount(index, value);
            return;
        case 1:
            PSXRootCounter::WriteMode(index, value);
            return;
        case 2:
            PSXRootCounter::WriteTarget(index, value);
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
