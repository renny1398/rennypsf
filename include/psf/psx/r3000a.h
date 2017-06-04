#pragma once
// #include "common.h"
#include <wx/thread.h>
#include "common/debug.h"
#include "memory.h"
#include "hardware.h"

class SoundBlock;

namespace SPU {
class SPUBase;
}

namespace psx {

enum GPR_ENUM {
  GPR_ZR, GPR_AT, GPR_V0, GPR_V1, GPR_A0, GPR_A1, GPR_A2, GPR_A3,
  GPR_T0, GPR_T1, GPR_T2, GPR_T3, GPR_T4, GPR_T5, GPR_T6, GPR_T7,
  GPR_S0, GPR_S1, GPR_S2, GPR_S3, GPR_S4, GPR_S5, GPR_S6, GPR_S7,
  GPR_T8, GPR_T9, GPR_K0, GPR_K1, GPR_GP, GPR_SP, GPR_FP, GPR_RA,
  GPR_HI, GPR_LO, GPR_PC
};

enum COP0_ENUM {
  COP0_INDX, COP0_RAND, COP0_TLBL, COP0_BPC,  COP0_CTXT, COP0_BDA,   COP0_PIDMASK, COP0_DCIC,
  COP0_BADV, COP0_BDAM, COP0_TLBH, COP0_BPCM, COP0_SR,   COP0_CAUSE, COP0_EPC,     COP0_PRID,
  COP0_ERREG
};

namespace mips {

////////////////////////////////////////////////////////////////////////
// R3000A Registers
////////////////////////////////////////////////////////////////////////

class GeneralPurposeRegisters {
public:
  GeneralPurposeRegisters();
  GeneralPurposeRegisters(const GeneralPurposeRegisters& src);
  GeneralPurposeRegisters& operator=(const GeneralPurposeRegisters&) = delete;
  void Set(const GeneralPurposeRegisters& src);
  void Reset();
  u32& operator()(u32 i);
  const u32& operator()(u32 i) const;
private:
  u32 R[35];
public:
  volatile const u32 &ZR;
  u32 &AT;
  u32 &V0, &V1;
  u32 &A0, &A1, &A2, &A3;
  u32 &T0, &T1, &T2, &T3, &T4, &T5, &T6, &T7;
  u32 &S0, &S1, &S2, &S3, &S4, &S5, &S6, &S7;
  u32 &T8, &T9;
  u32 &K0, &K1;
  u32 &GP, &SP, &FP, &RA;
  u32 &HI, &LO, &PC;
};

// extern const char* strGPR[35];

class Cop0Registers {
public:
  Cop0Registers();
  Cop0Registers& operator=(const Cop0Registers&) = delete;
  void Reset();
  u32& operator()(u32 i);
private:
  u32 R[17];
public:
  u32& INDX;
  u32& RAND;
  u32& TLBL;
  u32& BPC;
  u32& CTXT;
  u32& BDA;
  u32& PIDMASK;
  u32& DCIC;
  u32& BADV;
  u32& BDAM;
  u32& TLBH;
  u32& BPCM;
  u32& SR;
  u32& CAUSE;
  u32& EPC;
  u32& PRID;
  u32& ERREG;
};

////////////////////////////////////////////////////////////////////////
// Instruction class
////////////////////////////////////////////////////////////////////////

enum OPCODE_ENUM {
  OPCODE_SPECIAL,     OPCODE_BCOND, OPCODE_J,    OPCODE_JAL,
  OPCODE_BEQ,         OPCODE_BNE,   OPCODE_BLEZ, OPCODE_BGTZ,
  OPCODE_ADDI,        OPCODE_ADDIU, OPCODE_SLTI, OPCODE_SLTIU,
  OPCODE_ANDI,        OPCODE_ORI,   OPCODE_XORI, OPCODE_LUI,
  OPCODE_COP0,        OPCODE_COP1,  OPCODE_COP2, OPCODE_COP3,
  OPCODE_LB = 0x20,   OPCODE_LH,    OPCODE_LWL,  OPCODE_LW,
  OPCODE_LBU,         OPCODE_LHU,   OPCODE_LWR,
  OPCODE_SB = 0x28,   OPCODE_SH,    OPCODE_SWL,  OPCODE_SW,
  OPCODE_SWR = 0x2e,
  OPCODE_LWC0 = 0x30, OPCODE_LWC1,  OPCODE_LWC2, OPCODE_LWC3,
  OPCODE_SWC0 = 0x38, OPCODE_SWC1,  OPCODE_SWC2, OPCODE_SWC3,
  OPCODE_HLECALL = 0x3b,
  OPCODE_MAX = 0x40
};

enum SPECIAL_ENUM {
  SPECIAL_SLL,
  SPECIAL_SRL = 0x02,     SPECIAL_SRA,   SPECIAL_SLLV,
  SPECIAL_SRLV = 0x06,    SPECIAL_SRAV,  SPECIAL_JR,   SPECIAL_JALR,
  SPECIAL_SYSCALL = 0x0c, SPECIAL_BREAK,
  SPECIAL_MFHI = 0x10,    SPECIAL_MTHI,  SPECIAL_MFLO, SPECIAL_MTLO,
  SPECIAL_MULT = 0x18,    SPECIAL_MULTU, SPECIAL_DIV,  SPECIAL_DIVU,
  SPECIAL_ADD = 0x20,     SPECIAL_ADDU,  SPECIAL_SUB,  SPECIAL_SUBU,
  SPECIAL_AND,            SPECIAL_OR,    SPECIAL_XOR,  SPECIAL_NOR,
  SPECIAL_SLT = 0x2a,     SPECIAL_SLTU,
  SPECIAL_MAX = 0x40
};

enum BCOND_ENUM {
  BCOND_BLTZ,          BCOND_BGEZ,
  BCOND_BLTZAL = 0x10, BCOND_BGEZAL
};

////////////////////////////////////////////////////////////////////////
// R3000A Registers PSX
////////////////////////////////////////////////////////////////////////

class Registers {
public:
  Registers()
    : HI(GPR.HI), LO(GPR.LO), PC(GPR.PC), Interrupt(0),
      shift_amount(0) {}
  Registers& operator=(const Registers&) = delete;
  void Reset();

