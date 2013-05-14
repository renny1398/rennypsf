#pragma once
#include "r3000a.h"
#include "disassembler.h"
#include "memory.h"
#include <wx/thread.h>


namespace PSX {
namespace R3000A {

class InterpreterThread: public wxThread
{
protected:
    InterpreterThread();

public:
    void Shutdown();

private:
    ExitCode Entry();
    void OnExit();

    bool isRunning_;

    friend class Interpreter;
};

class Interpreter
{
private:
    Interpreter() {}
    ~Interpreter() {}

public:
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
    static Interpreter& GetInstance();

private:
    static void delayRead(Instruction code, uint32_t reg, uint32_t branch_pc);
    static void delayWrite(Instruction code, uint32_t reg, uint32_t branch_pc);
    static void delayReadWrite(Instruction code, uint32_t reg, uint32_t branch_pc);

    static bool delayNOP(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayRs(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayWt(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayWd(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsRt(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsWt(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsWd(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayRtWd(Instruction code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsRtWd(Instruction code, uint32_t reg, uint32_t branch_pc);

    typedef bool (*const DelayFunc)(Instruction, uint32_t, uint32_t);

    static bool delaySLL(Instruction code, uint32_t reg, uint32_t branch_pc);
    static DelayFunc delaySRL, delaySRA;
    static DelayFunc delayJR, delayJALR;
    static DelayFunc delayADD, delayADDU;
    static DelayFunc delaySLLV;
    static DelayFunc delayMFHI, delayMFLO;
    static DelayFunc delayMTHI, delayMTLO;
    static DelayFunc delayMULT, delayDIV;

    static bool delaySPECIAL(Instruction, uint32_t, uint32_t);
    static DelayFunc delayBCOND;

    static bool delayJAL(Instruction, uint32_t, uint32_t);
    static DelayFunc delayBEQ, delayBNE;
    static DelayFunc delayBLEZ, delayBGTZ;
    static DelayFunc delayADDI, delayADDIU;
    static DelayFunc delayLUI;

    static bool delayCOP0(Instruction, uint32_t, uint32_t);
    static bool delayLWL(Instruction, uint32_t, uint32_t);
    static DelayFunc delayLWR;
    static DelayFunc delayLB, delayLH, delayLW, delayLBU, delayLHU;
    static DelayFunc delaySB, delaySH, delaySWL, delaySW, delaySWR;
    static DelayFunc delayLWC2, delaySWC2;

    static void delayTest(uint32_t reg, uint32_t branch_pc);
    static void doBranch(uint32_t branch_pc);

    // Opcodes
    static void LB(Instruction);
    static void LBU(Instruction);
    static void LH(Instruction);
    static void LHU(Instruction);
    static void LW(Instruction);
    static void LWL(Instruction);
    static void LWR(Instruction);

    static void SB(Instruction);
    static void SH(Instruction);
    static void SW(Instruction);
    static void SWL(Instruction);
    static void SWR(Instruction);

    static void ADDI(Instruction);
    static void ADDIU(Instruction);
    static void SLTI(Instruction);
    static void SLTIU(Instruction);
    static void ANDI(Instruction);
    static void ORI(Instruction);
    static void XORI(Instruction);
    static void LUI(Instruction);

    static void ADD(Instruction);
    static void ADDU(Instruction);
    static void SUB(Instruction);
    static void SUBU(Instruction);
    static void SLT(Instruction);
    static void SLTU(Instruction);
    static void AND(Instruction);
    static void OR(Instruction);
    static void XOR(Instruction);
    static void NOR(Instruction);

    static void SLL(Instruction);
    static void SRL(Instruction);
    static void SRA(Instruction);
    static void SLLV(Instruction);
    static void SRLV(Instruction);
    static void SRAV(Instruction);

    static void MULT(Instruction);
    static void MULTU(Instruction);
    static void DIV(Instruction);
    static void DIVU(Instruction);
    static void MFHI(Instruction);
    static void MFLO(Instruction);
    static void MTHI(Instruction);
    static void MTLO(Instruction);

    static void J(Instruction);
    static void JAL(Instruction);
    static void JR(Instruction);
    static void JALR(Instruction);
    static void BEQ(Instruction);
    static void BNE(Instruction);
    static void BLEZ(Instruction);
    static void BGTZ(Instruction);
    static void BLTZ(Instruction);
    static void BGEZ(Instruction);
    static void BLTZAL(Instruction);
    static void BGEZAL(Instruction);

    static void BCOND(Instruction);
    static void SYSCALL(Instruction);
    static void BREAK(Instruction);
    static void SPECIAL(Instruction);

    static void LWC0(Instruction);
    static void LWC1(Instruction);
    static void LWC2(Instruction);
    static void LWC3(Instruction);
    static void SWC0(Instruction);
    static void SWC1(Instruction);
    static void SWC2(Instruction);
    static void SWC3(Instruction);
    static void COP0(Instruction);
    static void COP1(Instruction);
    static void COP2(Instruction);
    static void COP3(Instruction);

    static void HLECALL(Instruction);

private:
    static const DelayFunc delaySpecials[64];
    static const DelayFunc delayOpcodes[64];

    static void (*OPCODES[64])(Instruction);
    static void (*SPECIALS[64])(Instruction);
    static void (*BCONDS[24])(Instruction);
    static void (*COPz[16])(Instruction);

    static InterpreterThread* thread;
};


inline void Interpreter::ExecuteOpcode(Instruction code) {
    OPCODES[code.Opcode()](code);
}

inline void Interpreter::ExecuteOnce()
{
    uint32_t pc = R3000a.PC;
    Instruction code(u32M(pc));
    pc += 4;
    ++R3000a.Cycle;
    R3000a.PC = pc;
    //last_code = code;
    //if (pc == 0x00001e94 - 4) {
    //    wxMessageOutputDebug().Printf("Int8");
    //}
    ExecuteOpcode(code);
}

}   // namespace Interpreter

// an alias of InterpretImpl instance
extern R3000A::Interpreter& Interpreter_;

}   // namespace PSX
