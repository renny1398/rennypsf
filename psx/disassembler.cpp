#include "disassembler.h"
#include "r3000a.h"
#include "memory.h"
#include <wx/sstream.h>

// memo: display as follows
// mov  sp, [eax+4]
// sp := 0x80001000

namespace {

const uint32_t& PC = PSX::R3000a.PC;

const char *opcodeLowerList[64] = {
    "SPECIAL", "BCOND", "j", "jal", "beq", "bne", "blez", "bgtz",
    "addi", "addiu", "slti", "sltiu", "andi", "ori", "xori", "lui",
    "cop0", "cop1", "cop2", "cop3", "unk", "unk", "unk", "unk",
    "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk",
    "lb", "lh", "lwl", "lw", "lbu", "lhu", "lwr", "unk",
    "sb", "sh", "swl", "sw", "unk", "unk", "swr", "unk",
    "lwc0", "lwc1", "lwc2", "lwc3", "unk", "unk", "unk", "unk",
    "swc0", "swc1", "swc2", "swc3", "unk", "unk", "unk", "unk"
};

const char *specialLowerList[64] = {
    "sll", "unk", "srl", "sra", "sllv", "unk", "srlv", "srav",
    "jr", "jalr", "unk", "unk", "syscall", "break", "unk", "unk",
    "mfhi", "mthi", "mflo", "mtlo", "unk", "unk", "unk", "unk",
    "mult", "multu", "div", "divu", "unk", "unk", "unk", "unk",
    "add", "addu", "sub", "subu", "and", "or", "xor", "nor",
    "unk", "unk", "slt", "sltu", "unk", "unk", "unk", "unk",
    "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk",
    "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk"
};

const char *bcondLowerList[24] = {
    "bltz", "bgez", "unk", "unk", "unk", "unk", "unk", "unk",
    "unk", "unk", "unk", "unk", "unk", "unk", "unk", "unk",
    "bltzal", "bgezal", "unk", "unk", "unk", "unk", "unk", "unk"
};

const char *copzLowerList[16] = {
    "mfc", "unk", "cfc", "unk", "mtc", "unk", "ctc", "unk",
    "bcc", "unk", "unk", "unk", "unk", "unk", "unk", "unk"
};

const char *regNames[] = {
    "$zr", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
    "$hi", "$lo", "$pc"
};



}   // namespace



