#include "disassembler.h"
#include "r3000a.h"
#include "memory.h"
#include <wx/sstream.h>

// memo: display as follows
// mov  sp, [eax+4]
// sp := 0x80001000

namespace {

// const uint32_t& PC = R3000ARegs().PC;

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

/*
const wxChar *copzLowerList[16] = {
    wxT("mfc"), strUNKNOWN, wxT("cfc"), strUNKNOWN, wxT("mtc"), strUNKNOWN, wxT("ctc"), strUNKNOWN,
    wxT("bcc"), strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN, strUNKNOWN
};
*/

const wxChar *regNames[] = {
    wxT("zr"), wxT("at"), wxT("v0"), wxT("v1"), wxT("a0"), wxT("a1"), wxT("a2"), wxT("a3"),
    wxT("t0"), wxT("t1"), wxT("t2"), wxT("t3"), wxT("t4"), wxT("t5"), wxT("t6"), wxT("t7"),
    wxT("s0"), wxT("s1"), wxT("s2"), wxT("s3"), wxT("s4"), wxT("s5"), wxT("s6"), wxT("s7"),
    wxT("t8"), wxT("t9"), wxT("k0"), wxT("k1"), wxT("gp"), wxT("sp"), wxT("fp"), wxT("ra"),
    wxT("hi"), wxT("lo"), wxT("pc")
};



}   // namespace



