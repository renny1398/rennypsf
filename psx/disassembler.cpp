#include "disassembler.h"
#include "r3000a.h"
#include "memory.h"
#include <wx/sstream.h>

// memo: display as follows
// mov  sp, [eax+4]
// sp := 0x80001000

namespace {

const uint32_t& PC = PSX::R3000a.PC;

const wxChar strSPECIAL[] = wxT("_SPECIAL");
const wxChar strBCOND[] = wxT("_BCOND");
const wxChar strUNKNOWN[] = wxT("_unk");

const wxChar *opcodeLowerList[64] = {
    strSPECIAL, strBCOND, wxT("j"), wxT("jal"), wxT("beq"), wxT("bne"), wxT("blez"), wxT("bgtz"),
    wxT("addi"), wxT("addiu"), wxT("slti"), wxT("sltiu"), wxT("andi"), wxT("ori"), wxT("xori"), wxT("lui"),
    wxT("cop0"), wxT("cop1"), wxT("cop2"), wxT("cop3"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    wxT("lb"), wxT("lh"), wxT("lwl"), wxT("lw"), wxT("lbu"), wxT("lhu"), wxT("lwr"), strUNKNOWN,
    wxT("sb"), wxT("sh"), wxT("swl"), wxT("sw"), strUNKNOWN, strUNKNOWN, wxT("swr"), strUNKNOWN,
    wxT("lwc0"), wxT("lwc1"), wxT("lwc2"), wxT("lwc3"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    wxT("swc0"), wxT("swc1"), wxT("swc2"), wxT("swc3"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN
};

const wxChar *specialLowerList[64] = {
    wxT("sll"), strUNKNOWN, wxT("srl"), wxT("sra"), wxT("sllv"), strUNKNOWN, wxT("srlv"), wxT("srav"),
    wxT("jr"), wxT("jalr"), strUNKNOWN, strUNKNOWN, wxT("syscall"), wxT("break"), strUNKNOWN, strUNKNOWN,
    wxT("mfhi"), wxT("mthi"), wxT("mflo"), wxT("mtlo"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    wxT("mult"), wxT("multu"), wxT("div"), wxT("divu"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    wxT("add"), wxT("addu"), wxT("sub"), wxT("subu"), wxT("and"), wxT("or"), wxT("xor"), wxT("nor"),
    strUNKNOWN, strUNKNOWN, wxT("slt"), wxT("sltu"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN
};

const wxChar *bcondLowerList[24] = {
    wxT("bltz"), wxT("bgez"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN,
    wxT("bltzal"), wxT("bgezal"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN
};

const wxChar *copzLowerList[16] = {
    wxT("mfc"), strUNKNOWN, wxT("cfc"), strUNKNOWN, wxT("mtc"), strUNKNOWN, wxT("ctc"), strUNKNOWN,
    wxT("bcc"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN
};

const wxChar *regNames[] = {
    wxT("zr"), wxT("at"), wxT("v0"), wxT("v1"), wxT("a0"), wxT("a1"), wxT("a2"), wxT("a3"),
    wxT("t0"), wxT("t1"), wxT("t2"), wxT("t3"), wxT("t4"), wxT("t5"), wxT("t6"), wxT("t7"),
    wxT("s0"), wxT("s1"), wxT("s2"), wxT("s3"), wxT("s4"), wxT("s5"), wxT("s6"), wxT("s7"),
    wxT("t8"), wxT("t9"), wxT("k0"), wxT("k1"), wxT("gp"), wxT("sp"), wxT("fp"), wxT("ra"),
    wxT("hi"), wxT("lo"), wxT("pc")
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
    addr.Printf(wxT("0x%04x($%s)"), immediate_(code), regNames[rs_(code)]);
    operands.push_back(addr);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parseStore(uint32_t code)
{
    wxString strRt = regNames[rt_(code)];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf(wxT("0x%04x($%s)"), immediate_(code), regNames[rs_(code)]);
    operands.push_back(addr);

    wxString dest_addr;
    dest_addr.Printf(wxT("0x%08x"), immediate_(code) + regSrc_(code));
    changedRegisters.push_back(dest_addr);
    return true;
}

bool Disassembler::parseALUI(uint32_t code)
{
    wxString strRt = regNames[rt_(code)];
    operands.push_back(strRt);
    operands.push_back(regNames[rs_(code)]);
    wxString imm;
    imm.Printf(wxT("0x%04x"), static_cast<int32_t>(immediate_(code)));
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
    strShift.Printf(wxT("0x%x"), shamt_(code));
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
    changedRegisters.push_back(regNames[R3000A::GPR_HI]);
    changedRegisters.push_back(regNames[R3000A::GPR_LO]);
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
    addr.Printf(wxT("0x%08x"), target_(code) << 2 | ((R3000a.PC-4) & 0xf0000000));
    operands.push_back(addr);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseJAL(uint32_t code)
{
    bool ret = parseJ(code);
    changedRegisters.push_back(regNames[R3000A::GPR_RA]);
    return ret;
}

bool Disassembler::parseJR(uint32_t code)
{
    operands.push_back(regNames[rs_(code)]);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
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
    strAddr.Printf(wxT("0x%08x"), addr);
    operands.push_back(strRs);
    operands.push_back(strRt);
    operands.push_back(strAddr);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseBranchZ(uint32_t code)
{
    wxString strRs = regNames[rs_(code)];
    uint32_t addr = (PC-4) + (static_cast<int32_t>(immediate_(code)) << 2);
    wxString strAddr;
    strAddr.Printf(wxT("0x%08x"), addr);
    operands.push_back(strRs);
    operands.push_back(strAddr);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseBranchZAL(uint32_t code)
{
    bool ret = parseBranchZ(code);
    operands.push_back(regNames[R3000A::GPR_RA]);
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
    if (opcodeName == strSPECIAL) {
        return parseSpecial(code);
    }
    if (opcodeName == strBCOND) {
        return parseBcond(code);
    }
    if (opcodeName == strUNKNOWN) {
        return false;
    }

    return (this->*OPCODES[opcode_(code)])(code);
}

void Disassembler::PrintCode()
{
    wxString addr;
    addr.Printf(wxT("%08X:  "), PC-4);
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
        ss.Write("$", 1);
        ss.Write(*it, it->size());
        ss.Write(" := ", 4);
        wxString strAddr;
        if ((*it)[0] == _T('0') && (*it)[1] == _T('x')) {
            unsigned long addr;
            it->ToULong(&addr);  // warning: this code may be wrong
            strAddr.Printf(wxT("0x%08x"), Memory::Read<uint32_t>(addr));
            ss.Write(strAddr, strAddr.size());
        } else {
            for (int i = 0; i < 35; i++) {
                if (it->Cmp(regNames[i]) == 0) {
                    strAddr.Printf(wxT("0x%08x"), R3000a.GPR.R[i]);
                    ss.Write(strAddr, strAddr.size());
                    break;
                }
            }
        }
        wxMessageOutputDebug().Printf(ss.GetString());
    }
}


void Disassembler::DumpRegisters()
{
    wxString line;
    line.Printf(wxT("%s=0x%08x "), regNames[R3000A::GPR_PC], R3000a.PC);
    for (int i = 1; i < 34; i++) {
        line.Printf(wxT("%s%s=0x%08x "), line.c_str(), regNames[i], R3000a.GPR.R[i]);
        if (i % 4 == 3) {
            wxMessageOutputDebug().Printf(line);
            line.Clear();
        }
    }
    wxMessageOutputDebug().Printf(line);
}


Disassembler& Disassembler::GetInstance()
{
    static Disassembler instance;
    return instance;
}

Disassembler& Disasm = Disassembler::GetInstance();

}   // namespace PSX
