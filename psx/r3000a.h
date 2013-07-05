#pragma once
#include "common.h"
#include <wx/thread.h>
#include <wx/msgout.h>


namespace PSX {
namespace R3000A {

////////////////////////////////
// R3000A Registers
////////////////////////////////

union GeneralPurposeRegisters {
    struct {
        u32 ZR, AT;
        union {
            struct { u32 V0, V1; };
            u32 V[2];
        };
        union {
            struct { u32 A0, A1, A2, A3; };
            u32 A[4];
        };
        union {
            struct { u32 T0, T1, T2, T3, T4, T5, T6, T7; };
            u32 T[8];
        };
        union {
            struct { u32 S0, S1, S2, S3, S4, S5, S6, S7; };
            u32 S[8];
        };
        u32 T8, T9;
        union {
            struct { u32 K0, K1; };
            u32 K[2];
        };
        u32 GP, SP, FP, RA, HI, LO, PC;
    };
    u32 R[35];
    GeneralPurposeRegisters(): ZR(0) {}
};

enum GPR_ENUM {
    GPR_ZR, GPR_AT, GPR_V0, GPR_V1, GPR_A0, GPR_A1, GPR_A2, GPR_A3,
    GPR_T0, GPR_T1, GPR_T2, GPR_T3, GPR_T4, GPR_T5, GPR_T6, GPR_T7,
    GPR_S0, GPR_S1, GPR_S2, GPR_S3, GPR_S4, GPR_S5, GPR_S6, GPR_S7,
    GPR_T8, GPR_T9, GPR_K0, GPR_K1, GPR_GP, GPR_SP, GPR_FP, GPR_RA,
    GPR_HI, GPR_LO, GPR_PC
};

extern const char *strGPR[35];

union Cop0Registers {
    struct {
        u32 INDX, RAND, TLBL, BPC, CTXT, BDA, PIDMASK, DCIC,
        BADV, BDAM, TLBH, BPCM, SR, CAUSE, EPC, PRID, ERREG;
    };
    u32 R[17];
};


////////////////////////////////////////////////
// Instruction class
////////////////////////////////////////////////


enum OPCODE_ENUM {
    OPCODE_SPECIAL, OPCODE_BCOND, OPCODE_J, OPCODE_JAL, OPCODE_BEQ, OPCODE_BNE, OPCODE_BLEZ, OPCODE_BGTZ,
    OPCODE_ADDI, OPCODE_ADDIU, OPCODE_SLTI, OPCODE_SLTIU, OPCODE_ANDI, OPCODE_ORI, OPCODE_XORI, OPCODE_LUI,
    OPCODE_COP0, OPCODE_COP1, OPCODE_COP2, OPCODE_COP3,
    OPCODE_LB = 0x20, OPCODE_LH, OPCODE_LWL, OPCODE_LW, OPCODE_LBU, OPCODE_LHU, OPCODE_LWR,
    OPCODE_SB = 0x28, OPCODE_SH, OPCODE_SWL, OPCODE_SW, OPCODE_SWR = 0x2e,
    OPCODE_LWC0 = 0x30, OPCODE_LWC1, OPCODE_LWC2, OPCODE_LWC3,
    OPCODE_SWC0 = 0x38, OPCODE_SWC1, OPCODE_SWC2, OPCODE_SWC3, OPCODE_HLECALL = 0x3b
};

enum SPECIAL_ENUM {
    SPECIAL_SLL, SPECIAL_SRL = 0x02, SPECIAL_SRA, SPECIAL_SLLV, SPECIAL_SRLV = 0x06, SPECIAL_SRAV,
    SPECIAL_JR, SPECIAL_JALR, SPECIAL_SYSCALL = 0x0c, SPECIAL_BREAK,
    SPECIAL_MFHI = 0x10, SPECIAL_MTHI, SPECIAL_MFLO, SPECIAL_MTLO,
    SPECIAL_MULT = 0x18, SPECIAL_MULTU, SPECIAL_DIV, SPECIAL_DIVU,
    SPECIAL_ADD = 0x20, SPECIAL_ADDU, SPECIAL_SUB, SPECIAL_SUBU, SPECIAL_AND, SPECIAL_OR, SPECIAL_XOR, SPECIAL_NOR
};

enum BCOND_ENUM {
    BCOND_BLTZ, BCOND_BGEZ, BCOND_BLTZAL = 0x10, BCOND_BGEZAL
};


class Processor;

class Instruction
{
public:
    Instruction(Processor*, u32);

