#pragma once
#include "common.h"
#include "memory.h"

#include <wx/string.h>
#include <wx/vector.h>
#include <wx/file.h>

class wxOutputStream;

namespace psx {
namespace mips {

class Disassembler : private UserMemoryAccessor
{
public:
  Disassembler(PSX *composite);

  bool Parse(u32 code);
  void PrintCode(wxOutputStream* out = nullptr);
  void PrintChangedRegisters();

  void StartOutputToFile();
  bool OutputToFile();
  void StopOutputToFile();

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
  Registers* const regs_;
  u32 pc0;
  u32 code_;
  wxString opcodeName;
  wxVector<wxString> operands;
  wxVector<wxString> changedRegisters;
  wxFile output_to_;

protected:
  static bool (Disassembler::*const OPCODES[64])(u32);
  static bool (Disassembler::*const SPECIALS[64])(u32);
  //static bool (*const BCONDS[64])(u32);
  //static bool (*const COPz[64])(u32);
};

}   // namespace R3000A

extern mips::Disassembler& Disasm;

}   // namespace PSX