  GeneralPurposeRegisters GPR;
  Cop0Registers CP0;
  u32& HI;
  u32& LO;
  u32& PC;
  u32  Interrupt;

  u32 shift_amount;

  friend class RegisterAccessor;
};

////////////////////////////////////////////////////////////////////////
// R3000A Registers Accessor
////////////////////////////////////////////////////////////////////////

class RegisterAccessor {
public:
  RegisterAccessor(Registers& regs);
  RegisterAccessor(PSX* psx);

  u32 GPR(u32 i) const { return regs_.GPR(i); }
  const GeneralPurposeRegisters& GPR() const { return regs_.GPR; }
  void SetGPR(u32 i, u32 value) { regs_.GPR(i) = value; }
  void SetGPR(const GeneralPurposeRegisters& src) { regs_.GPR.Set(src); }
  void MoveGPR(u32 dest, u32 src) { regs_.GPR(dest) = regs_.GPR(src); }
  void ResetGPR() { regs_.GPR.Reset(); }

  u32 CP0(u32 i) const { return regs_.CP0(i); }
  void SetCP0(u32 i, u32 value) { regs_.CP0(i) = value; }
  void ResetCP0() { regs_.CP0.Reset(); }

  u32 SA() const { return regs_.shift_amount; }
  void SetSA(u32 sa) { regs_.shift_amount = sa; }

