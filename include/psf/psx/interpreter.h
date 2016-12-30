#pragma once
#include "common.h"
#include "r3000a.h"
#include "disassembler.h"
#include "memory.h"

namespace PSX {

class BIOS;
class IOP;

namespace R3000A {

class Interpreter : /*public Component, */private RegisterAccessor, private UserMemoryAccessor
{
public:
  Interpreter(Composite* psx, Processor* cpu, BIOS* bios, IOP* iop);
  ~Interpreter() {}

  void Init(RootCounterManager* rcnt);
  void Reset();
  void ExecuteBIOS();

  uint32_t Execute(uint32_t cycles);
  void ExecuteOnce();
  void ExecuteBlock();

  void Shutdown();
private:
  void ExecuteOpcode(u32 code);

private:
  void delayRead(u32 code, u32 reg, u32 branch_pc);
  void delayWrite(u32 code, u32 reg, u32 branch_pc);
  void delayReadWrite(u32 code, u32 reg, u32 branch_pc);

  bool delayNOP(u32 code, u32 reg, u32 branch_pc);
  bool delayRs(u32 code, u32 reg, u32 branch_pc);
  bool delayWt(u32 code, u32 reg, u32 branch_pc);
  bool delayWd(u32 code, u32 reg, u32 branch_pc);
  bool delayRsRt(u32 code, u32 reg, u32 branch_pc);
  bool delayRsWt(u32 code, u32 reg, u32 branch_pc);
  bool delayRsWd(u32 code, u32 reg, u32 branch_pc);
  bool delayRtWd(u32 code, u32 reg, u32 branch_pc);
  bool delayRsRtWd(u32 code, u32 reg, u32 branch_pc);

  typedef bool (Interpreter::*const DelayFunc)(u32, u32, u32);

  bool delaySLL(u32 code, u32 reg, u32 branch_pc);
  static DelayFunc delaySRL, delaySRA;
  static DelayFunc delayJR, delayJALR;
  static DelayFunc delayADD, delayADDU;
  static DelayFunc delaySLLV;
  static DelayFunc delayMFHI, delayMFLO;
  static DelayFunc delayMTHI, delayMTLO;
  static DelayFunc delayMULT, delayDIV;

  bool delaySPCL(u32, u32, u32);
  static DelayFunc delayBCOND;

  bool delayJAL(u32, u32, u32);
  static DelayFunc delayBEQ, delayBNE;
  static DelayFunc delayBLEZ, delayBGTZ;
  static DelayFunc delayADDI, delayADDIU;
  static DelayFunc delayLUI;

  bool delayCOP0(u32, u32, u32);
  bool delayLWL(u32, u32, u32);
  static DelayFunc delayLWR;
  static DelayFunc delayLB, delayLH, delayLW, delayLBU, delayLHU;
  static DelayFunc delaySB, delaySH, delaySWL, delaySW, delaySWR;
  static DelayFunc delayLWC2, delaySWC2;

  void delayTest(u32 reg, u32 branch_pc);
  void doBranch(u32 branch_pc);

  void Load(u32 rt, u32 value);

  // Opcodes
  void NLOP(u32);

  void LB(u32);
  void LBU(u32);
  void LH(u32);
  void LHU(u32);
  void LW(u32);
  void LWL(u32);
  void LWR(u32);

  void SB(u32);
  void SH(u32);
  void SW(u32);
  void SWL(u32);
  void SWR(u32);

  void ADDI(u32);
  void ADDIU(u32);
  void SLTI(u32);
  void SLTIU(u32);
  void ANDI(u32);
  void ORI(u32);
  void XORI(u32);
  void LUI(u32);

  void ADD(u32);
  void ADDU(u32);
  void SUB(u32);
  void SUBU(u32);
  void SLT(u32);
  void SLTU(u32);
  void AND(u32);
  void OR(u32);
  void XOR(u32);
  void NOR(u32);

  void SLL(u32);
  void SRL(u32);
  void SRA(u32);
  void SLLV(u32);
  void SRLV(u32);
  void SRAV(u32);

  void MULT(u32);
  void MULTU(u32);
  void DIV(u32);
  void DIVU(u32);
  void MFHI(u32);
  void MFLO(u32);
  void MTHI(u32);
  void MTLO(u32);

  void J(u32);
  void JAL(u32);
  void JR(u32);
  void JALR(u32);
  void BEQ(u32);
  void BNE(u32);
  void BLEZ(u32);
  void BGTZ(u32);
  void BLTZ(u32);
  void BGEZ(u32);
  void BLTZAL(u32);
  void BGEZAL(u32);

  void BCOND(u32);
  void SYSCALL(u32);
  void BREAK(u32);
  void SPCL(u32);

  void LWC0(u32);
  void LWC1(u32);
  void LWC2(u32);
  void LWC3(u32);
  void SWC0(u32);
  void SWC1(u32);
  void SWC2(u32);
  void SWC3(u32);
  void COP0(u32);
  void COP1(u32);
  void COP2(u32);
  void COP3(u32);

  void HLECALL(u32);

  // HLE functions
  void hleDummy();
  void hleA0();
  void hleB0();
  void hleC0();
  void hleBootstrap();
  void hleExecRet();

private:
  Processor& cpu_;
  BIOS& bios_;
  IOP& iop_;

  RootCounterManager* rcnt_;  // for DeadLoopSkip

  static DelayFunc delaySpecials[64];
  static DelayFunc delayOpcodes[64];

  static void (Interpreter::*const OPCODES[64])(u32);
  static void (Interpreter::*const SPECIALS[64])(u32);
  static void (Interpreter::*const BCONDS[24])(u32);
  static void (Interpreter::*const COPz[16])(u32);

  static void (Interpreter::*const HLEt[])();
};

inline void Interpreter::ExecuteOpcode(u32 code) {
  // Disasm().OutputToFile();
  (this->*OPCODES[Opcode(code)])(code);
}

}   // namespace Interpreter

}   // namespace PSX
