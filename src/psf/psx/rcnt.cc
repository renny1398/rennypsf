
#include "psf/psx/rcnt.h"
#include "psf/psx/r3000a.h"
#include "psf/psx/memory.h"
#include "psf/spu/spu.h"
#include "common/debug.h"
#include <cstring>


namespace {
const int BIAS = 2;
const int PSXCLK = 33868800;	/* 33.8688 Mhz */

const int counter_num = 4;
uint32_t last = 0;

} // namespace


namespace PSX {


void RootCounterManager::UpdateCycle(u32 index)
{
  counters[index].count_start_clk = R3000ARegs().sysclock;

  if ( ((counters[index].mode_ & RootCounter::kCounterStopped) == 0 || (index != kCounterSystemClock)) &&
       counters[index].mode_ & (RootCounter::kIrqOnTarget | RootCounter::kIrqOnOverflow) ) {
    if (counters[index].mode_ & RootCounter::kIrqOnTarget) {
      counters[index].rest_of_count_clk = (counters[index].target_ - counters[index].count_) * counters[index].rate / BIAS;
    } else {
      counters[index].rest_of_count_clk = (0xffff - counters[index].count_) * counters[index].rate / BIAS;
    }
  } else {
    counters[index].rest_of_count_clk = 0xffffffff;
  }

}

void RootCounterManager::Reset(unsigned int index)
{
  rennyAssert(index < 4);

  counters[index].count_ = 0;
  UpdateCycle(index);

  U32H_ref(0x1070) |= BFLIP32(counters[index].interrupt);
  if ( (counters[index].mode_ & RootCounter::kIrqRegenerate) == 0 ) {
    counters[index].rest_of_count_clk = 0xffffffff;
  }
}

void RootCounterManager::SetNextCounter()
{
  clks_to_update_min_ = 0x7fffffff;
  nextiCounter = -1;
  uint32_t sysclock = R3000ARegs().sysclock;
  nextsCounter = sysclock;

  for (int i = 0; i < counter_num; i++) {
    if (counters[i].rest_of_count_clk == 0xffffffff) continue;
    const int clks_to_update = counters[i].rest_of_count_clk - (sysclock - counters[i].count_start_clk);
    if (clks_to_update < 0) {
      clks_to_update_min_ = 0;
      nextiCounter = i;
      break;
    }
    if (clks_to_update < static_cast<int>(clks_to_update_min_)) {
      clks_to_update_min_ = clks_to_update;
      nextiCounter = i;
    }
  }
}


void RootCounterManager::UpdateVSyncRate()
{
  counters[3].rate = (PSXCLK / 60);   // 60 Hz
}


void RootCounterManager::Init()
{
  memset(counters, 0, sizeof(counters));

  // pixelclock
  counters[0].rate = 1; counters[0].interrupt = 0x10;
  // horizontal retrace
  counters[1].rate = 1; counters[1].interrupt = 0x20;
  // 1/8 system clock
  counters[2].rate = 1; counters[2].interrupt = 0x40;
  // vertical retrace
  counters[3].interrupt = 1;
  counters[3].mode_ = RootCounter::kTargetReached | RootCounter::kCountToTarget;
  counters[3].target_ = 1;
  UpdateVSyncRate();

  // cnt = 4; // if SPU_async == NULL

  UpdateCycle(0); UpdateCycle(1); UpdateCycle(2); UpdateCycle(3);
  SetNextCounter();
  last = 0;

  rennyLogDebug("PSXRootCounter", "Initialized PSX root counter.");
}


void RootCounterManager::Update()
{
  const uint32_t sysclk = R3000ARegs().sysclock;

  if (sysclk - nextsCounter < clks_to_update_min_) return;

  if ( (sysclk - counters[3].count_start_clk) >= counters[3].rest_of_count_clk ) {
    UpdateCycle(3);
    U32H_ref(0x1070) |= BFLIP32(1);
  }
  if ( (sysclk - counters[0].count_start_clk) >= counters[0].rest_of_count_clk ) {
    Reset(0);
  }
  if ( (sysclk - counters[1].count_start_clk) >= counters[1].rest_of_count_clk ) {
    Reset(1);
  }
  if ( (sysclk - counters[2].count_start_clk) >= counters[2].rest_of_count_clk ) {
    Reset(2);
  }

  SetNextCounter();
}


void RootCounterManager::WriteCount(unsigned int index, unsigned int value)
{
  counters[index].count_ = value;
  UpdateCycle(index);
  SetNextCounter();
}


void RootCounterManager::WriteMode(unsigned int index, unsigned int value)
{
  counters[index].mode_ = value;
  counters[index].count_ = 0;

  if (index == 0) {   // pixel clock
    switch (value & 0x300) {
    case 0x100: // pixel clock
      counters[index].rate = (counters[3].rate / 386) / 262; // seems ok
      break;
    default:
      counters[index].rate = 1;
    }
  }
  else if (index == 1) {  // horizontal clock
    switch (value & 0x300) {
    case 0x100: // horizontal clock
      counters[index].rate = (counters[3].rate) / 262; // seems ok
      break;
    default:
      counters[index].rate = 1;
    }
  }
  else if (index == 2) {  // 1/8 system clock
    switch (value & 0x300) {
    case 0x200: // 1/8 * system clock
      counters[index].rate = 8; // 1/8 speed
      break;
    default:
      counters[index].rate = 1; // normal speed
    }
  }

  UpdateCycle(index);
  SetNextCounter();
}


void RootCounterManager::WriteTarget(unsigned int index, unsigned int value)
{
  counters[index].target_ = value;
  UpdateCycle(index);
  SetNextCounter();
}


unsigned int RootCounterManager::ReadCount(unsigned int index)
{
  unsigned int ret;

  Update();
  // if (counters[index].mode_ & RootCounter::kCountToTarget) {
    ret = counters[index].count_ + BIAS*((R3000ARegs().sysclock - counters[index].count_start_clk) / counters[index].rate);
  // } else {
  //   ret = counters[index].count_ + BIAS*(R3000ARegs().sysclock / counters[index].rate);
  // }

  return ret & 0xffff;
}


unsigned int RootCounterManager::SPURun()
{
  // for debug
/*
  int sec = R3000ARegs().Cycle / (PSXCLK / 2);
  int last_sec = last / (PSXCLK / 2);
  if (last_sec < sec) {
    wxMessageOutputDebug().Printf(wxT("%d second processed..."), sec);
  }
*/
  uint32_t cycles;
  if (R3000ARegs().sysclock < last) {
    cycles = 0xffffffff - last;
    cycles += R3000ARegs().sysclock + 1;
  } else {
    cycles = R3000ARegs().sysclock - last;
  }

  const uint32_t clk_p_hz = PSXCLK / Spu().GetCurrentSamplingRate() / 2;
  if (cycles >= clk_p_hz) {
    uint32_t step_count = cycles / clk_p_hz;
    uint32_t pool = cycles % clk_p_hz;
    bool ret = Spu().Step(step_count);
    last = R3000ARegs().sysclock - pool;
    if (ret == false) {
      // wxMessageOutputDebug().Printf(wxT("RootCounter: counter = %d"), R3000ARegs().Cycle);
      return 0;
    }
  }

  return 1;
}

void RootCounterManager::DeadLoopSkip()
{
  // wxMessageOutputDebug().Printf(wxT("CounterDeadLoopSkip"));

  int32_t min, lmin;
  uint32_t cycle = R3000ARegs().sysclock;

  lmin = 0x7fffffff;

  for (int i = 0; i < 4; i++) {
    if (counters[i].rest_of_count_clk != 0xffffffff) {
      min = counters[i].rest_of_count_clk;
      min -= cycle - counters[i].count_start_clk;
      // rennyAssert(min >= 0);
      if (min < lmin) {
        lmin = min;
      }
    }
  }

  if (lmin > 0) {
    R3000ARegs().sysclock = cycle + lmin;
  }
}


////////////////////////////////////////////////////////////////////////
// NEW Root Counter Functions
////////////////////////////////////////////////////////////////////////

unsigned int RootCounterManager::ReadCountEx(unsigned int index) const {
  return counters[index].count_;
}

unsigned int RootCounterManager::ReadModeEx(unsigned int index) const {
  return counters[index].mode_;
}

unsigned int RootCounterManager::ReadTargetEx(unsigned int index) const {
  return counters[index].target_;
}


void RootCounterManager::WriteCountEx(unsigned int index, unsigned int value) {
  counters[index].count_ = value;
}

void RootCounterManager::WriteModeEx(unsigned int index, unsigned int value) {
  counters[index].mode_ = value;
}

void RootCounterManager::WriteTargetEx(unsigned int index, unsigned int value) {
  counters[index].target_ = value;
}








}   // namespace PSX