    u32 Opcode() const;
    u32 Rs() const;
    u32 Rt() const;
    u32 Rd() const;
    int32_t Imm() const;
    u32 ImmU() const;
    u32 Target() const;
    u32 Shamt() const;
    u32 Funct() const;

    u32& RsVal() const;
    u32& RtVal() const;
    u32& RdVal() const;

    u32 Addr() const;

    operator u32() const;

private:
    Processor& cpu_;
    u32 code_;
};


inline Instruction::Instruction(Processor* cpu, u32 code)
  : cpu_(*cpu), code_(code) {}

inline Instruction::operator u32() const {
    return code_;
}



////////////////////////////////////////////////////////////////////////
// R3000A Registers Composite
////////////////////////////////////////////////////////////////////////


struct Registers {
  GeneralPurposeRegisters GPR;
  Cop0Registers CP0;
  u32& HI;
  u32& LO;
  u32& PC;
  u32 Cycle;  // clock count
  u32 Interrupt;

  Registers() : HI(GPR.HI), LO(GPR.LO), PC(GPR.PC) {}
};





////////////////////////////////////////////////////////////////////////
// implement of R3000A processor
////////////////////////////////////////////////////////////////////////

class Processor : public Component
{
public:
    Processor(Composite* composite)
      : Component(composite),
        Regs(R3000ARegs()),
        GPR(Regs.GPR),
        CP0(Regs.CP0),
        HI(Regs.GPR.HI), LO(Regs.GPR.LO),
        PC(Regs.GPR.PC),
        Cycle(Regs.Cycle),
        Interrupt(Regs.Interrupt) {}
    ~Processor() {}

    void Init();
    void Reset();
    void Execute();
    void ExecuteBlock();
    // void Clear(u32 addr, u32 size);
    void Shutdown();

    static Processor& GetInstance();

    void BranchTest();
    void Exception(Instruction code, bool branch_delay);

    bool IsInDelaySlot() const;
    void EnterDelaySlot();
    void LeaveDelaySlot();

    bool isDoingBranch() const;

public:
    Registers& Regs;

private:
    GeneralPurposeRegisters& GPR;
    Cop0Registers& CP0;
    u32& HI;
    u32& LO;
    u32& PC;
    u32& Cycle;
    u32& Interrupt;

    bool inDelaySlot;    // for SYSCALL
    bool doingBranch;    // set when doBranch is called

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

inline bool Processor::isDoingBranch() const {
    return doingBranch;
}

} // namespace R3000A

// an alias of R3000A instance
// extern R3000A::Processor& R3000a;


////////////////////////////////
// Instruction Macros
////////////////////////////////


namespace R3000A {

inline u32 Instruction::Opcode() const {
    return code_ >> 26;
}

inline u32 Instruction::Rs() const {
    return (code_ >> 21) & 0x01f;
}

inline u32 Instruction::Rt() const {
    return (code_ >> 16) & 0x001f;
}

inline u32 Instruction::Rd() const {
    return (code_ >> 11) & 0x0001f;
}


inline u32& Instruction::RsVal() const {
    return cpu_.Regs.GPR.R[Rs()];
}

inline u32& Instruction::RtVal() const {
    return cpu_.Regs.GPR.R[Rt()];
}

inline u32& Instruction::RdVal() const {
    return cpu_.Regs.GPR.R[Rd()];
}


inline int32_t Instruction::Imm() const {
    return static_cast<int32_t>( static_cast<int16_t>(code_ & 0xffff) );
}

inline u32 Instruction::ImmU() const {
    return code_ & 0xffff;
}

inline u32 Instruction::Target() const {
    return code_ & 0x03ffffff;
}

inline u32 Instruction::Shamt() const {
    return (code_ >> 6) & 0x1f;
}

inline u32 Instruction::Funct() const {
    return code_ & 0x3f;
}

inline u32 Instruction::Addr() const {
    return RsVal() + Imm();
}


}   // namespace R3000A

}   // namespace PSX
