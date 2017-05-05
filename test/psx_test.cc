#include <cppunit/extensions/HelperMacros.h>
#include "psf/psx/psx.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/interpreter.h"

using namespace psx;
using namespace psx::mips;

////////////////////////////////////////////////////////////////////////
/// \brief The Interpreter Test class
////////////////////////////////////////////////////////////////////////

class InterpreterTest : public CPPUNIT_NS::TestFixture {

  CPPUNIT_TEST_SUITE(InterpreterTest);
  CPPUNIT_TEST(LB_test);
  CPPUNIT_TEST(LBU_test);
  CPPUNIT_TEST(LH_test);
  CPPUNIT_TEST(LHU_test);
  CPPUNIT_TEST(LW_test);
  CPPUNIT_TEST(LWL_test);
  CPPUNIT_TEST(LWR_test);
  CPPUNIT_TEST(SB_test);
  CPPUNIT_TEST(SH_test);
  CPPUNIT_TEST(SW_test);
  CPPUNIT_TEST(SWL_test);
  CPPUNIT_TEST(SWR_test);
  CPPUNIT_TEST(ADDI_test);
  CPPUNIT_TEST(ADDIU_test);
  CPPUNIT_TEST(ANDI_test);
  CPPUNIT_TEST(ORI_test);
  CPPUNIT_TEST(XORI_test);
  CPPUNIT_TEST(SLTI_test);
  CPPUNIT_TEST(SLTIU_test);
  CPPUNIT_TEST(ADD_test);
  CPPUNIT_TEST(ADDU_test);
  CPPUNIT_TEST(SUB_test);
  CPPUNIT_TEST(SUBU_test);
  CPPUNIT_TEST(AND_test);
  CPPUNIT_TEST(OR_test);
  CPPUNIT_TEST(XOR_test);
  CPPUNIT_TEST(NOR_test);
  CPPUNIT_TEST(SLT_test);
  CPPUNIT_TEST(SLTU_test);
  CPPUNIT_TEST_SUITE_END();

  static const psx::PSXAddr kBaseAddr = 0x80010000;

protected:
  psx::PSX psx;
  // PSX::R3000A::Interpreter interp;
  psx::UserMemoryAccessor psxM;
  GeneralPurposeRegisters& GPR;
  u32& PC;

  void set_pc_and_opcode(u32 opcode1, u32 opcode2 = 0) {
    PC = kBaseAddr;
    psxM.psxMu32ref(kBaseAddr + 0) = opcode1;
    psxM.psxMu32ref(kBaseAddr + 4) = opcode2;
  }

public:
  InterpreterTest()
    : psx(), psxM(&psx.Mem()),
      GPR(psx.R3000a().Regs.GPR),
      PC(GPR(psx::GPR_PC)) {}

  void setUp() {}
  void tearDown() {}

  ////////////////////////////////////////////////////////
  /// \brief Load Test
  ////////////////////////////////////////////////////////

