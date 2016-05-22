#include "psf/psx/hardware.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/dma.h"
#include "psf/spu/spu.h"
#include <wx/msgout.h>


namespace PSX {

  uint8_t HardwareRegisters::Read8(u32 addr)
  {
    return U8H_ref(addr);
  }


  uint16_t HardwareRegisters::Read16(u32 addr)
  {
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
      int index = (addr & 0x00000030) >> 4;
      int ofs = (addr & 0xf) >> 2;
      switch (ofs) {
      case 0:
        return static_cast<uint16_t>(RCnt().ReadCount(index));
      case 1:
        return static_cast<uint16_t>(RCnt().counters[index].mode);
      case 2:
        return static_cast<uint16_t>(RCnt().counters[index].target);
      }
      wxMessageOutputStderr().Printf(wxT("ERROR: invalid PSX memory address (0x%08x)"), addr);
      return 0;
    }
    if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
      return Spu().ReadRegister(addr);
    }
    if ((addr & 0xfffff800) == 0xbf900000) {
      wxMessageOutputStderr().Printf(wxT("WARNING: SPU2read16 is not implemented."));
    }
    return U16H_ref(addr);
  }


  u32 HardwareRegisters::Read32(u32 addr)
  {
    if ((addr & 0xffffffc0) == 0x1f801100) { // root counters
      int index = (addr & 0x00000030) >> 4;
      int ofs = (addr & 0xf) >> 2;
      switch (ofs) {
      case 0:
        return RCnt().ReadCount(index);
      case 1:
        return RCnt().counters[index].mode;
      case 2:
        return RCnt().counters[index].target;
      }
      wxMessageOutputStderr().Printf(wxT("ERROR: invalid PSX memory address (0x%08x)"), addr);
      return 0;
    }
    if ((addr & 0xfffff800) == 0xbf900000) {
      wxMessageOutputStderr().Printf(wxT("WARNING: SPU2read32 is not implemented."));
    }
    return U32H_ref(addr);
  }


  void HardwareRegisters::Write8(u32 addr, uint8_t value)
  {
    U8H_ref(addr) = value;
  }


  void HardwareRegisters::Write16(u32 addr, uint16_t value)
  {
    if (addr == 0x1f801070) {
      U16H_ref(0x1070) &= BFLIP16(U16H_ref(0x1074) & value);
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
        wxMessageOutputStderr().Printf(wxT("ERROR: invalid PSX memory address (0x%08x)"), addr);
        return;
      }
    }
    if ((addr & 0xfffffe00) == 0x1f801c00) {    // SPU
      Spu().WriteRegister(addr, value);
      return;
    }
    if ((addr & 0xfffff800) == 0xbf900000) {
      wxMessageOutputStderr().Printf(wxT("WARNING: SPU2write16 is not implemented."));
    }
    U16H_ref(addr) = BFLIP16(value);
  }


  void HardwareRegisters::Write32(u32 addr, u32 value)
  {
    if (addr == 0x1f801070) {
      U32H_ref(0x1070) &= BFLIP32(U32H_ref(0x1074) & value);
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
        wxMessageOutputStderr().Printf(wxT("ERROR: invalid PSX memory address (0x%08x)"), addr);
        return;
      }
    }
    if ((addr & 0xfffffff1) == 0xbf8010c0) {
      wxMessageOutputDebug().Printf(wxT("WARNING: SPU2writeDMA4 is not implemented."));
    }
    if ((addr & 0xfffffff0) == 0xbf801500) {
      wxMessageOutputDebug().Printf(wxT("WARNING: SPU2writeDMA7 is not implemented."));
    }
    if ((addr & 0xfffff800) == 0xbf900000) {
      wxMessageOutputStderr().Printf(wxT("WARNING: SPU2write32 is not implemented."));
    }

    U32H_ref(addr) = BFLIP32(value);
  }


}   // namespace PSX
