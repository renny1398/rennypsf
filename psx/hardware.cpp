#include "hardware.h"
#include "rcnt.h"
#include "dma.h"
#include "../spu/spu.h"
#include <wx/msgout.h>


namespace PSX {
namespace Hardware {

uint8_t Read8(uint32_t addr)
{
    return u8H(addr);
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
        }
        wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
        return 0;
    }
    if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
        Spu.ReadRegister(addr);
        return 0;
    }
    return u16H(addr);
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
        }
        wxMessageOutputStderr().Printf("ERROR: invalid PSX memory address (0x%08x)", addr);
        return 0;
    }
    return u32H(addr);
}


void Write8(uint32_t addr, uint8_t value)
{
    u8Href(addr) = value;
}


void Write16(uint32_t addr, uint16_t value)
{
    if (addr == 0x1f801070) {
        u16Href(0x1070) &= BFLIP16(u16H(0x1074) & value);
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
        Spu.WriteRegister(addr, value);
        return;
    }
    u16Href(addr) = BFLIP16(value);
}


void Write32(uint32_t addr, uint32_t value)
{
    if (addr == 0x1f801070) {
        u32Href(0x1070) &= BFLIP32(u32H(0x1074) & value);
        return;
    }
    if (addr == 0x1f8010c8) {
        DMA::Write(4, value);
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
    u32Href(addr) = BFLIP32(value);
}


}   // namespace Hardware
}   // namespace PSX
