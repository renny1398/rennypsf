#include "psf/psx/r3000a.h"
#include "psf/psx/psx.h"
#include "psf/psx/memory.h"
#include "psf/psx/bios.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/disassembler.h"
#include "psf/psx/interpreter.h"
#include "psf/spu/spu.h"
#include "common/SoundFormat.h"

#include "common/debug.h"
#include <cstring>


namespace psx {

const char *strGPR[35] = {
  "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
  "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
  "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
  "T8", "T9", "K0", "K1", "GP", "SP", "FP", "RA",
  "HI", "LO", "PC"
};


namespace mips {

// for delay_load
//  uint32_t delayed_load_target;
//  uint32_t delayed_load_value;


GeneralPurposeRegisters::GeneralPurposeRegisters()
  : ZR(R[GPR_ZR]), AT(R[GPR_AT]), V0(R[GPR_V0]), V1(R[GPR_V1]),
    A0(R[GPR_A0]), A1(R[GPR_A1]), A2(R[GPR_A2]), A3(R[GPR_A3]),
    T0(R[GPR_T0]), T1(R[GPR_T1]), T2(R[GPR_T2]), T3(R[GPR_T3]),
    T4(R[GPR_T4]), T5(R[GPR_T5]), T6(R[GPR_T6]), T7(R[GPR_T7]),
    S0(R[GPR_S0]), S1(R[GPR_S1]), S2(R[GPR_S2]), S3(R[GPR_S3]),
    S4(R[GPR_S4]), S5(R[GPR_S5]), S6(R[GPR_S6]), S7(R[GPR_S7]),
    T8(R[GPR_T8]), T9(R[GPR_T9]), K0(R[GPR_K0]), K1(R[GPR_K1]),
    GP(R[GPR_GP]), SP(R[GPR_SP]), FP(R[GPR_FP]), RA(R[GPR_RA]),
    HI(R[GPR_HI]), LO(R[GPR_LO]), PC(R[GPR_PC]) {
  Reset();
}

GeneralPurposeRegisters::GeneralPurposeRegisters(const GeneralPurposeRegisters &src)
  : ZR(R[GPR_ZR]), AT(R[GPR_AT]), V0(R[GPR_V0]), V1(R[GPR_V1]),
    A0(R[GPR_A0]), A1(R[GPR_A1]), A2(R[GPR_A2]), A3(R[GPR_A3]),
    T0(R[GPR_T0]), T1(R[GPR_T1]), T2(R[GPR_T2]), T3(R[GPR_T3]),
    T4(R[GPR_T4]), T5(R[GPR_T5]), T6(R[GPR_T6]), T7(R[GPR_T7]),
    S0(R[GPR_S0]), S1(R[GPR_S1]), S2(R[GPR_S2]), S3(R[GPR_S3]),
    S4(R[GPR_S4]), S5(R[GPR_S5]), S6(R[GPR_S6]), S7(R[GPR_S7]),
    T8(R[GPR_T8]), T9(R[GPR_T9]), K0(R[GPR_K0]), K1(R[GPR_K1]),
    GP(R[GPR_GP]), SP(R[GPR_SP]), FP(R[GPR_FP]), RA(R[GPR_RA]),
    HI(R[GPR_HI]), LO(R[GPR_LO]), PC(R[GPR_PC]) {
  Set(src);
}

void GeneralPurposeRegisters::Set(const GeneralPurposeRegisters &src) {
  ::memcpy(R, src.R, sizeof(R));
}

void GeneralPurposeRegisters::Reset() {
  ::memset(this, 0, sizeof(R));
  PC = 0xbfc00000;    // start in bootstrap
}

u32& GeneralPurposeRegisters::operator()(u32 i) {
  return R[i];
}

const u32& GeneralPurposeRegisters::operator() (u32 i) const {
  return R[i];
}


Cop0Registers::Cop0Registers()
  : INDX(R[COP0_INDX]), RAND(R[COP0_RAND]), TLBL(R[COP0_TLBL]), BPC(R[COP0_BPC]),
    CTXT(R[COP0_CTXT]), BDA(R[COP0_BDA]), PIDMASK(R[COP0_PIDMASK]), DCIC(R[COP0_DCIC]),
    BADV(R[COP0_BADV]), BDAM(R[COP0_BDAM]), TLBH(R[COP0_TLBH]), BPCM(R[COP0_BPCM]),
    SR(R[COP0_SR]), CAUSE(R[COP0_CAUSE]), EPC(R[COP0_EPC]), PRID(R[COP0_PRID]),
    ERREG(R[COP0_ERREG]) {
  Reset();
}

void Cop0Registers::Reset() {
  ::memset(this, 0, sizeof(R));
  SR   = 0x10900000; // COP0_ENABLED | BEV | TS
  PRID = 0x00000002; // Revision Id, same as R3000A
}

u32& Cop0Registers::operator()(u32 i) {
  return R[i];
}


void Registers::Reset() {
  GPR.Reset();
  CP0.Reset();
  Interrupt = 0;
}


RegisterAccessor::RegisterAccessor(Registers &regs)
  : regs_(regs), HI(regs.GPR.HI), LO(regs.GPR.LO), PC(regs.GPR.PC),
    Interrupt(regs.Interrupt) {
  // rennyAssert(&regs_ != nullptr);
}

RegisterAccessor::RegisterAccessor(PSX *psx)
  : regs_(psx->R3000ARegs()), HI(regs_.GPR.HI), LO(regs_.GPR.LO), PC(regs_.GPR.PC),
    Interrupt(regs_.Interrupt) {
  // rennyAssert(&regs_ != nullptr);
}

void RegisterAccessor::ResetRegisters() {
  regs_.Reset();
}


Processor::Processor(PSX* psx, RootCounterManager* rcnt)
  : Component(psx),
    RegisterAccessor(Regs),
    MemoryAccessor(psx),
    IRQAccessor(psx),
    Regs(), GPR(Regs.GPR),
    rcnt_(rcnt)
{
  rennyAssert(rcnt != nullptr);

  inDelaySlot = false;
  doingBranch = false;

  /*
  GPR.AT = 0xffffff8e;
  GPR.V0 = 0x00000000;
  GPR.V1 = 0xa000e00c;
  GPR.A0 = 0xa000b1e0;
  GPR.A1 = 0x00006cc8;
  GPR.A2 = 0x00006cb8;
  GPR.A3 = 0xa000e1f4;
  GPR.T0 = 0x000014f8;
  GPR.T1 = 0x00000000;
  GPR.T2 = 0x000000c0;
  GPR.T3 = 0x00000304;
  GPR.T4 = 0x000000c1;
  GPR.T5 = 0x00000304;
  GPR.T6 = 0xa000e004;
  GPR.T7 = 0x00000008;
  GPR.S0 = 0x00000000;
  GPR.S1 = 0x00000000;
  GPR.S2 = 0x00000000;
  GPR.S3 = 0x00000000;
  GPR.S4 = 0x00000000;
  GPR.S5 = 0x00000000;
  GPR.S6 = 0x00000000;
  GPR.S7 = 0x00000000;
  GPR.T8 = 0x00000004;
  GPR.T9 = 0x00000300;
  GPR.K0 = 0x00000f0c;
  GPR.K1 = 0x00000f0c;
  GPR.FP = 0x801fff00;
  GPR.RA = 0xbfc52350;
  */

  rennyLogDebug("PSXProcessor", "Initialized R3000A processor.");
}


void Processor::Reset()
{
  ResetRegisters();
  inDelaySlot = doingBranch = false;

  //    delayed_load_target = 0;
  //    delayed_load_value = 0;

  rennyLogDebug("PSXProcessor", "R3000A processor is reset.");
}

unsigned int Processor::cycle32() const {
  return rcnt_->cycle32();
}

void Processor::IncreaseCycle() {
  rcnt_->IncreaseCycle();
}

////////////////////////////////////////////////////////////////////////
// Load Functions (TODO: delay load)
////////////////////////////////////////////////////////////////////////

void Processor::Load8s(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  const s32 value = static_cast<s32>(ReadMemory8s(addr)); // sign-extended
  GPR(reg_enum) = static_cast<u32>(value);
}

void Processor::Load8u(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  GPR(reg_enum) = static_cast<u32>(ReadMemory8u(addr));
}

void Processor::Load16s(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  const s32 value = static_cast<s32>(ReadMemory16s(addr));
  GPR(reg_enum) = static_cast<u32>(value);
}

void Processor::Load16u(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  GPR(reg_enum) = static_cast<u32>(ReadMemory16u(addr));
}

void Processor::Load32(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  GPR(reg_enum) = ReadMemory32u(addr);
}

void Processor::Load32Left(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  const u32 shift = (addr & 3) << 3;
  const u32 mem = ReadMemory32u(addr & 0xfffffffc);
  u32& reg = GPR(reg_enum);
  reg = (reg & (0x00ffffff >> shift)) | (mem << (24 - shift));
}

void Processor::Load32Right(u32 reg_enum, PSXAddr addr) {
  if (reg_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  const u32 shift = (addr & 3) << 3;
  const u32 mem = ReadMemory32u(addr & 0xfffffffc);
  u32& reg = GPR(reg_enum);
  reg = (reg & (0xffffff00 << (24 - shift))) | (mem >> shift);
}

////////////////////////////////////////////////////////////////////////
// Store Functions
////////////////////////////////////////////////////////////////////////

void Processor::Store8(u32 reg_enum, PSXAddr addr) {
  WriteMemory8u(addr, static_cast<u8>(GPR(reg_enum)));
}

void Processor::Store16(u32 reg_enum, PSXAddr addr) {
  WriteMemory16u(addr, static_cast<u16>(GPR(reg_enum)));
}

void Processor::Store32(u32 reg_enum, PSXAddr addr) {
  WriteMemory32u(addr, GPR(reg_enum));
}

void Processor::Store32Left(u32 reg_enum, PSXAddr addr) {
  const u32 shift = (addr & 3) << 3;
  const u32 aligned_addr = addr & 0xfffffffc;
  const u32 mem = ReadMemory32u(aligned_addr);
  WriteMemory32u( aligned_addr, (GPR(reg_enum) >> (24 - shift)) |
                  (mem & (0xffffff00 << shift)) );
}

void Processor::Store32Right(u32 reg_enum, PSXAddr addr) {
  const u32 shift = (addr & 3) << 3;
  const u32 aligned_addr = addr & 0xfffffffc;
  const u32 mem = ReadMemory32u(aligned_addr);
  WriteMemory32u( aligned_addr, (GPR(reg_enum) << shift) |
                  (mem & (0x00ffffff >> (24 - shift))) );
}

////////////////////////////////////////////////////////////////////////
// Arithmetic Functions
////////////////////////////////////////////////////////////////////////

void Processor::Add(u32 rd_enum, u32 rs_enum, u32 rt_enum, bool trap_on_ovf) {
  if (rd_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  if (trap_on_ovf) {
    const u64 l = static_cast<u64>(GPR(rs_enum)) + static_cast<u64>(GPR(rt_enum));
    if (0xffffffffL < l) {
      // TODO: throw exception
      rennyLogWarning("MIPSProcessor", "Overflow: %ld", l);
      return;
    }
    GPR(rd_enum) = static_cast<u32>(l);
  } else {
    GPR(rd_enum) = GPR(rs_enum) + GPR(rt_enum);
  }
}

void Processor::AddImmediate(u32 rt_enum, u32 rs_enum, s32 imm, bool trap_on_ovf) {
  if (rt_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  if (trap_on_ovf) {
    const u64 l = static_cast<u64>(GPR(rs_enum)) + static_cast<s64>(imm);
    if (0xffffffffL < l) {
      // TODO: throw exception
      rennyLogWarning("MIPSProcessor", "Overflow: %ld", l);
      return;
    }
    GPR(rt_enum) = static_cast<u32>(l);
  } else {
    GPR(rt_enum) = GPR(rs_enum) + imm;
  }
}

void Processor::Sub(u32 rd_enum, u32 rs_enum, u32 rt_enum, bool trap_on_ovf) {
  if (rd_enum == GPR_ZR) {
    // TODO: throw exception
    return;
  }
  if (trap_on_ovf) {
    const u64 l = static_cast<u64>(GPR(rs_enum)) - static_cast<u64>(GPR(rt_enum));
    if (0xffffffffL < l) {
      // TODO: throw exception
      rennyLogWarning("MIPSProcessor", "Overflow: %ld", l);
      return;
    }
    GPR(rd_enum) = static_cast<u32>(l);
  } else {
    GPR(rd_enum) = GPR(rs_enum) - GPR(rt_enum);
  }
}

void Processor::Mul(u32 rs_enum, u32 rt_enum) {
  const s32 rs = static_cast<s32>(GPR(rs_enum));
  const s32 rt = static_cast<s32>(GPR(rt_enum));
  // CommitDelayedLoad();
  const s64 res = static_cast<s64>(rs) * static_cast<s64>(rt);
  GPR(GPR_LO) = static_cast<u32>(res & 0xffffffff);
  GPR(GPR_HI) = static_cast<u32>(res >> 32);
}

void Processor::MulUnsigned(u32 rs_enum, u32 rt_enum) {
  const u32& rs = GPR(rs_enum);
  const u32& rt = GPR(rt_enum);
  // CommitDelayedLoad();
  const u64 res = static_cast<u64>(rs) * static_cast<u64>(rt);
  GPR(GPR_LO) = static_cast<u32>(res & 0xffffffff);
  GPR(GPR_HI) = static_cast<u32>(res >> 32);
}

void Processor::Div(u32 rs_enum, u32 rt_enum) {
  const s32 rs = static_cast<s32>(GPR(rs_enum));
  const s32 rt = static_cast<s32>(GPR(rt_enum));
  // CommitDelayedLoad();
  GPR(GPR_LO) = static_cast<u32>(rs / rt);
  GPR(GPR_HI) = static_cast<u32>(rs % rt);
}

void Processor::DivUnsigned(u32 rs_enum, u32 rt_enum) {
  const u32& rs = GPR(rs_enum);
  const u32& rt = GPR(rt_enum);
  // CommitDelayedLoad();
  GPR(GPR_LO) = rs / rt;
  GPR(GPR_HI) = rs % rt;
}

////////////////////////////////////////////////////////////////////////
// Execute Function
////////////////////////////////////////////////////////////////////////

void Processor::Execute(Interpreter* interp, bool in_softcall) {
  rennyAssert(interp != nullptr);
  // const uint32_t spusync_cycle_unit = 33868800 / spu_->GetCurrentSamplingRate();
  if (in_softcall) {
    doingBranch = false;
  }
  do {
    interp->ExecuteOnce();
    /*
    uint32_t spusync_cycles = rcnt_->cycle32() - last_spusync_cycle_;
    if (spusync_cycle_unit <= spusync_cycles) {
      if (last_spusync_cycle_ == 0) {
        spu_->Advance(1);
        spusync_cycles -= spusync_cycle_unit;
        last_spusync_cycle_ += spusync_cycle_unit;
      }
      for (; spusync_cycle_unit <= spusync_cycles; spusync_cycles -= spusync_cycle_unit) {
        spu_->GetAsync(spu_out_);
        last_spusync_cycle_ += spusync_cycle_unit;
      }

      // printf("Cycle = %d (in softcall: %d)\n", Cycle, in_softcall ? 1 : 0);
      if (in_softcall == false || doingBranch) return;
    } else if (in_softcall && doingBranch) {
      return;
    }*/
  } while (in_softcall && doingBranch == false);
}


////////////////////////////////////////////////////////////////////////
// Exception (TODO: divide into ThrowException and ProcessException)
////////////////////////////////////////////////////////////////////////

void Processor::Exception(u32 code, bool branch_delay) {
  SetCP0(COP0_CAUSE, code);

  u32& pc_ = Regs.PC;
  u32 pc = pc_;
  if (branch_delay) {
    SetCP0(COP0_CAUSE, CP0(COP0_CAUSE) | 0x80000000);    // set Branch Delay
    pc -= 4;
  }
  SetCP0(COP0_EPC, pc);

  u32 status = CP0(COP0_SR);
  if (status & 0x400000) { // BEV
    pc_ = 0xbfc00180;
  } else {
    pc_ = 0x80000080;
  }

  // (KUo, IEo, KUp, IEp) <- (KUp, IEp, KUc, IEc)
  SetCP0(COP0_SR, (status & ~0x3f) | ((status & 0xf) << 2) );
  Bios().Exception();
}

void Processor::BranchTest() {
  rcnt_->Update();
  if (irq() && (CP0(COP0_SR) & 0x401) == 0x401) {
    Exception(0x400, false);
  }
}

void Processor::DeadLoopSkip() {
  rcnt_->DeadLoopSkip();
}

}   // namespace mips

}   // namespace psx
