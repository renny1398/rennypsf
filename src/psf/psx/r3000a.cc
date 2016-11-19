#include "psf/psx/r3000a.h"
#include "psf/psx/memory.h"
#include "psf/psx/bios.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/disassembler.h"

#include "common/debug.h"
#include <cstring>


namespace PSX {

const char *strGPR[35] = {
  "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
  "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
  "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
  "T8", "T9", "K0", "K1", "GP", "SP", "FP", "RA",
  "HI", "LO", "PC"
};


namespace R3000A {

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


Processor::Processor(Composite* composite)
  : Component(composite),
    IRQAccessor(composite),
    Regs(R3000ARegs()),
    GPR(Regs.GPR),
    CP0(Regs.CP0),
    HI(Regs.GPR.HI), LO(Regs.GPR.LO),
    PC(Regs.GPR.PC),
    Cycle(Regs.sysclock),
    Interrupt(Regs.Interrupt)
{
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
  inDelaySlot = doingBranch = false;

  /*last_code = */
  Cycle = Interrupt = 0;

  //    delayed_load_target = 0;
  //    delayed_load_value = 0;

  GPR.Reset();
  CP0.Reset();

  rennyLogDebug("PSXProcessor", "R3000A processor is reset.");
}


void Processor::Exception(u32 code, bool branch_delay)
{
  CP0.CAUSE = code;

  u32 pc = GPR.PC;
  if (branch_delay) {
    CP0.CAUSE |= 0x80000000;    // set Branch Delay
    pc -= 4;
  }
  CP0.EPC = pc;

  u32 status = CP0.SR;
  if (status & 0x400000) { // BEV
    PC = 0xbfc00180;
  } else {
    PC = 0x80000080;
  }

  // (KUo, IEo, KUp, IEp) <- (KUp, IEp, KUc, IEc)
  CP0.SR = (status & ~0x3f) | ((status & 0xf) << 2);
  Bios().Exception();
}


void Processor::BranchTest()
{
  RCnt().Update();

  // interrupt mask register ($1f801074)
  if (irq()) {
    if ((CP0.SR & 0x401) == 0x401) {
      Exception(0x400, false);
    }
  }
}


}   // namespace R3000A

}   // namespace PSX