namespace PSX {

bool Disassembler::parseNop(uint32_t)
{
    return true;
}

bool Disassembler::parseLoad(uint32_t code)
{
    wxString strRt = regNames[rt_(code)];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf("%04x(%s)", immediate_(code), regNames[rs_(code)]);
    operands.push_back(addr);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parseStore(uint32_t code)
{
    wxString strRt = regNames[rt_(code)];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf("%04x(%s)", immediate_(code), regNames[rs_(code)]);
    operands.push_back(addr);

    wxString dest_addr;
    dest_addr.Printf("$%08x", immediate_(code) + regSrc_(code));
    changedRegisters.push_back(dest_addr);
    return true;
}

bool Disassembler::parseALUI(uint32_t code)
{
    wxString strRt = regNames[rt_(code)];
    operands.push_back(strRt);
    operands.push_back(regNames[rs_(code)]);
    wxString imm;
    imm.Printf("%04x", static_cast<int32_t>(immediate_(code)));
    operands.push_back(imm);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parse3OpReg(uint32_t code)
{
    wxString strRd = regNames[rd_(code)];
    operands.push_back(strRd);
    operands.push_back(regNames[rs_(code)]);
    operands.push_back(regNames[rt_(code)]);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseShift(uint32_t code)
{
    wxString strRd = regNames[rd_(code)];
    operands.push_back(strRd);
    operands.push_back(regNames[rt_(code)]);
    wxString strShift;
    strShift.Printf("%d", shamt_(code));
    operands.push_back(strShift);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseShiftVar(uint32_t code)
{
    wxString strRd = regNames[rd_(code)];
    operands.push_back(strRd);
    operands.push_back(regNames[rt_(code)]);
    operands.push_back(regNames[rs_(code)]);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseMulDiv(uint32_t code)
{
    operands.push_back(regNames[rs_(code)]);
    operands.push_back(regNames[rt_(code)]);
    changedRegisters.push_back("$hi");
    changedRegisters.push_back("$lo");
    return true;
}

bool Disassembler::parseHILO(uint32_t code)
{
    wxString strRd = regNames[rd_(code)];
    operands.push_back(strRd);
    operands.push_back(strRd);
    return true;
}

bool Disassembler::parseJ(uint32_t code)
{
    wxString addr;
    addr.Printf("%08x", target_(code) << 2 | ((R3000a.PC-4) & 0xf0000000));
    operands.push_back(addr);
    changedRegisters.push_back("$pc");
    return true;
}

bool Disassembler::parseJAL(uint32_t code)
{
    bool ret = parseJ(code);
    changedRegisters.push_back("$ra");
    return ret;
}

bool Disassembler::parseJR(uint32_t code)
{
    operands.push_back(regNames[rs_(code)]);
    changedRegisters.push_back("$pc");
    return true;
}

bool Disassembler::parseJALR(uint32_t code)
{
    bool ret = parseJR(code);
    wxString strRd = regNames[rd_(code)];
    operands.push_back(strRd);
    changedRegisters.push_back(strRd);
    return ret;
}

bool Disassembler::parseBranch(uint32_t code)
{
    wxString strRs = regNames[rs_(code)];
    wxString strRt = regNames[rt_(code)];
    uint32_t addr = (PC-4) + (static_cast<int32_t>(immediate_(code)) << 2);
    wxString strAddr;
    strAddr.Printf("$%08x", addr);
    operands.push_back(strRs);
    operands.push_back(strRt);
    operands.push_back(strAddr);
    changedRegisters.push_back("$pc");
    return true;
}

bool Disassembler::parseBranchZ(uint32_t code)
{
    wxString strRs = regNames[rs_(code)];
    uint32_t addr = (PC-4) + (static_cast<int32_t>(immediate_(code)) << 2);
    wxString strAddr;
    strAddr.Printf("$%08x", addr);
    operands.push_back(strRs);
    operands.push_back(strAddr);
    changedRegisters.push_back("$pc");
    return true;
}

bool Disassembler::parseBranchZAL(uint32_t code)
{
    bool ret = parseBranchZ(code);
    operands.push_back("$ra");
    return ret;
}

bool Disassembler::parseCopz(uint32_t)
{
    return true;
}


bool (Disassembler::*const Disassembler::SPECIALS[64])(uint32_t) = {
    &Disassembler::parseShift, &Disassembler::parseNop, &Disassembler::parseShift, &Disassembler::parseShift, &Disassembler::parseShift, &Disassembler::parseNop, &Disassembler::parseShift, &Disassembler::parseShift,
    &Disassembler::parseJR, &Disassembler::parseJALR, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop
};


bool Disassembler::parseSpecial(uint32_t code)
{
    opcodeName = specialLowerList[funct_(code)];
    return (this->*SPECIALS[funct_(code)])(code);
}

bool Disassembler::parseBcond(uint32_t code)
{
    opcodeName = bcondLowerList[funct_(code)];
    return true;
}

bool (Disassembler::*const Disassembler::OPCODES[64])(uint32_t) = {
    &Disassembler::parseSpecial, &Disassembler::parseBcond, &Disassembler::parseJ, &Disassembler::parseJAL, &Disassembler::parseBranch, &Disassembler::parseBranch, &Disassembler::parseBranchZ, &Disassembler::parseBranchZ,
    &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI,
    &Disassembler::parseCopz, &Disassembler::parseCopz, &Disassembler::parseCopz, &Disassembler::parseCopz, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseNop,
    &Disassembler::parseStore, &Disassembler::parseStore, &Disassembler::parseStore, &Disassembler::parseStore, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseStore, &Disassembler::parseNop,
    &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop
};



Disassembler::Disassembler()
{
    operands.reserve(4);
}



bool Disassembler::Parse(uint32_t code)
{
    pc0 = R3000a.PC - 4;
    operands.clear();
    changedRegisters.clear();

    opcodeName = opcodeLowerList[opcode_(code)];
    if (opcodeName == "SPECIAL") {
        return parseSpecial(code);
    }
    if (opcodeName == "BCOND") {
        return parseBcond(code);
    }
    if (opcodeName == "unk") {
        return false;
    }

    return (this->*OPCODES[opcode_(code)])(code);
}

void Disassembler::PrintCode()
{
    wxString addr;
    addr.Printf("%08X:  ", PC-4);
    wxStringOutputStream ss;
    ss.Write(addr, addr.size());
    ss.Write(opcodeName, opcodeName.size());
    ss.Write(" ", 1);
    for (wxVector<wxString>::const_iterator it = operands.begin(), it_end = operands.end(); it != it_end; ++it) {
        if (it != operands.begin()) {
            ss.Write(", ", 2);
        }
        ss.Write(*it, it->size());
    }
    wxMessageOutputDebug().Printf(ss.GetString());
}

void Disassembler::PrintChangedRegisters()
{
    for (wxVector<wxString>::const_iterator it = changedRegisters.begin(), it_end = changedRegisters.end(); it != it_end; ++it) {
        wxStringOutputStream ss;
        ss.Write(*it, it->size());
        ss.Write(" <= ", 4);
        const char *str = it->c_str();
        char *endptr;
        uint32_t addr = static_cast<uint32_t>(strtol(str+1, &endptr, 16));
        wxString strAddr;
        if (addr != 0L && *endptr == 0) {
            strAddr.Printf("%08x", Memory::Read<uint32_t>(addr));
            ss.Write(strAddr, strAddr.size());
        } else {
            for (int i = 0; i < 35; i++) {
                wxString s;
                if (it->Cmp(regNames[i]) == 0) {
                    strAddr.Printf("%08x (i = %d)", R3000a.GPR.R[i], i);
                    ss.Write(strAddr, strAddr.size());
                    break;
                }
            }
        }
        wxMessageOutputDebug().Printf(ss.GetString());
    }
}

Disassembler& Disassembler::GetInstance()
{
    static Disassembler instance;
    return instance;
}

Disassembler& Disasm = Disassembler::GetInstance();

}   // namespace PSX
