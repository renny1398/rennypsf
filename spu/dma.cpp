#include "spu.h"
#include "../psx/memory.h"
#include <cstring>

namespace SPU {

using namespace PSX;

uint16_t SPU::ReadDMA()
{
    uint32_t spuAddr = Addr;
    uint16_t s = m_spuMem[spuAddr >> 1];
    spuAddr += 2;
    if (spuAddr > 0x7ffff) spuAddr = 0;
    Addr = spuAddr;
    // iSpuAsyncWait = 0;
    return s;
}

void SPU::ReadDMAMemory(uint32_t psxAddr, uint32_t size)
{
    uint32_t spuAddr = Addr;
    uint16_t* p = u16Mptr(psxAddr);
    wxMessageOutputDebug().Printf(wxT("Transfer Sound Buffer from SPU(0x%08x) to PSX(0x%08x) by 0x%05x bytes"), spuAddr, psxAddr, size);

#ifdef MSB_FIRST    // for big-endian architecture
    size /= 2;
    for (uint32_t i = 0; i < size; i++) {
        *p++ = m_spuMem[spuAddr >> 1];
        spuAddr += 2;
        if (spuAddr > 0x7ffff) spuAddr = 0;
    }
#else   // for little-endian architecture
    do {
        uint32_t blockSize = 0x80000 - spuAddr;
        if (size < blockSize) blockSize = size;
        ::memcpy(p, &Memory[spuAddr], blockSize);
        spuAddr += blockSize;
        size -= blockSize;
        if (size == 0) break;
        wxASSERT(spuAddr == 0x80000);
        p += blockSize / sizeof(uint16_t);
        spuAddr = 0;
    } while (true);
#endif
    Addr = spuAddr;
}


void SPU::WriteDMA(uint16_t value)
{
    uint32_t spuAddr = Addr;
    m_spuMem[Addr >> 1] = value;
    spuAddr += 2;
    if (spuAddr > 0x7ffff) spuAddr = 0;
    Addr = spuAddr;
    // iSpuAsyncWait = 0;
    wxMessageOutputDebug().Printf(wxT("Transfer WORD(0x%04x) to SPU(0x%08x)"), value, Addr-2);
}

void SPU::WriteDMAMemory(uint32_t psxAddr, uint32_t size)
{
    wxASSERT(size < 0x80000);
    uint32_t spuAddr = Addr;
    uint16_t* p = u16Mptr(psxAddr);

    wxMessageOutputDebug().Printf(wxT("Transfer Sound Buffer from PSX(0x%08x) to SPU(0x%08x) by 0x%05x bytes"), psxAddr, spuAddr, size);

#ifdef MSB_FIRST    // for big-endian architecture
    size /= 2;
    for (uint32_t i = 0; i < size; i++) {
        m_spuMem[spuAddr >> 1] = *p++;
        spuAddr += 2;
        if (spuAddr > 0x7ffff) spuAddr = 0;
    }
#else   // for little-endian architecture
    do {
        uint32_t blockSize = 0x80000 - spuAddr;
        if (size < blockSize) blockSize = size;
        ::memcpy(&Memory[spuAddr], p, blockSize);
        spuAddr += blockSize;
        size -= blockSize;
        if (size == 0) break;
        wxASSERT(spuAddr == 0x80000);
        p += blockSize / sizeof(uint16_t);
        spuAddr = 0;
    } while (true);
#endif
    Addr = spuAddr;
}


}   // namespace SPU
