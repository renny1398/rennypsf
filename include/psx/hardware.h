#pragma once

#include "common.h"


namespace PSX {

  class HardwareRegisters : public Component {

   public:
    HardwareRegisters(Composite* composite)
      : Component(composite) {}

    // void Init() {}
    void Reset();

    template<typename T> T Read(PSXAddr addr);
    template<typename T> void Write(PSXAddr addr, T value);

    u8 Read8(PSXAddr addr);
    u16 Read16(PSXAddr addr);
    u32 Read32(PSXAddr addr);

    void Write8(PSXAddr addr, u8 value);
    void Write16(PSXAddr addr, u16 value);
    void Write32(PSXAddr addr, u32 value);
  };


  template<typename T>
  inline T HardwareRegisters::Read(PSXAddr /*addr*/) {
    // return H_ref<T>(addr);   // dummy
    return 0;
  }
  template<typename T>
  inline void HardwareRegisters::Write(PSXAddr /*addr*/, T /*value*/) {
    // H_ref<T>(addr) = value;   // dummy
  }

  template<>
  inline u8 HardwareRegisters::Read(PSXAddr addr) {
    return Read8(addr);
  }
  template<>
  inline u16 HardwareRegisters::Read(PSXAddr addr) {
    return Read16(addr);
  }
  template<>
  inline u32 HardwareRegisters::Read(PSXAddr addr) {
    return Read32(addr);
  }

  template<>
  inline void HardwareRegisters::Write(PSXAddr addr, u8 value) {
    Write8(addr, value);
  }
  template<>
  inline void HardwareRegisters::Write(PSXAddr addr, u16 value) {
    Write16(addr, value);
  }
  template<>
  inline void HardwareRegisters::Write(PSXAddr addr, u32 value) {
    Write32(addr, value);
  }

}   // namespace PSX



