#include "psf/psx/interpreter.h"
#include "psf/psx/memory.h"
#include "psf/psx/bios.h"
#include "psf/psx/iop.h"

#include "psf/spu/spu.h"

#include "common/SoundFormat.h"
#include "common/debug.h"

namespace psx {
namespace mips {

Interpreter::Interpreter(PSX* psx, Processor* cpu, BIOS* bios, IOP* iop)
  : RegisterAccessor(psx), UserMemoryAccessor(psx),
    cpu_(*cpu), bios_(*bios), iop_(*iop) {
  // rennyAssert(&cpu_ != nullptr);
  // rennyAssert(&bios_ != nullptr);
  // rennyAssert(&iop_ != nullptr);
}

////////////////////////////////////////////////////////////////
// Delay Functions
////////////////////////////////////////////////////////////////

void Interpreter::delayRead(u32 code, u32 reg, u32 branch_pc)
{
  rennyAssert(reg < 32);

  u32 reg_old, reg_new;

  reg_old = GPR(reg);
  ExecuteOpcode(code);  // branch delay load
  reg_new = GPR(reg);

  SetGPR(GPR_PC, branch_pc);
  cpu_.BranchTest();

  SetGPR(reg, reg_old);
  ExecuteOnce();
  SetGPR(reg, reg_new);

  cpu_.LeaveDelaySlot();
}

void Interpreter::delayWrite(u32 code, u32 /*reg*/, u32 branch_pc)
{
  // rennyAssert(reg < 32);
  ExecuteOpcode(code);
  cpu_.LeaveDelaySlot();
  SetGPR(GPR_PC, branch_pc);
  cpu_.BranchTest();
}

void Interpreter::delayReadWrite(u32, u32 /*reg*/, u32 branch_pc)
{
  // rennyAssert(reg < 32);
  cpu_.LeaveDelaySlot();
  SetGPR(GPR_PC, branch_pc);
  cpu_.BranchTest();
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
  if (reg == GPR_RA) {
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
  cpu_.EnterDelaySlot();
  u32 op = Opcode(branch_code);
  if ((this->*delayOpcodes[op])(branch_code, reg, branch_pc)) {
    return;
  }

  SetGPR(GPR_PC, branch_pc);
  ExecuteOpcode(branch_code);
  cpu_.LeaveDelaySlot();
  cpu_.BranchTest();
}


void Interpreter::doBranch(u32 branch_pc)
{
  cpu_.doingBranch = true;

  u32 pc = GPR(GPR_PC);
  u32 code(psxMu32val(pc));
  pc += 4;
  SetGPR(GPR_PC, pc);
  cpu_.IncreaseCycle(); // Cycle++;

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
  if (GPR(GPR_PC) - 8 == branch_pc && !op) {
    cpu_.DeadLoopSkip();
  }
  cpu_.LeaveDelaySlot();
  SetGPR(GPR_PC, branch_pc);

  cpu_.BranchTest();
}


void Interpreter::NLOP(u32 code) {
  rennyLogWarning("PSXInterpreter", "Unknown Opcode (0x%02x) is executed!", code >> 26);
}


/*
inline void CommitDelayedLoad() {
    if (delayed_load_target) {
        SetGPR(GPR_R(delayed_load_target, delayed_load_value);
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
    SetGPR(rt, value);
  }
}

/*
// for load function from memory or not GPR into GPR
inline void DelayedLoad(u32 rt, u32 value) {
    rennyAssert_MSG(rt != GPR_PC, "Delayed-load target must be other than GPR(GPR_PC).");
    if (delayed_load_target == GPR_PC) {
        // delay load
        GPR(GPR_PC) = delayed_load_value;
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
  cpu_.Load8s(Rt(code), Addr(GPR(), code));
}

// Load Byte Unsigned
void Interpreter::LBU(u32 code) {
  cpu_.Load8u(Rt(code), Addr(GPR(), code));
}

// Load Halfword
void Interpreter::LH(u32 code) {
  cpu_.Load16s(Rt(code), Addr(GPR(), code));
}

// Load Halfword Unsigned
void Interpreter::LHU(u32 code) {
  cpu_.Load16u(Rt(code), Addr(GPR(), code));
}

// Load Word
void Interpreter::LW(u32 code) {
  cpu_.Load32(Rt(code), Addr(GPR(), code));
}

// Load Word Left
void Interpreter::LWL(u32 code) {
  cpu_.Load32Left(Rt(code), Addr(GPR(), code));
}

// Load Word Right
void Interpreter::LWR(u32 code) {
  cpu_.Load32Right(Rt(code), Addr(GPR(), code));
}

// Store Byte
void Interpreter::SB(u32 code) {
  cpu_.Store8(Rt(code), Addr(GPR(), code));
}

// Store Halfword
void Interpreter::SH(u32 code) {
  cpu_.Store16(Rt(code), Addr(GPR(), code));
}

// Store Word
void Interpreter::SW(u32 code) {
  cpu_.Store32(Rt(code), Addr(GPR(), code));
}

// Store Word Left
void Interpreter::SWL(u32 code) {
  cpu_.Store32Left(Rt(code), Addr(GPR(), code));
}

// Store Word Right
void Interpreter::SWR(u32 code) {
  cpu_.Store32Right(Rt(code), Addr(GPR(), code));
}


////////////////////////////////////////
// Computational u32s
////////////////////////////////////////

// ADD Immediate
void Interpreter::ADDI(u32 code) {
  cpu_.AddImmediate(Rt(code), Rs(code), Imm(code), true);
}

// ADD Immediate Unsigned
void Interpreter::ADDIU(u32 code) {
  if (Rt(code) == GPR_ZR) {
    iop_.Call(GPR(GPR_PC) - 4, Imm(code));
  } else {
    cpu_.AddImmediate(Rt(code), Rs(code), Imm(code), false);
  }
}

// Set on Less Than Immediate
void Interpreter::SLTI(u32 code) {
  Load(Rt(code), (static_cast<s32>(RsVal(GPR(), code)) < Imm(code)) ? 1 : 0);
}

// Set on Less Than Unsigned Immediate
void Interpreter::SLTIU(u32 code) {
  Load(Rt(code), (RsVal(GPR(), code) < static_cast<u32>(Imm(code))) ? 1 : 0);
}

// AND Immediate
void Interpreter::ANDI(u32 code) {
  Load(Rt(code), RsVal(GPR(), code) & ImmU(code));
}

// OR Immediate
void Interpreter::ORI(u32 code) {
  Load(Rt(code), RsVal(GPR(), code) | ImmU(code));
}

// eXclusive OR Immediate
void Interpreter::XORI(u32 code) {
  Load(Rt(code), RsVal(GPR(), code) ^ ImmU(code));
}

// Load Upper Immediate
void Interpreter::LUI(u32 code) {
  Load(Rt(code), ImmU(code) << 16);
}


// ADD
void Interpreter::ADD(u32 code) {
  cpu_.Add(Rd(code), Rs(code), Rt(code), true);
}

// ADD Unsigned
void Interpreter::ADDU(u32 code) {
  cpu_.Add(Rd(code), Rs(code), Rt(code), false);
}

// SUBtract
void Interpreter::SUB(u32 code) {
  cpu_.Sub(Rd(code), Rs(code), Rt(code), true);
}

// SUBtract Unsigned
void Interpreter::SUBU(u32 code) {
  cpu_.Sub(Rd(code), Rs(code), Rt(code), false);
}

// Set on Less Than
void Interpreter::SLT(u32 code) {
  Load(Rd(code), (static_cast<s32>(RsVal(GPR(), code)) < static_cast<s32>(RtVal(GPR(), code))) ? 1 : 0);
}

// Set on Less Than Unsigned
void Interpreter::SLTU(u32 code) {
  Load(Rd(code), (RsVal(GPR(), code) < RtVal(GPR(), code)) ? 1 : 0);
}

// AND
void Interpreter::AND(u32 code) {
  Load(Rd(code), RsVal(GPR(), code) & RtVal(GPR(), code));
}

// OR
void Interpreter::OR(u32 code) {
  Load(Rd(code), RsVal(GPR(), code) | RtVal(GPR(), code));
}

// eXclusive OR
void Interpreter::XOR(u32 code) {
  Load(Rd(code), RsVal(GPR(), code) ^ RtVal(GPR(), code));
}

// NOR
void Interpreter::NOR(u32 code) {
  Load(Rd(code), ~(RsVal(GPR(), code) | RtVal(GPR(), code)));
}


// Shift Left Logical
void Interpreter::SLL(u32 code) {
  Load(Rd(code), RtVal(GPR(), code) << Shamt(code));
}

// Shift Right Logical
void Interpreter::SRL(u32 code) {
  Load(Rd(code), RtVal(GPR(), code) >> Shamt(code));
}

// Shift Right Arithmetic
void Interpreter::SRA(u32 code) {
  Load(Rd(code), static_cast<s32>(RtVal(GPR(), code)) >> Shamt(code));
}

// Shift Left Logical Variable
void Interpreter::SLLV(u32 code) {
  Load(Rd(code), RtVal(GPR(), code) << RsVal(GPR(), code));
}

// Shift Right Logical Variable
void Interpreter::SRLV(u32 code) {
  Load(Rd(code), RtVal(GPR(), code) >> RsVal(GPR(), code));
}

// Shift Right Arithmetic Variable
void Interpreter::SRAV(u32 code) {
  Load(Rd(code), static_cast<s32>(RtVal(GPR(), code)) >> RsVal(GPR(), code));
}


// MULTiply
void Interpreter::MULT(u32 code) {
  cpu_.Mul(Rs(code), Rt(code));
}

// MULtiply Unsigned
void Interpreter::MULTU(u32 code) {
  cpu_.MulUnsigned(Rs(code), Rt(code));
}

// Divide
void Interpreter::DIV(u32 code) {
  cpu_.Div(Rs(code), Rt(code));
}

// Divide Unsigned
void Interpreter::DIVU(u32 code) {
  cpu_.DivUnsigned(Rs(code), Rt(code));
}

// Move From HI
void Interpreter::MFHI(u32 code) {
  MoveGPR(Rd(code), GPR_HI);
}

// Move From LO
void Interpreter::MFLO(u32 code) {
  MoveGPR(Rd(code), GPR_LO);
}

// Move To HI
void Interpreter::MTHI(u32 code) {
  u32 rd_enum = Rd(code);
  // CommitDelayedLoad();
  MoveGPR(GPR_HI, rd_enum);
}

// Move To LO
void Interpreter::MTLO(u32 code) {
  u32 rd_enum = Rd(code);
  // CommitDelayedLoad();
  MoveGPR(GPR_LO, rd_enum);
}


////////////////////////////////////////
// Jump and Branch u32s
////////////////////////////////////////

// Jump
void Interpreter::J(u32 code) {
  doBranch((Target(code) << 2) | (GPR(GPR_PC) & 0xf0000000));
}

// Jump And Link
void Interpreter::JAL(u32 code) {
  SetGPR(GPR_RA, GPR(GPR_PC) + 4);
  doBranch((Target(code) << 2) | (GPR(GPR_PC) & 0xf0000000));
}

// Jump Register
void Interpreter::JR(u32 code) {
  doBranch(RsVal(GPR(), code));
}

// Jump And Link Register
void Interpreter::JALR(u32 code) {
  u32 rd = Rd(code);
  if (rd != 0) {
    SetGPR(rd, GPR(GPR_PC) + 4);
  }
  doBranch(RsVal(GPR(), code));
}


// Branch on EQual
void Interpreter::BEQ(u32 code) {
  if (RsVal(GPR(), code) == RtVal(GPR(), code)) {
    doBranch(GPR(GPR_PC) + (Imm(code) << 2));
  }
}

// Branch on Not Equal
void Interpreter::BNE(u32 code) {
  if (RsVal(GPR(), code) != RtVal(GPR(), code)) {
    doBranch(GPR(GPR_PC) + (Imm(code) << 2));
  }
}

// Branch on Less than or Equal Zero
void Interpreter::BLEZ(u32 code) {
  if (static_cast<int32_t>(RsVal(GPR(), code)) <= 0) {
    doBranch(GPR(GPR_PC) + (Imm(code) << 2));
  }
}

// Branch on Greate Than Zero
void Interpreter::BGTZ(u32 code) {
  if (static_cast<int32_t>(RsVal(GPR(), code)) > 0) {
    doBranch(GPR(GPR_PC) + (Imm(code) << 2));
  }
}

// Branch on Less Than Zero
void Interpreter::BLTZ(u32 code) {
  if (static_cast<int32_t>(RsVal(GPR(), code)) < 0) {
    doBranch(GPR(GPR_PC) + (Imm(code) << 2));
  }
}

// Branch on Greater than or Equal Zero
void Interpreter::BGEZ(u32 code) {
  if (static_cast<int32_t>(RsVal(GPR(), code)) >= 0) {
    doBranch(GPR(GPR_PC) + (Imm(code) << 2));
  }
}

// Branch on Less Than Zero And Link
void Interpreter::BLTZAL(u32 code) {
  if (static_cast<int32_t>(RsVal(GPR(), code)) < 0) {
    u32 pc = GPR(GPR_PC);
    SetGPR(GPR_RA, pc + 4);
    doBranch(pc + (Imm(code) << 2));
  }
}

// Branch on Greater than or Equal Zero And Link
void Interpreter::BGEZAL(u32 code) {
  if (static_cast<int32_t>(RsVal(GPR(), code)) >= 0) {
    u32 pc = GPR(GPR_PC);
    SetGPR(GPR_RA, pc + 4);
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
  SetGPR(GPR_PC, GPR(GPR_PC) - 4);
  cpu_.Exception(0x20, cpu_.IsInDelaySlot());
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



void Interpreter::COP0(u32 code) {
  rennyLogWarning("PSXInterpreter", "COP0 is not implemented (PC = 0x%08x).", (int)code);
}
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
  // std::printf("HLE Dummy: PC = 0x%08x\n", GPR(GPR_PC) - 4);
  SetGPR(GPR_PC, GPR(GPR_RA));
  cpu_.BranchTest();
}

void Interpreter::hleA0()
{
  rennyAssert(GPR(GPR_PC) == 0x00a0 + 4);
  (bios_.*BIOS::biosA0[GPR(GPR_T1) & 0xff])();
  cpu_.BranchTest();
}

void Interpreter::hleB0()
{
  rennyAssert(GPR(GPR_PC) == 0x00b0 + 4);
  (bios_.*BIOS::biosB0[GPR(GPR_T1) & 0xff])();
  cpu_.BranchTest();
}

void Interpreter::hleC0()
{
  rennyAssert(GPR(GPR_PC) == 0x00c0 + 4);
  (bios_.*BIOS::biosC0[GPR(GPR_T1) & 0xff])();
  cpu_.BranchTest();
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
  EXEC *header = static_cast<EXEC*>(psxMptr(GPR(GPR_S0)));
  SetGPR(GPR_RA, BFLIP32(header->ret));
  SetGPR(GPR_SP, BFLIP32(header->sp));
  SetGPR(GPR_FP, BFLIP32(header->fp));
  SetGPR(GPR_GP, BFLIP32(header->gp));
  SetGPR(GPR_S0, BFLIP32(header->base));
  SetGPR(GPR_V0, 1);
  SetGPR(GPR_PC, GPR(GPR_RA));
}

void (Interpreter::*const Interpreter::HLEt[])() = {
    &Interpreter::hleDummy, &Interpreter::hleA0, &Interpreter::hleB0,
    &Interpreter::hleC0, &Interpreter::hleBootstrap, &Interpreter::hleExecRet
};


void Interpreter::HLECALL(u32 code) {
  (this->*HLEt[code & 0xff])();
}

////////////////////////////////////////////////////////////////
// Opcode LUT
////////////////////////////////////////////////////////////////

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


void Interpreter::ExecuteOnce()
{
  u32 pc = GPR(GPR_PC);
  u32 code(psxMu32val(pc));
  pc += 4;
  SetGPR(GPR_PC, pc);

  ExecuteOpcode(code);
  cpu_.IncreaseCycle();
}

// called from BIOS::Softcall()
void Interpreter::ExecuteBlock() {
  cpu_.doingBranch = false;
  do {
    ExecuteOnce();
  } while (!cpu_.doingBranch);
}

// deprecated
uint32_t Interpreter::Execute(uint32_t cycles) {
  const uint32_t cycle_start = cpu_.cycle32();
  const uint64_t cycle_end = static_cast<uint64_t>(cpu_.cycle32()) + cycles;
  while (cpu_.cycle32() < cycle_end) {
    ExecuteOnce();
  }
  return cpu_.cycle32() - cycle_start;
}

void Interpreter::Shutdown() {}

}   // namespace mips
}   // namespace psx
