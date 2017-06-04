#include "psf/psx/psx.h"
#include "psf/psx/hardware.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/dma.h"
#include "psf/spu/spu.h"

namespace psx {

////////////////////////////////////////////////////////////////////////
// IRQAccessor function definitions
////////////////////////////////////////////////////////////////////////

IRQAccessor::IRQAccessor(HardwareRegisters* p_hwregs)
  : irq_data_(nullptr), irq_mask_(nullptr), irq_is_masked_(nullptr) {
  rennyAssert(p_hwregs != nullptr);
  SetReferent(p_hwregs);
}

IRQAccessor::IRQAccessor(PSX* psx)
  : IRQAccessor(&psx->HwRegs()) {
}

void IRQAccessor::SetReferent(HardwareRegisters* p_hwregs) {
  rennyAssert(p_hwregs != nullptr);
  irq_data_ = reinterpret_cast<u32*>(&p_hwregs->hw_regs_[0x1070]);
  irq_mask_ = reinterpret_cast<u32*>(&p_hwregs->hw_regs_[0x1074]);
  irq_is_masked_ = reinterpret_cast<u32*>(&p_hwregs->hw_regs_[0x1078]);
}

////////////////////////////////////////////////////////////////////////
// HardwareRegisterAccessor function definitions
////////////////////////////////////////////////////////////////////////

HardwareRegisterAccessor::HardwareRegisterAccessor(HardwareRegisters* hw_regs)
  : regs_(hw_regs->hw_regs_) {
  rennyAssert(hw_regs != nullptr);
}

HardwareRegisterAccessor::HardwareRegisterAccessor(PSX* psx)
  : regs_(psx->HwRegs().hw_regs_) {
  rennyAssert(regs_ != nullptr);
}

////////////////////////////////////////////////////////////////////////
// HardwareRegisters function definitions
////////////////////////////////////////////////////////////////////////

HardwareRegisters::HardwareRegisters(PSX* composite)
  : Component(composite), IRQAccessor(this), version_(composite->version()) {
  ::memset(hw_regs_, 0, 0x3000);
}

////////////////////////////////////////////////////////////////
// Root Counter accessors
////////////////////////////////////////////////////////////////

template<typename T>
inline T HardwareRegisters::ReadRcnt(int index, int offset) const {
  switch (offset) {
  case 0:
    return static_cast<T>(RCnt().ReadCountEx(index));
  case 1:
    return static_cast<T>(RCnt().ReadModeEx(index));
  case 2:
    return static_cast<T>(RCnt().ReadTargetEx(index));
  default:
    rennyLogWarning("PSXHardware", "ReadRcnt(%d, %d): invalid parameters.",
                    index, offset);
    return 0;
  }
}

template<typename T>
void HardwareRegisters::WriteRcnt(int index, int offset, T value) {
  switch (offset) {
  case 0:
    RCnt().WriteCountEx(index, value);
    return;
  case 1:
    RCnt().WriteModeEx(index, value);
    return;
  case 2:
    RCnt().WriteTargetEx(index, value);
    return;
  default:
    rennyLogWarning("PSXHardware", "WriteRcnt(%d, %d, %d): invalid parameters.",
                    index, offset, value);
    return;
  }
}

////////////////////////////////////////////////////////////////
// SPU accessors
////////////////////////////////////////////////////////////////

template<typename T>
inline T HardwareRegisters::ReadSPURegister(PSXAddr addr) const {
  return static_cast<T>(Spu().ReadRegister(addr));
}

template<>
inline u32 HardwareRegisters::ReadSPURegister(PSXAddr addr) const {
  return Spu().ReadRegister(addr) | (Spu().ReadRegister(addr + 2) << 16);
}

template<typename T>
inline void HardwareRegisters::WriteSPURegister(PSXAddr addr, T value) {
  Spu().WriteRegister(addr, static_cast<u16>(value));
}

template<>
inline void HardwareRegisters::WriteSPURegister(PSXAddr addr, u32 value) {
  Spu().WriteRegister(addr, static_cast<u16>(value & 0xffff));
  Spu().WriteRegister(addr + 2, static_cast<u16>(value >> 16));
}

////////////////////////////////////////////////////////////////
// common accessors
////////////////////////////////////////////////////////////////

template<typename T>
inline T HardwareRegisters::Read(PSXAddr addr) const {
  if ((addr & 0xffffffc0) == 0x1f801100 ||
      (addr & 0xffffffc0) == 0x1f801480) { // root counters
    return ReadRcnt<T>((addr & 0x00000030) >> 4, (addr & 0xf) >> 2);
  }
  switch (addr) {
  case 0x1f801070:
    return static_cast<T>(irq());
  case 0x1f801074:
    return static_cast<T>(irq_mask());
  case 0x1f8010c8:
    rennyLogWarning("PSXHardware", "ReadDMA(core0) is not implemented.");
    return 0;
  case 0x1f8010f4:  // DICR
    return const_cast<HardwareRegisters*>(this)->Dma().DICR;
  case 0x1f801450:
    return (version_ == 0) ? 8 : 0;
  case 0x1f801548:
    rennyLogWarning("PSXHardware", "ReadDMA(core1) is not implemented.");
    return 0;
  default:
    break;
  }
  if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
    return ReadSPURegister<T>(addr);
  }
  if ((addr & 0xfffff800) == 0x1f900000 || (addr & 0xfffff800) == 0xbf900000) {
    rennyLogWarning("PSXHardware", "SPU2read is not implemented.");
  }
  rennyAssert((addr & 0x3fff) < 0x3000);
  return *reinterpret_cast<const T*>(hw_regs_ + (addr & 0x3fff));
}

