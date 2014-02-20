#include "spu/spu.h"
#include "psx/memory.h"
#include <cstring>



namespace SPU {

using namespace PSX;


////////////////////////////////////////////////////////////////////////
// Common DMA
////////////////////////////////////////////////////////////////////////


void SPUBase::ReadDMAMemoryEx(SPUCore* core, PSXAddr psx_addr, uint32_t size) {
  wxASSERT(core != NULL);
  SPUAddr spu_addr = core->addr_;
  uint16_t* p_psx_mem16 = U16M_ptr(psx_addr);
  unsigned int kMemorySize = memory_size();
#ifdef MSB_FIRST
  for (uint32_t i = 0; i < size; i += 2) {
    *p_psx_mem16++ = mem16_val(spu_addr);
    spu_addr += 2;
    if (kMemorySize <= spu_addr) spu_addr = 0;
  }
#else
  do {
    uint32_t block_size = kMemorySize - spu_addr;
    if (size < block_size) block_size = size;
    ::memcpy(p_psx_mem16, mem16_ptr(spu_addr), block_size);
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


void SPUBase::WriteDMAMemoryEx(SPUCore* core, PSXAddr psx_addr, uint32_t size) {
  wxASSERT(core != NULL);
  SPUAddr spu_addr = core->addr_;
  uint16_t* p_psx_mem16 = U16M_ptr(psx_addr);
  unsigned int kMemorySize = memory_size();
#ifdef MSB_FIRST
  for (uint32_t i = 0; i < size; i += 2) {
    mem16_ref(spu_addr) = *p_psx_mem16++;
    spu_addr += 2;
    if (kMemorySize <= spu_addr) spu_addr = 0;
  }
#else
  do {
    uint32_t block_size = kMemorySize - spu_addr;
    if (size < block_size) block_size = size;
    ::memcpy(mem16_ptr(spu_addr), p_psx_mem16, block_size);
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




////////////////////////////////////////////////////////////////////////
// PS1 DMA
////////////////////////////////////////////////////////////////////////


uint16_t SPU::ReadDMA4() {
  SPUAddr spuAddr = core_.addr_;
  uint16_t s = mem16_[spuAddr >> 1];
  spuAddr += 2;
  if (spuAddr > 0x7ffff) spuAddr = 0;
  core_.addr_ = spuAddr;
  // iSpuAsyncWait = 0;
  return s;
}


void SPU::ReadDMA4Memory(PSXAddr psxAddr, uint32_t size) {
  ReadDMAMemoryEx(&core_, psxAddr, size);
}


void SPU::WriteDMA4(uint16_t value) {
  SPUAddr spuAddr = core_.addr_;
  mem16_[spuAddr >> 1] = value;
  spuAddr += 2;
  if (spuAddr > 0x7ffff) spuAddr = 0;
  core_.addr_ = spuAddr;
  // iSpuAsyncWait = 0;
  // wxMessageOutputDebug().Printf(wxT("Transfer WORD(0x%04x) to SPU(0x%08x)"), value, Addr-2);
}


void SPU::WriteDMA4Memory(uint32_t psxAddr, uint32_t size) {
  WriteDMAMemoryEx(&core_, psxAddr, size);
}




////////////////////////////////////////////////////////////////////////
// PS2 DMA
////////////////////////////////////////////////////////////////////////


void SPU2::ReadDMA4Memory(PSXAddr psx_addr, uint32_t size) {
  ReadDMAMemoryEx(&cores_[0], psx_addr, size);
}

void SPU2::ReadDMA7Memory(PSXAddr psx_addr, uint32_t size) {
  ReadDMAMemoryEx(&cores_[1], psx_addr, size);
}

void SPU2::WriteDMA4Memory(PSXAddr psx_addr, uint32_t size) {
  WriteDMAMemoryEx(&cores_[0], psx_addr, size);
}

void SPU2::WriteDMA7Memory(PSXAddr psx_addr, uint32_t size) {
  WriteDMAMemoryEx(&cores_[1], psx_addr, size);
}


}   // namespace SPU
