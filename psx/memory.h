#pragma once

#include "common.h"


namespace PSX {


  class Memory : public Component {

   public:
    Memory(Composite* composite)
      : Component(composite) {}

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
    template<typename T> T& M_ref(PSXAddr addr);
    void* M_ptr(PSXAddr addr);

    template<typename T> T& H_ref(PSXAddr addr);
    void* H_ptr(PSXAddr addr);

    template<typename T> T& R_ref(PSXAddr addr);
    virtual void* R_ptr(PSXAddr addr);

    virtual u8& U8M_ref(PSXAddr addr);
    virtual u16& U16M_ref(PSXAddr addr);
    virtual u32& U32M_ref(PSXAddr addr);

    virtual s8* S8M_ptr(PSXAddr addr);
    virtual u8* U8M_ptr(PSXAddr addr);
    virtual u16* U16M_ptr(PSXAddr addr);
    virtual u32* U32M_ptr(PSXAddr addr);

    virtual u8& U8H_ref(PSXAddr addr);
    virtual u16& U16H_ref(PSXAddr addr);
    virtual u32& U32H_ref(PSXAddr addr);

    virtual u8* U8H_ptr(PSXAddr addr);
    virtual u16* U16H_ptr(PSXAddr addr);
    virtual u32* U32H_ptr(PSXAddr addr);

    virtual u32& U32R_ref(PSXAddr addr);

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
  inline T& Memory::M_ref(PSXAddr addr) {
      return *reinterpret_cast<T*>(mem_user_ + (addr & 0x1fffff));
  }

  inline void* Memory::M_ptr(PSXAddr addr) {
      return static_cast<void*>(mem_user_ + (addr & 0x1fffff));
  }

  /*
  template<typename T>
  inline T Hwm(u32 addr) {
      return BFLIP( *pointer_cast<T*>(PSX::Memory::memHardware + (addr & 0x3fff)) );
  }
  */

  template<typename T>
  inline T& Memory::H_ref(PSXAddr addr) {
    return *reinterpret_cast<T*>(mem_hardware_registers_ + (addr & 0x3fff));
  }

  inline void* Memory::H_ptr(PSXAddr addr) {
    return static_cast<void*>(mem_hardware_registers_ + (addr & 0x3fff));
  }


  template<typename T>
  inline T& Memory::R_ref(PSXAddr addr) {
      return *reinterpret_cast<T*>(mem_bios_ + (addr & 0xffff));
  }

  inline void* Memory::R_ptr(PSXAddr addr) {
    return static_cast<void*>(mem_bios_ + (addr & 0xffff));
  }


  inline u8& Memory::U8M_ref(PSXAddr addr) {
      return M_ref<u8>(addr);
  }

  inline u16& Memory::U16M_ref(PSXAddr addr) {
      return M_ref<u16>(addr);
  }

  inline u32& Memory::U32M_ref(PSXAddr addr) {
      return M_ref<u32>(addr);
  }


  inline s8* Memory::S8M_ptr(PSXAddr addr) {
      return static_cast<s8*>(M_ptr(addr));
  }

  inline u8* Memory::U8M_ptr(PSXAddr addr) {
    return static_cast<u8*>(M_ptr(addr));
  }

  inline u16* Memory::U16M_ptr(PSXAddr addr) {
      return static_cast<u16*>(M_ptr(addr));
  }

  inline u32* Memory::U32M_ptr(PSXAddr addr) {
      return static_cast<u32*>(M_ptr(addr));
  }


  inline u8& Memory::U8H_ref(PSXAddr addr) {
      return H_ref<u8>(addr);
  }

  inline u16& Memory::U16H_ref(PSXAddr addr) {
      return H_ref<u16>(addr);
  }

  inline u32& Memory::U32H_ref(PSXAddr addr) {
      return H_ref<u32>(addr);
  }


  inline u8* Memory::U8H_ptr(PSXAddr addr) {
    return static_cast<u8*>(H_ptr(addr));
  }

  inline u16* Memory::U16H_ptr(PSXAddr addr) {
    return static_cast<u16*>(H_ptr(addr));
  }

  inline u32* Memory::U32H_ptr(PSXAddr addr) {
    return static_cast<u32*>(H_ptr(addr));
  }


  inline u32& Memory::U32R_ref(PSXAddr addr) {
      return R_ref<u32>(addr);
  }

}
