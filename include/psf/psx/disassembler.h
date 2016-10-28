#pragma once
#include "common.h"
#include <wx/string.h>
#include <wx/vector.h>


namespace PSX {
namespace R3000A {

class Disassembler : public Component
{
public:
  Disassembler(Composite *composite);

  bool Parse(u32 code);
  void PrintCode();
  void PrintChangedRegisters();

  void DumpRegisters();

  static Disassembler& GetInstance();

private:
  bool parseNop(u32);
  bool parseLoad(u32 code);
  bool parseStore(u32 code);
  bool parseALUI(u32 code);
  bool parse3OpReg(u32 code);
  bool parseShift(u32 code);
  bool parseShiftVar(u32 code);
  bool parseMulDiv(u32 code);
  bool parseHILO(u32 code);
  bool parseJ(u32 code);
  bool parseJAL(u32 code);
  bool parseJR(u32 code);
  bool parseJALR(u32 code);
  bool parseJumpReg(u32 code);
  bool parseBranch(u32 code);
  bool parseBranchZ(u32 code);
  bool parseBranchZAL(u32 code);
  bool parseCopz(u32 code);

  bool parseSpecial(u32 code);
  bool parseBcond(u32 code);
  // void HLECALL(u32);

private:
  u32 pc0;
  wxString opcodeName;
  wxVector<wxString> operands;
  wxVector<wxString> changedRegisters;

protected:
  static bool (Disassembler::*const OPCODES[64])(u32);
  static bool (Disassembler::*const SPECIALS[64])(u32);
  //static bool (*const BCONDS[64])(u32);
  //static bool (*const COPz[64])(u32);
};

}   // namespace R3000A

extern R3000A::Disassembler& Disasm;

}   // namespace PSX
