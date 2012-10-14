#pragma once
#include <wx/string.h>
#include <wx/vector.h>
//#include <wx/hashmap.h>

//WX_DECLARE_HASH_MAP(uint32_t, uint32_t, wxIntegerHash, wxIntegerEqual, RegValueMap);

namespace PSX {

class Disassembler /*: public Processor*/
{
public:
    Disassembler();

    bool Parse(uint32_t code);
    void PrintCode();
    void PrintChangedRegisters();

    void DumpRegisters();

    static Disassembler& GetInstance();

private:
    bool parseNop(uint32_t);
    bool parseLoad(uint32_t code);
    bool parseStore(uint32_t code);
    bool parseALUI(uint32_t code);
    bool parse3OpReg(uint32_t code);
    bool parseShift(uint32_t code);
    bool parseShiftVar(uint32_t code);
    bool parseMulDiv(uint32_t code);
    bool parseHILO(uint32_t code);
    bool parseJ(uint32_t code);
    bool parseJAL(uint32_t code);
    bool parseJR(uint32_t code);
    bool parseJALR(uint32_t code);
    bool parseJumpReg(uint32_t code);
    bool parseBranch(uint32_t code);
    bool parseBranchZ(uint32_t code);
    bool parseBranchZAL(uint32_t code);
    bool parseCopz(uint32_t code);

    bool parseSpecial(uint32_t code);
    bool parseBcond(uint32_t code);
    // void HLECALL(uint32_t);

private:
    uint32_t pc0;
    wxString opcodeName;
    wxVector<wxString> operands;
    wxVector<wxString> changedRegisters;

    static bool (Disassembler::*const OPCODES[64])(uint32_t);
    static bool (Disassembler::*const SPECIALS[64])(uint32_t);
    //static bool (*const BCONDS[64])(uint32_t);
    //static bool (*const COPz[64])(uint32_t);
};

extern Disassembler& Disasm;

}   // namespace PSX
