#include "psf/psx/psx.h"
#include "psf/psx/hardware.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/dma.h"
#include "psf/spu/spu.h"
#include "common/debug.h"


namespace PSX {


IRQAccessor::IRQAccessor(HardwareRegisters* p_regs)
  : irq_data_(*reinterpret_cast<u32*>(&p_regs->hw_regs_[0x1070])),
    irq_mask_(*reinterpret_cast<u32*>(&p_regs->hw_regs_[0x1074]))
{}

IRQAccessor::IRQAccessor(Composite* psx)
  : irq_data_(*reinterpret_cast<u32*>(&psx->HwRegs().hw_regs_[0x1070])),
    irq_mask_(*reinterpret_cast<u32*>(&psx->HwRegs().hw_regs_[0x1074]))
{}

HardwareRegisterAccessor::HardwareRegisterAccessor(Composite* psx)
  :regs_(psx->HwRegs().hw_regs_)
{}


HardwareRegisters::HardwareRegisters(Composite* composite)
  : Component(composite), IRQAccessor(this) {
  ::memset(hw_regs_, 0, 0x3000);
}


uint8_t HardwareRegisters::Read8(u32 addr) {
  rennyAssert((addr & 0x3fff) < 0x3000);
  return hw_regs_[addr & 0x3fff];
}


uint16_t HardwareRegisters::Read16(u32 addr) {
  if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
    int index = (addr & 0x00000030) >> 4;
    int ofs = (addr & 0xf) >> 2;
    switch (ofs) {
    case 0:
      return static_cast<uint16_t>(RCnt().ReadCount(index));
    case 1:
      return static_cast<uint16_t>(RCnt().counters[index].mode_);
    case 2:
      return static_cast<uint16_t>(RCnt().counters[index].target_);
    }
    rennyLogWarning("PSXHardware", "Read16(0x%08x): invalid PSX memory address.", addr);
    return 0;
  }
  if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
    return Spu().ReadRegister(addr);
  }
  if ((addr & 0xfffff800) == 0xbf900000) {
    rennyLogWarning("PSXHardware", "SPU2read16 is not implemented.");
  }
  rennyAssert((addr & 0x3fff) < 0x3000);
  return *reinterpret_cast<u16*>(hw_regs_ + (addr & 0x3fff));
}


u32 HardwareRegisters::Read32(u32 addr) {
  if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
    int index = (addr & 0x00000030) >> 4;
    int ofs = (addr & 0xf) >> 2;
    switch (ofs) {
    case 0:
      return RCnt().ReadCount(index);
    case 1:
      return RCnt().counters[index].mode_;
    case 2:
      return RCnt().counters[index].target_;
    }
    rennyLogWarning("PSXHardware", "Read32(0x%08x): invalid PSX memory address.", addr);
    return 0;
  }
  if ((addr & 0xfffff800) == 0xbf900000) {
    rennyLogWarning("PSXHardware", "SPU2read32 is not implemented.");
  } else if (addr == 0xbf920344) {
    rennyLogDebug("PSXHardware", "PSX Hardware Registers: return 0x80808080.");
    return 0x80808080;
  }
  rennyAssert((addr & 0x3fff) < 0x3000);
  return *reinterpret_cast<u32*>(hw_regs_ + (addr & 0x3fff));
}


void HardwareRegisters::Write8(u32 addr, uint8_t value) {
  rennyAssert((addr & 0x3fff) < 0x3000);
  hw_regs_[addr & 0x3fff] = value;
}


void HardwareRegisters::Write16(u32 addr, uint16_t value) {
  if (addr == 0x1f801070) {
    set_irq16(value);
    return;
  }
  if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
    int index = (addr & 0x00000030) >> 4;
    int ofs = (addr & 0xf) >> 2;
    switch (ofs) {
    case 0:
      RCnt().WriteCount(index, value);
      return;
    case 1:
      RCnt().WriteMode(index, value);
      return;
    case 2:
      RCnt().WriteTarget(index, value);
      return;
    default:
      rennyLogWarning("PSXHardware", "Write16(0x%08x): invalid PSX memory address.", addr);
      return;
    }
  }
  if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
    Spu().WriteRegister(addr, value);
    return;
  }
  if ((addr & 0xfffff800) == 0xbf900000) {
    rennyLogWarning("PSXHardware", "SPU2write16 is not implemented.");
  }
  rennyAssert((addr & 0x3fff) < 0x3000);
  *reinterpret_cast<u16*>(hw_regs_ + (addr & 0x3fff)) = BFLIP16(value);
}


void HardwareRegisters::Write32(u32 addr, u32 value) {
  if (addr == 0x1f801070) {
    set_irq32(value);
    return;
  }
  if (addr == 0x1f8010c8) {
    Dma().Write(4, value);
    return;
  }
  if (addr == 0x1f8010f4) {   // DICR
    u32 tmp = (~value) & BFLIP32(Dma().DICR);
    Dma().DICR = BFLIP32(((tmp ^ value) & 0xffffff) ^ tmp);
    return;
  }
  if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
    int index = (addr & 0x00000030) >> 4;
    int ofs = (addr & 0xf) >> 2;
    switch (ofs) {
    case 0:
      RCnt().WriteCount(index, value);
      return;
    case 1:
      RCnt().WriteMode(index, value);
      return;
    case 2:
      RCnt().WriteTarget(index, value);
      return;
    default:
      rennyLogWarning("PSXHardware", "Write32(0x%08x): invalid PSX memory address.", addr);
      return;
    }
  }
  if ((addr & 0xfffffff1) == 0xbf8010c0) {
    rennyLogWarning("PSXHardware", "SPU2writeDMA4 is not implemented.");
  }
  if ((addr & 0xfffffff0) == 0xbf801500) {
    rennyLogWarning("PSXHardware", "SPU2writeDMA7 is not implemented.");
  }
  if ((addr & 0xfffff800) == 0xbf900000) {
    rennyLogWarning("PSXHardware", "SPU2write32 is not implemented.");
  }
  rennyAssert((addr & 0x3fff) < 0x3000);
  *reinterpret_cast<u32*>(hw_regs_ + (addr & 0x3fff)) = BFLIP32(value);
}


}   // namespace PSX
