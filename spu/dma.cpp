#include "spu.h"
#include "../psx/memory.h"
#include <cstring>

namespace SPU {

using namespace PSX;

uint16_t SPU::ReadDMA()
{
    uint32_t spuAddr = m_spuAddr;
    uint16_t s = m_spuMem[spuAddr >> 1];
    spuAddr += 2;
    if (spuAddr > 0x7ffff) spuAddr = 0;
    m_spuAddr = spuAddr;
    // iSpuAsyncWait = 0;
    return s;
}

void SPU::ReadDMAMemory(uint32_t psxAddr, uint32_t size)
{
    uint32_t spuAddr = m_spuAddr;
    uint16_t* p = u16Mptr(psxAddr);
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
        ::memcpy(p, &m_spuMemC[spuAddr], blockSize);
        spuAddr += blockSize;
        size -= blockSize;
        if (size == 0) break;
        wxASSERT(spuAddr == 0x80000);
        p += blockSize / sizeof(uint16_t);
        spuAddr = 0;
    } while (true);
#endif
    m_spuAddr = spuAddr;
}


void SPU::WriteDMA(uint16_t value)
{
    uint32_t spuAddr = m_spuAddr;
    m_spuMem[m_spuAddr >> 1] = value;
    spuAddr += 2;
    if (spuAddr > 0x7ffff) spuAddr = 0;
    m_spuAddr = spuAddr;
    // iSpuAsyncWait = 0;
}

void SPU::WriteDMAMemory(uint32_t psxAddr, uint32_t size)
{
    uint32_t spuAddr = m_spuAddr;
    uint16_t* p = u16Mptr(psxAddr);
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
        ::memcpy(&m_spuMemC[spuAddr], p, blockSize);
        spuAddr += blockSize;
        size -= blockSize;
        if (size == 0) break;
        wxASSERT(spuAddr == 0x80000);
        p += blockSize / sizeof(uint16_t);
        spuAddr = 0;
    } while (true);
#endif
    m_spuAddr = spuAddr;
}


}   // namespace SPU
