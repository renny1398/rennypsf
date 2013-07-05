#include "interpreter.h"
#include "memory.h"
#include "rcnt.h"
#include "bios.h"

#include "../spu/spu.h"

//using namespace PSX;
//using namespace PSX::R3000A;
//using namespace PSX::Interpreter;

namespace {

}   // namespace


namespace PSX {
  namespace R3000A {

    ////////////////////////////////////////////////////////////////
    // Delay Functions
    ////////////////////////////////////////////////////////////////

    void Interpreter::delayRead(Instruction code, u32 reg, u32 branch_pc)
    {
      wxASSERT(reg < 32);

      u32 reg_old, reg_new;

      reg_old = R3000ARegs().GPR.R[reg];
      ExecuteOpcode(code);  // branch delay load
      reg_new = R3000ARegs().GPR.R[reg];

      R3000ARegs().PC = branch_pc;
      R3000a().BranchTest();

      R3000ARegs().GPR.R[reg] = reg_old;
      ExecuteOnce();
      R3000ARegs().GPR.R[reg] = reg_new;

      R3000a().LeaveDelaySlot();
    }

    void Interpreter::delayWrite(Instruction, u32 /*reg*/, u32 branch_pc)
    {
      wxASSERT(reg < 32);
      // OPCODES[code >> 26]();
      R3000a().LeaveDelaySlot();
      R3000ARegs().PC = branch_pc;
      R3000a().BranchTest();
    }

    void Interpreter::delayReadWrite(Instruction, u32 /*reg*/, u32 branch_pc)
    {
      wxASSERT(reg < 32);
      R3000a().LeaveDelaySlot();
      R3000ARegs().PC = branch_pc;
      R3000a().BranchTest();
    }


    ////////////////////////////////////////////////////////////////
    // Delay Opcodes
    ////////////////////////////////////////////////////////////////

    ////////////////////////////////////////
    // Reusable Functions
    ////////////////////////////////////////

    bool Interpreter::delayNOP(Instruction, u32, u32)
    {
      return false;
    }

