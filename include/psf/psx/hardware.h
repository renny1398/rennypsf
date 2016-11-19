#pragma once
#include "common.h"

namespace PSX {

////////////////////////////////////////////////////////////////
// IRQ(Interrupt Request) Accessor Definitions
////////////////////////////////////////////////////////////////

class HardwareRegisters;

class IRQAccessor {

public:
  IRQAccessor(HardwareRegisters*);
  IRQAccessor(Composite*);
  u32 irq() const { return irq_data_ & irq_mask_; }
  u32 irq_data() const { return irq_data_; }
  u32 irq_mask() const { return irq_mask_; }
  void set_irq16(u16 irq) {
    irq_data_ &= 0xffff0000 | BFLIP16(static_cast<u16>(irq_mask_) & irq);
  }
  void set_irq32(u32 irq) { irq_data_ &= BFLIP32(irq_mask_ & irq); }
  void set_irq_data(u32 irq) { irq_data_ |= BFLIP32(irq); }
  void set_irq_mask(u32 mask) { irq_mask_ = mask; }

private:
  u32& irq_data_;
  u32& irq_mask_;
};

////////////////////////////////////////////////////////////////
// HardwareRegisters Class Definition
////////////////////////////////////////////////////////////////

class HardwareRegisters : public Component, private IRQAccessor {

 public:
  HardwareRegisters(Composite* composite);

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

 private:
  u8 hw_regs_[0x3000];

  friend class IRQAccessor;
  friend class HardwareRegisterAccessor;
};

template<typename T>
inline T HardwareRegisters::Read(PSXAddr) {
  // return H_ref<T>(addr);   // dummy
  return 0;
}
template<typename T>
inline void HardwareRegisters::Write(PSXAddr, T) {
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

////////////////////////////////////////////////////////////////
// HardwareRegisters Accessor Class Definition
////////////////////////////////////////////////////////////////

class HardwareRegisterAccessor {

public:
  // HardwareRegisterAccessor(HardwareRegisters* hw_regs) : regs_(hw_regs->hw_regs_) {}
  HardwareRegisterAccessor(Composite* psx);

  u8* psxHu8ptr(PSXAddr addr) {
    return regs_ + (addr & 0x3fff);
  }
  u16* psxHu16ptr(PSXAddr addr) {
    return reinterpret_cast<u16*>(regs_ + (addr & 0x3fff));
  }
  u32* psxHu32ptr(PSXAddr addr) {
    return reinterpret_cast<u32*>(regs_ + (addr & 0x3fff));
  }

  template<typename T> T psxHval(PSXAddr addr) const {
    return *reinterpret_cast<const T*>(regs_ + (addr & 0x3fff));
  }
  u8 psxHu8val(PSXAddr addr) const {
    return *const_cast<HardwareRegisterAccessor*>(this)->psxHu8ptr(addr);
  }
  u16 psxHu16val(PSXAddr addr) const {
    return *const_cast<HardwareRegisterAccessor*>(this)->psxHu16ptr(addr);
  }
  u32 psxHu32val(PSXAddr addr) const {
    return *const_cast<HardwareRegisterAccessor*>(this)->psxHu32ptr(addr);
  }

  template<typename T> T& psxHref(PSXAddr addr) {
    return *reinterpret_cast<T*>(regs_ + (addr & 0x3fff));
  }
  u8& psxHu8ref(PSXAddr addr) { return *psxHu8ptr(addr); }
  u16& psxHu16ref(PSXAddr addr) { return *psxHu16ptr(addr); }
  u32& psxHu32ref(PSXAddr addr) { return *psxHu32ptr(addr); }

private:
  u8* const regs_;
};

}   // namespace PSX
