#include "psf/psx/psx.h"
#include "psf/psx/memory.h"
#include "psf/psx/hardware.h"
#include <memory.h>
#include "common/debug.h"

namespace PSX {

UserMemoryAccessor::UserMemoryAccessor(Composite* psx)
  : mem_(psx->Mem().mem_user_)
{}


Memory::Memory(Composite* composite)
  : Component(composite), HardwareRegisterAccessor(composite) {
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
  segment_LUT_[0x1f80] = psxHu8ptr(0);
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
T Memory::Read(PSXAddr addr)
{
  u32 segment = addr >> 16;
  if (segment == 0x1f80) {
    if (addr < 0x1f801000) {
      // read Scratch Pad
      return psxHval<T>(addr);
    }
    // read Hardware Registers
    return HwRegs().Read<T>(addr);
  } else if (Psx().version() == 2 && (segment & 0xfff0) == 0xbf90) {
    return HwRegs().Read<T>(addr);
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

template u8 Memory::Read<u8>(PSXAddr addr);
template u16 Memory::Read<u16>(PSXAddr addr);
template u32 Memory::Read<u32>(PSXAddr addr);


u8 Memory::Read8(PSXAddr addr) {
  return Read<u8>(addr);
}

u16 Memory::Read16(PSXAddr addr) {
  return Read<u16>(addr);
}

u32 Memory::Read32(PSXAddr addr) {
  return Read<u32>(addr);
}


template<typename T>
void Memory::Write(PSXAddr addr, T value)
{
  u32 segment = addr >> 16;
  if (segment == 0x1f80) {
    if (addr < 0x1f801000) {
      psxHref<T>(addr) = BFLIP(value);
      return;
    }
    HwRegs().Write(addr, value);
    return;
  } else if (Psx().version() == 2 && (segment == 0xbf80 || segment == 0xbf90)) {
    HwRegs().Write(addr, addr);
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

}   // namespace PSX