  void LB_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0x12345678;
    psxM.psxMu8ref(GPR(GPR_A0)+100) = (u8)(0xf4);
    set_pc_and_opcode(EncodeI(OPCODE_LB, GPR_A0, GPR_A1, 100));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0xfffffff4));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void LBU_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0x12345678;
    psxM.psxMu8ref(GPR(GPR_A0)+100) = (u8)(0xf4);
    set_pc_and_opcode(EncodeI(OPCODE_LBU, GPR_A0, GPR_A1, 100));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x000000f4));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void LH_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0x12345678;
    psxM.psxMu16ref(GPR(GPR_A0)+100) = (u16)(0xf321);
    set_pc_and_opcode(EncodeI(OPCODE_LH, GPR_A0, GPR_A1, 100));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0xfffff321));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void LHU_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0x12345678;
    psxM.psxMu16ref(GPR(GPR_A0)+100) = (u16)(0xf321);
    set_pc_and_opcode(EncodeI(OPCODE_LHU, GPR_A0, GPR_A1, 100));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x0000f321));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void LW_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0x12345678;
    psxM.psxMu32ref(GPR(GPR_A0)+100) = 0xf7654321;
    set_pc_and_opcode(EncodeI(OPCODE_LW, GPR_A0, GPR_A1, 100));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0xf7654321));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void LWL_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWL, GPR_A0, GPR_A1, 0x80+0));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x44bbccdd));

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWL, GPR_A0, GPR_A1, 0x80+1));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x3344ccdd));

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWL, GPR_A0, GPR_A1, 0x80+2));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x223344dd));

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWL, GPR_A0, GPR_A1, 0x80+3));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x11223344));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void LWR_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWR, GPR_A0, GPR_A1, 0x80+0));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0x11223344));

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWR, GPR_A0, GPR_A1, 0x80+1));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0xaa112233));

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWR, GPR_A0, GPR_A1, 0x80+2));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0xaabb1122));

    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_LWR, GPR_A0, GPR_A1, 0x80+3));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(GPR(GPR_A1), (u32)(0xaabbcc11));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  ////////////////////////////////////////////////////////
  /// \brief Store Test
  ////////////////////////////////////////////////////////

  void SB_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0xaabbccdd;
    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SB, GPR_A0, GPR_A1, 0x80));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0x112233dd));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SH_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0xaabbccdd;
    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SH, GPR_A0, GPR_A1, 0x80));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0x1122ccdd));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SW_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0xaabbccdd;
    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SW, GPR_A0, GPR_A1, 0x80));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0xaabbccdd));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SWL_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0xaabbccdd;

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWL, GPR_A0, GPR_A1, 0x80+0));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0x112233aa));

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWL, GPR_A0, GPR_A1, 0x80+1));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0x1122aabb));

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWL, GPR_A0, GPR_A1, 0x80+2));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0x11aabbcc));

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWL, GPR_A0, GPR_A1, 0x80+3));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0xaabbccdd));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SWR_test() {
    GPR(GPR_A0) = kBaseAddr + 4;
    GPR(GPR_A1) = 0xaabbccdd;

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWR, GPR_A0, GPR_A1, 0x80+0));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0xaabbccdd));

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWR, GPR_A0, GPR_A1, 0x80+1));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0xbbccdd44));

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWR, GPR_A0, GPR_A1, 0x80+2));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0xccdd3344));

    psxM.psxMu32ref(GPR(GPR_A0)+0x80) = 0x11223344;
    set_pc_and_opcode(EncodeI(OPCODE_SWR, GPR_A0, GPR_A1, 0x80+3));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL(psxM.psxMu32ref(GPR(GPR_A0)+0x80), (u32)(0xdd223344));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  ////////////////////////////////////////////////////////
  /// \brief Immediate Arithmetic Test
  ////////////////////////////////////////////////////////

  void ADDI_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_ADDI, GPR_A0, GPR_A1, 0x7777));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x1122aabb), GPR(GPR_A1));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void ADDIU_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_ADDIU, GPR_A0, GPR_A1, 0x7777));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x1122aabb), GPR(GPR_A1));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void ANDI_test() {
    GPR(GPR_A0) = 0x99aabbcc;
    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_ANDI, GPR_A0, GPR_A1, 0x7777));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x00003344), GPR(GPR_A1));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void ORI_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_ORI, GPR_A0, GPR_A1, 0x8888));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x1122bbcc), GPR(GPR_A1));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void XORI_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0xaabbccdd;
    set_pc_and_opcode(EncodeI(OPCODE_XORI, GPR_A0, GPR_A1, 0xffff));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x1122ccbb), GPR(GPR_A1));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SLTI_test() {
    GPR(GPR_A0) = -12345;
    GPR(GPR_A1) = 0xaabbccdd;

    set_pc_and_opcode(EncodeI(OPCODE_SLTI, GPR_A0, GPR_A1, -12344));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(1), GPR(GPR_A1));

    set_pc_and_opcode(EncodeI(OPCODE_SLTI, GPR_A0, GPR_A1, -12345));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0), GPR(GPR_A1));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SLTIU_test() {
    GPR(GPR_A0) = 12345;
    GPR(GPR_A1) = 0xaabbccdd;

    set_pc_and_opcode(EncodeI(OPCODE_SLTIU, GPR_A0, GPR_A1, 12346));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(1), GPR(GPR_A1));

    set_pc_and_opcode(EncodeI(OPCODE_SLTIU, GPR_A0, GPR_A1, 12345));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0), GPR(GPR_A1));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  ////////////////////////////////////////////////////////
  /// \brief Register Arithmetic Test
  ////////////////////////////////////////////////////////

  void ADD_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0x88888888;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_ADD));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x99aabbcc), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void ADDU_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0x77777777;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_ADDU));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x8899aabb), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SUB_test() {
    GPR(GPR_A0) = 0x99aabbcc;
    GPR(GPR_A1) = 0x88888888;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_SUB));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x11223344), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SUBU_test() {
    GPR(GPR_A0) = 0x8899aabb;
    GPR(GPR_A1) = 0x77777777;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_SUBU));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x11223344), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void AND_test() {
    GPR(GPR_A0) = 0x99aabbcc;
    GPR(GPR_A1) = 0x77777777;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_AND));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x11223344), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void OR_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0x88888888;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_OR));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x99aabbcc), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void XOR_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0xffffffff;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_XOR));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0xeeddccbb), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void NOR_test() {
    GPR(GPR_A0) = 0x11223344;
    GPR(GPR_A1) = 0x88888888;
    GPR(GPR_V0) = 0xaabbccdd;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_NOR));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0x66554433), GPR(GPR_V0));
    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SLT_test() {
    GPR(GPR_A0) = -11223344;
    GPR(GPR_V0) = 0xaabbccdd;

    GPR(GPR_A1) = -11223343;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_SLT));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(1), GPR(GPR_V0));

    GPR(GPR_A1) = -11223344;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_SLT));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0), GPR(GPR_V0));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }

  void SLTU_test() {
    GPR(GPR_A0) = 11223344;
    GPR(GPR_V0) = 0xaabbccdd;

    GPR(GPR_A1) = 11223345;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_SLTU));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(1), GPR(GPR_V0));

    GPR(GPR_A1) = 11223344;
    set_pc_and_opcode(EncodeR(OPCODE_SPECIAL, GPR_A0, GPR_A1, GPR_V0, 0, SPECIAL_SLTU));
    psx.Interp().Execute(2);
    CPPUNIT_ASSERT_EQUAL((u32)(0), GPR(GPR_V0));

    CPPUNIT_ASSERT_EQUAL(PC, kBaseAddr + 8);
  }
};