u8 HardwareRegisters::Read8(PSXAddr addr) const {
  return Read<u8>(addr);
}

u16 HardwareRegisters::Read16(PSXAddr addr) const {
  return Read<u16>(addr);
}

u32 HardwareRegisters::Read32(PSXAddr addr) const {
  return Read<u32>(addr);
}

template<typename T>
inline void HardwareRegisters::Write(PSXAddr addr, T value) {
  if ((addr & 0xffffffc0) == 0x1f801100 ||
      (addr & 0xffffffc0) == 0x1f801480) { // root counters
    WriteRcnt<T>((addr & 0x00000030) >> 4, (addr & 0xf) >> 2,
                 static_cast<T>(value));
    return;
  }
  switch (addr) {
  case 0x1f801070:
    set_irq<T>(value);
    return;
  case 0x1f801074:
    set_irq_mask(static_cast<u32>(value));  // WARNING
    return;
  case 0x1f8010c8:
    Dma().Write(4, static_cast<T>(value));
    return;
  case 0x1f8010f4:  // DICR
    do {
      u32 tmp = (~value) & BFLIP32(Dma().DICR);
      Dma().DICR = BFLIP32(((tmp ^ value) & 0xffffff) ^ tmp);
    } while (false);
    return;
  case 0x1f801548:
    rennyLogWarning("PSXHardware", "WriteDMA(core1) is not implemented.");
    return;
  }
  if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
    WriteSPURegister(addr, value);
    return;
  }
  if ((addr & 0xfffffff1) == 0xbf8010c0) {
    rennyLogWarning("PSXHardware", "SPU2writeDMA4 is not implemented.");
  }
  if ((addr & 0xfffffff0) == 0xbf801500) {
    rennyLogWarning("PSXHardware", "SPU2writeDMA7 is not implemented.");
  }
  if ((addr & 0xfffff800) == 0x1f900000 || (addr & 0xfffff800) == 0xbf900000) {
    rennyLogWarning("PSXHardware", "SPU2write is not implemented.");
  }
  rennyAssert((addr & 0x3fff) < 0x3000);
  *reinterpret_cast<T*>(hw_regs_ + (addr & 0x3fff)) = BFLIP<T>(value);
}

void HardwareRegisters::Write8(PSXAddr addr, u8 value) {
  Write<u8>(addr, value);
}

void HardwareRegisters::Write16(PSXAddr addr, u16 value) {
  Write<u16>(addr, value);
}
void HardwareRegisters::Write32(PSXAddr addr, u32 value) {
  Write<u32>(addr, value);
}

}   // namespace PSX