    bool Interpreter::delayRs(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rs() == reg) {
        delayRead(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayWt(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rt() == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayWd(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rd() == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayRsRt(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rs() == reg || code.Rt() == reg) {
        delayRead(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayRsWt(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rs() == reg) {
        if (code.Rt() == reg) {
          delayReadWrite(code, reg, branch_pc);
          return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
      }
      if (code.Rt() == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayRsWd(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rs() == reg) {
        if (code.Rd() == reg) {
          delayReadWrite(code, reg, branch_pc);
          return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
      }
      if (code.Rd() == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayRtWd(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rt() == reg) {
        if (code.Rd() == reg) {
          delayReadWrite(code, reg, branch_pc);
          return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
      }
      if (code.Rd() == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    bool Interpreter::delayRsRtWd(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rs() == reg || code.Rt() == reg) {
        if (code.Rd() == reg) {
          delayReadWrite(code, reg, branch_pc);
          return true;
        }
        delayRead(code, reg, branch_pc);
        return true;
      }
      if (code.Rd() == reg) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    ////////////////////////////////////////
    // SPECIAL functions
    ////////////////////////////////////////

    bool Interpreter::delaySLL(Instruction code, u32 reg, u32 branch_pc) {
      if (code) {
        return delayRtWd(code, reg, branch_pc);
      }
      return false;   // delay nop
    }

    Interpreter::DelayFunc Interpreter::delaySRL = &Interpreter::delayRtWd;
    Interpreter::DelayFunc Interpreter::delaySRA = &Interpreter::delayRtWd;

    Interpreter::DelayFunc Interpreter::delayJR = &Interpreter::delayRs;

    Interpreter::DelayFunc Interpreter::delayJALR = &Interpreter::delayRsWd;

    Interpreter::DelayFunc Interpreter::delayADD = &Interpreter::delayRsRtWd;
    Interpreter::DelayFunc Interpreter::delayADDU = &Interpreter::delayRsRtWd;
    Interpreter::DelayFunc Interpreter::delaySLLV = &Interpreter::delayRsRtWd;

    Interpreter::DelayFunc Interpreter::delayMFHI = &Interpreter::delayWd;
    Interpreter::DelayFunc Interpreter::delayMFLO = &Interpreter::delayWd;
    Interpreter::DelayFunc Interpreter::delayMTHI = &Interpreter::delayRs;
    Interpreter::DelayFunc Interpreter::delayMTLO = &Interpreter::delayRs;

    Interpreter::DelayFunc Interpreter::delayMULT = &Interpreter::delayRsRt;
    Interpreter::DelayFunc Interpreter::delayDIV = &Interpreter::delayRsRt;


    Interpreter::DelayFunc Interpreter::delaySpecials[64] = {
      &Interpreter::delaySLL,  &Interpreter::delayNOP,   Interpreter::delaySRL,   Interpreter::delaySRA,
      Interpreter::delaySLLV, &Interpreter::delayNOP,   Interpreter::delaySLLV,  Interpreter::delaySLLV,
      Interpreter::delayJR,    Interpreter::delayJALR, &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      Interpreter::delayMFHI,  Interpreter::delayMTHI,  Interpreter::delayMFLO,  Interpreter::delayMTLO,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      Interpreter::delayMULT,  Interpreter::delayMULT,  Interpreter::delayDIV,   Interpreter::delayDIV,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      Interpreter::delayADD,   Interpreter::delayADDU,  Interpreter::delayADD,   Interpreter::delayADDU,
      Interpreter::delayADD,   Interpreter::delayADD,   Interpreter::delayADD,   Interpreter::delayADD,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,   Interpreter::delayADD,   Interpreter::delayADD,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP,  &Interpreter::delayNOP
    };

    bool Interpreter::delaySPCL(Instruction code, u32 reg, u32 branch_pc)
    {
      return (this->*delaySpecials[code.Funct()])(code, reg, branch_pc);
    }

    Interpreter::DelayFunc Interpreter::delayBCOND = &Interpreter::delayRs;

    bool Interpreter::delayJAL(Instruction code, u32 reg, u32 branch_pc)
    {
      if (reg == R3000A::GPR_RA) {
        delayWrite(code, reg, branch_pc);
        return true;
      }
      return false;
    }

    Interpreter::DelayFunc Interpreter::delayBEQ = &Interpreter::delayRsRt;
    Interpreter::DelayFunc Interpreter::delayBNE = &Interpreter::delayRsRt;

    Interpreter::DelayFunc Interpreter::delayBLEZ = &Interpreter::delayRs;
    Interpreter::DelayFunc Interpreter::delayBGTZ = &Interpreter::delayRs;

    Interpreter::DelayFunc Interpreter::delayADDI = &Interpreter::delayRsWt;
    Interpreter::DelayFunc Interpreter::delayADDIU = &Interpreter::delayRsWt;

    Interpreter::DelayFunc Interpreter::delayLUI = &Interpreter::delayWt;

    bool Interpreter::delayCOP0(Instruction code, u32 reg, u32 branch_pc)
    {
      switch (code.Funct() & ~0x03) {
      case 0x00:  // MFC0, CFC0
        if (code.Rt() == reg) {
          delayWrite(code, reg, branch_pc);
          return true;
        }
        return false;
      case 0x04:  // MTC0, CTC0
        if (code.Rt() == reg) {
          delayRead(code, reg, branch_pc);
          return true;
        }
        return false;
      }
      return false;
    }

    bool Interpreter::delayLWL(Instruction code, u32 reg, u32 branch_pc)
    {
      if (code.Rt() == reg) {
        delayReadWrite(code, reg, branch_pc);
        return true;
      }
      if (code.Rs() == reg) {
        delayRead(code, reg, branch_pc);
        return true;
      }
      return false;
    }
    Interpreter::DelayFunc Interpreter::delayLWR = &Interpreter::delayLWL;

    Interpreter::DelayFunc Interpreter::delayLB = &Interpreter::delayRsWt;
    Interpreter::DelayFunc Interpreter::delayLH = &Interpreter::delayRsWt;
    Interpreter::DelayFunc Interpreter::delayLW = &Interpreter::delayRsWt;
    Interpreter::DelayFunc Interpreter::delayLBU = &Interpreter::delayRsWt;
    Interpreter::DelayFunc Interpreter::delayLHU = &Interpreter::delayRsWt;

    Interpreter::DelayFunc Interpreter::delaySB = &Interpreter::delayRsRt;
    Interpreter::DelayFunc Interpreter::delaySH = &Interpreter::delayRsRt;
    Interpreter::DelayFunc Interpreter::delaySWL = &Interpreter::delayRsRt;
    Interpreter::DelayFunc Interpreter::delaySW = &Interpreter::delayRsRt;
    Interpreter::DelayFunc Interpreter::delaySWR = &Interpreter::delayRsRt;

    Interpreter::DelayFunc Interpreter::delayLWC2 = &Interpreter::delayRs;
    Interpreter::DelayFunc Interpreter::delaySWC2 = &Interpreter::delayRs;


    Interpreter::DelayFunc Interpreter::delayOpcodes[64] = {
      &Interpreter::delaySPCL,  Interpreter::delayBCOND, &Interpreter::delayNOP,  &Interpreter::delayJAL,
      Interpreter::delayBEQ,   Interpreter::delayBNE,    Interpreter::delayBLEZ,  Interpreter::delayBGTZ,
      Interpreter::delayADDI,  Interpreter::delayADDIU,  Interpreter::delayADDI,  Interpreter::delayADDI,
      Interpreter::delayADDI,  Interpreter::delayADDI,   Interpreter::delayADDI,  Interpreter::delayLUI,
      &Interpreter::delayCOP0, &Interpreter::delayNOP,   &Interpreter::delayCOP0, &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
      Interpreter::delayLB,    Interpreter::delayLH,    &Interpreter::delayLWL,   Interpreter::delayLW,
      Interpreter::delayLBU,   Interpreter::delayLHU,    Interpreter::delayLWR,  &Interpreter::delayNOP,
      Interpreter::delaySB,    Interpreter::delaySH,     Interpreter::delaySWL,   Interpreter::delaySW,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,    Interpreter::delaySWR,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,    Interpreter::delayLWC2, &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,    Interpreter::delaySWC2, &Interpreter::delayNOP,
      &Interpreter::delayNOP,  &Interpreter::delayNOP,   &Interpreter::delayNOP,  &Interpreter::delayNOP
    };


    void Interpreter::delayTest(u32 reg, u32 branch_pc)
    {
      Instruction branch_code(&cpu_, U32M_ref(branch_pc));
      R3000a().EnterDelaySlot();
      u32 op = branch_code.Opcode();
      if ((this->*delayOpcodes[op])(branch_code, reg, branch_pc)) {
        return;
      }

      R3000ARegs().PC = branch_pc;
      ExecuteOpcode(branch_code);
      R3000a().LeaveDelaySlot();
      R3000a().BranchTest();
    }


    void Interpreter::doBranch(u32 branch_pc)
    {
      R3000a().doingBranch = true;

      u32 pc = R3000ARegs().PC;
      Instruction code(&cpu_, U32M_ref(pc));
      pc += 4;
      R3000ARegs().PC = pc;
      R3000ARegs().Cycle++;

      const u32 op = code.Opcode();
      switch (op) {
      case 0x10:  // COP0
        if ((code.Rs() & 0x03) == code.Rs()) {  // MFC0, CFC0
          delayTest(code.Rt(), branch_pc);
          return;
        }
        break;
      case 0x32:  // LWC2
        delayTest(code.Rt(), branch_pc);
        return;
      default:
        if (static_cast<u32>(op - 0x20) < 8) { // Load opcode
          wxASSERT(op >= 0x20);
          delayTest(code.Rt(), branch_pc);
          return;
        }
        break;
      }

      ExecuteOpcode(code);

      // branch itself & nop
      if (R3000ARegs().PC - 8 == branch_pc && !op) {
        RCnt().DeadLoopSkip();
      }
      R3000a().LeaveDelaySlot();
      R3000ARegs().PC = branch_pc;

      R3000a().BranchTest();
    }


    void Interpreter::NLOP(Instruction code) {
      wxMessageOutputDebug().Printf(wxT("WARNING: Unknown Opcode (0x%02x) is executed!"), code >> 26);
    }


    /*
inline void CommitDelayedLoad() {
    if (delayed_load_target) {
        R3000ARegs().GPR.R[delayed_load_target] = delayed_load_value;
        delayed_load_target = 0;
        delayed_load_value = 0;
    }
}

inline void DelayedBranch(u32 target) {
    CommitDelayedLoad();
    delayed_load_target = GPR_PC;
    delayed_load_value = target;
    wxMessageOutputDebug().Printf("Branch to 0x%08x", target);
}
*/

    inline void Interpreter::Load(u32 rt, u32 value) {
      // CommitDelayedLoad();
      if (rt != 0) {
        R3000ARegs().GPR.R[rt] = value;
      }
    }

    /*
// for load function from memory or not GPR into GPR
inline void DelayedLoad(u32 rt, u32 value) {
    wxASSERT_MSG(rt != GPR_PC, "Delayed-load target must be other than R3000ARegs().PC.");
    if (delayed_load_target == GPR_PC) {
        // delay load
        R3000ARegs().PC = delayed_load_value;
        delayed_load_target = rt;
        delayed_load_value = value;
        wxMessageOutputDebug().Printf("Delayed-loaded.");
        return;
    }
    // normal load
    Load(rt, value);
}
*/


    ////////////////////////////////////////
    // Load and Store Instructions
    ////////////////////////////////////////

    // Load Byte
    void Interpreter::LB(Instruction code) {
      Load(code.Rt(), ReadMemory8(code.Addr()));
    }

    // Load Byte Unsigned
    void Interpreter::LBU(Instruction code) {
      Load(code.Rt(), ReadMemory8(code.Addr()));
    }

    // Load Halfword
    void Interpreter::LH(Instruction code) {
      Load(code.Rt(), ReadMemory16(code.Addr()));
    }

    // Load Halfword Unsigned
    void Interpreter::LHU(Instruction code) {
      Load(code.Rt(), ReadMemory16(code.Addr()));
    }

    // Load Word
    void Interpreter::LW(Instruction code) {
      Load(code.Rt(), ReadMemory32(code.Addr()));
    }

    // Load Word Left
    void Interpreter::LWL(Instruction code) {
      static const u32 LWL_MASK[4] = { 0x00ffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
      static const u32 LWL_SHIFT[4] = { 24, 16, 8, 0 };
      u32 rt = code.Rt();
      u32 addr = code.Addr();
      u32 shift = addr & 3;
      u32 mem = U32M_ref(addr & ~3);
      Load(rt, (R3000ARegs().GPR.R[rt] & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]));
    }

    // Load Word Right
    void Interpreter::LWR(Instruction code) {
      static const u32 LWR_MASK[4] = { 0xff000000, 0xffff0000, 0xffffff00, 0x00000000 };
      static const u32 LWR_SHIFT[4] = { 0, 8, 16, 24 };
      u32 rt = code.Rt();
      u32 addr = code.Addr();
      u32 shift = addr & 3;
      u32 mem = U32M_ref(addr & ~3);
      Load(rt, (R3000ARegs().GPR.R[rt] & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]));
    }

    // Store Byte
    void Interpreter::SB(Instruction code) {
      WriteMemory8(code.Addr(), code.RtVal() & 0xff);
    }

    // Store Halfword
    void Interpreter::SH(Instruction code) {
      WriteMemory16(code.Addr(), code.RtVal() & 0xffff);
    }

    // Store Word
    void Interpreter::SW(Instruction code) {
      WriteMemory32(code.Addr(), code.RtVal());
    }

    // Store Word Left
    void Interpreter::SWL(Instruction code) {
      static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
      static const u32 SWL_SHIFT[4] = { 24, 16, 8, 0 };
      u32 addr = code.Addr();
      u32 shift = addr & 3;
      u32 mem = U32M_ref(addr & ~3);
      U32M_ref(addr & ~3) = (code.RtVal() >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]);
    }

    // Store Word Right
    void Interpreter::SWR(Instruction code) {
      static const u32 SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };
      static const u32 SWR_SHIFT[4] = { 0, 8, 16, 24 };
      u32 addr = code.Addr();
      u32 shift = addr & 3;
      u32 mem = U32M_ref(addr & ~3);
      U32M_ref(addr & ~3) = (code.RtVal() << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]);
    }


    ////////////////////////////////////////
    // Computational Instructions
    ////////////////////////////////////////

    // ADD Immediate
    void Interpreter::ADDI(Instruction code) {
      Load(code.Rt(), code.RsVal() + code.Imm());
      // ? Trap on two's complement overflow.
    }

    // ADD Immediate Unsigned
    void Interpreter::ADDIU(Instruction code) {
      Load(code.Rt(), code.RsVal() + code.Imm());
    }

    // Set on Less Than Immediate
    void Interpreter::SLTI(Instruction code) {
      Load(code.Rt(), (static_cast<int32_t>(code.RsVal()) < code.Imm()) ? 1 : 0);
    }

    // Set on Less Than Unsigned Immediate
    void Interpreter::SLTIU(Instruction code) {
      Load(code.Rt(), (code.RsVal() < static_cast<u32>(code.Imm())) ? 1 : 0);
    }

    // AND Immediate
    void Interpreter::ANDI(Instruction code) {
      Load(code.Rt(), code.RsVal() & code.ImmU());
    }

    // OR Immediate
    void Interpreter::ORI(Instruction code) {
      Load(code.Rt(), code.RsVal() | code.ImmU());
    }

    // eXclusive OR Immediate
    void Interpreter::XORI(Instruction code) {
      Load(code.Rt(), code.RsVal() ^ code.ImmU());
    }

    // Load Upper Immediate
    void Interpreter::LUI(Instruction code) {
      Load(code.Rt(), code << 16);
    }


    // ADD
    void Interpreter::ADD(Instruction code) {
      Load(code.Rd(), code.RsVal() + code.RtVal());
      // ? Trap on two's complement overflow.
    }

    // ADD Unsigned
    void Interpreter::ADDU(Instruction code) {
      Load(code.Rd(), code.RsVal() + code.RtVal());
    }

    // SUBtract
    void Interpreter::SUB(Instruction code) {
      Load(code.Rd(), code.RsVal() - code.RtVal());
      // ? Trap on two's complement overflow.
    }

    // SUBtract Unsigned
    void Interpreter::SUBU(Instruction code) {
      Load(code.Rd(), code.RsVal() - code.RtVal());
    }

    // Set on Less Than
    void Interpreter::SLT(Instruction code) {
      Load(code.Rd(), (static_cast<int32_t>(code.RsVal()) < static_cast<int32_t>(code.RtVal())) ? 1 : 0);
    }

    // Set on Less Than Unsigned
    void Interpreter::SLTU(Instruction code) {
      Load(code.Rd(), (code.RsVal() < code.RtVal()) ? 1 : 0);
    }

    // AND
    void Interpreter::AND(Instruction code) {
      Load(code.Rd(), code.RsVal() & code.RtVal());
    }

    // OR
    void Interpreter::OR(Instruction code) {
      Load(code.Rd(), code.RsVal() | code.RtVal());
    }

    // eXclusive OR
    void Interpreter::XOR(Instruction code) {
      Load(code.Rd(), code.RsVal() ^ code.RtVal());
    }

    // NOR
    void Interpreter::NOR(Instruction code) {
      Load(code.Rd(), ~(code.RsVal() | code.RtVal()));
    }


    // Shift Left Logical
    void Interpreter::SLL(Instruction code) {
      Load(code.Rd(), code.RtVal() << code.Shamt());
    }

    // Shift Right Logical
    void Interpreter::SRL(Instruction code) {
      Load(code.Rd(), code.RtVal() >> code.Shamt());
    }

    // Shift Right Arithmetic
    void Interpreter::SRA(Instruction code) {
      Load(code.Rd(), static_cast<int32_t>(code.RtVal()) >> code.Shamt());
    }

    // Shift Left Logical Variable
    void Interpreter::SLLV(Instruction code) {
      Load(code.Rd(), code.RtVal() << code.RsVal());
    }

    // Shift Right Logical Variable
    void Interpreter::SRLV(Instruction code) {
      Load(code.Rd(), code.RtVal() >> code.RsVal());
    }

    // Shift Right Arithmetic Variable
    void Interpreter::SRAV(Instruction code) {
      Load(code.Rd(), static_cast<int32_t>(code.RtVal()) >> code.RsVal());
    }


    // MULTiply
    void Interpreter::MULT(Instruction code) {
      int32_t rs = static_cast<int32_t>(code.RsVal());
      int32_t rt = static_cast<int32_t>(code.RtVal());
      // CommitDelayedLoad();
      int64_t res = (int64_t)rs * (int64_t)rt;
      R3000ARegs().GPR.LO = static_cast<u32>(res & 0xffffffff);
      R3000ARegs().GPR.HI = static_cast<u32>(res >> 32);
    }

    // MULtiply Unsigned
    void Interpreter::MULTU(Instruction code) {
      u32 rs = code.RsVal();
      u32 rt = code.RtVal();
      // CommitDelayedLoad();
      uint64_t res = (uint64_t)rs * (uint64_t)rt;
      R3000ARegs().GPR.LO = static_cast<u32>(res & 0xffffffff);
      R3000ARegs().GPR.HI = static_cast<u32>(res >> 32);
    }

    // Divide
    void Interpreter::DIV(Instruction code) {
      int32_t rs = static_cast<int32_t>(code.RsVal());
      int32_t rt = static_cast<int32_t>(code.RtVal());
      // CommitDelayedLoad();
      R3000ARegs().GPR.LO = static_cast<u32>(rs / rt);
      R3000ARegs().GPR.HI = static_cast<u32>(rs % rt);
    }

    // Divide Unsigned
    void Interpreter::DIVU(Instruction code) {
      u32 rs = code.RsVal();
      u32 rt = code.RtVal();
      // CommitDelayedLoad();
      R3000ARegs().GPR.LO = rs / rt;
      R3000ARegs().GPR.HI = rs % rt;
    }

    // Move From HI
    void Interpreter::MFHI(Instruction code) {
      Load(code.Rd(), R3000ARegs().GPR.HI);
    }

    // Move From LO
    void Interpreter::MFLO(Instruction code) {
      Load(code.Rd(), R3000ARegs().GPR.LO);
    }

    // Move To HI
    void Interpreter::MTHI(Instruction code) {
      u32 rd = code.RdVal();
      // CommitDelayedLoad();
      R3000ARegs().GPR.HI = rd;
    }

    // Move To LO
    void Interpreter::MTLO(Instruction code) {
      u32 rd = code.RdVal();
      // CommitDelayedLoad();
      R3000ARegs().GPR.LO = rd;
    }


    ////////////////////////////////////////
    // Jump and Branch Instructions
    ////////////////////////////////////////

    // Jump
    void Interpreter::J(Instruction code) {
      doBranch((code.Target() << 2) | (R3000ARegs().PC & 0xf0000000));
    }

    // Jump And Link
    void Interpreter::JAL(Instruction code) {
      R3000ARegs().GPR.RA = R3000ARegs().PC + 4;
      doBranch((code.Target() << 2) | (R3000ARegs().PC & 0xf0000000));
    }

    // Jump Register
    void Interpreter::JR(Instruction code) {
      doBranch(code.RsVal());
    }

    // Jump And Link Register
    void Interpreter::JALR(Instruction code) {
      u32 rd = code.Rd();
      if (rd != 0) {
        R3000ARegs().GPR.R[rd] = R3000ARegs().PC + 4;
      }
      doBranch(code.RsVal());
    }


    // Branch on EQual
    void Interpreter::BEQ(Instruction code) {
      if (code.RsVal() == code.RtVal()) {
        doBranch(R3000ARegs().PC + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Not Equal
    void Interpreter::BNE(Instruction code) {
      if (code.RsVal() != code.RtVal()) {
        doBranch(R3000ARegs().PC + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Less than or Equal Zero
    void Interpreter::BLEZ(Instruction code) {
      if (static_cast<int32_t>(code.RsVal()) <= 0) {
        doBranch(R3000ARegs().PC + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Greate Than Zero
    void Interpreter::BGTZ(Instruction code) {
      if (static_cast<int32_t>(code.RsVal()) > 0) {
        doBranch(R3000ARegs().PC + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Less Than Zero
    void Interpreter::BLTZ(Instruction code) {
      if (static_cast<int32_t>(code.RsVal()) < 0) {
        doBranch(R3000ARegs().PC + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Greater than or Equal Zero
    void Interpreter::BGEZ(Instruction code) {
      if (static_cast<int32_t>(code.RsVal()) >= 0) {
        doBranch(R3000ARegs().PC + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Less Than Zero And Link
    void Interpreter::BLTZAL(Instruction code) {
      if (static_cast<int32_t>(code.RsVal()) < 0) {
        u32 pc = R3000ARegs().PC;
        R3000ARegs().GPR.RA = pc + 4;
        doBranch(pc + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch on Greater than or Equal Zero And Link
    void Interpreter::BGEZAL(Instruction code) {
      if (static_cast<int32_t>(code.RsVal()) >= 0) {
        u32 pc = R3000ARegs().PC;
        R3000ARegs().GPR.RA = pc + 4;
        doBranch(pc + (static_cast<int32_t>(code.Imm()) << 2));
      }
    }

    // Branch Condition
    void Interpreter::BCOND(Instruction code) {
      (this->*BCONDS[code.Rt()])(code);
    }



    ////////////////////////////////////////
    // Special Instructions
    ////////////////////////////////////////

    void Interpreter::SYSCALL(Instruction) {
      R3000ARegs().PC -= 4;
      R3000a().Exception(Instruction(&cpu_, 0x20), R3000a().IsInDelaySlot());
    }
    void Interpreter::BREAK(Instruction) {}

    void Interpreter::SPCL(Instruction code) {
      (this->*SPECIALS[code.Funct()])(code);
    }


    ////////////////////////////////////////
    // Co-processor Instructions
    ////////////////////////////////////////

    void Interpreter::LWC0(Instruction) {

    }
    void Interpreter::LWC1(Instruction) {
      wxMessageOutputDebug().Printf(wxT("WARNING: LWC1 is not supported."));
    }
    void Interpreter::LWC2(Instruction) {}
    void Interpreter::LWC3(Instruction) {
      wxMessageOutputDebug().Printf(wxT("WARNING: LWC3 is not supported."));
    }

    void Interpreter::SWC0(Instruction) {}
    void Interpreter::SWC1(Instruction) {
      wxMessageOutputDebug().Printf(wxT("WARNING: SWC1 is not supported."));
    }

    void Interpreter::SWC2(Instruction) {}
    void Interpreter::SWC3(Instruction) {
      // HLE?
      wxMessageOutputDebug().Printf(wxT("WARNING: SWC3 is not supported."));
    }



    void Interpreter::COP0(Instruction) {}
    void Interpreter::COP1(Instruction code) {
      wxMessageOutputDebug().Printf(wxT("WARNING: COP1 is not supported (PC = 0x%08x)."), (int)code);
    }
    void Interpreter::COP2(Instruction) {
      // Cop2 is not supported.
    }
    void Interpreter::COP3(Instruction code) {
      wxMessageOutputDebug().Printf(wxT("WARNING: COP3 is not supported (PC = 0x%08x)."), (int)code);
    }


    ////////////////////////////////////////
    // HLE functions
    ////////////////////////////////////////

    void Interpreter::hleDummy()
    {
      R3000ARegs().PC = R3000ARegs().GPR.RA;
      R3000a().BranchTest();
    }

    void Interpreter::hleA0()
    {
      (Bios().*BIOS::biosA0[R3000ARegs().GPR.T1 & 0xff])();
      R3000a().BranchTest();
    }

    void Interpreter::hleB0()
    {
      (Bios().*BIOS::biosB0[R3000ARegs().GPR.T1 & 0xff])();
      R3000a().BranchTest();
    }

    void Interpreter::hleC0()
    {
      (Bios().*BIOS::biosC0[R3000ARegs().GPR.T1 & 0xff])();
      R3000a().BranchTest();
    }

    void Interpreter::hleBootstrap() {}

    struct EXEC {
      u32 pc0;
      u32 gp0;
      u32 t_addr;
      u32 t_size;
      u32 d_addr;
      u32 d_size;
      u32 b_addr;
      u32 b_size;
      u32 S_addr;
      u32 s_size;
      u32 sp, fp, gp, ret, base;
    };

    void Interpreter::hleExecRet()
    {
      EXEC *header = static_cast<EXEC*>(M_ptr(R3000ARegs().GPR.S0));
      R3000ARegs().GPR.RA = BFLIP32(header->ret);
      R3000ARegs().GPR.SP = BFLIP32(header->sp);
      R3000ARegs().GPR.FP = BFLIP32(header->fp);
      R3000ARegs().GPR.GP = BFLIP32(header->gp);
      R3000ARegs().GPR.S0 = BFLIP32(header->base);
      R3000ARegs().GPR.V0 = 1;
      R3000ARegs().PC = R3000ARegs().GPR.RA;
    }

    void (Interpreter::*const Interpreter::HLEt[])() = {
                                                       &Interpreter::hleDummy, &Interpreter::hleA0, &Interpreter::hleB0,
    &Interpreter::hleC0, &Interpreter::hleBootstrap, &Interpreter::hleExecRet
  };


  void Interpreter::HLECALL(Instruction code) {
    /*
    if ((u32)((code & 0xff) -1) < 3) {
        wxMessageOutputDebug().Printf("HLECALL %c0:%02x", 'A' + (code & 0xff)-1, R3000ARegs().GPR.T1 & 0xff);
    }
*/
    (this->*HLEt[code & 0xff])();
  }



  ////////////////////////////////////////////////////////////////
  // Thread
  ////////////////////////////////////////////////////////////////

  InterpreterThread::InterpreterThread(Interpreter *interp)
    : wxThread(wxTHREAD_JOINABLE),
      interp_(*interp), isRunning_(false)
  {}


  wxThread::ExitCode InterpreterThread::Entry()
  {
    wxMessageOutputDebug().Printf(wxT("PSX Threads is started."));

    isRunning_ = true;
    do {
      if (interp_.RCnt().SPURun() == 0) break;
      //#warning PSX::Interpreter::Thread don't call SPUendflush
      interp_.ExecuteOnce();
    } while (isRunning_);
    return 0;
  }


  void InterpreterThread::Shutdown()
  {
    if (isRunning_ == false) return;
    isRunning_ = false;
    wxThread::Wait();
  }


  void InterpreterThread::OnExit()
  {
    interp_.Spu().Close();
    wxMessageOutputDebug().Printf(wxT("PSX Thread is ended."));
  }



  ////////////////////////////////////////////////////////////////
  // Main Loop
  ////////////////////////////////////////////////////////////////

  InterpreterThread *Interpreter::thread = 0;

  InterpreterThread *Interpreter::Execute()
  {
    if (thread != 0) {
      Shutdown();
    }
    thread = new InterpreterThread(this);
    thread->Create();
    thread->Run();
    return thread;
  }

  void Interpreter::Shutdown()
  {
    if (thread == 0) return;
    if (thread->IsRunning() == false) return;
    thread->Shutdown();
    delete thread;  // WARN
    thread = 0;
  }


  void (Interpreter::*const Interpreter::OPCODES[64])(Instruction) = {
                                                                     &Interpreter::SPCL, &Interpreter::BCOND, &Interpreter::J,    &Interpreter::JAL,
  &Interpreter::BEQ,  &Interpreter::BNE,   &Interpreter::BLEZ, &Interpreter::BGTZ,
  &Interpreter::ADDI, &Interpreter::ADDIU, &Interpreter::SLTI, &Interpreter::SLTIU,
  &Interpreter::ANDI, &Interpreter::ORI,   &Interpreter::XORI, &Interpreter::LUI,
  &Interpreter::COP0, &Interpreter::COP1,  &Interpreter::COP2, &Interpreter::COP3,
  &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
  &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
  &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
  &Interpreter::LB,   &Interpreter::LH,    &Interpreter::LWL,  &Interpreter::LW,
  &Interpreter::LBU,  &Interpreter::LHU,   &Interpreter::LWR,  &Interpreter::NLOP,
  &Interpreter::SB,   &Interpreter::SH,    &Interpreter::SWL,  &Interpreter::SW,
  &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::SWR,  &Interpreter::NLOP,
  &Interpreter::LWC0, &Interpreter::LWC1,  &Interpreter::LWC2, &Interpreter::LWC3,
  &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
  &Interpreter::SWC0, &Interpreter::SWC1,  &Interpreter::SWC2, &Interpreter::HLECALL,
  &Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP
};


void (Interpreter::*const Interpreter::SPECIALS[64])(Instruction) = {
                                                                    &Interpreter::SLL,  &Interpreter::NLOP,  &Interpreter::SRL,  &Interpreter::SRA,
&Interpreter::SLLV, &Interpreter::NLOP,  &Interpreter::SRLV, &Interpreter::SRAV,
&Interpreter::JR,   &Interpreter::JALR,  &Interpreter::NLOP, &Interpreter::HLECALL,
&Interpreter::SYSCALL, &Interpreter::BREAK, &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::MFHI, &Interpreter::MTHI,  &Interpreter::MFLO, &Interpreter::MTLO,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::MULT, &Interpreter::MULTU, &Interpreter::DIV,  &Interpreter::DIVU,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::ADD,  &Interpreter::ADDU,  &Interpreter::SUB,  &Interpreter::SUBU,
&Interpreter::AND,  &Interpreter::OR,    &Interpreter::XOR,  &Interpreter::NOR,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::SLT,  &Interpreter::SLTU,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP, &Interpreter::NLOP,  &Interpreter::NLOP, &Interpreter::NLOP
};


void (Interpreter::*const Interpreter::BCONDS[24])(Instruction) = {
                                                                  &Interpreter::BLTZ,   &Interpreter::BGEZ,   &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::BLTZAL, &Interpreter::BGEZAL, &Interpreter::NLOP, &Interpreter::NLOP,
&Interpreter::NLOP,   &Interpreter::NLOP,   &Interpreter::NLOP, &Interpreter::NLOP
};


void Interpreter::Init()
{
}


void Interpreter::ExecuteOnce()
{
  u32 pc = R3000ARegs().PC;
  Instruction code(&cpu_, U32M_ref(pc));
  pc += 4;
  ++R3000ARegs().Cycle;
  R3000ARegs().PC = pc;

  ExecuteOpcode(code);
/*
  if (R3000ARegs().Cycle % 10 == 0 &&
      R3000ARegs().Cycle >= 366000 &&
      R3000ARegs().Cycle < 367000) {
    R3000A::Disassembler disasm(&Psx());
    disasm.Parse(code);
    disasm.PrintCode();
    disasm.PrintChangedRegisters();
  }
  */
}


void Interpreter::ExecuteBlock() {
  R3000a().doingBranch = false;
  do {
    ExecuteOnce();
  } while (!R3000a().doingBranch);
}

/*
Interpreter& Interpreter::GetInstance()
{
    static Interpreter instance;
    return instance;
}
*/
}   // namespace R3000A

// R3000A::Interpreter& Interpreter_ = R3000A::Interpreter::GetInstance();

}   // namespace PSX
