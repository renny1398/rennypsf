#pragma once
#include <stdint.h>
#include <wx/thread.h>
#include <wx/msgout.h>


namespace R3000A {


typedef bool (*INIT_FUNC)();
typedef void (*RESET_FUNC)();
typedef void (*SHUTDOWN_FUNC)();
typedef void (*EXCEPTION_FUNC)(uint32_t, uint32_t);
typedef void (*BRANCH_TEST_FUNC)();
typedef void (*EXECUTE_BIOS_FUNC)();


//INIT_FUNC Init;
//RESET_FUNC Reset;
//SHUTDOWN_FUNC Shutdown;
//EXCEPTION_FUNC Exception;
//BRANCH_TEST_FUNC BranchTest;
//EXECUTE_BIOS_FUNC ExecuteBIOS;

void Reset();


void Exception(uint32_t code, bool branch_delay);



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



extern GeneralPurposeRegisters GPR;
extern Cop0Registers CP0;
extern uint32_t& PC;

extern uint_fast32_t cycle;
extern uint_fast32_t interrupt;


////////////////////////////////
// Instruction Macros
////////////////////////////////

inline uint32_t _opcode(uint32_t code) {
    return code >> 26;
}

inline uint32_t _rs(uint32_t code) {
    return (code >> 21) & 0x01f;
}

inline uint32_t _rt(uint32_t code) {
    return (code >> 16) & 0x001f;
}

inline uint32_t _rd(uint32_t code) {
    return (code >> 11) & 0x0001f;
}


inline uint32_t& _regSrc(uint32_t code) {
    return GPR.R[_rs(code)];
}

inline uint32_t& _regTrg(uint32_t code) {
    return GPR.R[_rt(code)];
}

inline uint32_t& _regDst(uint32_t code) {
    return GPR.R[_rd(code)];
}


inline int16_t _immediate(uint32_t code) {
    return code & 0xffff;
}

inline uint16_t _immediateU(uint32_t code) {
    return code & 0xffff;
}

inline uint32_t _target(uint32_t code) {
    return code & 0x03ffffff;
}

inline uint32_t _shamt(uint32_t code) {
    return (code >> 6) & 0x1f;
}

inline uint32_t _funct(uint32_t code) {
    return code & 0x3f;
}

inline uint32_t _addr(uint32_t code) {
    return _regSrc(code) + _immediate(code);
}


enum OPCODE_ENUM {
    OPCODE_SPECIAL, OPCODE_BCOND, OPCODE_J, OPCODE_JAL, OPCODE_BEQ, OPCODE_BNE, OPCODE_BLEZ, OPCODE_BGTZ,
    OPCODE_ADDI, OPCODE_ADDIU, OPCODE_SLTI, OPCODE_SLTIU, OPCODE_ANDI, OPCODE_ORI, OPCODE_XORI, OPCODE_LUI,
    OPCODE_COP0, OPCODE_COP1, OPCODE_COP2, OPCODE_COP3,
    OPCODE_LB = 0x20, OPCODE_LH, OPCODE_LWL, OPCODE_LW, OPCODE_LBU, OPCODE_LHU, OPCODE_LWR,
    OPCODE_SB = 0x28, OPCODE_SH, OPCODE_SWL, OPCODE_SW, OPCODE_SWR = 0x2e,
    OPCODE_LWC0 = 0x30, OPCODE_LWC1, OPCODE_LWC2, OPCODE_LWC3,
    OPCODE_SWC0 = 0x38, OPCODE_SWC1, OPCODE_SWC2, OPCODE_SWC3
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



namespace Interpreter {

void Init();
void Reset();
//void Shutdown();
void Exception(uint32_t, uint32_t);
// void BranchTest();
void ExecuteBIOS();

void ExecuteOnce();
void ExecuteBlock();



class Thread: public wxThread
{
public:
    Thread(): wxThread(wxTHREAD_DETACHED) {}

protected:
    ExitCode Entry()
    {
        do {
            ExecuteOnce();
        } while (TestDestroy() == false);
        return 0;
    }

    void OnExit()
    {
        wxMessageOutputDebug().Printf("PSX Thread is ended.");
    }
};




Thread* Execute();
void Shutdown();


extern void (*OPCODES[64])(uint32_t);
extern void (*SPECIALS[64])(uint32_t);
extern void (*BCONDS[24])(uint32_t);
extern void (*COPz[16])(uint32_t);

inline void ExecuteOpcode(uint32_t code) {
    OPCODES[_opcode(code)](code);
}

}   // namespace R3000A::Interpreter

}   // namespace R3000A
