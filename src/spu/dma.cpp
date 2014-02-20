#include "spu/spu.h"
#include "psx/memory.h"
#include <cstring>



namespace SPU {

using namespace PSX;


uint16_t SPU::ReadDMA4()
{
    SPUAddr spuAddr = core_.addr_;
    uint16_t s = mem16_[spuAddr >> 1];
    spuAddr += 2;
    if (spuAddr > 0x7ffff) spuAddr = 0;
    core_.addr_ = spuAddr;
    // iSpuAsyncWait = 0;
    return s;
}


void SPU::ReadDMA4Memory(PSXAddr psxAddr, uint32_t size)
{
    SPUAddr spu_addr = core_.addr_;
    uint16_t* p = U16M_ptr(psxAddr);
    // wxMessageOutputDebug().Printf(wxT("Transfer Sound Buffer from SPU(0x%08x) to PSX(0x%08x) by 0x%05x bytes"), spuAddr, psxAddr, size);

#ifdef MSB_FIRST    // for big-endian architecture
    size /= 2;
    for (uint32_t i = 0; i < size; i++) {
        *p++ = m_spuMem[spuAddr >> 1];
        spuAddr += 2;
        if (spuAddr > 0x7ffff) spuAddr = 0;
    }
#else               // for little-endian architecture
    do {
        uint32_t blockSize = 0x80000 - spu_addr;
        if (size < blockSize) blockSize = size;
        ::memcpy(p, &mem8_[spu_addr], blockSize);
        spu_addr += blockSize;
        size -= blockSize;
        if (size == 0) break;
        wxASSERT(spu_addr == 0x80000);
        p += blockSize / sizeof(uint16_t);
        spu_addr = 0;
    } while (true);
#endif
    core_.addr_ = spu_addr;
}


void SPU::WriteDMA4(uint16_t value)
{
    SPUAddr spuAddr = core_.addr_;
    mem16_[spuAddr >> 1] = value;
    spuAddr += 2;
    if (spuAddr > 0x7ffff) spuAddr = 0;
    core_.addr_ = spuAddr;
    // iSpuAsyncWait = 0;
    // wxMessageOutputDebug().Printf(wxT("Transfer WORD(0x%04x) to SPU(0x%08x)"), value, Addr-2);
}


void SPU::WriteDMA4Memory(uint32_t psxAddr, uint32_t size)
{
    wxASSERT(size < 0x80000);
    SPUAddr spuAddr = core_.addr_;
    uint16_t* p = U16M_ptr(psxAddr);

    // wxMessageOutputDebug().Printf(wxT("Transfer Sound Buffer from PSX(0x%08x) to SPU(0x%08x) by 0x%05x bytes"), psxAddr, spuAddr, size);

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
        ::memcpy(&mem8_[spuAddr], p, blockSize);
        spuAddr += blockSize;
        size -= blockSize;
        if (size == 0) break;
        wxASSERT(spuAddr == 0x80000);
        p += blockSize / sizeof(uint16_t);
        spuAddr = 0;
    } while (true);
#endif
    core_.addr_ = spuAddr;
}




////////////////////////////////////////////////////////////////////////
// PS2 DMA
////////////////////////////////////////////////////////////////////////


void SPU2::ReadDMAMemoryEx(SPUCore* core, PSXAddr psx_addr, uint32_t size) {
  wxASSERT(core != NULL);
  SPUAddr spu_addr = core->addr_;
  uint16_t* p_psx_mem16 = U16M_ptr(psx_addr);
#ifdef MSB_FIRST
  for (uint32_t i = 0; i < size; ++i) {
    *p_psx_mem16++ = mem16[spu_addr >> 1];
    spu_addr += 2;
    if (kMemorySize <= spu_addr) spu_addr = 0;
  }
#else
  do {
    uint32_t block_size = kMemorySize - spu_addr;
    if (size < block_size) block_size = size;
    ::memcpy(p_psx_mem16, &mem8_[spu_addr], block_size);
    spu_addr += block_size;
    size -= block_size;
    if (size == 0) break;
    wxASSERT(spu_addr == kMemorySize);
    p_psx_mem16 += block_size / sizeof(uint16_t);
    spu_addr = 0;
  } while (true);
#endif
  // iSpuAsyncWait = 0;
  core->addr_ = spu_addr;
  core->stat_ = SPUCore::kStateFlagDMACompleted;
}


void SPU2::WriteDMAMemoryEx(SPUCore *core, PSXAddr psx_addr, uint32_t size) {
  wxASSERT(core != NULL);
  SPUAddr spu_addr = core->addr_;
  uint16_t* p_psx_mem16 = U16M_ptr(psx_addr);
#ifdef MSB_FIRST
  for (uint32_t i = 0; i < size; ++i) {
    mem16[spu_addr >> 1] = *p_psx_mem16++;
    spu_addr += 2;
    if (kMemorySize <= spu_addr) spu_addr = 0;
  }
#else
  do {
    uint32_t block_size = kMemorySize - spu_addr;
    if (size < block_size) block_size = size;
    ::memcpy(&mem8_[spu_addr], p_psx_mem16, block_size);
    spu_addr += block_size;
    size -= block_size;
    if (size == 0) break;
    wxASSERT(spu_addr == kMemorySize);
    p_psx_mem16 += block_size / sizeof(uint16_t);
    spu_addr = 0;
  } while (true);
#endif
  // iSpuAsyncWait = 0;
  core->addr_ = spu_addr;
  core->stat_ = SPUCore::kStateFlagDMACompleted;
}



void SPU2::ReadDMA4Memory(PSXAddr psx_addr, uint32_t size) {
  ReadDMAMemoryEx(&cores_[0], psx_addr, size);
}

void SPU2::ReadDMA7Memory(PSXAddr psx_addr, uint32_t size) {
  ReadDMAMemoryEx(&cores_[1], psx_addr, size);
}



}   // namespace SPU
