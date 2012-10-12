#include "r3000a.h"
#include "memory.h"
#include "bios.h"
#include "rcnt.h"

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


//namespace R3000A {

R3000A::GeneralPurposeRegisters R3000AImpl::GPR;
R3000A::Cop0Registers R3000AImpl::CP0;
uint32_t& R3000AImpl::HI = GPR.HI;
uint32_t& R3000AImpl::LO = GPR.LO;
uint32_t& R3000AImpl::PC = GPR.PC;
uint32_t R3000AImpl::Cycle;
uint32_t R3000AImpl::Interrupt;

bool R3000AImpl::inDelaySlot = false;
bool R3000AImpl::doingBranch = false;

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


void R3000AImpl::Reset()
{
    memset(&GPR, 0, sizeof(GPR));
    memset(&CP0, 0, sizeof(CP0));
    PC = 0xbfc00000;
    /*last_code = */Cycle = Interrupt = 0;
    CP0.R[12] = 0x10900000; // COP0_ENABLED | BEV | TS
    CP0.R[15] = 0x00000002; // PRevId = Revision Id, same as R3000A
//    delayed_load_target = 0;
//    delayed_load_value = 0;
}





void R3000AImpl::Exception(uint32_t code, bool branch_delay)
{
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


void R3000AImpl::BranchTest()
{
    if (Cycle - RootCounter::nextsCounter >= RootCounter::nextCounter) {
        RootCounter::Update();
    }

    // interrupt mask register ($1f801074)
    if (psxHu32(0x1070) & psxHu32(0x1074)) {
        if ((CP0.SR & 0x401) == 0x401) {
            Exception(0x400, false);
        }
    }
}


R3000AImpl& R3000AImpl::GetInstance()
{
    static R3000AImpl instance;
    return instance;
}

R3000AImpl& R3000a = R3000AImpl::GetInstance();

//}   // namespace R3000A

//R3000A::R3000AImpl& R3000a = R3000A::R3000AImpl::R3000a;

}   // namespace PSX