////////////////////////////////////////////////////////////////////////
/// \brief The RootCounter Test class
////////////////////////////////////////////////////////////////////////

class RcntTest : public CPPUNIT_NS::TestFixture {

  CPPUNIT_TEST_SUITE(RcntTest);
  CPPUNIT_TEST(init_test);
  CPPUNIT_TEST(count_test);
  CPPUNIT_TEST(target_test);
  CPPUNIT_TEST(overflow_test);
  CPPUNIT_TEST_SUITE_END();

protected:
  psx::PSX psx;
  psx::RootCounterManager& rcnt;

public:
  RcntTest() : psx(), rcnt(psx.RCnt()) {}

  void setUp() {
    rcnt.Init();
  }

  void tearDown() {

  }

protected:
  void init_test() {

  }

  void count_test() {
    rcnt.cycle_ = 12345;
    rcnt.WriteCountEx(0, 0);
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadModeEx(0));
    rcnt.WriteCountEx(0, 0xffff - 1);
    CPPUNIT_ASSERT_EQUAL((uint32_t)0xffff - 1, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadModeEx(0));
    rcnt.WriteCountEx(0, 0xffff);
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL((uint32_t)RootCounter::kIrqRequest | (uint32_t)RootCounter::kOverflow, rcnt.ReadModeEx(0));
    rcnt.WriteModeEx(0, 0);
    rcnt.WriteCountEx(0, 0x10000 + 54321);
    CPPUNIT_ASSERT_EQUAL((uint32_t)54321, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadModeEx(0));

