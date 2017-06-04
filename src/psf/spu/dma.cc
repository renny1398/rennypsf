#include "psf/spu/spu.h"
#include "psf/psx/memory.h"
#include "common/debug.h"
#include <cstring>


namespace SPU {

using namespace psx;


////////////////////////////////////////////////////////////////////////
// Common DMA
////////////////////////////////////////////////////////////////////////

void SPUCore::SetDMADelay(int new_delay) {
  dma_delay_ = new_delay;
}

void SPUCore::DecreaseDMADelay() {
  auto delay = dma_delay_;
  if (0 < delay) {
    dma_delay_ = --delay;
    if (delay <= 0) {
      InterruptDMA();
      // TODO: psx irq
    }
  }
}

void SPUCore::ReadDMAMemory(PSXAddr psx_addr, uint32_t size) {
  SPUAddr spu_addr = addr_;
  uint16_t* p_psx_mem16 = p_spu_->psxMu16ptr(psx_addr);
  unsigned int kMemorySize = p_spu_->memory_size();
#ifdef MSB_FIRST
  for (uint32_t i = 0; i < size; i += 2) {
    *p_psx_mem16++ = p_spu_->mem16_val(spu_addr);
    spu_addr += 2;
    if (kMemorySize <= spu_addr) spu_addr = 0;
  }
#else
  do {
    uint32_t block_size = kMemorySize - spu_addr;
    if (size < block_size) block_size = size;
    ::memcpy(p_psx_mem16, p_spu_->mem16_ptr(spu_addr), block_size);
    spu_addr += block_size;
    size -= block_size;
    if (size == 0) break;
    rennyAssert(spu_addr == kMemorySize);
    p_psx_mem16 += block_size / sizeof(uint16_t);
    spu_addr = 0;
  } while (true);
#endif
  // iSpuAsyncWait = 0;
  addr_ = spu_addr;
  stat_ = kStateFlagDMACompleted;
  SetDMADelay(80);
}

void SPUCore::WriteDMAMemory(PSXAddr psx_addr, uint32_t size) {
  SPUAddr spu_addr = addr_;
  uint16_t* p_psx_mem16 = p_spu_->psxMu16ptr(psx_addr);
  unsigned int kMemorySize = p_spu_->memory_size();
#ifdef MSB_FIRST
  for (uint32_t i = 0; i < size; i += 2) {
    p_spu_->mem16_ref(spu_addr) = *p_psx_mem16++;
    spu_addr += 2;
    if (kMemorySize <= spu_addr) spu_addr = 0;
  }
#else
  do {
    uint32_t block_size = kMemorySize - spu_addr;
    if (size < block_size) block_size = size;
    ::memcpy(p_spu_->mem16_ptr(spu_addr), p_psx_mem16, block_size);
    spu_addr += block_size;
    size -= block_size;
    if (size == 0) break;
    rennyAssert(spu_addr == kMemorySize);
    p_psx_mem16 += block_size / sizeof(uint16_t);
    spu_addr = 0;
  } while (true);
#endif
  // iSpuAsyncWait = 0;
  addr_ = spu_addr;
  stat_ = kStateFlagDMACompleted;
  SetDMADelay(80);
}

void SPUCore::InterruptDMA() {
  if (p_spu()->core_count() == 1) return;
  ctrl_ &= 0x30;
  // regArea[PS2_C?_ADMAS] = 0;
  stat_ |= 0x80;
}

////////////////////////////////////////////////////////////////////////
// DMA4
////////////////////////////////////////////////////////////////////////

void SPUBase::ReadDMA4Memory(PSXAddr psx_addr, uint32_t size) {
  cores_[0].ReadDMAMemory(psx_addr, size);
}

void SPUBase::WriteDMA4Memory(PSXAddr psx_addr, uint32_t size) {
  cores_[0].WriteDMAMemory(psx_addr, size);
}

void SPUBase::InterruptDMA4() {
  cores_[0].InterruptDMA();
}

////////////////////////////////////////////////////////////////////////
// DMA7
////////////////////////////////////////////////////////////////////////

void SPUBase::ReadDMA7Memory(PSXAddr psx_addr, uint32_t size) {
  cores_[1].ReadDMAMemory(psx_addr, size);
}

void SPUBase::WriteDMA7Memory(PSXAddr psx_addr, uint32_t size) {
  cores_[1].WriteDMAMemory(psx_addr, size);
}

void SPUBase::InterruptDMA7() {
  cores_[1].InterruptDMA();
}

}   // namespace SPU
