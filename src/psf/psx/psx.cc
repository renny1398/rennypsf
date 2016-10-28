#include "psf/psx/psx.h"


namespace PSX {


Composite::Composite(u32 version)
  : version_(version),
    r3000a_(this), interp_(this, &r3000a_), rcnt_(this), spu_(this),
    r3000a_regs_(), mem_(this), hw_regs_(this), dma_(this),
    bios_(this), iop_(this) {
}


void Composite::Init(bool /*enable_spu2*/) {
}


void Composite::Reset() {
  // r3000a_.Init();
  rcnt_.Init();
  spu_.Open();
}


u32 Composite::version() const {
  return version_;
}


R3000A::InterpreterThread* Composite::Run() {
  Reset();
  bios_.Init();
  return interp_.Execute();
}


void Composite::Terminate() {
  interp_.Shutdown();
  spu_.Shutdown();
  mem_.Reset();
}


Memory& Composite::Mem() {
  return mem_;
}

R3000A::Processor& Composite::R3000a() {
  return r3000a_;
}

R3000A::Interpreter& Composite::Interp() {
  return interp_;
}

RootCounterManager& Composite::RCnt() {
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

IOP& Composite::Iop() {
  return iop_;
}

SPU::SPUBase& Composite::Spu() {
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
  ::memset(mem_.Mvptr(dest), data, length);
}


uint32_t Composite::GetSamplingRate() const {
  return spu_.GetCurrentSamplingRate();
}

void Composite::ChangeOutputSamplingRate(uint32_t rate) {
  spu_.ChangeOutputSamplingRate(rate);
}


void Composite::SetRootDirectory(const PSF2Directory* root) {
  iop_.SetRootDirectory(root);
}


unsigned int Composite::LoadELF(PSF2File *psf2irx) {
  return iop_.LoadELF(psf2irx);
}


// Component

Component::Component(Composite *composite)
  : psx_(*composite) {}

R3000A::Processor& Component::R3000a() { return psx_.R3000a(); }
R3000A::Interpreter& Component::Interp() { return psx_.Interp(); }
RootCounterManager& Component::RCnt() { return psx_.RCnt(); }

R3000A::Registers& Component::R3000ARegs() { return psx_.R3000ARegs(); }
HardwareRegisters& Component::HwRegs() { return psx_.HwRegs(); }
DMA& Component::Dma() { return psx_.Dma(); }
BIOS& Component::Bios() { return psx_.Bios(); }
IOP& Component::Iop() { return psx_.Iop(); }

SPU::SPUBase& Component::Spu() { return psx_.Spu(); }


u8 Component::ReadMemory8(PSXAddr addr) {
  return psx_.ReadMemory8(addr);
}

u16 Component::ReadMemory16(PSXAddr addr) {
  return psx_.ReadMemory16(addr);
}

u32 Component::ReadMemory32(PSXAddr addr) {
  return psx_.ReadMemory32(addr);
}


void Component::WriteMemory8(PSXAddr addr, u8 value) {
  psx_.WriteMemory8(addr, value);
}

void Component::WriteMemory16(PSXAddr addr, u16 value) {
  psx_.WriteMemory16(addr, value);
}

void Component::WriteMemory32(PSXAddr addr, u32 value) {
  psx_.WriteMemory32(addr, value);
}


// User Memory Accessor Definitions (Value)
s8  Component::psxMs8val(PSXAddr addr)  const { return psx_.Ms8val(addr);  }
u8  Component::psxMu8val(PSXAddr addr)  const { return psx_.Mu8val(addr);  }
s16 Component::psxMs16val(PSXAddr addr) const { return psx_.Ms16val(addr); }
u16 Component::psxMu16val(PSXAddr addr) const { return psx_.Mu16val(addr); }
u32 Component::psxMu32val(PSXAddr addr) const { return psx_.Mu32ref(addr); }

// User Memory Accessor Definitions (Reference)
s8&  Component::psxMs8ref(PSXAddr addr)  { return psx_.Ms8ref(addr);  }
u8&  Component::psxMu8ref(PSXAddr addr)  { return psx_.Mu8ref(addr);  }
s16& Component::psxMs16ref(PSXAddr addr) { return psx_.Ms16ref(addr); }
u16& Component::psxMu16ref(PSXAddr addr) { return psx_.Mu16ref(addr); }
u32& Component::psxMu32ref(PSXAddr addr) { return psx_.Mu32ref(addr); }

// User Memory Accessor Definitions (Pointer)
void* Component::psxMptr(PSXAddr addr)    { return psx_.Mvptr(addr);    }
s8*   Component::psxMs8ptr(PSXAddr addr)  { return psx_.Ms8ptr(addr);  }
u8*   Component::psxMu8ptr(PSXAddr addr)  { return psx_.Mu8ptr(addr);  }
u16*  Component::psxMu16ptr(PSXAddr addr) { return psx_.Mu16ptr(addr); }
u32*  Component::psxMu32ptr(PSXAddr addr) { return psx_.Mu32ptr(addr); }

// Hardware Register Accessor Definitions (Value)
u8  Component::psxHu8val(PSXAddr addr) const  { return psx_.Hu8val(addr);  }
u16 Component::psxHu16val(PSXAddr addr) const { return psx_.Hu16val(addr); }
u32 Component::psxHu32val(PSXAddr addr) const { return psx_.Hu32val(addr); }

// Hardware Register Accessor Definitions (Reference)
u8&  Component::psxHu8ref(PSXAddr addr)  { return psx_.Hu8ref(addr);  }
u16& Component::psxHu16ref(PSXAddr addr) { return psx_.Hu16ref(addr); }
u32& Component::psxHu32ref(PSXAddr addr) { return psx_.Hu32ref(addr); }

// Hardware Register Accessor Definitions (Pointer)
u32* Component::psxHu32ptr(PSXAddr addr) { return psx_.Hu32ptr(addr); }

// BIOS Accessor Definitions (Reference)
u32& Component::psxRu32ref(PSXAddr addr) { return psx_.Ru32ref(addr); }

// BIOS Accessor Definitions (Pointer)
void* Component::psxRptr(PSXAddr addr) { return psx_.Rvptr(addr); }

}   // namespace PSX
