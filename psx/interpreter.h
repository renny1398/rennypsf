#pragma once
#include "r3000a.h"
#include "disassembler.h"
#include "memory.h"
#include <wx/thread.h>


namespace PSX {

class InterpreterThread: public wxThread
{
protected:
    InterpreterThread(): wxThread(wxTHREAD_DETACHED) {}

private:
    ExitCode Entry();
    void OnExit();

    friend class InterpreterImpl;
};

class InterpreterImpl
{
private:
    InterpreterImpl() {}
    ~InterpreterImpl() {}

public:
    void Init();
    void Reset();
    void ExecuteBIOS();

    void ExecuteOnce();
    void ExecuteBlock();

    InterpreterThread* Execute();
    void Shutdown();
private:
    void ExecuteOpcode(uint32_t code);

public:
    static InterpreterImpl& GetInstance();

private:
    static void delayRead(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static void delayWrite(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static void delayReadWrite(uint32_t code, uint32_t reg, uint32_t branch_pc);

    static bool delayNOP(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayRs(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayWt(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayWd(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsRt(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsWt(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsWd(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayRtWd(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static bool delayRsRtWd(uint32_t code, uint32_t reg, uint32_t branch_pc);

    typedef bool (*const DelayFunc)(uint32_t, uint32_t, uint32_t);

    static bool delaySLL(uint32_t code, uint32_t reg, uint32_t branch_pc);
    static DelayFunc delaySRL, delaySRA;
    static DelayFunc delayJR, delayJALR;
    static DelayFunc delayADD, delayADDU;
    static DelayFunc delaySLLV;
    static DelayFunc delayMFHI, delayMFLO;
    static DelayFunc delayMTHI, delayMTLO;
    static DelayFunc delayMULT, delayDIV;

    static bool delaySPECIAL(uint32_t, uint32_t, uint32_t);
    static DelayFunc delayBCOND;

    static bool delayJAL(uint32_t, uint32_t, uint32_t);
    static DelayFunc delayBEQ, delayBNE;
    static DelayFunc delayBLEZ, delayBGTZ;
    static DelayFunc delayADDI, delayADDIU;
    static DelayFunc delayLUI;

    static bool delayCOP0(uint32_t, uint32_t, uint32_t);
    static bool delayLWL(uint32_t, uint32_t, uint32_t);
    static DelayFunc delayLWR;
    static DelayFunc delayLB, delayLH, delayLW, delayLBU, delayLHU;
    static DelayFunc delaySB, delaySH, delaySWL, delaySW, delaySWR;
    static DelayFunc delayLWC2, delaySWC2;

    static void delayTest(uint32_t reg, uint32_t branch_pc);
    static void doBranch(uint32_t branch_pc);

    // Opcodes
    static void LB(uint32_t);
    static void LBU(uint32_t);
    static void LH(uint32_t);
    static void LHU(uint32_t);
    static void LW(uint32_t);
    static void LWL(uint32_t);
    static void LWR(uint32_t);

    static void SB(uint32_t);
    static void SH(uint32_t);
    static void SW(uint32_t);
    static void SWL(uint32_t);
    static void SWR(uint32_t);

    static void ADDI(uint32_t);
    static void ADDIU(uint32_t);
    static void SLTI(uint32_t);
    static void SLTIU(uint32_t);
    static void ANDI(uint32_t);
    static void ORI(uint32_t);
    static void XORI(uint32_t);
    static void LUI(uint32_t);

    static void ADD(uint32_t);
    static void ADDU(uint32_t);
    static void SUB(uint32_t);
    static void SUBU(uint32_t);
    static void SLT(uint32_t);
    static void SLTU(uint32_t);
    static void AND(uint32_t);
    static void OR(uint32_t);
    static void XOR(uint32_t);
    static void NOR(uint32_t);

    static void SLL(uint32_t);
    static void SRL(uint32_t);
    static void SRA(uint32_t);
    static void SLLV(uint32_t);
    static void SRLV(uint32_t);
    static void SRAV(uint32_t);

    static void MULT(uint32_t);
    static void MULTU(uint32_t);
    static void DIV(uint32_t);
    static void DIVU(uint32_t);
    static void MFHI(uint32_t);
    static void MFLO(uint32_t);
    static void MTHI(uint32_t);
    static void MTLO(uint32_t);

    static void J(uint32_t);
    static void JAL(uint32_t);
    static void JR(uint32_t);
    static void JALR(uint32_t);
    static void BEQ(uint32_t);
    static void BNE(uint32_t);
    static void BLEZ(uint32_t);
    static void BGTZ(uint32_t);
    static void BLTZ(uint32_t);
    static void BGEZ(uint32_t);
    static void BLTZAL(uint32_t);
    static void BGEZAL(uint32_t);

    static void BCOND(uint32_t);
    static void SYSCALL(uint32_t);
    static void BREAK(uint32_t);
    static void SPECIAL(uint32_t);

    static void LWC0(uint32_t);
    static void LWC1(uint32_t);
    static void LWC2(uint32_t);
    static void LWC3(uint32_t);
    static void SWC0(uint32_t);
    static void SWC1(uint32_t);
    static void SWC2(uint32_t);
    static void SWC3(uint32_t);
    static void COP0(uint32_t);
    static void COP1(uint32_t);
    static void COP2(uint32_t);
    static void COP3(uint32_t);

    static void HLECALL(uint32_t);

private:
    static const DelayFunc delaySpecials[64];
    static const DelayFunc delayOpcodes[64];

    static void (*OPCODES[64])(uint32_t);
    static void (*SPECIALS[64])(uint32_t);
    static void (*BCONDS[24])(uint32_t);
    static void (*COPz[16])(uint32_t);

    static InterpreterThread* thread;
};

// an alias of InterpretImpl instance
extern InterpreterImpl& Interpreter_;


inline void InterpreterImpl::ExecuteOpcode(uint32_t code) {
    //Disasm.Parse(code);
    //Disasm.PrintCode();
    OPCODES[opcode_(code)](code);
    //Disasm.PrintChangedRegisters();
}

inline void InterpreterImpl::ExecuteOnce()
{
    uint32_t pc = R3000a.PC;
    uint32_t code = psxMu32(pc);
    //wxMessageOutputDebug().Printf("PC = 0x%08x", pc);
    if ((pc & 0x3ffff) == 0) {
        wxMessageOutputDebug().Printf("PC = 0, code = %02x", code);
    }
    pc += 4;
    ++R3000a.Cycle;
    R3000a.PC = pc;
    //last_code = code;
    ExecuteOpcode(code);
}
}   // namespace PSX