namespace PSX {
namespace R3000A {

bool Disassembler::parseNop(Instruction)
{
    return true;
}

bool Disassembler::parseLoad(Instruction code)
{
    wxString strRt = regNames[code.Rt()];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf(wxT("0x%04x($%s)"), code.Imm(), regNames[code.Rs()]);
    operands.push_back(addr);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parseStore(Instruction code)
{
    wxString strRt = regNames[code.Rt()];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf(wxT("0x%04x($%s)"), code.Imm(), regNames[code.Rs()]);
    operands.push_back(addr);

    wxString dest_addr;
    dest_addr.Printf(wxT("0x%08x"), code.Imm() + code.RsVal());
    changedRegisters.push_back(dest_addr);
    return true;
}

bool Disassembler::parseALUI(Instruction code)
{
    wxString strRt = regNames[code.Rt()];
    operands.push_back(strRt);
    operands.push_back(regNames[code.Rs()]);
    wxString imm;
    imm.Printf(wxT("0x%04x"), code.Imm());
    operands.push_back(imm);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parse3OpReg(Instruction code)
{
    wxString strRd = regNames[code.Rd()];
    operands.push_back(strRd);
    operands.push_back(regNames[code.Rs()]);
    operands.push_back(regNames[code.Rt()]);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseShift(Instruction code)
{
    wxString strRd = regNames[code.Rd()];
    operands.push_back(strRd);
    operands.push_back(regNames[code.Rt()]);
    wxString strShift;
    strShift.Printf(wxT("0x%x"), code.Shamt());
    operands.push_back(strShift);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseShiftVar(Instruction code)
{
    wxString strRd = regNames[code.Rd()];
    operands.push_back(strRd);
    operands.push_back(regNames[code.Rt()]);
    operands.push_back(regNames[code.Rs()]);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseMulDiv(Instruction code)
{
    operands.push_back(regNames[code.Rs()]);
    operands.push_back(regNames[code.Rt()]);
    changedRegisters.push_back(regNames[R3000A::GPR_HI]);
    changedRegisters.push_back(regNames[R3000A::GPR_LO]);
    return true;
}

bool Disassembler::parseHILO(Instruction code)
{
    wxString strRd = regNames[code.Rd()];
    operands.push_back(strRd);
    operands.push_back(strRd);
    return true;
}

bool Disassembler::parseJ(Instruction code)
{
    wxString addr;
    addr.Printf(wxT("0x%08x"), code.Target() << 2 | ((R3000ARegs().PC-4) & 0xf0000000));
    operands.push_back(addr);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseJAL(Instruction code)
{
    bool ret = parseJ(code);
    changedRegisters.push_back(regNames[R3000A::GPR_RA]);
    return ret;
}

bool Disassembler::parseJR(Instruction code)
{
    operands.push_back(regNames[code.Rs()]);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseJALR(Instruction code)
{
    bool ret = parseJR(code);
    wxString strRd = regNames[code.Rd()];
    operands.push_back(strRd);
    changedRegisters.push_back(strRd);
    return ret;
}

bool Disassembler::parseBranch(Instruction code)
{
    wxString strRs = regNames[code.Rs()];
    wxString strRt = regNames[code.Rt()];
    uint32_t addr = (R3000ARegs().PC-4) + (code.Imm() << 2);
    wxString strAddr;
    strAddr.Printf(wxT("0x%08x"), addr);
    operands.push_back(strRs);
    operands.push_back(strRt);
    operands.push_back(strAddr);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseBranchZ(Instruction code)
{
    wxString strRs = regNames[code.Rs()];
    uint32_t addr = (R3000ARegs().PC-4) + (code.Imm() << 2);
    wxString strAddr;
    strAddr.Printf(wxT("0x%08x"), addr);
    operands.push_back(strRs);
    operands.push_back(strAddr);
    changedRegisters.push_back(regNames[R3000A::GPR_PC]);
    return true;
}

bool Disassembler::parseBranchZAL(Instruction code)
{
    bool ret = parseBranchZ(code);
    operands.push_back(regNames[R3000A::GPR_RA]);
    return ret;
}

bool Disassembler::parseCopz(Instruction)
{
    return true;
}


bool (Disassembler::*const Disassembler::SPECIALS[64])(Instruction) = {
    &Disassembler::parseShift, &Disassembler::parseNop, &Disassembler::parseShift, &Disassembler::parseShift, &Disassembler::parseShift, &Disassembler::parseNop, &Disassembler::parseShift, &Disassembler::parseShift,
    &Disassembler::parseJR, &Disassembler::parseJALR, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop
};


bool Disassembler::parseSpecial(Instruction code)
{
    opcodeName = specialLowerList[code.Funct()];
    return (this->*SPECIALS[code.Funct()])(code);
}

bool Disassembler::parseBcond(Instruction code)
{
    opcodeName = bcondLowerList[code.Funct()];
    return true;
}

bool (Disassembler::*const Disassembler::OPCODES[64])(Instruction) = {
    &Disassembler::parseSpecial, &Disassembler::parseBcond, &Disassembler::parseJ, &Disassembler::parseJAL, &Disassembler::parseBranch, &Disassembler::parseBranch, &Disassembler::parseBranchZ, &Disassembler::parseBranchZ,
    &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI, &Disassembler::parseALUI,
    &Disassembler::parseCopz, &Disassembler::parseCopz, &Disassembler::parseCopz, &Disassembler::parseCopz, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseNop,
    &Disassembler::parseStore, &Disassembler::parseStore, &Disassembler::parseStore, &Disassembler::parseStore, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseStore, &Disassembler::parseNop,
    &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseLoad, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop
};



Disassembler::Disassembler(Composite* composite)
  : Component(composite)
{
    operands.reserve(4);
}



bool Disassembler::Parse(Instruction code)
{
    pc0 = R3000ARegs().PC - 4;
    operands.clear();
    changedRegisters.clear();

    opcodeName = opcodeLowerList[code.Opcode()];
    if (opcodeName == strSPECIAL) {
        return parseSpecial(code);
    }
    if (opcodeName == strBCOND) {
        return parseBcond(code);
    }
    if (opcodeName == strUNKNOWN) {
        return false;
    }

    return (this->*OPCODES[code.Opcode()])(code);
}

void Disassembler::PrintCode()
{
    wxString addr;
    addr.Printf(wxT("%08X:  "), R3000ARegs().PC-4);
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
            strAddr.Printf(wxT("0x%08x"), U32M_ref(addr));
            ss.Write(strAddr, strAddr.size());
        } else {
            for (int i = 0; i < 35; i++) {
                if (it->Cmp(regNames[i]) == 0) {
                    strAddr.Printf(wxT("0x%08x"), R3000ARegs().GPR.R[i]);
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
    line.Printf(wxT("%s=0x%08x "), regNames[R3000A::GPR_PC], R3000ARegs().PC);
    for (int i = 1; i < 34; i++) {
        line.Printf(wxT("%s%s=0x%08x "), line.c_str(), regNames[i], R3000ARegs().GPR.R[i]);
        if (i % 4 == 3) {
            wxMessageOutputDebug().Printf(line);
            line.Clear();
        }
    }
    wxMessageOutputDebug().Printf(line);
    line.Clear();

    line.Printf(wxT("epc=0x%08x cause=0x%08x status=0x%08x"), R3000ARegs().CP0.EPC, R3000ARegs().CP0.CAUSE, R3000ARegs().CP0.SR);
    wxMessageOutputDebug().Printf(line);
    line.Clear();

//    line.Printf(wxT("$1070=0x%08x $1074=0x%08x"), PSX::u32H(0x1070), PSX::u32H(0x1074));
//    wxMessageOutputDebug().Printf(line);
}

/*
Disassembler& Disassembler::GetInstance()
{
    static Disassembler instance;
    return instance;
}
*/

}   // namespace R3000A

// R3000A::Disassembler& Disasm = R3000A::Disassembler::GetInstance();

}   // namespace PSX
