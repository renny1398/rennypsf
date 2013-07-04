#pragma once

#include "common.h"
#include "r3000a.h"
#include "disassembler.h"
#include "memory.h"
#include <wx/thread.h>


namespace PSX {
  namespace R3000A {

    class InterpreterThread: public wxThread
    {
     public:
      void Shutdown();

     protected:
      InterpreterThread(Interpreter* interp);
      ExitCode Entry();
      void OnExit();

     private:
      Interpreter& interp_;
      bool isRunning_;

      friend class Interpreter;
    };

    class Interpreter : public Component
    {
    public:
      Interpreter(Composite* composite, Processor* cpu)
        : Component(composite), cpu_(*cpu) {}
      ~Interpreter() {}

      void Init();
      void Reset();
      void ExecuteBIOS();

      void ExecuteOnce();
      void ExecuteBlock();

      InterpreterThread* Execute();
      void Shutdown();
    private:
      void ExecuteOpcode(Instruction code);

    public:
      // static Interpreter& GetInstance();

    private:
      void delayRead(Instruction code, u32 reg, u32 branch_pc);
      void delayWrite(Instruction code, u32 reg, u32 branch_pc);
      void delayReadWrite(Instruction code, u32 reg, u32 branch_pc);

      bool delayNOP(Instruction code, u32 reg, u32 branch_pc);
      bool delayRs(Instruction code, u32 reg, u32 branch_pc);
      bool delayWt(Instruction code, u32 reg, u32 branch_pc);
      bool delayWd(Instruction code, u32 reg, u32 branch_pc);
      bool delayRsRt(Instruction code, u32 reg, u32 branch_pc);
      bool delayRsWt(Instruction code, u32 reg, u32 branch_pc);
      bool delayRsWd(Instruction code, u32 reg, u32 branch_pc);
      bool delayRtWd(Instruction code, u32 reg, u32 branch_pc);
      bool delayRsRtWd(Instruction code, u32 reg, u32 branch_pc);

      typedef bool (Interpreter::*const DelayFunc)(Instruction, u32, u32);

      bool delaySLL(Instruction code, u32 reg, u32 branch_pc);
      static DelayFunc delaySRL, delaySRA;
      static DelayFunc delayJR, delayJALR;
      static DelayFunc delayADD, delayADDU;
      static DelayFunc delaySLLV;
      static DelayFunc delayMFHI, delayMFLO;
      static DelayFunc delayMTHI, delayMTLO;
      static DelayFunc delayMULT, delayDIV;

      bool delaySPCL(Instruction, u32, u32);
      static DelayFunc delayBCOND;

      bool delayJAL(Instruction, u32, u32);
      static DelayFunc delayBEQ, delayBNE;
      static DelayFunc delayBLEZ, delayBGTZ;
      static DelayFunc delayADDI, delayADDIU;
      static DelayFunc delayLUI;

      bool delayCOP0(Instruction, u32, u32);
      bool delayLWL(Instruction, u32, u32);
      static DelayFunc delayLWR;
      static DelayFunc delayLB, delayLH, delayLW, delayLBU, delayLHU;
      static DelayFunc delaySB, delaySH, delaySWL, delaySW, delaySWR;
      static DelayFunc delayLWC2, delaySWC2;

      void delayTest(u32 reg, u32 branch_pc);
      void doBranch(u32 branch_pc);

      void Load(u32 rt, u32 value);

      // Opcodes
      void NLOP(Instruction);

      void LB(Instruction);
      void LBU(Instruction);
      void LH(Instruction);
      void LHU(Instruction);
      void LW(Instruction);
      void LWL(Instruction);
      void LWR(Instruction);

      void SB(Instruction);
      void SH(Instruction);
      void SW(Instruction);
      void SWL(Instruction);
      void SWR(Instruction);

      void ADDI(Instruction);
      void ADDIU(Instruction);
      void SLTI(Instruction);
      void SLTIU(Instruction);
      void ANDI(Instruction);
      void ORI(Instruction);
      void XORI(Instruction);
      void LUI(Instruction);

      void ADD(Instruction);
      void ADDU(Instruction);
      void SUB(Instruction);
      void SUBU(Instruction);
      void SLT(Instruction);
      void SLTU(Instruction);
      void AND(Instruction);
      void OR(Instruction);
      void XOR(Instruction);
      void NOR(Instruction);

      void SLL(Instruction);
      void SRL(Instruction);
      void SRA(Instruction);
      void SLLV(Instruction);
      void SRLV(Instruction);
      void SRAV(Instruction);

      void MULT(Instruction);
      void MULTU(Instruction);
      void DIV(Instruction);
      void DIVU(Instruction);
      void MFHI(Instruction);
      void MFLO(Instruction);
      void MTHI(Instruction);
      void MTLO(Instruction);

      void J(Instruction);
      void JAL(Instruction);
      void JR(Instruction);
      void JALR(Instruction);
      void BEQ(Instruction);
      void BNE(Instruction);
      void BLEZ(Instruction);
      void BGTZ(Instruction);
      void BLTZ(Instruction);
      void BGEZ(Instruction);
      void BLTZAL(Instruction);
      void BGEZAL(Instruction);

      void BCOND(Instruction);
      void SYSCALL(Instruction);
      void BREAK(Instruction);
      void SPCL(Instruction);

      void LWC0(Instruction);
      void LWC1(Instruction);
      void LWC2(Instruction);
      void LWC3(Instruction);
      void SWC0(Instruction);
      void SWC1(Instruction);
      void SWC2(Instruction);
      void SWC3(Instruction);
      void COP0(Instruction);
      void COP1(Instruction);
      void COP2(Instruction);
      void COP3(Instruction);

      void HLECALL(Instruction);

      // HLE functions
      void hleDummy();
      void hleA0();
      void hleB0();
      void hleC0();
      void hleBootstrap();
      void hleExecRet();

    private:
      Processor& cpu_;

      static DelayFunc delaySpecials[64];
      static DelayFunc delayOpcodes[64];

      static void (Interpreter::*const OPCODES[64])(Instruction);
      static void (Interpreter::*const SPECIALS[64])(Instruction);
      static void (Interpreter::*const BCONDS[24])(Instruction);
      static void (Interpreter::*const COPz[16])(Instruction);

      static void (Interpreter::*const HLEt[])();

      static InterpreterThread* thread;
    };


    inline void Interpreter::ExecuteOpcode(Instruction code) {
      (this->*OPCODES[code.Opcode()])(code);
    }

    inline void Interpreter::ExecuteOnce()
    {
      u32 pc = R3000ARegs().PC;
      Instruction code(&cpu_, U32M_ref(pc));
      pc += 4;
      ++R3000ARegs().Cycle;
      R3000ARegs().PC = pc;
      //last_code = code;
      //if (pc == 0x00001e94 - 4) {
      //    wxMessageOutputDebug().Printf("Int8");
      //}
      ExecuteOpcode(code);
    }

  }   // namespace Interpreter

  // an alias of InterpretImpl instance
  // extern R3000A::Interpreter& Interpreter_;

}   // namespace PSX
