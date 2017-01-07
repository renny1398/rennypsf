#pragma once

////////////////////////////////////////////////////////////////////////
// Short-typename Definitions
////////////////////////////////////////////////////////////////////////

typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;

////////////////////////////////////////////////////////////////////////
// BFLIP Macros
////////////////////////////////////////////////////////////////////////

template<typename T> inline T BFLIP(T x)
{
  return x;
}

#ifdef MSB_FIRST    // for big-endian architecture
template<> inline u16 BFLIP(u16 x)
{
  return ((x >> 8) & 0xff) | ((x & 0xff) << 8);
}
template<> inline u32 BFLIP(u32 x)
{
  return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}
//#else   // for little-endian architecture
#endif

inline u16 BFLIP16(u16 x) {
  return BFLIP(x);
}
inline u32 BFLIP32(u32 x) {
  return BFLIP(x);
}

////////////////////////////////////////////////////////////////////////
// Prototype Definitions
////////////////////////////////////////////////////////////////////////

namespace PSX {
typedef u32 PSXAddr;
typedef u32 PSXOffset;

class Composite;
class Component;

namespace R3000A {
class Processor;
class Interpreter;
class Recompiler;
class Registers;
class Disassembler;
}

class RootCounterManager;
class HardwareRegisters;
class DMA;
class BIOS;
class IOP;
}

namespace SPU {
struct SPUCore;
class SPUBase;
class SPU2;
}

////////////////////////////////////////////////////////////////////////
// PSX::Component Definitions
////////////////////////////////////////////////////////////////////////

namespace PSX {

class Component {

public:
  explicit Component(Composite* composite);

  Composite& Psx() { return psx_; }

  u8 ReadMemory8(PSXAddr addr);
  u16 ReadMemory16(PSXAddr addr);
  u32 ReadMemory32(PSXAddr addr);

  void WriteMemory8(PSXAddr addr, u8 value);
  void WriteMemory16(PSXAddr addr, u16 value);
  void WriteMemory32(PSXAddr addr, u32 value);

  R3000A::Processor& R3000a();
  R3000A::Interpreter& Interp();
  RootCounterManager& RCnt();
  const RootCounterManager& RCnt() const;

  R3000A::Disassembler& Disasm();

  R3000A::Registers& R3000ARegs();
  HardwareRegisters& HwRegs();
  DMA& Dma();
  BIOS& Bios();
  IOP& Iop();

  // TODO: replace SPU with SPUCore
  SPU::SPUBase& Spu();
  const SPU::SPUBase& Spu() const;
  SPU::SPU2& Spu2();

protected:

  // BIOS Accessors (Reference)
  u32& psxRu32ref(PSXAddr addr);

  // BIOS Accessors (Pointer)
  void* psxRptr(PSXAddr addr);

private:
  Composite& psx_;
};

}   // namespace PSX
