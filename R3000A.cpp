#include "R3000A.h"
#include "PSXMemory.h"
#include "PSXBIOS.h"

#include <wx/debug.h>
#include <cstring>


#ifndef FASTCALL
#ifdef _WINDOWS
#define FASTCALL __fastcall
#else
#define FASTCALL __attribute__((fastcall))
#endif
#endif


namespace R3000A {
GeneralPurposeRegisters GPR;
Cop0Registers CP0;
uint32_t& PC = GPR.PC;

//uint32_t last_code;
uint_fast32_t cycle;
uint_fast32_t interrupt;

// for delay_load
//uint32_t delayed_load_target;
//uint32_t delayed_load_value;

const char *strGPR[35] = {
    "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
    "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
    "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
    "T8", "T9", "K0", "K1", "GP", "SP", "FP", "RA",
    "HI", "LO", "PC"
};

}   // namespace R3000A


void R3000A::Reset()
{
    memset(&GPR, 0, sizeof(GPR));
    memset(&CP0, 0, sizeof(CP0));
    PC = 0xbfc00000;
    /*last_code = */cycle = interrupt = 0;
    CP0.R[12] = 0x10900000; // COP0_ENABLED | BEV | TS
    CP0.R[15] = 0x00000002; // PRevId = Revision Id, same as R3000A
//    delayed_load_target = 0;
//    delayed_load_value = 0;
}



void R3000A::Interpreter::Init()
{
    ////////////////////////////////
    // create Opcode LUT
    ////////////////////////////////

}


inline void R3000A::Interpreter::ExecuteOnce()
{
    uint32_t pc = PC;
    uint32_t code = PSXMemory::LoadUserMemory32(pc);
    //wxMessageOutputDebug().Printf("PC = 0x%08x, OPCODE = 0x%02x", pc, code >> 26);
    pc += 4;
    ++cycle;
    PC = pc;
    //last_code = code;
    ExecuteOpcode(code);
}



void R3000A::Exception(uint32_t code, bool branch_delay)
{
    CP0.CAUSE = code;

    uint32_t pc = GPR.PC;
    if (branch_delay) {
        CP0.CAUSE |= 0x80000000;    // set Branch Delay
        pc -= 4;
    }
    CP0.EPC = pc;

    uint32_t status = CP0.SR;
    if (status & 0x400000) { // BEV
        PC = 0xbfc00180;
    } else {
        PC = 0x80000080;
    }

    // (KUo,IEo, KUp, IEp) <- (KUp, IEp, KUc, IEc)
    CP0.SR = (status & ~0x3f) | ((status & 0xf) << 2);
    PSXBIOS::Exception();
}





////////////////////////////////////////////////////////////////
// OPCODES
////////////////////////////////////////////////////////////////

