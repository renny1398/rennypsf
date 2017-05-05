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

namespace psx {

class Component;

// Mediator rather than PSX
class PSX {

  friend class Component;

public:
  PSX(u32 version = 1);

  void Init(bool enable_spu2 = false);
  void Reset();

  u32 version() const;

  void Terminate();

  Memory& Mem();
  mips::Processor& R3000a();
  mips::Interpreter& Interp();
  RootCounterManager& RCnt();
  const RootCounterManager& RCnt() const;

  mips::Disassembler& Disasm();

  mips::Registers& R3000ARegs();
  HardwareRegisters& HwRegs();
  DMA& Dma();
  BIOS& Bios();
  IOP& Iop();

  SPU::SPUBase& Spu();
  const SPU::SPUBase& Spu() const;

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

  u32& Ru32ref(PSXAddr addr);
  void* Rvptr(PSXAddr addr);

  uint32_t GetSamplingRate() const;
  void ChangeOutputSamplingRate(uint32_t rate);

  void SetRootDirectory(const PSF2Directory* root);

  unsigned int LoadELF(PSF2File* psf2irx);


private:
  u32 version_; // 1 or 2

  HardwareRegisters hw_regs_;
  Memory mem_;
  DMA dma_;
  RootCounterManager rcnt_;
  mips::Processor r3000a_;
  BIOS bios_;
  IOP iop_;

  mips::Interpreter interp_;
  mips::Disassembler disasm_;

  SPU::SPUBase spu_;
};

// BIOS Accessor Definitions (Reference)
inline u32& PSX::Ru32ref(PSXAddr addr) { return mem_.Rref<u32>(addr); }

// BIOS Accessor Definitions (Pointer)
inline void* PSX::Rvptr(PSXAddr addr) { return mem_.Rvptr(addr); }

}   // namespace PSX
