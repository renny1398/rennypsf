#include <cppunit/extensions/HelperMacros.h>
#include "psf/psx/psx.h"
#include "psf/psx/rcnt.h"


class RcntTest : public CPPUNIT_NS::TestFixture {

  CPPUNIT_TEST_SUITE(RcntTest);
  CPPUNIT_TEST(init_test);
  CPPUNIT_TEST(count_test);
  CPPUNIT_TEST(target_test);
  CPPUNIT_TEST_SUITE_END();

protected:
  PSX::Composite psx;
  PSX::RootCounterManager& rcnt;

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
    psx.R3000ARegs().sysclock = 12345;
    rcnt.WriteCount(0, 0);
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadCount(0));
    rcnt.WriteCount(0, 65535);
    CPPUNIT_ASSERT_EQUAL((uint32_t)65535, rcnt.ReadCount(0));
    rcnt.WriteCount(0, 65536);
    CPPUNIT_ASSERT_EQUAL((uint32_t)0, rcnt.ReadCount(0));
    rcnt.WriteCount(0, 65536 + 54321);
    CPPUNIT_ASSERT_EQUAL((uint32_t)54321, rcnt.ReadCount(0));

    psx.R3000ARegs().sysclock = 0;
    rcnt.WriteCount(0, 0);
    psx.R3000ARegs().sysclock = 12345;
    CPPUNIT_ASSERT_EQUAL((uint32_t)12345*2, rcnt.ReadCount(0));
  }

  void target_test() {
    psx.R3000ARegs().sysclock = 0;
    rcnt.WriteTarget(0, 12000);
    rcnt.WriteMode(0, PSX::RootCounter::kCountToTarget);
    rcnt.WriteCount(0, 0);
    psx.R3000ARegs().sysclock = 6000;
    CPPUNIT_ASSERT_EQUAL((uint32_t)12000, rcnt.ReadCount(0));
    psx.R3000ARegs().sysclock = 6001;
    CPPUNIT_ASSERT_EQUAL((uint32_t)12002, rcnt.ReadCount(0));
  }
};

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
