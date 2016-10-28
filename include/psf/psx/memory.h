#pragma once

#include "common.h"


namespace PSX {


class Memory : public Component {

 public:
  Memory(Composite* composite);

  void Init();
  void Reset();

  template<typename T> T Read(PSXAddr addr);
  template<typename T> void Write(PSXAddr addr, T value);

  u8 Read8(PSXAddr addr);
  u16 Read16(PSXAddr addr);
  u32 Read32(PSXAddr addr);

  void Write8(PSXAddr addr, u8 value);
  void Write16(PSXAddr addr, u16 value);
  void Write32(PSXAddr addr, u32 value);

  void Copy(PSXAddr dest, const void* src, int length);
  void Copy(void* dest, PSXAddr src, int length) const;

 public:
  template<typename T> T Mval(PSXAddr addr) const;
  template<typename T> T& Mref(PSXAddr addr);
  void* Mvptr(PSXAddr addr);
  template<typename T> T* Mptr(PSXAddr addr);

  template<typename T> T Hval(PSXAddr addr) const;
  template<typename T> T& Href(PSXAddr addr);
  void* Hvptr(PSXAddr addr);
  template<typename T> T* Hptr(PSXAddr addr);

  template<typename T> T& Rref(PSXAddr addr);
  void* Rvptr(PSXAddr addr);

 private:
  u8* segment_LUT_[0x10000];
  u8 mem_user_[0x200000];
  u8 mem_parallel_port_[0x10000];
  u8 mem_hardware_registers_[0x3000];
  u8 mem_bios_[0x80000];
};


////////////////////////////////////////////////////////////////
// Memory access macro
////////////////////////////////////////////////////////////////

template<typename T>
inline T Memory::Mval(PSXAddr addr) const {
  return *reinterpret_cast<const T*>(mem_user_ + (addr & 0x1fffff));
}

template<typename T>
inline T& Memory::Mref(PSXAddr addr) {
  return *reinterpret_cast<T*>(mem_user_ + (addr & 0x1fffff));
}

inline void* Memory::Mvptr(PSXAddr addr) {
  return static_cast<void*>(mem_user_ + (addr & 0x1fffff));
}

template<typename T>
inline T* Memory::Mptr(PSXAddr addr) {
  return reinterpret_cast<T*>(mem_user_ + (addr & 0x1fffff));
}


template<typename T>
inline T Memory::Hval(PSXAddr addr) const {
  return *reinterpret_cast<const T*>(mem_hardware_registers_ + (addr & 0x3fff));
}

template<typename T>
inline T& Memory::Href(PSXAddr addr) {
  return *reinterpret_cast<T*>(mem_hardware_registers_ + (addr & 0x3fff));
}

inline void* Memory::Hvptr(PSXAddr addr) {
  return static_cast<void*>(mem_hardware_registers_ + (addr & 0x3fff));
}

template<typename T>
inline T* Memory::Hptr(PSXAddr addr) {
  return reinterpret_cast<T*>(mem_hardware_registers_ + (addr & 0x3fff));
}


template<typename T>
inline T& Memory::Rref(PSXAddr addr) {
    return *reinterpret_cast<T*>(mem_bios_ + (addr & 0xffff));
}


inline void* Memory::Rvptr(PSXAddr addr) {
  return static_cast<void*>(mem_bios_ + (addr & 0xffff));
}

// inline u32 Memory::Bios()

}
