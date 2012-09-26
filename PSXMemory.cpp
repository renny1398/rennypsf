#include "PSXMemory.h"
#include <wx/msgout.h>

namespace PSXMemory
{
    uint8_t *SegmentLUT[0x10000];
    uint8_t memUser[0x200000];
    uint8_t memParallelPort[0x10000];
    uint8_t memHardware[0x3000];
    uint8_t memBIOS[0x80000];
}


void PSXMemory::Init()
{
    if (SegmentLUT[0x0000] != 0) return;

    int i;

    // Kusrg (for 4 threads?)
    for (i = 0; i < 0x80; i++) {
        SegmentLUT[0x0000 + i] = memUser + ((i & 0x1f) << 16);
    }
    // Kseg0, Kseg1
    memcpy(SegmentLUT + 0x8000, SegmentLUT, 0x20 * sizeof (char*));
    memcpy(SegmentLUT + 0xa000, SegmentLUT, 0x20 * sizeof (char*));

    SegmentLUT[0x1f00] = memParallelPort;
    SegmentLUT[0x1f80] = memHardware;
    SegmentLUT[0xbfc0] = memBIOS;

    wxMessageOutputDebug().Printf("Initialized PSX Memory.");
}

void PSXMemory::Reset()
{
    memset(memUser, 0, 0x00200000);
    memset(memParallelPort, 0, 0x00010000);
    wxMessageOutputDebug().Printf("Reset PSX Memory.");
}


// name 'Load' is not right?
void PSXMemory::Load(uint32_t address, int32_t length, char *data)
{
    // Init();

    uint32_t offset = address & 0xffff;
    if (offset) {
        uint32_t len = (0x10000 - offset) > length ? length : 0x10000 - offset;
        wxASSERT_MSG(SegmentLUT[address >> 16] != 0, "Invalid PSX memory address");
        memcpy(SegmentLUT[address << 16] + offset, data, len);
        address += len;
        data += len;
        length -= len;
    }

    uint32_t segment = address >> 16;
    while (length > 0) {
        memcpy(SegmentLUT[segment++], data, length < 0x10000 ? length : 0x10000);
        data += 0x10000;
        length -= 0x10000;
    }
}

