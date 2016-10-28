#include "psf/psx/interpreter.h"
#include "psf/psx/psx.h"
#include "psf/psx/memory.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/bios.h"
#include "psf/psx/iop.h"

#include "psf/spu/spu.h"

#include "common/SoundFormat.h"
#include "common/debug.h"

//using namespace PSX;
//using namespace PSX::R3000A;
//using namespace PSX::Interpreter;

namespace {

}   // namespace


namespace PSX {
namespace R3000A {

////////////////////////////////////////////////////////////////
// Delay Functions
////////////////////////////////////////////////////////////////

void Interpreter::delayRead(u32 code, u32 reg, u32 branch_pc)
{
  rennyAssert(reg < 32);

  u32 reg_old, reg_new;

  reg_old = R3000ARegs().GPR.R[reg];
  ExecuteOpcode(code);  // branch delay load
  reg_new = R3000ARegs().GPR.R[reg];

  R3000ARegs().PC = branch_pc;
  R3000a().BranchTest();

  R3000ARegs().GPR.R[reg] = reg_old;
  ExecuteOnce();
  R3000ARegs().GPR.R[reg] = reg_new;

  R3000a().LeaveDelaySlot();
}

void Interpreter::delayWrite(u32, u32 /*reg*/, u32 branch_pc)
{
  // rennyAssert(reg < 32);
  // OPCODES[code >> 26]();
  R3000a().LeaveDelaySlot();
  R3000ARegs().PC = branch_pc;
  R3000a().BranchTest();
}

void Interpreter::delayReadWrite(u32, u32 /*reg*/, u32 branch_pc)
{
  // rennyAssert(reg < 32);
  R3000a().LeaveDelaySlot();
  R3000ARegs().PC = branch_pc;
  R3000a().BranchTest();
}


////////////////////////////////////////////////////////////////
// Delay Opcodes
////////////////////////////////////////////////////////////////

////////////////////////////////////////
// Reusable Functions
////////////////////////////////////////

bool Interpreter::delayNOP(u32, u32, u32)
{
  return false;
}

bool Interpreter::delayRs(u32 code, u32 reg, u32 branch_pc)
{
  if (Rs(code) == reg) {
    delayRead(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayWt(u32 code, u32 reg, u32 branch_pc)
{
  if (Rt(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayWd(u32 code, u32 reg, u32 branch_pc)
{
  if (Rd(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayRsRt(u32 code, u32 reg, u32 branch_pc)
{
  if (Rs(code) == reg || Rt(code) == reg) {
    delayRead(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayRsWt(u32 code, u32 reg, u32 branch_pc)
{
  if (Rs(code) == reg) {
    if (Rt(code) == reg) {
      delayReadWrite(code, reg, branch_pc);
      return true;
    }
    delayRead(code, reg, branch_pc);
    return true;
  }
  if (Rt(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayRsWd(u32 code, u32 reg, u32 branch_pc)
{
  if (Rs(code) == reg) {
    if (Rd(code) == reg) {
      delayReadWrite(code, reg, branch_pc);
      return true;
    }
    delayRead(code, reg, branch_pc);
    return true;
  }
  if (Rd(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayRtWd(u32 code, u32 reg, u32 branch_pc)
{
  if (Rt(code) == reg) {
    if (Rd(code) == reg) {
      delayReadWrite(code, reg, branch_pc);
      return true;
    }
    delayRead(code, reg, branch_pc);
    return true;
  }
  if (Rd(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

bool Interpreter::delayRsRtWd(u32 code, u32 reg, u32 branch_pc)
{
  if (Rs(code) == reg || Rt(code) == reg) {
    if (Rd(code) == reg) {
      delayReadWrite(code, reg, branch_pc);
      return true;
    }
    delayRead(code, reg, branch_pc);
    return true;
  }
  if (Rd(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

////////////////////////////////////////
// SPECIAL functions
////////////////////////////////////////

bool Interpreter::delaySLL(u32 code, u32 reg, u32 branch_pc) {
  if (code) {
    return delayRtWd(code, reg, branch_pc);
  }
  return false;   // delay nop
}

Interpreter::DelayFunc Interpreter::delaySRL = &Interpreter::delayRtWd;
Interpreter::DelayFunc Interpreter::delaySRA = &Interpreter::delayRtWd;

Interpreter::DelayFunc Interpreter::delayJR = &Interpreter::delayRs;

Interpreter::DelayFunc Interpreter::delayJALR = &Interpreter::delayRsWd;

Interpreter::DelayFunc Interpreter::delayADD = &Interpreter::delayRsRtWd;
Interpreter::DelayFunc Interpreter::delayADDU = &Interpreter::delayRsRtWd;
Interpreter::DelayFunc Interpreter::delaySLLV = &Interpreter::delayRsRtWd;

Interpreter::DelayFunc Interpreter::delayMFHI = &Interpreter::delayWd;
Interpreter::DelayFunc Interpreter::delayMFLO = &Interpreter::delayWd;
Interpreter::DelayFunc Interpreter::delayMTHI = &Interpreter::delayRs;
Interpreter::DelayFunc Interpreter::delayMTLO = &Interpreter::delayRs;

Interpreter::DelayFunc Interpreter::delayMULT = &Interpreter::delayRsRt;
Interpreter::DelayFunc Interpreter::delayDIV = &Interpreter::delayRsRt;


Interpreter::DelayFunc Interpreter::delaySpecials[64] = {
    &Interpreter::delaySLL,  &Interpreter::delayNOP,   Interpreter::delaySRL,   Interpreter::delaySRA,
    Interpreter::delaySLLV, &Interpreter::delayNOP,   Interpreter::delaySLLV,  Interpreter::delaySLLV,
    Interpreter::delayJR,    Interpreter::delayJALR, &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    Interpreter::delayMFHI,  Interpreter::delayMTHI,  Interpreter::delayMFLO,  Interpreter::delayMTLO,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    Interpreter::delayMULT,  Interpreter::delayMULT,  Interpreter::delayDIV,   Interpreter::delayDIV,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    Interpreter::delayADD,   Interpreter::delayADDU,  Interpreter::delayADD,   Interpreter::delayADDU,
    Interpreter::delayADD,   Interpreter::delayADD,   Interpreter::delayADD,   Interpreter::delayADD,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,   Interpreter::delayADD,   Interpreter::delayADD,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP
};

bool Interpreter::delaySPCL(u32 code, u32 reg, u32 branch_pc)
{
  return (this->*delaySpecials[Funct(code)])(code, reg, branch_pc);
}

Interpreter::DelayFunc Interpreter::delayBCOND = &Interpreter::delayRs;

bool Interpreter::delayJAL(u32 code, u32 reg, u32 branch_pc)
{
  if (reg == R3000A::GPR_RA) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
}

Interpreter::DelayFunc Interpreter::delayBEQ = &Interpreter::delayRsRt;
Interpreter::DelayFunc Interpreter::delayBNE = &Interpreter::delayRsRt;

Interpreter::DelayFunc Interpreter::delayBLEZ = &Interpreter::delayRs;
Interpreter::DelayFunc Interpreter::delayBGTZ = &Interpreter::delayRs;

Interpreter::DelayFunc Interpreter::delayADDI = &Interpreter::delayRsWt;
Interpreter::DelayFunc Interpreter::delayADDIU = &Interpreter::delayRsWt;

Interpreter::DelayFunc Interpreter::delayLUI = &Interpreter::delayWt;

bool Interpreter::delayCOP0(u32 code, u32 reg, u32 branch_pc)
{
  switch (Funct(code) & ~0x03) {
  case 0x00:  // MFC0, CFC0
  if (Rt(code) == reg) {
    delayWrite(code, reg, branch_pc);
    return true;
  }
  return false;
  case 0x04:  // MTC0, CTC0
  if (Rt(code) == reg) {
    delayRead(code, reg, branch_pc);
    return true;
  }
  return false;
  }
  return false;
}

bool Interpreter::delayLWL(u32 code, u32 reg, u32 branch_pc)
{
  if (Rt(code) == reg) {
    delayReadWrite(code, reg, branch_pc);
    return true;
  }
  if (Rs(code) == reg) {
    delayRead(code, reg, branch_pc);
    return true;
  }
  return false;
}
Interpreter::DelayFunc Interpreter::delayLWR = &Interpreter::delayLWL;

Interpreter::DelayFunc Interpreter::delayLB = &Interpreter::delayRsWt;
Interpreter::DelayFunc Interpreter::delayLH = &Interpreter::delayRsWt;
Interpreter::DelayFunc Interpreter::delayLW = &Interpreter::delayRsWt;
Interpreter::DelayFunc Interpreter::delayLBU = &Interpreter::delayRsWt;
Interpreter::DelayFunc Interpreter::delayLHU = &Interpreter::delayRsWt;

Interpreter::DelayFunc Interpreter::delaySB = &Interpreter::delayRsRt;
Interpreter::DelayFunc Interpreter::delaySH = &Interpreter::delayRsRt;
Interpreter::DelayFunc Interpreter::delaySWL = &Interpreter::delayRsRt;
Interpreter::DelayFunc Interpreter::delaySW = &Interpreter::delayRsRt;
Interpreter::DelayFunc Interpreter::delaySWR = &Interpreter::delayRsRt;

Interpreter::DelayFunc Interpreter::delayLWC2 = &Interpreter::delayRs;
Interpreter::DelayFunc Interpreter::delaySWC2 = &Interpreter::delayRs;


Interpreter::DelayFunc Interpreter::delayOpcodes[64] = {
    &Interpreter::delaySPCL,  Interpreter::delayBCOND, &Interpreter::delayNOP,  &Interpreter::delayJAL,
    Interpreter::delayBEQ,   Interpreter::delayBNE,    Interpreter::delayBLEZ,  Interpreter::delayBGTZ,
    Interpreter::delayADDI,  Interpreter::delayADDIU,  Interpreter::delayADDI,  Interpreter::delayADDI,
    Interpreter::delayADDI,  Interpreter::delayADDI,   Interpreter::delayADDI,  Interpreter::delayLUI,
    &Interpreter::delayCOP0, &Interpreter::delayNOP,   &Interpreter::delayCOP0, &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
    Interpreter::delayLB,    Interpreter::delayLH,    &Interpreter::delayLWL,   Interpreter::delayLW,
    Interpreter::delayLBU,   Interpreter::delayLHU,    Interpreter::delayLWR,  &Interpreter::delayNOP,
    Interpreter::delaySB,    Interpreter::delaySH,     Interpreter::delaySWL,   Interpreter::delaySW,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,    Interpreter::delaySWR,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,    Interpreter::delayLWC2, &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,    Interpreter::delaySWC2, &Interpreter::delayNOP,
    &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP
};


void Interpreter::delayTest(u32 reg, u32 branch_pc)
{
  u32 branch_code(psxMu32val(branch_pc));
  R3000a().EnterDelaySlot();
  u32 op = Opcode(branch_code);
  if ((this->*delayOpcodes[op])(branch_code, reg, branch_pc)) {
    return;
  }

  R3000ARegs().PC = branch_pc;
  ExecuteOpcode(branch_code);
  R3000a().LeaveDelaySlot();
  R3000a().BranchTest();
}


void Interpreter::doBranch(u32 branch_pc)
{
  R3000a().doingBranch = true;

  u32 pc = R3000ARegs().PC;
  u32 code(psxMu32val(pc));
  pc += 4;
  R3000ARegs().PC = pc;
  R3000ARegs().sysclock++;

  const u32 op = Opcode(code);
  switch (op) {
  case 0x10:  // COP0
    if ((Rs(code) & 0x03) == Rs(code)) {  // MFC0, CFC0
      delayTest(Rt(code), branch_pc);
      return;
    }
    break;
  case 0x32:  // LWC2
    delayTest(Rt(code), branch_pc);
    return;
  default:
    if (static_cast<u32>(op - 0x20) < 8) { // Load opcode
      rennyAssert(op >= 0x20);
      delayTest(Rt(code), branch_pc);
      return;
    }
    break;
  }

  ExecuteOpcode(code);

  // branch itself & nop
  if (R3000ARegs().PC - 8 == branch_pc && !op) {
    RCnt().DeadLoopSkip();
  }
  R3000a().LeaveDelaySlot();
  R3000ARegs().PC = branch_pc;

  R3000a().BranchTest();
}


void Interpreter::NLOP(u32 code) {
  rennyLogWarning("PSXInterpreter", "Unknown Opcode (0x%02x) is executed!", code >> 26);
}


/*
inline void CommitDelayedLoad() {
    if (delayed_load_target) {
        R3000ARegs().GPR.R[delayed_load_target] = delayed_load_value;
        delayed_load_target = 0;
        delayed_load_value = 0;
    }
}

inline void DelayedBranch(u32 target) {
    CommitDelayedLoad();
    delayed_load_target = GPR_PC;
    delayed_load_value = target;
    wxMessageOutputDebug().Printf("Branch to 0x%08x", target);
}
 */

inline void Interpreter::Load(u32 rt, u32 value) {
  // CommitDelayedLoad();
  if (rt != 0) {
    R3000ARegs().GPR.R[rt] = value;
  }
}

/*
// for load function from memory or not GPR into GPR
inline void DelayedLoad(u32 rt, u32 value) {
    rennyAssert_MSG(rt != GPR_PC, "Delayed-load target must be other than R3000ARegs().PC.");
    if (delayed_load_target == GPR_PC) {
        // delay load
        R3000ARegs().PC = delayed_load_value;
        delayed_load_target = rt;
        delayed_load_value = value;
        wxMessageOutputDebug().Printf("Delayed-loaded.");
        return;
    }
    // normal load
    Load(rt, value);
}
 */


////////////////////////////////////////
// Load and Store u32s
////////////////////////////////////////

// Load Byte
void Interpreter::LB(u32 code) {
  Load(Rt(code), static_cast<int8_t>(ReadMemory8(Addr(R3000ARegs().GPR, code))));
}

// Load Byte Unsigned
void Interpreter::LBU(u32 code) {
  Load(Rt(code), ReadMemory8(Addr(R3000ARegs().GPR, code)));
}

// Load Halfword
void Interpreter::LH(u32 code) {
  Load(Rt(code), static_cast<int16_t>(ReadMemory16(Addr(R3000ARegs().GPR, code))));
}

// Load Halfword Unsigned
void Interpreter::LHU(u32 code) {
  Load(Rt(code), ReadMemory16(Addr(R3000ARegs().GPR, code)));
}

// Load Word
void Interpreter::LW(u32 code) {
  Load(Rt(code), ReadMemory32(Addr(R3000ARegs().GPR, code)));
}

// Load Word Left
void Interpreter::LWL(u32 code) {
  static const u32 LWL_MASK[4] = { 0x00ffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
  static const u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };
  const u32 rt = Rt(code);
  const u32 addr = Addr(R3000ARegs().GPR, code);
  const u32 shift = addr & 3;
  const u32 mem = ReadMemory32(addr & ~3);
  Load(rt, (R3000ARegs().GPR.R[rt] & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]));
}

// Load Word Right
void Interpreter::LWR(u32 code) {
  static const u32 LWR_MASK[4] = { 0xff000000, 0xffff0000, 0xffffff00, 0x00000000 };
  static const u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };
  const u32 rt = Rt(code);
  const u32 addr = Addr(R3000ARegs().GPR, code);
  const u32 shift = addr & 3;
  const u32 mem = ReadMemory32(addr & ~3);
  Load(rt, (R3000ARegs().GPR.R[rt] & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]));
}

// Store Byte
void Interpreter::SB(u32 code) {
  WriteMemory8(Addr(R3000ARegs().GPR, code), RtVal(R3000ARegs().GPR, code) & 0xff);
}

// Store Halfword
void Interpreter::SH(u32 code) {
  WriteMemory16(Addr(R3000ARegs().GPR, code), RtVal(R3000ARegs().GPR, code) & 0xffff);
}

// Store Word
void Interpreter::SW(u32 code) {
  WriteMemory32(Addr(R3000ARegs().GPR, code), RtVal(R3000ARegs().GPR, code));
}

// Store Word Left
void Interpreter::SWL(u32 code) {
  static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
  static const u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };
  const u32 addr = Addr(R3000ARegs().GPR, code);
  const u32 shift = addr & 3;
  const u32 mem = ReadMemory32(addr & ~3);
  WriteMemory32(addr & ~3, (RtVal(R3000ARegs().GPR, code) >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]));
}

// Store Word Right
void Interpreter::SWR(u32 code) {
  static const u32 SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
  static const u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };
  const u32 addr = Addr(R3000ARegs().GPR, code);
  const u32 shift = addr & 3;
  const u32 mem = ReadMemory32(addr & ~3);
  WriteMemory32(addr & ~3, (RtVal(R3000ARegs().GPR, code) << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]));
}


////////////////////////////////////////
// Computational u32s
////////////////////////////////////////

// ADD Immediate
void Interpreter::ADDI(u32 code) {
  Load(Rt(code), RsVal(R3000ARegs().GPR, code) + Imm(code));
  // TODO: Trap on two's complement overflow.
}

// ADD Immediate Unsigned
void Interpreter::ADDIU(u32 code) {
  if (Rt(code) == 0) {
    Iop().Call(R3000ARegs().PC - 4, Imm(code));
  } else {
    Load(Rt(code), RsVal(R3000ARegs().GPR, code) + Imm(code));
  }
}

// Set on Less Than Immediate
void Interpreter::SLTI(u32 code) {
  Load(Rt(code), (static_cast<s32>(RsVal(R3000ARegs().GPR, code)) < Imm(code)) ? 1 : 0);
}

// Set on Less Than Unsigned Immediate
void Interpreter::SLTIU(u32 code) {
  Load(Rt(code), (RsVal(R3000ARegs().GPR, code) < static_cast<u32>(Imm(code))) ? 1 : 0);
}

// AND Immediate
void Interpreter::ANDI(u32 code) {
  Load(Rt(code), RsVal(R3000ARegs().GPR, code) & ImmU(code));
}

// OR Immediate
void Interpreter::ORI(u32 code) {
  Load(Rt(code), RsVal(R3000ARegs().GPR, code) | ImmU(code));
}

// eXclusive OR Immediate
void Interpreter::XORI(u32 code) {
  Load(Rt(code), RsVal(R3000ARegs().GPR, code) ^ ImmU(code));
}

// Load Upper Immediate
void Interpreter::LUI(u32 code) {
  Load(Rt(code), code << 16);
}


// ADD
void Interpreter::ADD(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) + RtVal(R3000ARegs().GPR, code));
  // ? Trap on two's complement overflow.
}

// ADD Unsigned
void Interpreter::ADDU(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) + RtVal(R3000ARegs().GPR, code));
}

// SUBtract
void Interpreter::SUB(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) - RtVal(R3000ARegs().GPR, code));
  // ? Trap on two's complement overflow.
}

// SUBtract Unsigned
void Interpreter::SUBU(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) - RtVal(R3000ARegs().GPR, code));
}

// Set on Less Than
void Interpreter::SLT(u32 code) {
  Load(Rd(code), (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) < static_cast<int32_t>(RtVal(R3000ARegs().GPR, code))) ? 1 : 0);
}

// Set on Less Than Unsigned
void Interpreter::SLTU(u32 code) {
  Load(Rd(code), (RsVal(R3000ARegs().GPR, code) < RtVal(R3000ARegs().GPR, code)) ? 1 : 0);
}

// AND
void Interpreter::AND(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) & RtVal(R3000ARegs().GPR, code));
}

// OR
void Interpreter::OR(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) | RtVal(R3000ARegs().GPR, code));
}

// eXclusive OR
void Interpreter::XOR(u32 code) {
  Load(Rd(code), RsVal(R3000ARegs().GPR, code) ^ RtVal(R3000ARegs().GPR, code));
}

// NOR
void Interpreter::NOR(u32 code) {
  Load(Rd(code), ~(RsVal(R3000ARegs().GPR, code) | RtVal(R3000ARegs().GPR, code)));
}


// Shift Left Logical
void Interpreter::SLL(u32 code) {
  Load(Rd(code), RtVal(R3000ARegs().GPR, code) << Shamt(code));
}

// Shift Right Logical
void Interpreter::SRL(u32 code) {
  Load(Rd(code), RtVal(R3000ARegs().GPR, code) >> Shamt(code));
}

// Shift Right Arithmetic
void Interpreter::SRA(u32 code) {
  Load(Rd(code), static_cast<s32>(RtVal(R3000ARegs().GPR, code)) >> Shamt(code));
}

// Shift Left Logical Variable
void Interpreter::SLLV(u32 code) {
  Load(Rd(code), RtVal(R3000ARegs().GPR, code) << RsVal(R3000ARegs().GPR, code));
}

// Shift Right Logical Variable
void Interpreter::SRLV(u32 code) {
  Load(Rd(code), RtVal(R3000ARegs().GPR, code) >> RsVal(R3000ARegs().GPR, code));
}

// Shift Right Arithmetic Variable
void Interpreter::SRAV(u32 code) {
  Load(Rd(code), static_cast<s32>(RtVal(R3000ARegs().GPR, code)) >> RsVal(R3000ARegs().GPR, code));
}


// MULTiply
void Interpreter::MULT(u32 code) {
  int32_t rs = static_cast<int32_t>(RsVal(R3000ARegs().GPR, code));
  int32_t rt = static_cast<int32_t>(RtVal(R3000ARegs().GPR, code));
  // CommitDelayedLoad();
  int64_t res = (int64_t)rs * (int64_t)rt;
  R3000ARegs().GPR.LO = static_cast<u32>(res & 0xffffffff);
  R3000ARegs().GPR.HI = static_cast<u32>(res >> 32);
}

// MULtiply Unsigned
void Interpreter::MULTU(u32 code) {
  u32 rs = RsVal(R3000ARegs().GPR, code);
  u32 rt = RtVal(R3000ARegs().GPR, code);
  // CommitDelayedLoad();
  uint64_t res = (uint64_t)rs * (uint64_t)rt;
  R3000ARegs().GPR.LO = static_cast<u32>(res & 0xffffffff);
  R3000ARegs().GPR.HI = static_cast<u32>(res >> 32);
}

// Divide
void Interpreter::DIV(u32 code) {
  int32_t rs = static_cast<int32_t>(RsVal(R3000ARegs().GPR, code));
  int32_t rt = static_cast<int32_t>(RtVal(R3000ARegs().GPR, code));
  // CommitDelayedLoad();
  R3000ARegs().GPR.LO = static_cast<u32>(rs / rt);
  R3000ARegs().GPR.HI = static_cast<u32>(rs % rt);
}

// Divide Unsigned
void Interpreter::DIVU(u32 code) {
  u32 rs = RsVal(R3000ARegs().GPR, code);
  u32 rt = RtVal(R3000ARegs().GPR, code);
  // CommitDelayedLoad();
  R3000ARegs().GPR.LO = rs / rt;
  R3000ARegs().GPR.HI = rs % rt;
}

// Move From HI
void Interpreter::MFHI(u32 code) {
  Load(Rd(code), R3000ARegs().GPR.HI);
}

// Move From LO
void Interpreter::MFLO(u32 code) {
  Load(Rd(code), R3000ARegs().GPR.LO);
}

// Move To HI
void Interpreter::MTHI(u32 code) {
  u32 rd = RdVal(R3000ARegs().GPR, code);
  // CommitDelayedLoad();
  R3000ARegs().GPR.HI = rd;
}

// Move To LO
void Interpreter::MTLO(u32 code) {
  u32 rd = RdVal(R3000ARegs().GPR, code);
  // CommitDelayedLoad();
  R3000ARegs().GPR.LO = rd;
}


////////////////////////////////////////
// Jump and Branch u32s
////////////////////////////////////////

// Jump
void Interpreter::J(u32 code) {
  doBranch((Target(code) << 2) | (R3000ARegs().PC & 0xf0000000));
}

// Jump And Link
void Interpreter::JAL(u32 code) {
  R3000ARegs().GPR.RA = R3000ARegs().PC + 4;
  doBranch((Target(code) << 2) | (R3000ARegs().PC & 0xf0000000));
}

// Jump Register
void Interpreter::JR(u32 code) {
  doBranch(RsVal(R3000ARegs().GPR, code));
}

// Jump And Link Register
void Interpreter::JALR(u32 code) {
  u32 rd = Rd(code);
  if (rd != 0) {
    R3000ARegs().GPR.R[rd] = R3000ARegs().PC + 4;
  }
  doBranch(RsVal(R3000ARegs().GPR, code));
}


// Branch on EQual
void Interpreter::BEQ(u32 code) {
  if (RsVal(R3000ARegs().GPR, code) == RtVal(R3000ARegs().GPR, code)) {
    doBranch(R3000ARegs().PC + (Imm(code) << 2));
  }
}

// Branch on Not Equal
void Interpreter::BNE(u32 code) {
  if (RsVal(R3000ARegs().GPR, code) != RtVal(R3000ARegs().GPR, code)) {
    doBranch(R3000ARegs().PC + (Imm(code) << 2));
  }
}

// Branch on Less than or Equal Zero
void Interpreter::BLEZ(u32 code) {
  if (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) <= 0) {
    doBranch(R3000ARegs().PC + (Imm(code) << 2));
  }
}

// Branch on Greate Than Zero
void Interpreter::BGTZ(u32 code) {
  if (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) > 0) {
    doBranch(R3000ARegs().PC + (Imm(code) << 2));
  }
}

// Branch on Less Than Zero
void Interpreter::BLTZ(u32 code) {
  if (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) < 0) {
    doBranch(R3000ARegs().PC + (Imm(code) << 2));
  }
}

// Branch on Greater than or Equal Zero
void Interpreter::BGEZ(u32 code) {
  if (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) >= 0) {
    doBranch(R3000ARegs().PC + (Imm(code) << 2));
  }
}

// Branch on Less Than Zero And Link
void Interpreter::BLTZAL(u32 code) {
  if (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) < 0) {
    u32 pc = R3000ARegs().PC;
    R3000ARegs().GPR.RA = pc + 4;
    doBranch(pc + (Imm(code) << 2));
  }
}

// Branch on Greater than or Equal Zero And Link
void Interpreter::BGEZAL(u32 code) {
  if (static_cast<int32_t>(RsVal(R3000ARegs().GPR, code)) >= 0) {
    u32 pc = R3000ARegs().PC;
    R3000ARegs().GPR.RA = pc + 4;
    doBranch(pc + (Imm(code) << 2));
  }
}

// Branch Condition
void Interpreter::BCOND(u32 code) {
  (this->*BCONDS[Rt(code)])(code);
}



////////////////////////////////////////
// Special u32s
////////////////////////////////////////

void Interpreter::SYSCALL(u32) {
  R3000ARegs().PC -= 4;
  R3000a().Exception(0x20, R3000a().IsInDelaySlot());
}
void Interpreter::BREAK(u32) {}

void Interpreter::SPCL(u32 code) {
  (this->*SPECIALS[Funct(code)])(code);
}


////////////////////////////////////////
// Co-processor u32s
////////////////////////////////////////

void Interpreter::LWC0(u32) {

}
void Interpreter::LWC1(u32) {
  rennyLogWarning("PSXInterpreter", "LWC1 is not supported.");
}
void Interpreter::LWC2(u32) {}
void Interpreter::LWC3(u32) {
  rennyLogWarning("PSXInterpreter", "LWC3 is not supported.");
}

void Interpreter::SWC0(u32) {}
void Interpreter::SWC1(u32) {
  rennyLogWarning("PSXInterpreter", "SWC1 is not supported.");
}

void Interpreter::SWC2(u32) {}
void Interpreter::SWC3(u32) {
  // HLE?
  rennyLogWarning("PSXInterpreter", "SWC3 is not supported.");
}



void Interpreter::COP0(u32) {}
void Interpreter::COP1(u32 code) {
  rennyLogWarning("PSXInterpreter", "COP1 is not supported (PC = 0x%08x).", (int)code);
}
void Interpreter::COP2(u32) {
  // Cop2 is not supported.
}
void Interpreter::COP3(u32 code) {
  rennyLogWarning("PSXInterpreter", "COP3 is not supported (PC = 0x%08x).", (int)code);
}


////////////////////////////////////////
// HLE functions
////////////////////////////////////////

void Interpreter::hleDummy()
{
  R3000ARegs().PC = R3000ARegs().GPR.RA;
  R3000a().BranchTest();
}

void Interpreter::hleA0()
{
  (Bios().*BIOS::biosA0[R3000ARegs().GPR.T1 & 0xff])();
  R3000a().BranchTest();
}

void Interpreter::hleB0()
{
  (Bios().*BIOS::biosB0[R3000ARegs().GPR.T1 & 0xff])();
  R3000a().BranchTest();
}

void Interpreter::hleC0()
{
  (Bios().*BIOS::biosC0[R3000ARegs().GPR.T1 & 0xff])();
  R3000a().BranchTest();
}

void Interpreter::hleBootstrap() {}

struct EXEC {
  u32 pc0;
  u32 gp0;
  u32 t_addr;
  u32 t_size;
  u32 d_addr;
  u32 d_size;
  u32 b_addr;
  u32 b_size;
  u32 S_addr;
  u32 s_size;
  u32 sp, fp, gp, ret, base;
};

void Interpreter::hleExecRet()
{
  EXEC *header = static_cast<EXEC*>(psxMptr(R3000ARegs().GPR.S0));
  R3000ARegs().GPR.RA = BFLIP32(header->ret);
  R3000ARegs().GPR.SP = BFLIP32(header->sp);
  R3000ARegs().GPR.FP = BFLIP32(header->fp);
  R3000ARegs().GPR.GP = BFLIP32(header->gp);
  R3000ARegs().GPR.S0 = BFLIP32(header->base);
  R3000ARegs().GPR.V0 = 1;
  R3000ARegs().PC = R3000ARegs().GPR.RA;
}

void (Interpreter::*const Interpreter::HLEt[])() = {
    &Interpreter::hleDummy, &Interpreter::hleA0, &Interpreter::hleB0,
    &Interpreter::hleC0, &Interpreter::hleBootstrap, &Interpreter::hleExecRet
};


void Interpreter::HLECALL(u32 code) {
  (this->*HLEt[code & 0xff])();
}



////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////

InterpreterThread::InterpreterThread(Interpreter *interp)
: wxThread(wxTHREAD_JOINABLE),
  interp_(*interp), isRunning_(false)
{}


wxThread::ExitCode InterpreterThread::Entry()
{
  rennyLogDebug("PSXInterpreterThread", "Started the thread.");

  SoundBlock tmp;

  isRunning_ = true;
  do {
    int ret = interp_.RCnt().SPURun(&tmp);
    if (ret < 0) break;
    if (ret == 1) {
      // TODO: transfer tmp to SoundDevice
    }
    //#warning PSX::Interpreter::Thread don't call SPUendflush
    interp_.ExecuteOnce();
  } while (isRunning_);

  return 0;
}


void InterpreterThread::Shutdown()
{
  if (isRunning_ == false) return;
  isRunning_ = false;
  rennyLogDebug("PSXInterpreterThread", "Requested that PSX interpreter thread stop running.");
  wxThread::Wait();
}


void InterpreterThread::OnExit()
{
  // interp_.Spu().Close();
  rennyLogDebug("PSXInterpreterThread", "PSX Thread is ended. (cycle = %d)", interp_.R3000ARegs().sysclock);
}



////////////////////////////////////////////////////////////////
// Main Loop
////////////////////////////////////////////////////////////////

InterpreterThread *Interpreter::thread = 0;

InterpreterThread *Interpreter::Execute()
{
  if (thread != 0) {
    Shutdown();
  }
  thread = new InterpreterThread(this);
  thread->Create();
  thread->Run();
  return thread;
}

void Interpreter::Shutdown()
{
  if (thread == 0) return;
  if (thread->IsRunning()) {
    thread->Shutdown();
  }
  delete thread;  // WARN
  thread = 0;
}


void (Interpreter::*const Interpreter::OPCODES[64])(u32) = {
    &Interpreter::SPCL, &Interpreter::BCOND, &Interpreter::J,    &Interpreter::JAL,
    &Interpreter::BEQ,  &Interpreter::BNE,   &Interpreter::BLEZ, &Interpreter::BGTZ,
    &Interpreter::ADDI, &Interpreter::ADDIU, &Interpreter::SLTI, &Interpreter::SLTIU,
    &Interpreter::ANDI, &Interpreter::ORI,   &Interpreter::XORI, &Interpreter::LUI,
    &Interpreter::COP0, &Interpreter::COP1,  &Interpreter::COP2, &Interpreter::COP3,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::LB,   &Interpreter::LH,    &Interpreter::LWL,  &Interpreter::LW,
    &Interpreter::LBU,  &Interpreter::LHU,   &Interpreter::LWR,  &Interpreter::NLOP,
    &Interpreter::SB,   &Interpreter::SH,    &Interpreter::SWL,  &Interpreter::SW,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::SWR,  &Interpreter::NLOP,
    &Interpreter::LWC0, &Interpreter::LWC1,  &Interpreter::LWC2, &Interpreter::LWC3,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::SWC0, &Interpreter::SWC1,  &Interpreter::SWC2, &Interpreter::HLECALL,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP
};


void (Interpreter::*const Interpreter::SPECIALS[64])(u32) = {
    &Interpreter::SLL,  &Interpreter::NLOP,  &Interpreter::SRL,  &Interpreter::SRA,
    &Interpreter::SLLV, &Interpreter::NLOP,  &Interpreter::SRLV, &Interpreter::SRAV,
    &Interpreter::JR,   &Interpreter::JALR,  &Interpreter::NLOP, &Interpreter::HLECALL,
    &Interpreter::SYSCALL, &Interpreter::BREAK, &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::MFHI, &Interpreter::MTHI,  &Interpreter::MFLO, &Interpreter::MTLO,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::MULT, &Interpreter::MULTU, &Interpreter::DIV,  &Interpreter::DIVU,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::ADD,  &Interpreter::ADDU,  &Interpreter::SUB,  &Interpreter::SUBU,
    &Interpreter::AND,  &Interpreter::OR,    &Interpreter::XOR,  &Interpreter::NOR,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::SLT,  &Interpreter::SLTU,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP
};


void (Interpreter::*const Interpreter::BCONDS[24])(u32) = {
    &Interpreter::BLTZ,   &Interpreter::BGEZ,   &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::BLTZAL, &Interpreter::BGEZAL, &Interpreter::NLOP, &Interpreter::NLOP,
    &Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP
};


void Interpreter::Init()
{
}


void Interpreter::ExecuteOnce()
{
  u32 pc = R3000ARegs().PC;
  u32 code(Psx().Mu32val(pc));
  pc += 4;
  ++R3000ARegs().sysclock;
  R3000ARegs().PC = pc;

  ExecuteOpcode(code);
}

// called from BIOS::Softcall()
void Interpreter::ExecuteBlock() {
  R3000a().doingBranch = false;
  do {
    ExecuteOnce();
  } while (!R3000a().doingBranch);
}

uint32_t Interpreter::Execute(uint32_t cycles) {
  const uint32_t& cycle = R3000ARegs().sysclock;
  const uint32_t cycle_start = cycle;
  const uint32_t cycle_end = cycle + cycles;

  while (cycle < cycle_end) {
    ExecuteOnce();
  }
  return cycle - cycle_start;
}

/*
Interpreter& Interpreter::GetInstance()
{
    static Interpreter instance;
    return instance;
}
 */
}   // namespace R3000A

// R3000A::Interpreter& Interpreter_ = R3000A::Interpreter::GetInstance();

}   // namespace PSX