  void ResetRegisters();

private:
  Registers& regs_;

protected:
  u32& HI;
  u32& LO;
  u32& PC;
  // u32& Cycle;
  u32& Interrupt;
};

////////////////////////////////////////////////////////////////////////
// R3000A Processor Class Definition
////////////////////////////////////////////////////////////////////////

class Interpreter;

// TODO: implement DelaySlot
class Processor
    : public RegisterAccessor, private MemoryAccessor,
      private IRQAccessor {
public:
  Processor(PSX* psx);
  ~Processor() = default;

  void SetRcntReferent(RootCounterManager* p_rcnt);
  void SetBIOSReferent(BIOS* p_bios);

  void Reset();
  void Execute();
  void ExecuteBlock();
  // void Clear(u32 addr, u32 size);
  void Shutdown();

  // Conter Cycle Functions
  unsigned int cycle32() const;
  void IncreaseCycle();

  // Execute Functions
  void Execute(Interpreter* interp, bool in_softcall = false);

  // Load Functions (TODO: Delay Load)
  void Load8s(u32 reg_enum, PSXAddr addr);
  void Load8u(u32 reg_enum, PSXAddr addr);
  void Load16s(u32 reg_enum, PSXAddr addr);
  void Load16u(u32 reg_enum, PSXAddr addr);
  void Load32(u32 reg_enum, PSXAddr addr);
  void Load32Left(u32 reg_enum, PSXAddr addr);
  void Load32Right(u32 reg_enum, PSXAddr addr);

  // Store Functions
  void Store8(u32 reg_enum, PSXAddr addr);
  void Store16(u32 reg_enum, PSXAddr addr);
  void Store32(u32 reg_enum, PSXAddr addr);
  void Store32Left(u32 reg_enum, PSXAddr addr);
  void Store32Right(u32 reg_enum, PSXAddr addr);

  // Arithmetic Functions
  void Add(u32 rd_enum, u32 rs_enum, u32 rt_enum, bool trap_on_ovf);
  void AddImmediate(u32 rt_enum, u32 rs_enum, s32 imm, bool trap_on_ovf);
  void Sub(u32 rd_enum, u32 rs_enum, u32 rt_enum, bool trap_on_ovf);
  void Mul(u32 rs_enum, u32 rt_enum);
  void MulUnsigned(u32 rs_enum, u32 rt_enum);
  void Div(u32 rs_enum, u32 rt_enum);
  void DivUnsigned(u32 rs_enum, u32 rt_enum);

  void BranchTest();
  void Exception(uint32_t code, bool branch_delay);

  bool IsInDelaySlot() const;
  void EnterDelaySlot();
  void LeaveDelaySlot();
  bool IsRAAlone() const;
  void LeaveRAAlone() { leaveRAalone = true; }
  void EnableEnteringRA();

  bool isDoingBranch() const;

  void DeadLoopSkip();

  Registers Regs; // TODO: into private variable
  GeneralPurposeRegisters& GPR;

private:
  RootCounterManager* p_rcnt_;
  BIOS* p_bios_;

  bool inDelaySlot;    // for SYSCALL
  bool doingBranch;    // set when doBranch is called
  bool leaveRAalone;   // for LoadModuleStart

  friend class Interpreter;
  friend class Recompiler;
  friend class Disassembler;
};

inline bool Processor::IsInDelaySlot() const {
  return inDelaySlot;
}

inline void Processor::EnterDelaySlot() {
  inDelaySlot = true;
}

inline void Processor::LeaveDelaySlot() {
  inDelaySlot = false;
}

inline bool Processor::IsRAAlone() const {
  return leaveRAalone;
}

/*
inline void Processor::LeaveRAAlone() {
  leaveRAalone = true;
}
*/

inline void Processor::EnableEnteringRA() {
  leaveRAalone = false;
}

inline bool Processor::isDoingBranch() const {
  return doingBranch;
}

} // namespace mips

////////////////////////////////////////////////////////////////////////
// Instruction Macros
////////////////////////////////////////////////////////////////////////

namespace mips {

inline u32 Opcode(u32 ins) { return ins >> 26; }
inline u32 Rs(u32 ins) { return (ins >> 21) & 0x01f; }
inline u32 Rt(u32 ins) { return (ins >> 16) & 0x001f; }
inline u32 Rd(u32 ins) { return (ins >> 11) & 0x0001f; }
inline s32 Imm(u32 ins) { return static_cast<s32>( static_cast<s16>(ins & 0xffff) ); }
inline u32 ImmU(u32 ins) { return ins & 0xffff; }
inline u32 Target(u32 ins) { return ins & 0x03ffffff; }
inline u32 Shamt(u32 ins) { return (ins >> 6) & 0x1f; }
inline u32 Funct(u32 ins) { return ins & 0x3f; }

inline u32 RsVal(const GeneralPurposeRegisters& reg, u32 ins) {
  return reg(Rs(ins));
}
inline u32 RtVal(const GeneralPurposeRegisters& reg, u32 ins) {
  return reg(Rt(ins));
}
inline u32 RdVal(const GeneralPurposeRegisters& reg, u32 ins) {
  return reg(Rd(ins));
}
inline u32 Addr(const GeneralPurposeRegisters& reg, u32 ins) {
  return RsVal(reg, ins) + Imm(ins);
}

inline u32 EncodeI(u32 opcode, u32 rs_enum, u32 rt_enum, s32 imm) {
  return (opcode << 26) | (rs_enum << 21) | (rt_enum << 16) | (imm & 0xffff);
}

inline u32 EncodeJ(u32 opcode, u32 target) {
  return (opcode << 26) | (target & 0x03ffffff);
}

inline u32 EncodeR(u32 opcode, u32 rs_enum, u32 rt_enum, u32 rd_enum, u32 shamt, u32 funct) {
  return (opcode << 26) | (rs_enum << 21) | (rt_enum << 16) | (rd_enum << 11) |
      (shamt << 6) | funct;
}

}   // namespace mips

}   // namespace psx
