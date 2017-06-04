#include <stdint.h>
#include <memory.h>
#include "psf/psx/psx.h"
#include "psf/psx/memory.h"
#include "psf/psx/hardware.h"
#include "common/debug.h"

namespace psx {

////////////////////////////////////////////////////////////////////////
// UserMemoryAccessor constructor/destructor definitions
////////////////////////////////////////////////////////////////////////

UserMemoryAccessor::UserMemoryAccessor(Memory* mem)
  : mem_(mem->mem_user_) {
  rennyAssert(mem_ != nullptr);
}

UserMemoryAccessor::UserMemoryAccessor(PSX* psx)
  : mem_(psx->Mem().mem_user_) {
  rennyAssert(mem_ != nullptr);
}

////////////////////////////////////////////////////////////////////////
// MemoryAccessor constructor/destructor definitions
////////////////////////////////////////////////////////////////////////

MemoryAccessor::MemoryAccessor(Memory *p_mem)
  : p_mem_(p_mem) {
  rennyAssert(p_mem != nullptr);
}

MemoryAccessor::MemoryAccessor(PSX *psx)
  : p_mem_(&psx->Mem()) {
  rennyAssert(p_mem_ != nullptr);
}

////////////////////////////////////////////////////////////////////////
// Memory function definitions
////////////////////////////////////////////////////////////////////////

Memory::Memory(int version, HardwareRegisters* hw_regs)
  : version_(version), hw_regs_(*hw_regs), psxH_(hw_regs) {
  rennyAssert(&hw_regs_ != nullptr);
  ::memset(segment_LUT_, 0, 0x10000);
  ::memset(mem_user_, 0, 0x200000);
  ::memset(mem_parallel_port_, 0, 0x10000);
  ::memset(mem_bios_, 0, 0x80000);
  Init();
}


void Memory::Init()
{
  if (segment_LUT_[0x0000] != 0) return;

  int i;

  // Kuseg (for 4 threads?)
  for (i = 0; i < 0x80; i++) {
    segment_LUT_[0x0000 + i] = mem_user_ + ((i & 0x1f) << 16);
  }
  // Kseg0, Kseg1
  ::memcpy(segment_LUT_ + 0x8000, segment_LUT_, 0x20 * sizeof (char*));
  ::memcpy(segment_LUT_ + 0xa000, segment_LUT_, 0x20 * sizeof (char*));

  segment_LUT_[0x1f00] = mem_parallel_port_;
  segment_LUT_[0x1f80] = psxH_.psxHu8ptr(0);
  segment_LUT_[0xbfc0] = mem_bios_;

  rennyLogDebug("PSXMemory", "Initialized memory.");
}


void Memory::Reset()
{
  ::memset(mem_user_, 0, sizeof (mem_user_));
  ::memset(mem_parallel_port_, 0, sizeof (mem_parallel_port_));
  rennyLogDebug("PSXMemory", "Reset memory.");
}


void Memory::Copy(PSXAddr dest, const void* src, int length)
{
  rennyAssert(src != 0);
  rennyLogDebug("PSXMemory", "Load data (length: %06x) at 0x%08p into 0x%08x", length, src, dest);

  const char* p_src = static_cast<const char*>(src);

  u32 offset = dest & 0xffff;
  if (offset) {
    u32 len = (0x10000 - offset) > static_cast<u32>(length) ? length : 0x10000 - offset;
    void* const dest_ptr = segment_LUT_[dest >> 16] + offset;
    ::memcpy(dest_ptr, src, len);
    dest += len;
    p_src += len;
    length -= len;
  }

  u32 segment = dest >> 16;
  while (length > 0) {
    rennyAssert(segment_LUT_[segment] != 0);
    ::memcpy(segment_LUT_[segment++], p_src, length < 0x10000 ? length : 0x10000);
    p_src += 0x10000;
    length -= 0x10000;
  }
}


template<typename T>
T Memory::Read(PSXAddr addr) const {
  u32 segment = addr >> 16;
  switch (segment) {
  case 0x1d00:
    if ((addr & 0xff80) == 0) {
      // SIF
      rennyLogDebug("PSXMemory", "SIF!");
      return hw_regs_.Read<T>(addr);
    }
    return hw_regs_.Read<T>(addr);
  case 0x1f80:
    if (addr < 0x1f801000) {
      // read Scratch Pad
      return psxH_.psxHval<T>(addr);
    }
    // read Hardware Registers
    return hw_regs_.Read<T>(addr);
  case 0x1f90:
    if (version_ == 2) {
      rennyLogDebug("PSXMemory", "Read(0x%08x) : read SPU2.", addr);
      return hw_regs_.Read<T>(addr);
    }
  case 0xbf80:
  case 0xbf90:
    if (version_ == 2) {
      return hw_regs_.Read<T>(addr);
    }
  }
  if ((segment & 0xffc0) == 0x1fc0) {
    rennyLogDebug("PSXMemory", "Read(0x%08x) : return 0.", addr);
    return 0;
  }
  u8 *base_addr = segment_LUT_[segment];
  u32 offset = addr & 0xffff;
  if (base_addr == 0) {
    rennyLogWarning("PSXMemory", "Read(0x%08x) : bad segment: 0x%04x", addr, segment);
    //PSX::Disasm.DumpRegisters();
    return 0;
  }
  return BFLIP(*reinterpret_cast<T*>(base_addr + offset));
}

template uint8_t Memory::Read<uint8_t>(PSXAddr addr) const;
template uint16_t Memory::Read<uint16_t>(PSXAddr addr) const;
template uint32_t Memory::Read<uint32_t>(PSXAddr addr) const;


u8 Memory::Read8(PSXAddr addr) const {
  return Read<u8>(addr);
}

u16 Memory::Read16(PSXAddr addr) const {
  return Read<u16>(addr);
}

u32 Memory::Read32(PSXAddr addr) const {
  return Read<u32>(addr);
}


template<typename T>
void Memory::Write(PSXAddr addr, T value) {
  u32 segment = addr >> 16;
  switch (segment) {
  case 0x1d00:
    if ((addr & 0xff80) == 0) {
      // SIF
      rennyLogDebug("PSXMemory", "SIF!");
      hw_regs_.Write(addr, value);
    }
    break;
  case 0x1f80:
    if (addr < 0x1f801000) {
      psxH_.psxHref<T>(addr) = BFLIP(value);
      return;
    }
    hw_regs_.Write(addr, value);
    return;
  case 0x1f90:
    if ((addr & 0x0000f800) == 0) {
      // SPU2
      rennyLogDebug("PSXMemory", "SPU2!");
      hw_regs_.Write(addr, value);
      return;
    }
    hw_regs_.Write(addr, value);
    return;
  case 0xbf80:
  case 0xbf90:
    if (version_ == 2) {
      hw_regs_.Write(addr, value);
      return;
    }
    break;
  case 0x1fc1:
    rennyLogDebug("PSXMemory", "EMUCALL??");
  }
  u8 *base_addr = segment_LUT_[segment];
  u32 offset = addr & 0xffff;
  if (base_addr == 0) {
    rennyLogWarning("PSXMemory", "Write(0x%08x, %d) : bad segment: 0x%04x", addr, value, segment);
    return;
  }
  *reinterpret_cast<T*>(base_addr + offset) = BFLIP(value);
}

template void Memory::Write<u8>(PSXAddr addr, u8 value);
template void Memory::Write<u16>(PSXAddr addr, u16 value);
template void Memory::Write<u32>(PSXAddr addr, u32 value);

void Memory::Write8(PSXAddr addr, u8 value) {
  Write<u8>(addr, value);
}

void Memory::Write16(PSXAddr addr, u16 value) {
  Write<u16>(addr, value);
}

void Memory::Write32(PSXAddr addr, u32 value) {
  Write<u32>(addr, value);
}


void Memory::Set(PSXAddr addr, int data, int length) {
  ::memset(reinterpret_cast<s8*>(&mem_user_[addr & 0x7fffff]), data, length);
}

}   // namespace psx
