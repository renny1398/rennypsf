#include "psx.h"


namespace PSX {


Composite::Composite()
  : r3000a_(this), interp_(this, &r3000a_), rcnt_(this), spu_(this),
    r3000a_regs_(), mem_(this), hw_regs_(this), dma_(this),
    bios_(this) {}


void Composite::Init(bool /*enable_spu2*/) {
}


void Composite::Reset() {
  // r3000a_.Init();
  rcnt_.Init();
  spu_.Open();
}


R3000A::InterpreterThread* Composite::Run() {
  Reset();
  bios_.Init();
  return interp_.Execute();
}


void Composite::Terminate() {
  interp_.Shutdown();
  mem_.Reset();
}



R3000A::Processor& Composite::R3000a() {
  return r3000a_;
}

R3000A::Interpreter& Composite::Interp() {
  return interp_;
}

RootCounter& Composite::RCnt() {
  return rcnt_;
}

R3000A::Registers& Composite::R3000ARegs() {
  return r3000a_regs_;
}

HardwareRegisters& Composite::HwRegs() {
  return hw_regs_;
}

DMA& Composite::Dma() {
  return dma_;
}

BIOS& Composite::Bios() {
  return bios_;
}

SPU::SPU& Composite::Spu() {
  return spu_;
}


void Composite::InitMemory() {
  mem_.Init();
}


u8 Composite::ReadMemory8(PSXAddr addr) {
  return mem_.Read8(addr);
}

u16 Composite::ReadMemory16(PSXAddr addr) {
  return mem_.Read16(addr);
}

u32 Composite::ReadMemory32(PSXAddr addr) {
  return mem_.Read32(addr);
}


void Composite::WriteMemory8(PSXAddr addr, u8 value) {
  mem_.Write8(addr, value);
}

void Composite::WriteMemory16(PSXAddr addr, u16 value) {
  mem_.Write16(addr, value);
}

void Composite::WriteMemory32(PSXAddr addr, u32 value) {
  mem_.Write32(addr, value);
}


void Composite::Memcpy(PSXAddr dest, const void *src, int length) {
  mem_.Copy(dest, src, length);
}


/*
  void Composite::Memcpy(void *dest, PSXAddr src, int length) const {
    mem_.Copy(dest, src, length);
  }
*/

void Composite::Memset(PSXAddr dest, int data, int length) {
  ::memset(mem_.M_ptr(dest), data, length);
}



s8& Composite::S8M_ref(PSXAddr addr) {
  return mem_.S8M_ref(addr);
}

u8& Composite::U8M_ref(PSXAddr addr) {
  return mem_.U8M_ref(addr);
}

s16& Composite::S16M_ref(PSXAddr addr) {
  return mem_.S16M_ref(addr);
}

u16& Composite::U16M_ref(PSXAddr addr) {
  return mem_.U16M_ref(addr);
}

u32& Composite::U32M_ref(PSXAddr addr) {
  return mem_.U32M_ref(addr);
}


void* Composite::M_ptr(PSXAddr addr) {
  return mem_.M_ptr(addr);
}

s8* Composite::S8M_ptr(PSXAddr addr) {
  return mem_.S8M_ptr(addr);
}

u8* Composite::U8M_ptr(PSXAddr addr) {
  return mem_.U8M_ptr(addr);
}

u16* Composite::U16M_ptr(PSXAddr addr) {
  return mem_.U16M_ptr(addr);
}

u32* Composite::U32M_ptr(PSXAddr addr) {
  return mem_.U32M_ptr(addr);
}


u8& Composite::U8H_ref(PSXAddr addr) {
  return mem_.U8H_ref(addr);
}

u16& Composite::U16H_ref(PSXAddr addr) {
  return mem_.U16H_ref(addr);
}

u32& Composite::U32H_ref(PSXAddr addr) {
  return mem_.U32H_ref(addr);
}


u8* Composite::U8H_ptr(PSXAddr addr) {
  return mem_.U8H_ptr(addr);
}

u16* Composite::U16H_ptr(PSXAddr addr) {
  return mem_.U16H_ptr(addr);
}

u32* Composite::U32H_ptr(PSXAddr addr) {
  return mem_.U32H_ptr(addr);
}


u32& Composite::U32R_ref(PSXAddr addr) {
  return mem_.U32R_ref(addr);
}


void* Composite::R_ptr(PSXAddr addr) {
  return mem_.R_ptr(addr);
}



// Component

Component::Component(Composite *composite)
  : composite_(*composite) {}

R3000A::Processor& Component::R3000a() { return composite_.R3000a(); }
R3000A::Interpreter& Component::Interp() { return composite_.Interp(); }
RootCounter& Component::RCnt() { return composite_.RCnt(); }

R3000A::Registers& Component::R3000ARegs() { return composite_.R3000ARegs(); }
HardwareRegisters& Component::HwRegs() { return composite_.HwRegs(); }
DMA& Component::Dma() { return composite_.Dma(); }
BIOS& Component::Bios() { return composite_.Bios(); }


SPU::SPU& Component::Spu() { return composite_.Spu(); }


u8 Component::ReadMemory8(PSXAddr addr) {
  return composite_.ReadMemory8(addr);
}

u16 Component::ReadMemory16(PSXAddr addr) {
  return composite_.ReadMemory16(addr);
}

u32 Component::ReadMemory32(PSXAddr addr) {
  return composite_.ReadMemory32(addr);
}


void Component::WriteMemory8(PSXAddr addr, u8 value) {
  composite_.WriteMemory8(addr, value);
}

void Component::WriteMemory16(PSXAddr addr, u16 value) {
  composite_.WriteMemory16(addr, value);
}

void Component::WriteMemory32(PSXAddr addr, u32 value) {
  composite_.WriteMemory32(addr, value);
}


s8& Component::S8M_ref(PSXAddr addr) {
  return composite_.S8M_ref(addr);
}

u8& Component::U8M_ref(PSXAddr addr) {
  return composite_.U8M_ref(addr);
}

s16& Component::S16M_ref(PSXAddr addr) {
  return composite_.S16M_ref(addr);
}

u16& Component::U16M_ref(PSXAddr addr) {
  return composite_.U16M_ref(addr);
}

u32& Component::U32M_ref(PSXAddr addr) {
  return composite_.U32M_ref(addr);
}


void* Component::M_ptr(PSXAddr addr) {
  return composite_.M_ptr(addr);
}

s8* Component::S8M_ptr(PSXAddr addr) {
  return composite_.S8M_ptr(addr);
}

u8* Component::U8M_ptr(PSXAddr addr) {
  return composite_.U8M_ptr(addr);
}

u16* Component::U16M_ptr(PSXAddr addr) {
  return composite_.U16M_ptr(addr);
}

u32* Component::U32M_ptr(PSXAddr addr) {
  return composite_.U32M_ptr(addr);
}


u8& Component::U8H_ref(PSXAddr addr) {
  return composite_.U8H_ref(addr);
}

u16& Component::U16H_ref(PSXAddr addr) {
  return composite_.U16H_ref(addr);
}

u32& Component::U32H_ref(PSXAddr addr) {
  return composite_.U32H_ref(addr);
}


u8* Component::U8H_ptr(PSXAddr addr) {
  return composite_.U8H_ptr(addr);
}

u16* Component::U16H_ptr(PSXAddr addr) {
  return composite_.U16H_ptr(addr);
}

u32* Component::U32H_ptr(PSXAddr addr) {
  return composite_.U32H_ptr(addr);
}


u32& Component::U32R_ref(PSXAddr addr) {
  return composite_.U32R_ref(addr);
}


void* Component::R_ptr(PSXAddr addr) {
  return composite_.R_ptr(addr);
}

}   // namespace PSX