    rcnt.cycle_ = 0;
    rcnt.WriteCountEx(0, 0);
    rcnt.cycle_ = 12345;
    CPPUNIT_ASSERT_EQUAL((uint32_t)12345, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadModeEx(0));
    rcnt.cycle_ = 10000;
    rcnt.WriteCountEx(0, 0);
    rcnt.cycle_ = 12345;
    CPPUNIT_ASSERT_EQUAL((uint32_t)2345, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadModeEx(0));
  }

  void target_test() {
    rcnt.cycle_ = 0;
    rcnt.WriteTargetEx(0, 600);
    rcnt.WriteModeEx(0, psx::RootCounter::kCountToTarget);
    rcnt.WriteCountEx(0, 0);
    rcnt.cycle_ = 599;
    CPPUNIT_ASSERT_EQUAL((uint32_t)599, rcnt.ReadCountEx(0));

    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, 0);
    rcnt.cycle_ = 600;
    CPPUNIT_ASSERT_EQUAL((uint32_t)600, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kIrqRequest | (uint32_t)RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));
    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, 0);
    rcnt.cycle_ = 601;
    CPPUNIT_ASSERT_EQUAL((uint32_t)601, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kIrqRequest | (uint32_t)RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));

    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, psx::RootCounter::kCountToTarget);
    rcnt.cycle_ = 600;
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kCountToTarget | RootCounter::kIrqRequest | (uint32_t)RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));
    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, psx::RootCounter::kCountToTarget);
    rcnt.cycle_ = 601;
    CPPUNIT_ASSERT_EQUAL((uint32_t)1, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kCountToTarget | RootCounter::kIrqRequest | (uint32_t)RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));

    HardwareRegisterAccessor hw_regs(&psx.HwRegs());
    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, RootCounter::kCountToTarget | RootCounter::kIrqOnTarget);
    rcnt.cycle_ = 900;
    CPPUNIT_ASSERT_EQUAL((uint32_t)300, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kCountToTarget | RootCounter::kIrqOnTarget | RootCounter::kIrqRequest | RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)rcnt.interrupt(0), psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;
    rcnt.cycle_ = 1300;
    CPPUNIT_ASSERT_EQUAL((uint32_t)100, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kCountToTarget | RootCounter::kIrqOnTarget | RootCounter::kIrqRequest | RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;

    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, RootCounter::kCountToTarget | RootCounter::kIrqOnTarget | RootCounter::kIrqRegenerate);
    rcnt.cycle_ = 900;
    CPPUNIT_ASSERT_EQUAL((uint32_t)300, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kCountToTarget | RootCounter::kIrqOnTarget | RootCounter::kIrqRegenerate | RootCounter::kIrqRequest | RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)rcnt.interrupt(0), psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;
    rcnt.cycle_ = 1300;
    CPPUNIT_ASSERT_EQUAL((uint32_t)100, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kCountToTarget | RootCounter::kIrqOnTarget | RootCounter::kIrqRegenerate | RootCounter::kIrqRequest | RootCounter::kCountEqTarget, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)rcnt.interrupt(0), psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;
  }

  void overflow_test() {
    HardwareRegisterAccessor hw_regs(&psx.HwRegs());
    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, RootCounter::kIrqOnOverflow);
    rcnt.cycle_ = 0xffff + 2;
    CPPUNIT_ASSERT_EQUAL((uint32_t)2, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kIrqOnOverflow | RootCounter::kIrqRequest | RootCounter::kOverflow, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)rcnt.interrupt(0), psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;
    rcnt.cycle_ = 0xffff*2 + 3;
    CPPUNIT_ASSERT_EQUAL((uint32_t)3, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kIrqOnOverflow | RootCounter::kIrqRequest | RootCounter::kOverflow, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;

    rcnt.cycle_ = 0;
    rcnt.WriteModeEx(0, RootCounter::kIrqOnOverflow | RootCounter::kIrqRegenerate);
    rcnt.cycle_ = 0xffff + 2;
    CPPUNIT_ASSERT_EQUAL((uint32_t)2, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kIrqOnOverflow | RootCounter::kIrqRegenerate | RootCounter::kIrqRequest | RootCounter::kOverflow, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)rcnt.interrupt(0), psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;
    rcnt.cycle_ = 0xffff*2 + 3;
    CPPUNIT_ASSERT_EQUAL((uint32_t)3, rcnt.ReadCountEx(0));
    CPPUNIT_ASSERT_EQUAL(RootCounter::kIrqOnOverflow | RootCounter::kIrqRegenerate | RootCounter::kIrqRequest | RootCounter::kOverflow, rcnt.ReadModeEx(0));
    rcnt.Update();
    CPPUNIT_ASSERT_EQUAL((uint32_t)rcnt.interrupt(0), psx.HwRegs().irq_data());
    hw_regs.psxHu32ref(0x1070) = 0;
  }
};

CPPUNIT_TEST_SUITE_REGISTRATION(InterpreterTest);
CPPUNIT_TEST_SUITE_REGISTRATION(RcntTest);


#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>

int main(/*int argc, char* argv[]*/) {

  CPPUNIT_NS::TestResult controller;

  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );

  CPPUNIT_NS::BriefTestProgressListener progress;
  controller.addListener( &progress );

  CPPUNIT_NS::TestRunner runner;
  runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );

  CPPUNIT_NS::CompilerOutputter outputter( &result, CPPUNIT_NS::stdCOut() );
  outputter.write();

  return result.wasSuccessful() ? 0 : 1;
}
