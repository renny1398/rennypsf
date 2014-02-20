#pragma once


typedef char s8;
typedef unsigned char u8;
typedef short s16;
typedef unsigned short u16;
typedef int s32;
typedef unsigned int u32;
typedef long long s64;
typedef unsigned long long u64;


////////////////////////////////
// BFLIP Macros
////////////////////////////////

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


namespace PSX {
typedef u32 PSXAddr;
typedef u32 PSXOffset;

class Composite;
class Component;

namespace R3000A {
class Processor;
class Interpreter;
class Recompiler;
struct Registers;
}

class RootCounter;
class HardwareRegisters;
class DMA;
class BIOS;
}


namespace SPU {
class SPUBase;
struct SPUCore;
class SPU;
class SPU2;
}



namespace PSX {

class Component {

public:
  explicit Component(Composite* composite);

  Composite& Psx() { return composite_; }

  u8 ReadMemory8(PSXAddr addr);
  u16 ReadMemory16(PSXAddr addr);
  u32 ReadMemory32(PSXAddr addr);

  void WriteMemory8(PSXAddr addr, u8 value);
  void WriteMemory16(PSXAddr addr, u16 value);
  void WriteMemory32(PSXAddr addr, u32 value);

  virtual s8& S8M_ref(PSXAddr addr);
  virtual u8& U8M_ref(PSXAddr addr);
  virtual s16& S16M_ref(PSXAddr addr);
  virtual u16& U16M_ref(PSXAddr addr);
  virtual u32& U32M_ref(PSXAddr addr);

  virtual void* M_ptr(PSXAddr addr);
  virtual s8* S8M_ptr(PSXAddr addr);
  virtual u8* U8M_ptr(PSXAddr addr);
  virtual u16* U16M_ptr(PSXAddr addr);
  virtual u32* U32M_ptr(PSXAddr addr);

  virtual u8& U8H_ref(PSXAddr addr);
  virtual u16& U16H_ref(PSXAddr addr);
  virtual u32& U32H_ref(PSXAddr addr);

  virtual u8* U8H_ptr(PSXAddr addr);
  virtual u16* U16H_ptr(PSXAddr addr);
  virtual u32* U32H_ptr(PSXAddr addr);

  virtual u32& U32R_ref(PSXAddr addr);

  virtual void* R_ptr(PSXAddr addr);

  R3000A::Processor& R3000a();
  R3000A::Interpreter& Interp();
  RootCounter& RCnt();

  R3000A::Registers& R3000ARegs();
  HardwareRegisters& HwRegs();
  DMA& Dma();
  BIOS& Bios();


  // TODO: replace SPU with SPUCore
  SPU::SPU& Spu();
  SPU::SPU2& Spu2();

private:
  Composite& composite_;
};


}   // namespace PSX
