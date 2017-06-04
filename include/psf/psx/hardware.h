#pragma once
#include "common.h"
#include "common/debug.h"

namespace psx {

////////////////////////////////////////////////////////////////
// IRQ(Interrupt Request) Accessor Definitions
////////////////////////////////////////////////////////////////

class HardwareRegisters;

class IRQAccessor {
 public:
  IRQAccessor(HardwareRegisters* p_hwregs);
  IRQAccessor(PSX*);
  void SetReferent(HardwareRegisters* p_hwregs);
  u32 irq() const { return *irq_data_ & *irq_mask_; }
  u32 irq_data() const { return *irq_data_; }
  u32 irq_mask() const { return *irq_mask_; }
  template<typename T> void set_irq(T irq);
  void set_irq16(u16 irq);
  void set_irq32(u32 irq) { set_irq<u32>(irq); }
  void set_irq_data(u32 irq) { *irq_data_ |= BFLIP32(irq); }
  void set_irq_mask(u32 mask) { *irq_mask_ = mask; }
 private:
  u32* irq_data_;
  u32* irq_mask_;
  u32* irq_is_masked_;
};

template<typename T>
inline void IRQAccessor::set_irq(T irq) {
  *irq_data_ &= BFLIP32(static_cast<u32>(irq) & *irq_mask_);
}

template<>
inline void IRQAccessor::set_irq(u16 irq) {
  *irq_data_ &= BFLIP32(0xffff0000 | (irq & *irq_mask_));
}

inline void IRQAccessor::set_irq16(u16 irq) {
  set_irq<u16>(irq);
}

////////////////////////////////////////////////////////////////
// HardwareRegisters Class Definition
////////////////////////////////////////////////////////////////

// TODO: remove Component
class HardwareRegisters : public Component, public IRQAccessor {
 public:
  HardwareRegisters(PSX* composite);

  // void Init() {}
  void Reset();

  template<typename T> T Read(PSXAddr addr) const;
  template<typename T> void Write(PSXAddr addr, T value);

  // common accessor
  u8  Read8(PSXAddr addr) const;
  u16 Read16(PSXAddr addr) const;
  u32 Read32(PSXAddr addr) const;
  void Write8(PSXAddr addr, u8 value);
  void Write16(PSXAddr addr, u16 value);
  void Write32(PSXAddr addr, u32 value);

 private:
  // Root Counter accessor
  template<typename T> T ReadRcnt(int index, int offset) const;
  template<typename T> void WriteRcnt(int index, int offset, T value);
  // SPU accessor
  template<typename T> T ReadSPURegister(PSXAddr addr) const;
  template<typename T> void WriteSPURegister(PSXAddr addr, T value);

 private:
  u8 hw_regs_[0x3000];
  int version_;

  friend class IRQAccessor;
  friend class HardwareRegisterAccessor;
};

////////////////////////////////////////////////////////////////
// HardwareRegisters Accessor Class Definition
////////////////////////////////////////////////////////////////

class HardwareRegisterAccessor {

public:
  HardwareRegisterAccessor(HardwareRegisters* hw_regs);
  HardwareRegisterAccessor(PSX* psx);

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
