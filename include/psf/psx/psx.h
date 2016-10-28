#pragma once

// #include "common.h"

#include "r3000a.h"
#include "interpreter.h"
#include "rcnt.h"
#include "hardware.h"
#include "dma.h"
#include "bios.h"
#include "iop.h"

#include "../spu/spu.h"


class PSF2Entry;
class PSF2Directory;

namespace PSX {

class Component;

// Mediator rather than Composite
class Composite {

  friend class Component;

public:
  Composite(u32 version = 1);

  void Init(bool enable_spu2 = false);
  void Reset();

  u32 version() const;

  R3000A::InterpreterThread *Run();
  void Terminate();

  Memory& Mem();
  R3000A::Processor& R3000a();
  R3000A::Interpreter& Interp();
  RootCounterManager& RCnt();

  R3000A::Registers& R3000ARegs();
  HardwareRegisters& HwRegs();
  DMA& Dma();
  BIOS& Bios();
  IOP& Iop();

  SPU::SPUBase& Spu();

  void InitMemory();

  u8 ReadMemory8(PSXAddr addr);
  u16 ReadMemory16(PSXAddr addr);
  u32 ReadMemory32(PSXAddr addr);

  void WriteMemory8(PSXAddr addr, u8 value);
  void WriteMemory16(PSXAddr addr, u16 value);
  void WriteMemory32(PSXAddr addr, u32 value);

  void Memcpy(PSXAddr dest, const void* src, int length);
  void Memcpy(void* dest, PSXAddr src, int length) const;
  void Memset(PSXAddr dest, int data, int length);

  s8  Ms8val(PSXAddr addr) const;
  u8  Mu8val(PSXAddr addr) const;
  s16 Ms16val(PSXAddr addr) const;
  u16 Mu16val(PSXAddr addr) const;
  u32 Mu32val(PSXAddr addr) const;

  s8& Ms8ref(PSXAddr addr);
  u8& Mu8ref(PSXAddr addr);
  s16& Ms16ref(PSXAddr addr);
  u16& Mu16ref(PSXAddr addr);
  u32& Mu32ref(PSXAddr addr);

  void* Mvptr(PSXAddr addr);
  s8* Ms8ptr(PSXAddr addr);
  u8* Mu8ptr(PSXAddr addr);
  u16* Mu16ptr(PSXAddr addr);
  u32* Mu32ptr(PSXAddr addr);

  u8  Hu8val(PSXAddr addr) const;
  u16 Hu16val(PSXAddr addr) const;
  u32 Hu32val(PSXAddr addr) const;

  u8& Hu8ref(PSXAddr addr);
  u16& Hu16ref(PSXAddr addr);
  u32& Hu32ref(PSXAddr addr);

  u8* Hu8ptr(PSXAddr addr);
  u16* Hu16ptr(PSXAddr addr);
  u32* Hu32ptr(PSXAddr addr);

  u32& Ru32ref(PSXAddr addr);

  void* Rvptr(PSXAddr addr);


  uint32_t GetSamplingRate() const;
  void ChangeOutputSamplingRate(uint32_t rate);

  void SetRootDirectory(const PSF2Directory* root);

  unsigned int LoadELF(PSF2File* psf2irx);


private:
  u32 version_; // 1 or 2

  R3000A::Processor r3000a_;
  R3000A::Interpreter interp_;
  RootCounterManager rcnt_;
  SPU::SPUBase spu_;

  R3000A::Registers r3000a_regs_;
  Memory mem_;
  HardwareRegisters hw_regs_;
  DMA dma_;
  BIOS bios_;
  IOP iop_;

};

// User Memory Accessor Definitions (Value)
inline s8  Composite::Ms8val(PSXAddr addr)  const { return mem_.Mval<s8>(addr);  }
inline u8  Composite::Mu8val(PSXAddr addr)  const { return mem_.Mval<u8>(addr);  }
inline s16 Composite::Ms16val(PSXAddr addr) const { return mem_.Mval<s16>(addr); }
inline u16 Composite::Mu16val(PSXAddr addr) const { return mem_.Mval<u16>(addr); }
inline u32 Composite::Mu32val(PSXAddr addr) const { return mem_.Mval<u32>(addr); }

// User Memory Accessor Definitions (Reference)
inline s8&  Composite::Ms8ref(PSXAddr addr)  { return mem_.Mref<s8>(addr);  }
inline u8&  Composite::Mu8ref(PSXAddr addr)  { return mem_.Mref<u8>(addr);  }
inline s16& Composite::Ms16ref(PSXAddr addr) { return mem_.Mref<s16>(addr); }
inline u16& Composite::Mu16ref(PSXAddr addr) { return mem_.Mref<u16>(addr); }
inline u32& Composite::Mu32ref(PSXAddr addr) { return mem_.Mref<u32>(addr); }

// User Memory Accessor Definitions (Pointer)
inline void* Composite::Mvptr(PSXAddr addr)   { return mem_.Mvptr(addr);     }
inline s8*   Composite::Ms8ptr(PSXAddr addr)  { return mem_.Mptr<s8>(addr);  }
inline u8*   Composite::Mu8ptr(PSXAddr addr)  { return mem_.Mptr<u8>(addr);  }
inline u16*  Composite::Mu16ptr(PSXAddr addr) { return mem_.Mptr<u16>(addr); }
inline u32*  Composite::Mu32ptr(PSXAddr addr) { return mem_.Mptr<u32>(addr); }

// Hardware Register Accessor Definitions (Value)
inline u8  Composite::Hu8val(PSXAddr addr) const  { return mem_.Hval<u8>(addr);  }
inline u16 Composite::Hu16val(PSXAddr addr) const { return mem_.Hval<u16>(addr); }
inline u32 Composite::Hu32val(PSXAddr addr) const { return mem_.Hval<u32>(addr); }

// Hardware Register Accessor Definitions (Reference)
inline u8&  Composite::Hu8ref(PSXAddr addr)  { return mem_.Href<u8>(addr);  }
inline u16& Composite::Hu16ref(PSXAddr addr) { return mem_.Href<u16>(addr); }
inline u32& Composite::Hu32ref(PSXAddr addr) { return mem_.Href<u32>(addr); }

// Hardware Register Accessor Definitions (Pointer)
inline u8*  Composite::Hu8ptr(PSXAddr addr)  { return mem_.Hptr<u8>(addr);  }
inline u16* Composite::Hu16ptr(PSXAddr addr) { return mem_.Hptr<u16>(addr); }
inline u32* Composite::Hu32ptr(PSXAddr addr) { return mem_.Hptr<u32>(addr); }

// BIOS Accessor Definitions (Reference)
inline u32& Composite::Ru32ref(PSXAddr addr) { return mem_.Rref<u32>(addr); }

// BIOS Accessor Definitions (Pointer)
inline void* Composite::Rvptr(PSXAddr addr) { return mem_.Rvptr(addr); }

}   // namespace PSX
