#include "psf/psx/psx.h"


namespace psx {


PSX::PSX(u32 version)
  : version_(version),
    hw_regs_(this), mem_(version, &hw_regs_), dma_(this),
    rcnt_(this), r3000a_(this), bios_(this), iop_(this),
    disasm_(this), interp_(this, &r3000a_, &bios_, &iop_, &disasm_),
    spu_(this) {
  r3000a_.SetRcntReferent(&rcnt_);
  r3000a_.SetBIOSReferent(&bios_);
}


void PSX::Init(bool /*enable_spu2*/) {
}


void PSX::Reset() {
  // r3000a_.Init();
  rcnt_.Init();
  spu_.Open();
}


u32 PSX::version() const {
  return version_;
}

/*
mips::InterpreterThread* PSX::Run() {
  Reset();
  bios_.Init();
  return interp_.Execute();
}

void PSX::Terminate() {
  interp_.Shutdown();
  spu_.Shutdown();
  mem_.Reset();
}
*/

Memory& PSX::Mem() {
  return mem_;
}

mips::Processor& PSX::R3000a() {
  return r3000a_;
}

mips::Interpreter& PSX::Interp() {
  return interp_;
}

RootCounterManager& PSX::RCnt() {
  return rcnt_;
}

const RootCounterManager& PSX::RCnt() const {
  return rcnt_;
}

mips::Disassembler& PSX::Disasm() {
  return disasm_;
}

mips::Registers& PSX::R3000ARegs() {
  return r3000a_.Regs;
}

HardwareRegisters& PSX::HwRegs() {
  return hw_regs_;
}

DMA& PSX::Dma() {
  return dma_;
}

BIOS& PSX::Bios() {
  return bios_;
}

IOP& PSX::Iop() {
  return iop_;
}

SPU::SPUBase& PSX::Spu() {
  return spu_;
}

const SPU::SPUBase& PSX::Spu() const {
  return spu_;
}


void PSX::InitMemory() {
  mem_.Init();
}


u8 PSX::ReadMemory8(PSXAddr addr) {
  return mem_.Read8(addr);
}

u16 PSX::ReadMemory16(PSXAddr addr) {
  return mem_.Read16(addr);
}

u32 PSX::ReadMemory32(PSXAddr addr) {
  return mem_.Read32(addr);
}


void PSX::WriteMemory8(PSXAddr addr, u8 value) {
  mem_.Write8(addr, value);
}

void PSX::WriteMemory16(PSXAddr addr, u16 value) {
  mem_.Write16(addr, value);
}

void PSX::WriteMemory32(PSXAddr addr, u32 value) {
  mem_.Write32(addr, value);
}


void PSX::Memcpy(PSXAddr dest, const void *src, int length) {
  mem_.Copy(dest, src, length);
}


/*
  void PSX::Memcpy(void *dest, PSXAddr src, int length) const {
    mem_.Copy(dest, src, length);
  }
*/

void PSX::Memset(PSXAddr dest, int data, int length) {
  Mem().Set(dest, data, length);
}


uint32_t PSX::GetSamplingRate() const {
  return spu_.GetCurrentSamplingRate();
}

void PSX::ChangeOutputSamplingRate(uint32_t rate) {
  spu_.ChangeOutputSamplingRate(rate);
}


void PSX::SetRootDirectory(const PSF2Directory* root) {
  iop_.SetRootDirectory(root);
}


unsigned int PSX::LoadELF(PSF2File *psf2irx) {
  return iop_.LoadELF(psf2irx);
}


// Component

Component::Component(PSX *composite)
  : psx_(*composite) {}

mips::Processor& Component::R3000a() { return psx_.R3000a(); }
mips::Interpreter& Component::Interp() { return psx_.Interp(); }
RootCounterManager& Component::RCnt() { return psx_.RCnt(); }
const RootCounterManager& Component::RCnt() const { return psx_.RCnt(); }

mips::Disassembler& Component::Disasm() { return psx_.Disasm(); }

mips::Registers& Component::R3000ARegs() { return psx_.R3000ARegs(); }
HardwareRegisters& Component::HwRegs() { return psx_.HwRegs(); }
DMA& Component::Dma() { return psx_.Dma(); }
BIOS& Component::Bios() { return psx_.Bios(); }
IOP& Component::Iop() { return psx_.Iop(); }

SPU::SPUBase& Component::Spu() { return psx_.Spu(); }
const SPU::SPUBase& Component::Spu() const { return psx_.Spu(); }


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

/*
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
*/

// BIOS Accessor Definitions (Reference)
u32& Component::psxRu32ref(PSXAddr addr) { return psx_.Ru32ref(addr); }

// BIOS Accessor Definitions (Pointer)
void* Component::psxRptr(PSXAddr addr) { return psx_.Rvptr(addr); }

}   // namespace psx
