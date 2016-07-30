#pragma once

// #include "common.h"

#include "r3000a.h"
#include "interpreter.h"
#include "rcnt.h"
#include "hardware.h"
#include "dma.h"
#include "bios.h"

#include "../spu/spu.h"


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


  R3000A::Processor& R3000a();
  R3000A::Interpreter& Interp();
  RootCounterManager& RCnt();

  R3000A::Registers& R3000ARegs();
  HardwareRegisters& HwRegs();
  DMA& Dma();
  BIOS& Bios();

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

  s8& S8M_ref(PSXAddr addr);
  u8& U8M_ref(PSXAddr addr);
  s16& S16M_ref(PSXAddr addr);
  u16& U16M_ref(PSXAddr addr);
  u32& U32M_ref(PSXAddr addr);

  void* M_ptr(PSXAddr addr);
  s8* S8M_ptr(PSXAddr addr);
  u8* U8M_ptr(PSXAddr addr);
  u16* U16M_ptr(PSXAddr addr);
  u32* U32M_ptr(PSXAddr addr);

  u8& U8H_ref(PSXAddr addr);
  u16& U16H_ref(PSXAddr addr);
  u32& U32H_ref(PSXAddr addr);

  u8* U8H_ptr(PSXAddr addr);
  u16* U16H_ptr(PSXAddr addr);
  u32* U32H_ptr(PSXAddr addr);

  u32& U32R_ref(PSXAddr addr);

  void* R_ptr(PSXAddr addr);

private:
  R3000A::Processor r3000a_;
  R3000A::Interpreter interp_;
  RootCounterManager rcnt_;
  SPU::SPUBase spu_;

  R3000A::Registers r3000a_regs_;
  Memory mem_;
  HardwareRegisters hw_regs_;
  DMA dma_;
  BIOS bios_;

  u32 version_; // 1 or 2
};




class PSX : public Composite {};


class PS2 : public Composite {

};


}   // namespace PSX