namespace R3000A {


bool inDelaySlot = false;   // for SYSCALL
bool flagBranch = false;    // set when doBranch is called


void BranchTest()
{
//    if (cycle - nexts_counter >= next_counter) {
//        RootCounter::Update();
//    }

    // interrupt mask register ($1f801074)
    if (PSXMemory::memHardware[0x1070] & PSXMemory::memHardware[0x1074]) {
        if ((CP0.SR & 0x401) == 0x401) {
            Exception(0x400, false);
        }
    }
}



namespace Interpreter {


void DelayRead(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    // 'code' must contain a load opcode.

    uint32_t reg_old, reg_new;

    reg_old = GPR.R[reg];
    ExecuteOpcode(code);  // branch delay load
    reg_new = GPR.R[reg];

    PC = branch_pc;
    BranchTest();

    GPR.R[reg] = reg_old;
    ExecuteOnce();
    GPR.R[reg] = reg_new;

    inDelaySlot = false;
}

void DelayWrite(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    // OPCODES[code >> 26]();
    inDelaySlot = false;
    PC = branch_pc;
    BranchTest();
}

void DelayReadWrite(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    inDelaySlot = false;
    PC = branch_pc;
    BranchTest();
}


////////////////////////////////////////////////////////////////
// Delay Opcodes
////////////////////////////////////////////////////////////////

typedef bool (*DELAY_OPCODE)(uint32_t, uint32_t, uint32_t);

////////////////////////////////////////
// Reusable Functions
////////////////////////////////////////

static bool DelayNOP(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    return false;
}

static bool DelayRs(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regSrc(code) == reg) {
        DelayRead(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayWt(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regTrg(code) == reg) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regDst(code) == reg) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayRsRt(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regSrc(code) == reg || _regTrg(code) == reg) {
        DelayRead(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayRsWt(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regSrc(code) == reg) {
        if (_regTrg(code) == reg) {
            DelayReadWrite(code, reg, branch_pc);
            return true;
        }
        DelayRead(code, reg, branch_pc);
        return true;
    }
    if (_regTrg(code) == reg) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayRsWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regSrc(code) == reg) {
        if (_regDst(code) == reg) {
            DelayReadWrite(code, reg, branch_pc);
            return true;
        }
        DelayRead(code, reg, branch_pc);
        return true;
    }
    if (_regDst(code) == reg) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayRtWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regTrg(code) == reg) {
        if (_regDst(code) == reg) {
            DelayReadWrite(code, reg, branch_pc);
            return true;
        }
        DelayRead(code, reg, branch_pc);
        return true;
    }
    if (_regDst(code) == reg) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

static bool DelayRsRtWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regSrc(code) == reg || _regTrg(code) == reg) {
        if (_regDst(code) == reg) {
            DelayReadWrite(code, reg, branch_pc);
            return true;
        }
        DelayRead(code, reg, branch_pc);
        return true;
    }
    if (_regDst(code) == reg) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

////////////////////////////////////////
// SPECIAL functions
////////////////////////////////////////

bool DelaySLL(uint32_t code, uint32_t reg, uint32_t branch_pc) {
    if (code) {
        return DelayRtWd(code, reg, branch_pc);
    }
    return false;
}
DELAY_OPCODE const DelaySRL = DelayRtWd;
DELAY_OPCODE const DelaySRA = DelayRtWd;

DELAY_OPCODE const DelayJR = DelayRs;

DELAY_OPCODE const DelayJALR = DelayRsWd;

DELAY_OPCODE const DelayADD = DelayRsRtWd;
DELAY_OPCODE const DelayADDU = DelayRsRtWd;
DELAY_OPCODE const DelaySLLV = DelayRsRtWd;

DELAY_OPCODE const DelayMFHI = DelayWd;
DELAY_OPCODE const DelayMFLO = DelayWd;
DELAY_OPCODE const DelayMTHI = DelayRs;
DELAY_OPCODE const DelayMTLO = DelayRs;

DELAY_OPCODE const DelayMULT = DelayRsRt;
DELAY_OPCODE const DelayDIV = DelayRsRt;


DELAY_OPCODE const DELAY_SPECIALS[64] = {
    DelaySLL, DelayNOP, DelaySRL, DelaySRA, DelaySLLV, DelayNOP, DelaySLLV, DelaySLLV,
    DelayJR, DelayJALR, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayMFHI, DelayMTHI, DelayMFLO, DelayMTLO, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayMULT, DelayMULT, DelayDIV, DelayDIV, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayADD, DelayADDU, DelayADD, DelayADDU, DelayADD, DelayADD, DelayADD, DelayADD,
    DelayNOP, DelayNOP, DelayADD, DelayADD, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP
};

static bool DelaySPECIAL(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    return DELAY_SPECIALS[_funct(code)](code, reg, branch_pc);
}

DELAY_OPCODE const DelayBCOND = DelayRs;

bool DelayJAL(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (reg == GPR_RA) {
        DelayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

DELAY_OPCODE const DelayBEQ = DelayRsRt;
DELAY_OPCODE const DelayBNE = DelayRsRt;

DELAY_OPCODE const DelayBLEZ = DelayRs;
DELAY_OPCODE const DelayBGTZ = DelayRs;

DELAY_OPCODE const DelayADDI = DelayRsWt;
DELAY_OPCODE const DelayADDIU = DelayRsWt;

DELAY_OPCODE const DelayLUI = DelayWt;

bool DelayCOP0(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    switch (_funct(code) & ~0x03) {
    case 0x00:  // MFC0, CFC0
        if (_regTrg(code) == reg) {
            DelayWrite(code, reg, branch_pc);
            return true;
        }
        return false;
    case 0x04:  // MTC0, CTC0
        if (_regTrg(code) == reg) {
            DelayRead(code, reg, branch_pc);
            return true;
        }
        return false;
    }
    return false;
}

bool DelayLWL(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (_regTrg(code) == reg) {
        DelayReadWrite(code, reg, branch_pc);
        return true;
    }
    if (_regSrc(code) == reg) {
        DelayRead(code, reg, branch_pc);
        return true;
    }
    return false;
}
DELAY_OPCODE const DelayLWR = DelayLWL;

DELAY_OPCODE const DelayLB = DelayRsWt;
DELAY_OPCODE const DelayLH = DelayRsWt;
DELAY_OPCODE const DelayLW = DelayRsWt;
DELAY_OPCODE const DelayLBU = DelayRsWt;
DELAY_OPCODE const DelayLHU = DelayRsWt;

DELAY_OPCODE const DelaySB = DelayRsRt;
DELAY_OPCODE const DelaySH = DelayRsRt;
DELAY_OPCODE const DelaySWL = DelayRsRt;
DELAY_OPCODE const DelaySW = DelayRsRt;
DELAY_OPCODE const DelaySWR = DelayRsRt;

DELAY_OPCODE const DelayLWC2 = DelayRs;
DELAY_OPCODE const DelaySWC2 = DelayRs;


DELAY_OPCODE const DELAY_OPCODES[64] = {
    DelaySPECIAL, DelayBCOND, DelayNOP, DelayJAL, DelayBEQ, DelayBNE, DelayBLEZ, DelayBGTZ,
    DelayADDI, DelayADDIU, DelayADDI, DelayADDI, DelayADDI, DelayADDI, DelayADDI, DelayLUI,
    DelayCOP0, DelayNOP, DelayCOP0, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayNOP, DelayNOP, DelayNOP, DelayNOP,DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayLB, DelayLH, DelayLWL, DelayLW, DelayLBU, DelayLHU, DelayLWR, DelayNOP,
    DelaySB, DelaySH, DelaySWL, DelaySW, DelayNOP, DelayNOP, DelaySWR, DelayNOP,
    DelayNOP, DelayNOP, DelayLWC2, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP,
    DelayNOP, DelayNOP, DelaySWC2, DelayNOP, DelayNOP, DelayNOP, DelayNOP, DelayNOP
};


void DelayTest(uint32_t reg, uint32_t branch_pc)
{
    uint32_t branch_code = PSXMemory::LoadUserMemory32(branch_pc);
    inDelaySlot = true;
    uint32_t opcode = branch_code >> 26;
    if (DELAY_OPCODES[opcode](branch_code, reg, branch_pc)) {
        return;
    }

    OPCODES[opcode](branch_code);
    inDelaySlot = false;
    PC = branch_pc;
    BranchTest();
}


static inline void DoBranch(uint32_t branch_pc)
{
    flagBranch = true;

    uint32_t pc = PC;
    uint32_t code = PSXMemory::LoadUserMemory32(pc);
    pc += 4;
    PC = pc;
    ++cycle;

    uint32_t opcode = code >> 26;
    switch (opcode) {
    case 0x10:  // COP0
        if ((_regSrc(code) & 0x03) == _regSrc(code)) {  // MFC0, CFC0
            DelayTest(_regTrg(code), branch_pc);
            return;
        }
        break;
    case 0x32:  // LWC2
        DelayTest(_regTrg(code), branch_pc);
        return;
    default:
        if ((opcode & 0x27) == opcode) { // Load opcode
            DelayTest(_regTrg(code), branch_pc);
            return;
        }
        break;
    }

    OPCODES[opcode](code);

    // branch itself & nop
    if (PC - 8 == branch_pc && !opcode) {
        // CounterDeadLoopSkip();
    }
    inDelaySlot = false;
    PC = branch_pc;
    BranchTest();
}


static void OPNULL(uint32_t code) {
    wxMessageOutputDebug().Printf("WARNING: Unknown Opcode (0x%02x) is executed!", code >> 26);
}


/*
static inline void CommitDelayedLoad() {
    if (delayed_load_target) {
        GPR.R[delayed_load_target] = delayed_load_value;
        delayed_load_target = 0;
        delayed_load_value = 0;
    }
}

static inline void DelayedBranch(uint32_t target) {
    CommitDelayedLoad();
    delayed_load_target = GPR_PC;
    delayed_load_value = target;
    wxMessageOutputDebug().Printf("Branch to 0x%08x", target);
}
*/

static inline void Load(uint32_t rt, uint32_t value) {
    // CommitDelayedLoad();
    if (rt != 0) {
        GPR.R[rt] = value;
    }
}

/*
// for load function from memory or not GPR into GPR
static inline void DelayedLoad(uint32_t rt, uint32_t value) {
    wxASSERT_MSG(rt != GPR_PC, "Delayed-load target must be other than PC.");
    if (delayed_load_target == GPR_PC) {
        // delay load
        PC = delayed_load_value;
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
// Load and Store Instructions
////////////////////////////////////////

// Load Byte
static void LB(uint32_t code) {
    Load(_rt(code), static_cast<int32_t>(PSXMemory::Read<int8_t>(_addr(code))));
}

// Load Byte Unsigned
static void LBU(uint32_t code) {
    Load(_rt(code), static_cast<uint32_t>(PSXMemory::Read<uint8_t>(_addr(code))));
}

// Load Halfword
static void LH(uint32_t code) {
    Load(_rt(code), static_cast<int32_t>(PSXMemory::Read<int16_t>(_addr(code))));
}

// Load Halfword Unsigned
static void LHU(uint32_t code) {
    Load(_rt(code), static_cast<uint32_t>(PSXMemory::Read<uint16_t>(_addr(code))));
}

// Load Word
static void LW(uint32_t code) {
    Load(_rt(code), PSXMemory::Read<uint32_t>(_addr(code)));
}

// Load Word Left
static void LWL(uint32_t code) {
    static const uint32_t LWL_MASK[4] = { 0x00ffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
    static const uint32_t LWL_SHIFT[4] = { 24, 16, 8, 0 };
    uint32_t rt = _rt(code);
    uint32_t addr = _addr(code);
    uint32_t shift = addr & 3;
    uint32_t mem = PSXMemory::Read<uint32_t>(addr & ~3);
    Load(rt, (GPR.R[rt] & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]));
}

// Load Word Right
static void LWR(uint32_t code) {
    static const uint32_t LWR_MASK[4] = { 0xff000000, 0xffff0000, 0xffffff00, 0x00000000 };
    static const uint32_t LWR_SHIFT[4] = { 0, 8, 16, 24 };
    uint32_t rt = _rt(code);
    uint32_t addr = _addr(code);
    uint32_t shift = addr & 3;
    uint32_t mem = PSXMemory::Read<uint32_t>(addr & ~3);
    Load(rt, (GPR.R[rt] & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]));
}

// Store Byte
static void SB(uint32_t code) {
    PSXMemory::Write(_addr(code), static_cast<uint8_t>(_regTrg(code)));
}

// Store Halfword
static void SH(uint32_t code) {
    PSXMemory::Write(_addr(code), static_cast<uint16_t>(_regTrg(code)));
}

// Store Word
static void SW(uint32_t code) {
    PSXMemory::Write(_addr(code), _regTrg(code));
}

// Store Word Left
static void SWL(uint32_t code) {
    static const uint32_t SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
    static const uint32_t SWL_SHIFT[4] = { 24, 16, 8, 0 };
    uint32_t addr = _addr(code);
    uint32_t shift = addr & 3;
    uint32_t mem = PSXMemory::Read<uint32_t>(addr & ~3);
    PSXMemory::Write(addr & ~3, (_regTrg(code) >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]));
}

// Store Word Right
static void SWR(uint32_t code) {
    static const uint32_t SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
    static const uint32_t SWR_SHIFT[4] = { 0, 8, 16, 24 };
    uint32_t addr = _addr(code);
    uint32_t shift = addr & 3;
    uint32_t mem = PSXMemory::Read<uint32_t>(addr & ~3);
    PSXMemory::Write(addr & ~3, (_regTrg(code) << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]));
}


////////////////////////////////////////
// Computational Instructions
////////////////////////////////////////

// ADD Immediate
static void ADDI(uint32_t code) {
    Load(_rt(code), _regSrc(code) + _immediate(code));
    // ? Trap on two's complement overflow.
}

// ADD Immediate Unsigned
static void ADDIU(uint32_t code) {
    Load(_rt(code), _regSrc(code) + _immediate(code));
}

// Set on Less Than Immediate
static void SLTI(uint32_t code) {
    Load(_rt(code), (static_cast<int32_t>(_regSrc(code)) < _immediate(code)) ? 1 : 0);
}

// Set on Less Than Unsigned Immediate
static void SLTIU(uint32_t code) {
    Load(_rt(code), (_regSrc(code) < static_cast<uint32_t>(_immediate(code))) ? 1 : 0);
}

// AND Immediate
static void ANDI(uint32_t code) {
    Load(_rt(code), _regSrc(code) & _immediateU(code));
}

// OR Immediate
static void ORI(uint32_t code) {
    Load(_rt(code), _regSrc(code) | _immediateU(code));
}

// eXclusive OR Immediate
static void XORI(uint32_t code) {
    Load(_rt(code), _regSrc(code) ^ _immediateU(code));
}

// Load Upper Immediate
static void LUI(uint32_t code) {
    Load(_rt(code), code << 16);
}


// ADD
static void ADD(uint32_t code) {
    Load(_rd(code), _regSrc(code) + _regTrg(code));
    // ? Trap on two's complement overflow.
}

// ADD Unsigned
static void ADDU(uint32_t code) {
    Load(_rd(code), _regSrc(code) + _regTrg(code));
}

// SUBtract
static void SUB(uint32_t code) {
    Load(_rd(code), _regSrc(code) - _regSrc(code));
    // ? Trap on two's complement overflow.
}

// SUBtract Unsigned
static void SUBU(uint32_t code) {
    Load(_rd(code), _regSrc(code) - _regSrc(code));
}

// Set on Less Than
static void SLT(uint32_t code) {
    Load(_rd(code), (static_cast<int32_t>(_regSrc(code)) < static_cast<int32_t>(_regTrg(code))) ? 1 : 0);
}

// Set on Less Than Unsigned
static void SLTU(uint32_t code) {
    Load(_rd(code), (_regSrc(code) < _regTrg(code)) ? 1 : 0);
}

// AND
static void AND(uint32_t code) {
    Load(_rd(code), _regSrc(code) & _regTrg(code));
}

// OR
static void OR(uint32_t code) {
    Load(_rd(code), _regSrc(code) | _regTrg(code));
}

// eXclusive OR
static void XOR(uint32_t code) {
    Load(_rd(code), _regSrc(code) ^ _regTrg(code));
}

// NOR
static void NOR(uint32_t code) {
    Load(_rd(code), ~(_regSrc(code) | _regTrg(code)));
}


// Shift Left Logical
static void SLL(uint32_t code) {
    Load(_rd(code), _regTrg(code) << _shamt(code));
}

// Shift Right Logical
static void SRL(uint32_t code) {
    Load(_rd(code), _regTrg(code) >> _shamt(code));
}

// Shift Right Arithmetic
static void SRA(uint32_t code) {
    Load(_rd(code), static_cast<int32_t>(_regTrg(code)) >> _shamt(code));
}

// Shift Left Logical Variable
static void SLLV(uint32_t code) {
    Load(_rd(code), _regTrg(code) << _regSrc(code));
}

// Shift Right Logical Variable
static void SRLV(uint32_t code) {
    Load(_rd(code), _regTrg(code) >> _regSrc(code));
}

// Shift Right Arithmetic Variable
static void SRAV(uint32_t code) {
    Load(_rd(code), static_cast<int32_t>(_regTrg(code)) >> _regSrc(code));
}


// MULTiply
static void MULT(uint32_t code) {
    int32_t rs = static_cast<int32_t>(_regSrc(code));
    int32_t rt = static_cast<int32_t>(_regTrg(code));
    // CommitDelayedLoad();
    int64_t res = (int64_t)rs * (int64_t)rt;
    GPR.LO = static_cast<uint32_t>(res & 0xffffffff);
    GPR.HI = static_cast<uint32_t>(res >> 32);
}

// MULtiply Unsigned
static void MULTU(uint32_t code) {
    uint32_t rs = _regSrc(code);
    uint32_t rt = _regTrg(code);
    // CommitDelayedLoad();
    uint64_t res = (uint64_t)rs * (uint64_t)rt;
    GPR.LO = static_cast<uint32_t>(res & 0xffffffff);
    GPR.HI = static_cast<uint32_t>(res >> 32);
}

// Divide
static void DIV(uint32_t code) {
    int32_t rs = static_cast<int32_t>(_regSrc(code));
    int32_t rt = static_cast<int32_t>(_regTrg(code));
    // CommitDelayedLoad();
    GPR.LO = static_cast<uint32_t>(rs / rt);
    GPR.HI = static_cast<uint32_t>(rs % rt);
}

// Divide Unsigned
static void DIVU(uint32_t code) {
    uint32_t rs = _regSrc(code);
    uint32_t rt = _regTrg(code);
    // CommitDelayedLoad();
    GPR.LO = rs / rt;
    GPR.HI = rs % rt;
}

// Move From HI
static void MFHI(uint32_t code) {
    Load(_rd(code), GPR.HI);
}

// Move From LO
static void MFLO(uint32_t code) {
    Load(_rd(code), GPR.LO);
}

// Move To HI
static void MTHI(uint32_t code) {
    uint32_t rd = _regDst(code);
    // CommitDelayedLoad();
    GPR.HI = rd;
}

// Move To LO
static void MTLO(uint32_t code) {
    uint32_t rd = _regDst(code);
    // CommitDelayedLoad();
    GPR.LO = rd;
}


////////////////////////////////////////
// Jump and Branch Instructions
////////////////////////////////////////

// Jump
static void J(uint32_t code) {
    DoBranch((_target(code) << 2) | (PC & 0xf0000000));
}

// Jump And Link
static void JAL(uint32_t code) {
    GPR.RA = PC + 4;
    DoBranch((_target(code) << 2) | (PC & 0xf0000000));
}

// Jump Register
static void JR(uint32_t code) {
    DoBranch(_regSrc(code));
}

// Jump And Link Register
static void JALR(uint32_t code) {
    uint32_t rd = _rd(code);
    if (rd != 0) {
        GPR.R[rd] = PC + 4;
    }
    DoBranch(_regSrc(code));
}


// Branch on EQual
static void BEQ(uint32_t code) {
    if (_regSrc(code) == _regTrg(code)) {
        DoBranch(PC + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Not Equal
static void BNE(uint32_t code) {
    if (_regSrc(code) != _regTrg(code)) {
        DoBranch(PC + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Less than or Equal Zero
static void BLEZ(uint32_t code) {
    if (static_cast<int32_t>(_regSrc(code)) <= 0) {
        DoBranch(PC + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Greate Than Zero
static void BGTZ(uint32_t code) {
    if (static_cast<int32_t>(_regSrc(code)) > 0) {
        DoBranch(PC + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Less Than Zero
static void BLTZ(uint32_t code) {
    if (static_cast<int32_t>(_regSrc(code)) < 0) {
        DoBranch(PC + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Greater than or Equal Zero
static void BGEZ(uint32_t code) {
    if (static_cast<int32_t>(_regSrc(code)) >= 0) {
        DoBranch(PC + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Less Than Zero And Link
static void BLTZAL(uint32_t code) {
    if (static_cast<int32_t>(_regSrc(code)) < 0) {
        uint32_t pc = PC;
        GPR.RA = pc + 4;
        DoBranch(pc + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch on Greater than or Equal Zero And Link
static void BGEZAL(uint32_t code) {
    if (static_cast<int32_t>(_regSrc(code)) >= 0) {
        uint32_t pc = PC;
        GPR.RA = pc + 4;
        DoBranch(pc + (static_cast<int32_t>(_immediate(code)) << 2));
    }
}

// Branch Condition
static void BCOND(uint32_t code) {
    BCONDS[_rt(code)](code);
}



////////////////////////////////////////
// Special Instructions
////////////////////////////////////////

static void SYSCALL(uint32_t code) {
    PC -= 4;
    Exception(0x20, inDelaySlot);
}
static void BREAK(uint32_t code) {}

static void SPECIAL(uint32_t code) {
    SPECIALS[_funct(code)](code);
}


////////////////////////////////////////
// Co-processor Instructions
////////////////////////////////////////

static void LWC0(uint32_t code) {

}
static void LWC1(uint32_t code) {
    wxMessageOutputDebug().Printf("WARNING: LWC1 is not supported.");
}
static void LWC2(uint32_t code) {}
static void LWC3(uint32_t code) {
    wxMessageOutputDebug().Printf("WARNING: LWC3 is not supported.");
}

static void SWC0(uint32_t code) {}
static void SWC1(uint32_t code) {
    wxMessageOutputDebug().Printf("WARNING: SWC1 is not supported.");
}

static void SWC2(uint32_t code) {}
static void SWC3(uint32_t code) {
    // HLE?
    wxMessageOutputDebug().Printf("WARNING: SWC3 is not supported.");
}



static void COP0(uint32_t code) {}
static void COP1(uint32_t code) {
    wxMessageOutputDebug().Printf("WARNING: COP1 is not supported.");
}
static void COP2(uint32_t code) {
    // Cop2 is not supported.
}
static void COP3(uint32_t code) {
    wxMessageOutputDebug().Printf("WARNING: COP3 is not supported.");
}


static void HLECALL(uint32_t code) {

}


////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////
// Main Loop
////////////////////////////////////////////////////////////////

Thread *thread = 0;

Thread *Execute()
{
    if (thread != 0) {
        Shutdown();
    }
    thread = new Thread();
    thread->Create();
    thread->Run();
    wxMessageOutputDebug().Printf("PSX Threads is started.");
    return thread;
}

void Shutdown()
{
    if (thread == 0) return;
    if (thread->IsRunning() == false) return;
    thread->Delete();
    thread = 0;
}



}   // namespace R3000A::Interpreter
}   // namespace R3000A


void (*R3000A::Interpreter::OPCODES[64])(uint32_t) = {
        SPECIAL, BCOND, J, JAL, BEQ, BNE, BLEZ, BGTZ,
        ADDI, ADDIU, SLTI, SLTIU, ANDI, ORI, XORI, LUI,
        COP0, COP1, COP2, COP3, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        LB, LH, LWL, LW, LBU, LHU, LWR, OPNULL,
        SB, SH, SWL, SW, OPNULL, OPNULL, SWR, OPNULL,
        LWC0, LWC1, LWC2, LWC3, OPNULL, OPNULL, OPNULL, OPNULL,
        SWC0, SWC1, SWC2, SWC3, OPNULL, OPNULL, OPNULL, OPNULL
};

void (*R3000A::Interpreter::SPECIALS[64])(uint32_t) = {
        SLL, OPNULL, SRL, SRA, SLLV, OPNULL, SRLV, SRAV,
        JR, JALR, OPNULL, HLECALL, SYSCALL, BREAK, OPNULL, OPNULL,
        MFHI, MTHI, MFLO, MTLO, OPNULL, OPNULL, OPNULL, OPNULL,
        MULT, MULTU, DIV, DIVU, OPNULL, OPNULL, OPNULL, OPNULL,
        ADD, ADDU, SUB, SUBU, AND, OR, XOR, NOR,
        OPNULL, OPNULL, SLT, SLTU, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL
};

void (*R3000A::Interpreter::BCONDS[24])(uint32_t) = {
        BLTZ, BGEZ, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        BLTZAL, BGEZAL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL
};



void R3000A::Interpreter::ExecuteBlock() {
    flagBranch = false;
    do {
        ExecuteOnce();
    } while (!flagBranch);
}
