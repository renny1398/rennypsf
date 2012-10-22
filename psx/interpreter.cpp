#include "interpreter.h"
#include "memory.h"
#include "rcnt.h"
#include "bios.h"

//using namespace PSX;
//using namespace PSX::R3000A;
//using namespace PSX::Interpreter;

namespace {

uint32_t& PC = PSX::R3000a.PC;

}   // namespace


namespace PSX {
namespace R3000A {

////////////////////////////////////////////////////////////////
// Delay Functions
////////////////////////////////////////////////////////////////

void Interpreter::delayRead(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    wxASSERT(reg < 32);

    uint32_t reg_old, reg_new;

    reg_old = R3000a.GPR.R[reg];
    Interpreter_.ExecuteOpcode(code);  // branch delay load
    reg_new = R3000a.GPR.R[reg];

    PC = branch_pc;
    R3000a.BranchTest();

    R3000a.GPR.R[reg] = reg_old;
    Interpreter_.ExecuteOnce();
    R3000a.GPR.R[reg] = reg_new;

    R3000a.LeaveDelaySlot();
}

void Interpreter::delayWrite(uint32_t, uint32_t reg, uint32_t branch_pc)
{
    wxASSERT(reg < 32);
    // OPCODES[code >> 26]();
    R3000a.LeaveDelaySlot();
    PC = branch_pc;
    R3000a.BranchTest();
}

void Interpreter::delayReadWrite(uint32_t, uint32_t reg, uint32_t branch_pc)
{
    wxASSERT(reg < 32);
    R3000a.LeaveDelaySlot();
    PC = branch_pc;
    R3000a.BranchTest();
}


////////////////////////////////////////////////////////////////
// Delay Opcodes
////////////////////////////////////////////////////////////////

////////////////////////////////////////
// Reusable Functions
////////////////////////////////////////

bool Interpreter::delayNOP(uint32_t, uint32_t, uint32_t)
{
    return false;
}

bool Interpreter::delayRs(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rs_(code) == reg) {
        delayRead(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayWt(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rt_(code) == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rd_(code) == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayRsRt(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rs_(code) == reg || rt_(code) == reg) {
        delayRead(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayRsWt(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rs_(code) == reg) {
        if (rt_(code) == reg) {
            delayReadWrite(code, reg, branch_pc);
            return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
    }
    if (rt_(code) == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayRsWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rs_(code) == reg) {
        if (rd_(code) == reg) {
            delayReadWrite(code, reg, branch_pc);
            return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
    }
    if (rd_(code) == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayRtWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rt_(code) == reg) {
        if (rd_(code) == reg) {
            delayReadWrite(code, reg, branch_pc);
            return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
    }
    if (rd_(code) == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

bool Interpreter::delayRsRtWd(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rs_(code) == reg || rt_(code) == reg) {
        if (rd_(code) == reg) {
            delayReadWrite(code, reg, branch_pc);
            return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
    }
    if (rd_(code) == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

////////////////////////////////////////
// SPECIAL functions
////////////////////////////////////////

bool Interpreter::delaySLL(uint32_t code, uint32_t reg, uint32_t branch_pc) {
    if (code) {
        return delayRtWd(code, reg, branch_pc);
    }
    return false;   // delay nop
}

Interpreter::DelayFunc Interpreter::delaySRL = delayRtWd;
Interpreter::DelayFunc Interpreter::delaySRA = delayRtWd;

Interpreter::DelayFunc Interpreter::delayJR = delayRs;

Interpreter::DelayFunc Interpreter::delayJALR = delayRsWd;

Interpreter::DelayFunc Interpreter::delayADD = delayRsRtWd;
Interpreter::DelayFunc Interpreter::delayADDU = delayRsRtWd;
Interpreter::DelayFunc Interpreter::delaySLLV = delayRsRtWd;

Interpreter::DelayFunc Interpreter::delayMFHI = delayWd;
Interpreter::DelayFunc Interpreter::delayMFLO = delayWd;
Interpreter::DelayFunc Interpreter::delayMTHI = delayRs;
Interpreter::DelayFunc Interpreter::delayMTLO = delayRs;

Interpreter::DelayFunc Interpreter::delayMULT = delayRsRt;
Interpreter::DelayFunc Interpreter::delayDIV = delayRsRt;


const Interpreter::DelayFunc Interpreter::delaySpecials[64] = {
    delaySLL, delayNOP, delaySRL, delaySRA, delaySLLV, delayNOP, delaySLLV, delaySLLV,
    delayJR, delayJALR, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP,
    delayMFHI, delayMTHI, delayMFLO, delayMTLO, delayNOP, delayNOP, delayNOP, delayNOP,
    delayMULT, delayMULT, delayDIV, delayDIV, delayNOP, delayNOP, delayNOP, delayNOP,
    delayADD, delayADDU, delayADD, delayADDU, delayADD, delayADD, delayADD, delayADD,
    delayNOP, delayNOP, delayADD, delayADD, delayNOP, delayNOP, delayNOP, delayNOP,
    delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP,
    delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP
};

bool Interpreter::delaySPECIAL(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    return delaySpecials[funct_(code)](code, reg, branch_pc);
}

Interpreter::DelayFunc Interpreter::delayBCOND = delayRs;

bool Interpreter::delayJAL(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (reg == R3000A::GPR_RA) {
        delayWrite(code, reg, branch_pc);
        return true;
    }
    return false;
}

Interpreter::DelayFunc Interpreter::delayBEQ = delayRsRt;
Interpreter::DelayFunc Interpreter::delayBNE = delayRsRt;

Interpreter::DelayFunc Interpreter::delayBLEZ = delayRs;
Interpreter::DelayFunc Interpreter::delayBGTZ = delayRs;

Interpreter::DelayFunc Interpreter::delayADDI = delayRsWt;
Interpreter::DelayFunc Interpreter::delayADDIU = delayRsWt;

Interpreter::DelayFunc Interpreter::delayLUI = delayWt;

bool Interpreter::delayCOP0(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    switch (funct_(code) & ~0x03) {
    case 0x00:  // MFC0, CFC0
        if (rt_(code) == reg) {
            delayWrite(code, reg, branch_pc);
            return true;
        }
        return false;
    case 0x04:  // MTC0, CTC0
        if (rt_(code) == reg) {
            delayRead(code, reg, branch_pc);
            return true;
        }
        return false;
    }
    return false;
}

bool Interpreter::delayLWL(uint32_t code, uint32_t reg, uint32_t branch_pc)
{
    if (rt_(code) == reg) {
        delayReadWrite(code, reg, branch_pc);
        return true;
    }
    if (rs_(code) == reg) {
        delayRead(code, reg, branch_pc);
        return true;
    }
    return false;
}
Interpreter::DelayFunc Interpreter::delayLWR = delayLWL;

Interpreter::DelayFunc Interpreter::delayLB = delayRsWt;
Interpreter::DelayFunc Interpreter::delayLH = delayRsWt;
Interpreter::DelayFunc Interpreter::delayLW = delayRsWt;
Interpreter::DelayFunc Interpreter::delayLBU = delayRsWt;
Interpreter::DelayFunc Interpreter::delayLHU = delayRsWt;

Interpreter::DelayFunc Interpreter::delaySB = delayRsRt;
Interpreter::DelayFunc Interpreter::delaySH = delayRsRt;
Interpreter::DelayFunc Interpreter::delaySWL = delayRsRt;
Interpreter::DelayFunc Interpreter::delaySW = delayRsRt;
Interpreter::DelayFunc Interpreter::delaySWR = delayRsRt;

Interpreter::DelayFunc Interpreter::delayLWC2 = delayRs;
Interpreter::DelayFunc Interpreter::delaySWC2 = delayRs;


const Interpreter::DelayFunc Interpreter::delayOpcodes[64] = {
    delaySPECIAL, delayBCOND, delayNOP, delayJAL, delayBEQ, delayBNE, delayBLEZ, delayBGTZ,
    delayADDI, delayADDIU, delayADDI, delayADDI, delayADDI, delayADDI, delayADDI, delayLUI,
    delayCOP0, delayNOP, delayCOP0, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP,
    delayNOP, delayNOP, delayNOP, delayNOP,delayNOP, delayNOP, delayNOP, delayNOP,
    delayLB, delayLH, delayLWL, delayLW, delayLBU, delayLHU, delayLWR, delayNOP,
    delaySB, delaySH, delaySWL, delaySW, delayNOP, delayNOP, delaySWR, delayNOP,
    delayNOP, delayNOP, delayLWC2, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP,
    delayNOP, delayNOP, delaySWC2, delayNOP, delayNOP, delayNOP, delayNOP, delayNOP
};


void Interpreter::delayTest(uint32_t reg, uint32_t branch_pc)
{
    uint32_t branch_code = u32M(branch_pc);
    R3000a.EnterDelaySlot();
    uint32_t op = opcode_(branch_code);
    if (delayOpcodes[op](branch_code, reg, branch_pc)) {
        return;
    }

    PC = branch_pc;
#ifdef __WXDEBUG__
    Interpreter_.ExecuteOpcode(branch_code);
#else
    OPCODES[op](branch_code);
#endif
    R3000a.LeaveDelaySlot();
    R3000a.BranchTest();
}


void Interpreter::doBranch(uint32_t branch_pc)
{
    R3000a.doingBranch = true;

    uint32_t pc = PC;
    uint32_t code = u32M(pc);
    pc += 4;
    PC = pc;
    R3000a.Cycle++;

    const uint32_t op = opcode_(code);
    switch (op) {
    case 0x10:  // COP0
        if ((rs_(code) & 0x03) == rs_(code)) {  // MFC0, CFC0
            delayTest(rt_(code), branch_pc);
            return;
        }
        break;
    case 0x32:  // LWC2
        delayTest(rt_(code), branch_pc);
        return;
    default:
        if (static_cast<uint32_t>(op - 0x20) < 8) { // Load opcode
            delayTest(rt_(code), branch_pc);
            return;
        }
        break;
    }

#ifdef __WXDEBUG__
    Interpreter_.ExecuteOpcode(code);
#else
    OPCODES[op](code);
#endif

    // branch itself & nop
    if (PC - 8 == branch_pc && !op) {
        RootCounter::DeadLoopSkip();
    }
    R3000a.LeaveDelaySlot();
    PC = branch_pc;
    R3000a.BranchTest();
}


void OPNULL(uint32_t code) {
    wxMessageOutputDebug().Printf(wxT("WARNING: Unknown Opcode (0x%02x) is executed!"), code >> 26);
}


/*
inline void CommitDelayedLoad() {
    if (delayed_load_target) {
        R3000a.GPR.R[delayed_load_target] = delayed_load_value;
        delayed_load_target = 0;
        delayed_load_value = 0;
    }
}

inline void DelayedBranch(uint32_t target) {
    CommitDelayedLoad();
    delayed_load_target = GPR_PC;
    delayed_load_value = target;
    wxMessageOutputDebug().Printf("Branch to 0x%08x", target);
}
*/

inline void Load(uint32_t rt, uint32_t value) {
    // CommitDelayedLoad();
    if (rt != 0) {
        R3000a.GPR.R[rt] = value;
    }
}

/*
// for load function from memory or not GPR into GPR
inline void DelayedLoad(uint32_t rt, uint32_t value) {
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
void Interpreter::LB(uint32_t code) {
    Load(rt_(code), static_cast<int32_t>(Memory::Read<int8_t>(addr_(code))));
}

// Load Byte Unsigned
void Interpreter::LBU(uint32_t code) {
    Load(rt_(code), static_cast<uint32_t>(Memory::Read<uint8_t>(addr_(code))));
}

// Load Halfword
void Interpreter::LH(uint32_t code) {
    Load(rt_(code), static_cast<int32_t>(Memory::Read<int16_t>(addr_(code))));
}

// Load Halfword Unsigned
void Interpreter::LHU(uint32_t code) {
    Load(rt_(code), static_cast<uint32_t>(Memory::Read<uint16_t>(addr_(code))));
}

// Load Word
void Interpreter::LW(uint32_t code) {
    Load(rt_(code), Memory::Read<uint32_t>(addr_(code)));
}

// Load Word Left
void Interpreter::LWL(uint32_t code) {
    static const uint32_t LWL_MASK[4] = { 0x00ffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
    static const uint32_t LWL_SHIFT[4] = { 24, 16, 8, 0 };
    uint32_t rt = rt_(code);
    uint32_t addr = addr_(code);
    uint32_t shift = addr & 3;
    uint32_t mem = Memory::Read<uint32_t>(addr & ~3);
    Load(rt, (R3000a.GPR.R[rt] & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]));
}

// Load Word Right
void Interpreter::LWR(uint32_t code) {
    static const uint32_t LWR_MASK[4] = { 0xff000000, 0xffff0000, 0xffffff00, 0x00000000 };
    static const uint32_t LWR_SHIFT[4] = { 0, 8, 16, 24 };
    uint32_t rt = rt_(code);
    uint32_t addr = addr_(code);
    uint32_t shift = addr & 3;
    uint32_t mem = Memory::Read<uint32_t>(addr & ~3);
    Load(rt, (R3000a.GPR.R[rt] & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]));
}

// Store Byte
void Interpreter::SB(uint32_t code) {
    Memory::Write(addr_(code), static_cast<uint8_t>(regTrg_(code)));
}

// Store Halfword
void Interpreter::SH(uint32_t code) {
    Memory::Write(addr_(code), static_cast<uint16_t>(regTrg_(code)));
}

// Store Word
void Interpreter::SW(uint32_t code) {
    Memory::Write(addr_(code), regTrg_(code));
}

// Store Word Left
void Interpreter::SWL(uint32_t code) {
    static const uint32_t SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
    static const uint32_t SWL_SHIFT[4] = { 24, 16, 8, 0 };
    uint32_t addr = addr_(code);
    uint32_t shift = addr & 3;
    uint32_t mem = Memory::Read<uint32_t>(addr & ~3);
    Memory::Write(addr & ~3, (regTrg_(code) >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]));
}

// Store Word Right
void Interpreter::SWR(uint32_t code) {
    static const uint32_t SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
    static const uint32_t SWR_SHIFT[4] = { 0, 8, 16, 24 };
    uint32_t addr = addr_(code);
    uint32_t shift = addr & 3;
    uint32_t mem = Memory::Read<uint32_t>(addr & ~3);
    Memory::Write(addr & ~3, (regTrg_(code) << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]));
}


////////////////////////////////////////
// Computational Instructions
////////////////////////////////////////

// ADD Immediate
void Interpreter::ADDI(uint32_t code) {
    Load(rt_(code), regSrc_(code) + immediate_(code));
    // ? Trap on two's complement overflow.
}

// ADD Immediate Unsigned
void Interpreter::ADDIU(uint32_t code) {
    Load(rt_(code), regSrc_(code) + immediate_(code));
}

// Set on Less Than Immediate
void Interpreter::SLTI(uint32_t code) {
    Load(rt_(code), (static_cast<int32_t>(regSrc_(code)) < immediate_(code)) ? 1 : 0);
}

// Set on Less Than Unsigned Immediate
void Interpreter::SLTIU(uint32_t code) {
    Load(rt_(code), (regSrc_(code) < static_cast<uint32_t>(immediate_(code))) ? 1 : 0);
}

// AND Immediate
void Interpreter::ANDI(uint32_t code) {
    Load(rt_(code), regSrc_(code) & immediateU_(code));
}

// OR Immediate
void Interpreter::ORI(uint32_t code) {
    Load(rt_(code), regSrc_(code) | immediateU_(code));
}

// eXclusive OR Immediate
void Interpreter::XORI(uint32_t code) {
    Load(rt_(code), regSrc_(code) ^ immediateU_(code));
}

// Load Upper Immediate
void Interpreter::LUI(uint32_t code) {
    Load(rt_(code), code << 16);
}


// ADD
void Interpreter::ADD(uint32_t code) {
    Load(rd_(code), regSrc_(code) + regTrg_(code));
    // ? Trap on two's complement overflow.
}

// ADD Unsigned
void Interpreter::ADDU(uint32_t code) {
    Load(rd_(code), regSrc_(code) + regTrg_(code));
}

// SUBtract
void Interpreter::SUB(uint32_t code) {
    Load(rd_(code), regSrc_(code) - regSrc_(code));
    // ? Trap on two's complement overflow.
}

// SUBtract Unsigned
void Interpreter::SUBU(uint32_t code) {
    Load(rd_(code), regSrc_(code) - regSrc_(code));
}

// Set on Less Than
void Interpreter::SLT(uint32_t code) {
    Load(rd_(code), (static_cast<int32_t>(regSrc_(code)) < static_cast<int32_t>(regTrg_(code))) ? 1 : 0);
}

// Set on Less Than Unsigned
void Interpreter::SLTU(uint32_t code) {
    Load(rd_(code), (regSrc_(code) < regTrg_(code)) ? 1 : 0);
}

// AND
void Interpreter::AND(uint32_t code) {
    Load(rd_(code), regSrc_(code) & regTrg_(code));
}

// OR
void Interpreter::OR(uint32_t code) {
    Load(rd_(code), regSrc_(code) | regTrg_(code));
}

// eXclusive OR
void Interpreter::XOR(uint32_t code) {
    Load(rd_(code), regSrc_(code) ^ regTrg_(code));
}

// NOR
void Interpreter::NOR(uint32_t code) {
    Load(rd_(code), ~(regSrc_(code) | regTrg_(code)));
}


// Shift Left Logical
void Interpreter::SLL(uint32_t code) {
    Load(rd_(code), regTrg_(code) << shamt_(code));
}

// Shift Right Logical
void Interpreter::SRL(uint32_t code) {
    Load(rd_(code), regTrg_(code) >> shamt_(code));
}

// Shift Right Arithmetic
void Interpreter::SRA(uint32_t code) {
    Load(rd_(code), static_cast<int32_t>(regTrg_(code)) >> shamt_(code));
}

// Shift Left Logical Variable
void Interpreter::SLLV(uint32_t code) {
    Load(rd_(code), regTrg_(code) << regSrc_(code));
}

// Shift Right Logical Variable
void Interpreter::SRLV(uint32_t code) {
    Load(rd_(code), regTrg_(code) >> regSrc_(code));
}

// Shift Right Arithmetic Variable
void Interpreter::SRAV(uint32_t code) {
    Load(rd_(code), static_cast<int32_t>(regTrg_(code)) >> regSrc_(code));
}


// MULTiply
void Interpreter::MULT(uint32_t code) {
    int32_t rs = static_cast<int32_t>(regSrc_(code));
    int32_t rt = static_cast<int32_t>(regTrg_(code));
    // CommitDelayedLoad();
    int64_t res = (int64_t)rs * (int64_t)rt;
    R3000a.GPR.LO = static_cast<uint32_t>(res & 0xffffffff);
    R3000a.GPR.HI = static_cast<uint32_t>(res >> 32);
}

// MULtiply Unsigned
void Interpreter::MULTU(uint32_t code) {
    uint32_t rs = regSrc_(code);
    uint32_t rt = regTrg_(code);
    // CommitDelayedLoad();
    uint64_t res = (uint64_t)rs * (uint64_t)rt;
    R3000a.GPR.LO = static_cast<uint32_t>(res & 0xffffffff);
    R3000a.GPR.HI = static_cast<uint32_t>(res >> 32);
}

// Divide
void Interpreter::DIV(uint32_t code) {
    int32_t rs = static_cast<int32_t>(regSrc_(code));
    int32_t rt = static_cast<int32_t>(regTrg_(code));
    // CommitDelayedLoad();
    R3000a.GPR.LO = static_cast<uint32_t>(rs / rt);
    R3000a.GPR.HI = static_cast<uint32_t>(rs % rt);
}

// Divide Unsigned
void Interpreter::DIVU(uint32_t code) {
    uint32_t rs = regSrc_(code);
    uint32_t rt = regTrg_(code);
    // CommitDelayedLoad();
    R3000a.GPR.LO = rs / rt;
    R3000a.GPR.HI = rs % rt;
}

// Move From HI
void Interpreter::MFHI(uint32_t code) {
    Load(rd_(code), R3000a.GPR.HI);
}

// Move From LO
void Interpreter::MFLO(uint32_t code) {
    Load(rd_(code), R3000a.GPR.LO);
}

// Move To HI
void Interpreter::MTHI(uint32_t code) {
    uint32_t rd = regDst_(code);
    // CommitDelayedLoad();
    R3000a.GPR.HI = rd;
}

// Move To LO
void Interpreter::MTLO(uint32_t code) {
    uint32_t rd = regDst_(code);
    // CommitDelayedLoad();
    R3000a.GPR.LO = rd;
}


////////////////////////////////////////
// Jump and Branch Instructions
////////////////////////////////////////

// Jump
void Interpreter::J(uint32_t code) {
    Interpreter_.doBranch((target_(code) << 2) | (PC & 0xf0000000));
}

// Jump And Link
void Interpreter::JAL(uint32_t code) {
    if (PC == 0x80016604) {
        Disasm.Parse(u32M(PC-4));
        Disasm.PrintCode();
    }
    R3000a.GPR.RA = PC + 4;
    Interpreter_.doBranch((target_(code) << 2) | (PC & 0xf0000000));
}

// Jump Register
void Interpreter::JR(uint32_t code) {
    Interpreter_.doBranch(regSrc_(code));
}

// Jump And Link Register
void Interpreter::JALR(uint32_t code) {
    uint32_t rd = rd_(code);
    if (rd != 0) {
        R3000a.GPR.R[rd] = PC + 4;
    }
    Interpreter_.doBranch(regSrc_(code));
}


// Branch on EQual
void Interpreter::BEQ(uint32_t code) {
    if (regSrc_(code) == regTrg_(code)) {
        Interpreter_.doBranch(PC + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Not Equal
void Interpreter::BNE(uint32_t code) {
    if (regSrc_(code) != regTrg_(code)) {
        Interpreter_.doBranch(PC + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Less than or Equal Zero
void Interpreter::BLEZ(uint32_t code) {
    if (static_cast<int32_t>(regSrc_(code)) <= 0) {
        Interpreter_.doBranch(PC + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Greate Than Zero
void Interpreter::BGTZ(uint32_t code) {
    if (static_cast<int32_t>(regSrc_(code)) > 0) {
        Interpreter_.doBranch(PC + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Less Than Zero
void Interpreter::BLTZ(uint32_t code) {
    if (static_cast<int32_t>(regSrc_(code)) < 0) {
        Interpreter_.doBranch(PC + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Greater than or Equal Zero
void Interpreter::BGEZ(uint32_t code) {
    if (static_cast<int32_t>(regSrc_(code)) >= 0) {
        Interpreter_.doBranch(PC + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Less Than Zero And Link
void Interpreter::BLTZAL(uint32_t code) {
    if (static_cast<int32_t>(regSrc_(code)) < 0) {
        uint32_t pc = PC;
        R3000a.GPR.RA = pc + 4;
        Interpreter_.doBranch(pc + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch on Greater than or Equal Zero And Link
void Interpreter::BGEZAL(uint32_t code) {
    if (static_cast<int32_t>(regSrc_(code)) >= 0) {
        uint32_t pc = PC;
        R3000a.GPR.RA = pc + 4;
        Interpreter_.doBranch(pc + (static_cast<int32_t>(immediate_(code)) << 2));
    }
}

// Branch Condition
void Interpreter::BCOND(uint32_t code) {
    BCONDS[rt_(code)](code);
}



////////////////////////////////////////
// Special Instructions
////////////////////////////////////////

void Interpreter::SYSCALL(uint32_t) {
    PC -= 4;
    R3000a.Exception(0x20, R3000a.IsInDelaySlot());
}
void Interpreter::BREAK(uint32_t) {}

void Interpreter::SPECIAL(uint32_t code) {
    SPECIALS[funct_(code)](code);
}


////////////////////////////////////////
// Co-processor Instructions
////////////////////////////////////////

void Interpreter::LWC0(uint32_t) {

}
void Interpreter::LWC1(uint32_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: LWC1 is not supported."));
}
void Interpreter::LWC2(uint32_t) {}
void Interpreter::LWC3(uint32_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: LWC3 is not supported."));
}

void Interpreter::SWC0(uint32_t) {}
void Interpreter::SWC1(uint32_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: SWC1 is not supported."));
}

void Interpreter::SWC2(uint32_t) {}
void Interpreter::SWC3(uint32_t) {
    // HLE?
    wxMessageOutputDebug().Printf(wxT("WARNING: SWC3 is not supported."));
}



void Interpreter::COP0(uint32_t) {}
void Interpreter::COP1(uint32_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: COP1 is not supported."));
}
void Interpreter::COP2(uint32_t) {
    // Cop2 is not supported.
}
void Interpreter::COP3(uint32_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: COP3 is not supported."));
}


////////////////////////////////////////
// HLE functions
////////////////////////////////////////

void hleDummy()
{
    PC = R3000a.GPR.RA;
    R3000a.BranchTest();
}

void hleA0()
{
    BIOS::biosA0[R3000a.GPR.T1 & 0xff]();
    R3000a.BranchTest();
}

void hleB0()
{
    BIOS::biosB0[R3000a.GPR.T1 & 0xff]();
    R3000a.BranchTest();
}

void hleC0()
{
    BIOS::biosC0[R3000a.GPR.T1 & 0xff]();
    R3000a.BranchTest();
}

void hleBootstrap() {}

struct EXEC {
    uint32_t pc0;
    uint32_t gp0;
    uint32_t t_addr;
    uint32_t t_size;
    uint32_t d_addr;
    uint32_t d_size;
    uint32_t b_addr;
    uint32_t b_size;
    uint32_t S_addr;
    uint32_t s_size;
    uint32_t sp, fp, gp, ret, base;
};

void hleExecRet()
{
    EXEC *header = pointer_cast<EXEC*>(Memory::CharPtr(R3000a.GPR.S0));
    R3000a.GPR.RA = BFLIP32(header->ret);
    R3000a.GPR.SP = BFLIP32(header->sp);
    R3000a.GPR.FP = BFLIP32(header->fp);
    R3000a.GPR.GP = BFLIP32(header->gp);
    R3000a.GPR.S0 = BFLIP32(header->base);
    R3000a.GPR.V0 = 1;
    PC = R3000a.GPR.RA;
}

void (*const HLEt[])() = {
        hleDummy, hleA0, hleB0, hleC0, hleBootstrap, hleExecRet
};


void Interpreter::HLECALL(uint32_t code) {
/*
    if ((uint32_t)((code & 0xff) -1) < 3) {
        wxMessageOutputDebug().Printf("HLECALL %c0:%02x", 'A' + (code & 0xff)-1, R3000a.GPR.T1 & 0xff);
    }
*/
    HLEt[code & 0xff]();
}



////////////////////////////////////////////////////////////////
// Thread
////////////////////////////////////////////////////////////////

wxThread::ExitCode InterpreterThread::Entry()
{
    wxMessageOutputDebug().Printf(wxT("PSX Threads is started."));

    do {
        if (RootCounter::SPURun() == 0) break;
//#warning PSX::Interpreter::Thread don't call SPUendflush
        Interpreter_.ExecuteOnce();
    } while (TestDestroy() == false);
    return 0;
}

void InterpreterThread::OnExit()
{
    wxMessageOutputDebug().Printf(wxT("PSX Thread is ended."));
}



////////////////////////////////////////////////////////////////
// Main Loop
////////////////////////////////////////////////////////////////

InterpreterThread *Interpreter::thread = 0;

InterpreterThread *Interpreter::Execute()
{
    if (thread != 0) {
        Shutdown();
    }
    thread = new InterpreterThread();
    thread->Create();
    thread->Run();
    return thread;
}

void Interpreter::Shutdown()
{
    if (thread == 0) return;
    if (thread->IsRunning() == false) return;
    thread->Delete();
    thread = 0;
}


void (*Interpreter::OPCODES[64])(uint32_t) = {
        SPECIAL, BCOND, J, JAL, BEQ, BNE, BLEZ, BGTZ,
        ADDI, ADDIU, SLTI, SLTIU, ANDI, ORI, XORI, LUI,
        COP0, COP1, COP2, COP3, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        LB, LH, LWL, LW, LBU, LHU, LWR, OPNULL,
        SB, SH, SWL, SW, OPNULL, OPNULL, SWR, OPNULL,
        LWC0, LWC1, LWC2, LWC3, OPNULL, OPNULL, OPNULL, OPNULL,
        SWC0, SWC1, SWC2, HLECALL, OPNULL, OPNULL, OPNULL, OPNULL
};

void (*Interpreter::SPECIALS[64])(uint32_t) = {
        SLL, OPNULL, SRL, SRA, SLLV, OPNULL, SRLV, SRAV,
        JR, JALR, OPNULL, HLECALL, SYSCALL, BREAK, OPNULL, OPNULL,
        MFHI, MTHI, MFLO, MTLO, OPNULL, OPNULL, OPNULL, OPNULL,
        MULT, MULTU, DIV, DIVU, OPNULL, OPNULL, OPNULL, OPNULL,
        ADD, ADDU, SUB, SUBU, AND, OR, XOR, NOR,
        OPNULL, OPNULL, SLT, SLTU, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL
};

void (*Interpreter::BCONDS[24])(uint32_t) = {
        BLTZ, BGEZ, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL,
        BLTZAL, BGEZAL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL, OPNULL
};


void Interpreter::Init()
{
}

void Interpreter::ExecuteBlock() {
    R3000a.doingBranch = false;
    do {
        ExecuteOnce();
    } while (!R3000a.doingBranch);
}

Interpreter& Interpreter::GetInstance()
{
    static Interpreter instance;
    return instance;
}

}   // namespace R3000A

R3000A::Interpreter& Interpreter_ = R3000A::Interpreter::GetInstance();

}   // namespace PSX
