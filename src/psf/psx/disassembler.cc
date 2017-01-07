#include "psf/psx/disassembler.h"
#include "psf/psx/psx.h"
#include "psf/psx/r3000a.h"
#include <wx/sstream.h>
#include <wx/wfstream.h>

// memo: display as follows
// mov  sp, [eax+4]
// sp := 0x80001000

namespace {

// const uint32_t& PC = regs_->PC;

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

bool Disassembler::parseNop(u32)
{
    return true;
}

bool Disassembler::parseLoad(u32 code)
{
    wxString strRt = regNames[Rt(code)];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf(wxT("0x%04x($%s)"), Imm(code), regNames[Rs(code)]);
    operands.push_back(addr);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parseStore(u32 code)
{
    wxString strRt = regNames[Rt(code)];
    operands.push_back(strRt);
    wxString addr;
    addr.Printf(wxT("0x%04x($%s)"), Imm(code), regNames[Rs(code)]);
    operands.push_back(addr);

    wxString dest_addr;
    dest_addr.Printf(wxT("0x%08x"), Imm(code) + RsVal(regs_->GPR, code));
    changedRegisters.push_back(dest_addr);
    return true;
}

bool Disassembler::parseALUI(u32 code)
{
    wxString strRt = regNames[Rt(code)];
    operands.push_back(strRt);
    operands.push_back(regNames[Rs(code)]);
    wxString imm;
    imm.Printf(wxT("0x%04x"), Imm(code));
    operands.push_back(imm);
    changedRegisters.push_back(strRt);
    return true;
}

bool Disassembler::parse3OpReg(u32 code)
{
    wxString strRd = regNames[Rd(code)];
    operands.push_back(strRd);
    operands.push_back(regNames[Rs(code)]);
    operands.push_back(regNames[Rt(code)]);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseShift(u32 code)
{
    wxString strRd = regNames[Rd(code)];
    operands.push_back(strRd);
    operands.push_back(regNames[Rt(code)]);
    wxString strShift;
    strShift.Printf(wxT("0x%x"), Shamt(code));
    operands.push_back(strShift);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseShiftVar(u32 code)
{
    wxString strRd = regNames[Rd(code)];
    operands.push_back(strRd);
    operands.push_back(regNames[Rt(code)]);
    operands.push_back(regNames[Rs(code)]);
    changedRegisters.push_back(strRd);
    return true;
}

bool Disassembler::parseMulDiv(u32 code)
{
    operands.push_back(regNames[Rs(code)]);
    operands.push_back(regNames[Rt(code)]);
    changedRegisters.push_back(regNames[GPR_HI]);
    changedRegisters.push_back(regNames[GPR_LO]);
    return true;
}

bool Disassembler::parseHILO(u32 code)
{
    wxString strRd = regNames[Rd(code)];
    operands.push_back(strRd);
    operands.push_back(strRd);
    return true;
}

bool Disassembler::parseJ(u32 code)
{
    wxString addr;
    addr.Printf(wxT("0x%08x"), Target(code) << 2 | ((regs_->PC-4) & 0xf0000000));
    operands.push_back(addr);
    changedRegisters.push_back(regNames[GPR_PC]);
    return true;
}

bool Disassembler::parseJAL(u32 code)
{
    bool ret = parseJ(code);
    changedRegisters.push_back(regNames[GPR_RA]);
    return ret;
}

bool Disassembler::parseJR(u32 code)
{
    operands.push_back(regNames[Rs(code)]);
    changedRegisters.push_back(regNames[GPR_PC]);
    return true;
}

bool Disassembler::parseJALR(u32 code)
{
    bool ret = parseJR(code);
    wxString strRd = regNames[Rd(code)];
    operands.push_back(strRd);
    changedRegisters.push_back(strRd);
    return ret;
}

bool Disassembler::parseBranch(u32 code)
{
    wxString strRs = regNames[Rs(code)];
    wxString strRt = regNames[Rt(code)];
    u32 addr = (regs_->PC-4) + (Imm(code) << 2);
    wxString strAddr;
    strAddr.Printf(wxT("0x%08x"), addr);
    operands.push_back(strRs);
    operands.push_back(strRt);
    operands.push_back(strAddr);
    changedRegisters.push_back(regNames[GPR_PC]);
    return true;
}

bool Disassembler::parseBranchZ(u32 code)
{
    wxString strRs = regNames[Rs(code)];
    u32 addr = (regs_->PC-4) + (Imm(code) << 2);
    wxString strAddr;
    strAddr.Printf(wxT("0x%08x"), addr);
    operands.push_back(strRs);
    operands.push_back(strAddr);
    changedRegisters.push_back(regNames[GPR_PC]);
    return true;
}

bool Disassembler::parseBranchZAL(u32 code)
{
    bool ret = parseBranchZ(code);
    operands.push_back(regNames[GPR_RA]);
    return ret;
}

bool Disassembler::parseCopz(u32)
{
    return true;
}


bool (Disassembler::*const Disassembler::SPECIALS[64])(u32) = {
    &Disassembler::parseShift, &Disassembler::parseNop, &Disassembler::parseShift, &Disassembler::parseShift, &Disassembler::parseShift, &Disassembler::parseNop, &Disassembler::parseShift, &Disassembler::parseShift,
    &Disassembler::parseJR, &Disassembler::parseJALR, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseHILO, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseMulDiv, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parse3OpReg, &Disassembler::parse3OpReg, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop,
    &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop, &Disassembler::parseNop
};


bool Disassembler::parseSpecial(u32 code)
{
    opcodeName = specialLowerList[Funct(code)];
    return (this->*SPECIALS[Funct(code)])(code);
}

bool Disassembler::parseBcond(u32 code)
{
    opcodeName = bcondLowerList[Funct(code)];
    return true;
}

bool (Disassembler::*const Disassembler::OPCODES[64])(u32) = {
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
  : UserMemoryAccessor(composite), regs_(&composite->R3000ARegs())
{
  operands.reserve(4);
}



bool Disassembler::Parse(u32 code)
{
  pc0 = regs_->PC - 4;
  code_ = code;
  operands.clear();
  changedRegisters.clear();

  opcodeName = opcodeLowerList[Opcode(code)];
  if (opcodeName == strSPECIAL) {
    return parseSpecial(code);
  }
  if (opcodeName == strBCOND) {
    return parseBcond(code);
  }
  if (opcodeName == strUNKNOWN) {
    return false;
  }

  return (this->*OPCODES[Opcode(code)])(code);
}

void Disassembler::PrintCode(wxOutputStream* out)
{
  wxString addr;
  addr.Printf(wxT("%08X:  "), regs_->PC-4);
  wxStringOutputStream ss;
  ss.Write(addr, addr.size());
  if (opcodeName == strUNKNOWN) {
    wxString str_opcode;
    str_opcode.Printf(wxT("(0x%02x, 0x%02x)"), Opcode(code_), Funct(code_));
    opcodeName.Append(str_opcode);
  }
  ss.Write(opcodeName, opcodeName.size());
  ss.Write("        ", std::min<unsigned int>(8 - opcodeName.size(), 8));
  for (wxVector<wxString>::const_iterator it = operands.begin(), it_end = operands.end(); it != it_end; ++it) {
    if (it != operands.begin()) {
      ss.Write(", ", 2);
    }
    ss.Write(*it, it->size());
  }
  if (out) {
    wxString str = ss.GetString();
    str.Append("\n");
    out->Write(str.c_str().AsChar(), str.size());
  } else {
    wxMessageOutputDebug().Printf(ss.GetString());
  }
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
      strAddr.Printf(wxT("0x%08x"), psxMu32val(addr));
      ss.Write(strAddr, strAddr.size());
    } else {
      for (int i = 0; i < 35; i++) {
        if (it->Cmp(regNames[i]) == 0) {
          strAddr.Printf(wxT("0x%08x"), regs_->GPR(i));
          ss.Write(strAddr, strAddr.size());
          break;
        }
      }
    }
    wxMessageOutputDebug().Printf(ss.GetString());
  }
}


void Disassembler::StartOutputToFile() {
  output_to_.Open("assembled.txt", wxFile::write);
}

bool Disassembler::OutputToFile() {
  if (output_to_.IsOpened() == false) return false;
  Parse(psxMu32val(regs_->PC - 4));
  wxFileOutputStream os(output_to_);
  PrintCode(&os);
  return true;
}

void Disassembler::StopOutputToFile() {
  output_to_.Close();
}


void Disassembler::DumpRegisters()
{
    wxString line;
  line.Printf(wxT("%s=0x%08x "), regNames[GPR_PC], regs_->PC);
  for (int i = 1; i < 34; i++) {
    line.Printf(wxT("%s%s=0x%08x "), line.c_str(), regNames[i], regs_->GPR(i));
    if (i % 4 == 3) {
      wxMessageOutputDebug().Printf(line);
      line.Clear();
    }
  }
  wxMessageOutputDebug().Printf(line);
  line.Clear();

  line.Printf(wxT("epc=0x%08x cause=0x%08x status=0x%08x"), regs_->CP0.EPC, regs_->CP0.CAUSE, regs_->CP0.SR);
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
