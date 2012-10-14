#pragma once
#include <stdint.h>
#include <wx/thread.h>
#include <wx/msgout.h>


namespace PSX {
namespace R3000A {

////////////////////////////////
// R3000A Registers
////////////////////////////////

union GeneralPurposeRegisters {
    struct {
        uint32_t ZR, AT;
        union {
            struct { uint32_t V0, V1; };
            uint32_t V[2];
        };
        union {
            struct { uint32_t A0, A1, A2, A3; };
            uint32_t A[4];
        };
        union {
            struct { uint32_t T0, T1, T2, T3, T4, T5, T6, T7; };
            uint32_t T[8];
        };
        union {
            struct { uint32_t S0, S1, S2, S3, S4, S5, S6, S7; };
            uint32_t S[8];
        };
        uint32_t T8, T9;
        union {
            struct { uint32_t K0, K1; };
            uint32_t K[2];
        };
        uint32_t GP, SP, FP, RA, HI, LO, PC;
    };
    uint32_t R[35];
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
        uint32_t INDX, RAND, TLBL, BPC, CTXT, BDA, PIDMASK, DCIC,
        BADV, BDAM, TLBH, BPCM, SR, CAUSE, EPC, PRID, ERREG;
    };
    uint32_t R[17];
};


////////////////////////////////////////////////////////////////////////
// implement of R3000A processor
////////////////////////////////////////////////////////////////////////

class Processor
{
private:
    Processor() {}
    ~Processor() {}

public:
    void Init();
    void Reset();
    void Execute();
    void ExecuteBlock();
    // void Clear(uint32_t addr, uint32_t size);
    void Shutdown();

    static Processor& GetInstance();

    void BranchTest();
    void Exception(uint32_t code, bool branch_delay);

    bool IsInDelaySlot() const;
    void EnterDelaySlot();
    void LeaveDelaySlot();

    bool isDoingBranch() const;

public:
    static GeneralPurposeRegisters GPR;
    static Cop0Registers CP0;
    static uint32_t& HI;
    static uint32_t& LO;
    static uint32_t& PC;
    static uint32_t Cycle;  // clock count
    static uint32_t Interrupt;

private:
    static bool inDelaySlot;    // for SYSCALL
    static bool doingBranch;    // set when doBranch is called

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
extern R3000A::Processor& R3000a;


////////////////////////////////
// Instruction Macros
////////////////////////////////

inline uint32_t opcode_(uint32_t code) {
    return code >> 26;
}

inline uint32_t rs_(uint32_t code) {
    return (code >> 21) & 0x01f;
}

inline uint32_t rt_(uint32_t code) {
    return (code >> 16) & 0x001f;
}

inline uint32_t rd_(uint32_t code) {
    return (code >> 11) & 0x0001f;
}


inline uint32_t& regSrc_(uint32_t code) {
    return R3000a.GPR.R[rs_(code)];
}

inline uint32_t& regTrg_(uint32_t code) {
    return R3000a.GPR.R[rt_(code)];
}

inline uint32_t& regDst_(uint32_t code) {
    return R3000a.GPR.R[rd_(code)];
}


inline int16_t immediate_(uint32_t code) {
    return code & 0xffff;
}

inline uint16_t immediateU_(uint32_t code) {
    return code & 0xffff;
}

inline uint32_t target_(uint32_t code) {
    return code & 0x03ffffff;
}

inline uint32_t shamt_(uint32_t code) {
    return (code >> 6) & 0x1f;
}

inline uint32_t funct_(uint32_t code) {
    return code & 0x3f;
}

inline uint32_t addr_(uint32_t code) {
    return regSrc_(code) + immediate_(code);
}


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

}   // namespace PSX
