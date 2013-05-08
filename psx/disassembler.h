#pragma once
#include <wx/string.h>
#include <wx/vector.h>
#include <stdint.h>

namespace PSX {
namespace R3000A {

class Instruction;

class Disassembler /*: public Processor*/
{
public:
    Disassembler();

    bool Parse(Instruction code);
    void PrintCode();
    void PrintChangedRegisters();

    void DumpRegisters();

    static Disassembler& GetInstance();

private:
    bool parseNop(Instruction);
    bool parseLoad(Instruction code);
    bool parseStore(Instruction code);
    bool parseALUI(Instruction code);
    bool parse3OpReg(Instruction code);
    bool parseShift(Instruction code);
    bool parseShiftVar(Instruction code);
    bool parseMulDiv(Instruction code);
    bool parseHILO(Instruction code);
    bool parseJ(Instruction code);
    bool parseJAL(Instruction code);
    bool parseJR(Instruction code);
    bool parseJALR(Instruction code);
    bool parseJumpReg(Instruction code);
    bool parseBranch(Instruction code);
    bool parseBranchZ(Instruction code);
    bool parseBranchZAL(Instruction code);
    bool parseCopz(Instruction code);

    bool parseSpecial(Instruction code);
    bool parseBcond(Instruction code);
    // void HLECALL(Instruction);

private:
    uint32_t pc0;
    wxString opcodeName;
    wxVector<wxString> operands;
    wxVector<wxString> changedRegisters;

protected:
    static bool (Disassembler::*const OPCODES[64])(Instruction);
    static bool (Disassembler::*const SPECIALS[64])(Instruction);
    //static bool (*const BCONDS[64])(Instruction);
    //static bool (*const COPz[64])(Instruction);
};

}   // namespace R3000A

extern R3000A::Disassembler& Disasm;

}   // namespace PSX
