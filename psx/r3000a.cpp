#include "r3000a.h"
#include "memory.h"
#include "bios.h"
#include "rcnt.h"
#include "disassembler.h"

#include <wx/debug.h>
#include <cstring>


namespace PSX {

const char *strGPR[35] = {
    "ZR", "AT", "V0", "V1", "A0", "A1", "A2", "A3",
    "T0", "T1", "T2", "T3", "T4", "T5", "T6", "T7",
    "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7",
    "T8", "T9", "K0", "K1", "GP", "SP", "FP", "RA",
    "HI", "LO", "PC"
};

namespace R3000A {

GeneralPurposeRegisters Processor::GPR;
Cop0Registers Processor::CP0;
uint32_t& Processor::HI = GPR.HI;
uint32_t& Processor::LO = GPR.LO;
uint32_t& Processor::PC = GPR.PC;
uint32_t Processor::Cycle;
uint32_t Processor::Interrupt;

bool Processor::inDelaySlot = false;
bool Processor::doingBranch = false;

/*
GeneralPurposeRegisters GPR;
Cop0Registers CP0;
uint32_t& PC = GPR.PC;

//uint32_t last_code;
uint_fast32_t cycle;
uint_fast32_t interrupt;
*/
// for delay_load
//uint32_t delayed_load_target;
//uint32_t delayed_load_value;


void Processor::Init()
{
    CP0.R[12] = 0x10900000; // COP0_ENABLED | BEV | TS
    CP0.R[15] = 0x00000002; // PRevId = Revision Id, same as R3000A
    wxMessageOutputDebug().Printf(wxT("Initialized R3000A processor."));
}


void Processor::Reset()
{
    inDelaySlot = doingBranch = false;
    memset(&GPR, 0, sizeof(GPR));
    memset(&CP0, 0, sizeof(CP0));
    PC = 0xbfc00000;
    /*last_code = */Cycle = Interrupt = 0;
    CP0.R[12] = 0x10900000; // COP0_ENABLED | BEV | TS
    CP0.R[15] = 0x00000002; // PRevId = Revision Id, same as R3000A
//    delayed_load_target = 0;
//    delayed_load_value = 0;
    wxMessageOutputDebug().Printf(wxT("R3000A processor is reset."));
}





void Processor::Exception(uint32_t code, bool branch_delay)
{
    wxMessageOutputDebug().Printf(wxT("R3000A Exception: 0x%x"), code);

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
    BIOS::Exception();
}





////////////////////////////////////////////////////////////////
// OPCODES
////////////////////////////////////////////////////////////////


void Processor::BranchTest()
{
    if (Cycle - RootCounter::nextsCounter >= RootCounter::nextCounter) {
        RootCounter::Update();
    }

    // interrupt mask register ($1f801074)
    if (u32H(0x1070) & u32H(0x1074)) {
        if ((CP0.SR & 0x401) == 0x401) {
            //wxMessageOutputDebug().Printf("WARNING: skip branch exception.");
            Disasm.DumpRegisters();
            Exception(0x400, false);
            Disasm.DumpRegisters();
        }
    }
}


Processor& Processor::GetInstance()
{
    static Processor instance;
    return instance;
}

}   // namespace R3000A

R3000A::Processor& R3000a = R3000A::Processor::GetInstance();

}   // namespace PSX
