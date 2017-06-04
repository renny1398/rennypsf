#pragma once

#include "common.h"
#include "hardware.h"

namespace psx {

////////////////////////////////////////////////////////////////
// Memory Class Declaration
////////////////////////////////////////////////////////////////

class HardwareRegisters;

class Memory {
 public:
  Memory(int version, HardwareRegisters* hw_regs);

  void Init();
  void Reset();

  template<typename T> T Read(PSXAddr addr) const;
  template<typename T> void Write(PSXAddr addr, T value);

  u8 Read8(PSXAddr addr) const;
  u16 Read16(PSXAddr addr) const;
  u32 Read32(PSXAddr addr) const;

  void Write8(PSXAddr addr, u8 value);
  void Write16(PSXAddr addr, u16 value);
  void Write32(PSXAddr addr, u32 value);

  void Copy(PSXAddr dest, const void* src, int length);
  void Copy(void* dest, PSXAddr src, int length) const;

  void Set(PSXAddr addr, int data, int length);

 public:
  template<typename T> T& Rref(PSXAddr addr);
  void* Rvptr(PSXAddr addr);

 private:
  u8* segment_LUT_[0x10000];
  u8 mem_user_[0x200000];
  u8 mem_parallel_port_[0x10000];
  u8 mem_bios_[0x80000];

  const int version_;
  HardwareRegisters& hw_regs_;
  HardwareRegisterAccessor psxH_;

  friend class UserMemoryAccessor;
};


template<typename T>
inline T& Memory::Rref(PSXAddr addr) {
  return *reinterpret_cast<T*>(mem_bios_ + (addr & 0xffff));
}


inline void* Memory::Rvptr(PSXAddr addr) {
  return static_cast<void*>(mem_bios_ + (addr & 0xffff));
}

// inline u32 Memory::Bios()

////////////////////////////////////////////////////////////////
// Memory Accessors
////////////////////////////////////////////////////////////////

class UserMemoryAccessor {
 public:
  UserMemoryAccessor(Memory* mem);
  UserMemoryAccessor(PSX* psx);

  void* psxMptr(PSXAddr addr) {
    return mem_ + (addr & 0x1fffff);
  }
  s8* psxMs8ptr(PSXAddr addr) {
    return reinterpret_cast<s8*>(mem_ + (addr & 0x1fffff));
  }
  u8* psxMu8ptr(PSXAddr addr) {
    return mem_ + (addr & 0x1fffff);
  }
  s16* psxMs16ptr(PSXAddr addr) {
    return reinterpret_cast<s16*>(mem_ + (addr & 0x1fffff));
  }
  u16* psxMu16ptr(PSXAddr addr) {
    return reinterpret_cast<u16*>(mem_ + (addr & 0x1fffff));
  }
  /*
  s32* psxMs32ptr(PSXAddr addr) {
    return reinterpret_cast<s32*>(mem_ + (addr & 0x1fffff));
  }
  */
  u32* psxMu32ptr(PSXAddr addr) {
    return reinterpret_cast<u32*>(mem_ + (addr & 0x1fffff));
  }

  s8 psxMs8val(PSXAddr addr) const {
    return *const_cast<UserMemoryAccessor*>(this)->psxMs8ptr(addr);
  }
  u8 psxMu8val(PSXAddr addr) const {
    return *const_cast<UserMemoryAccessor*>(this)->psxMu8ptr(addr);
  }
  s16 psxMs16val(PSXAddr addr) const {
    return *const_cast<UserMemoryAccessor*>(this)->psxMs16ptr(addr);
  }
  u16 psxMu16val(PSXAddr addr) const {
    return *const_cast<UserMemoryAccessor*>(this)->psxMu16ptr(addr);
  }
  /*
  s32 psxMs32val(PSXAddr addr) const {
    return *const_cast<UserMemoryAccessor*>(this)->psxMs32ptr(addr);
  }
  */
  u32 psxMu32val(PSXAddr addr) const {
    return *const_cast<UserMemoryAccessor*>(this)->psxMu32ptr(addr);
  }

  s8& psxMs8ref(PSXAddr addr) { return *psxMs8ptr(addr); }
  u8& psxMu8ref(PSXAddr addr) { return *psxMu8ptr(addr); }
  s16& psxMs16ref(PSXAddr addr) { return *psxMs16ptr(addr); }
  u16& psxMu16ref(PSXAddr addr) { return *psxMu16ptr(addr); }
  // s32& psxMs32ref(PSXAddr addr) { return *psxMs32ptr(addr); }
  u32& psxMu32ref(PSXAddr addr) { return *psxMu32ptr(addr); }

 private:
  u8* const mem_;
};

// deprecated
class MemoryAccessor {
 public:
  MemoryAccessor(Memory* p_mem);
  MemoryAccessor(PSX* psx);

  s8 ReadMemory8s(PSXAddr addr) const {
    return static_cast<s8>(p_mem_->Read8(addr));
  }
  u8 ReadMemory8u(PSXAddr addr) const {
    return p_mem_->Read8(addr);
  }
  s16 ReadMemory16s(PSXAddr addr) const {
    return static_cast<s16>(p_mem_->Read16(addr));
  }
  u16 ReadMemory16u(PSXAddr addr) const {
    return p_mem_->Read16(addr);
  }
  // s32 ReadMemory32s(PSXAddr addr) const {
  //   return static_cast<s32>(mem_.Read32(addr));
  // }
  u32 ReadMemory32u(PSXAddr addr) const {
    return p_mem_->Read32(addr);
  }

  // void WriteMemory8s(PSXAddr addr, s8 value) {
  //   mem_.Write8(addr, static_cast<u8>(value));
  // }
  void WriteMemory8u(PSXAddr addr, u8 value) {
    p_mem_->Write8(addr, value);
  }
  // void WriteMemory16s(PSXAddr addr, s16 value) {
  //   mem_.Write16(addr, static_cast<u16>(value));
  // }
  void WriteMemory16u(PSXAddr addr, u16 value) {
    p_mem_->Write16(addr, value);
  }
  // void WriteMemory32s(PSXAddr addr, s32 value) {
  //   mem_.Write32(addr, static_cast<u32>(value));
  // }
  void WriteMemory32u(PSXAddr addr, u32 value) {
    p_mem_->Write32(addr, value);
  }

 private:
  Memory* const p_mem_;
};

} // namespace PSX
