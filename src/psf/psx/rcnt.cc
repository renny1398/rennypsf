#include "psf/psx/rcnt.h"
#include "psf/psx/r3000a.h"
#include "psf/psx/memory.h"
#include "psf/spu/spu.h"
#include <cstring>
#include <wx/msgout.h>


namespace {
const int BIAS = 2;
const int PSXCLK = 33868800;	/* 33.8688 Mhz */

const int counter_num = 4;
uint32_t last = 0;

} // namespace


namespace PSX {


void RootCounter::UpdateEx(u32 index)
{
  counters[index].sCycle = R3000ARegs().Cycle;
  if ( (!(counters[index].mode & 1) || (index != 2)) && counters[index].mode & 0x30 ) {
    if (counters[index].mode & 0x10) {  // interrupt on target
      counters[index].Cycle = (counters[index].target - counters[index].count) * counters[index].rate / BIAS;
    } else {    // interrupt on 0xffff
      counters[index].Cycle = (0xffff - counters[index].count) * counters[index].rate / BIAS;
    }
  } else {
    counters[index].Cycle = 0xffffffff;
  }

}

void RootCounter::Reset(unsigned int index)
{
  wxASSERT(index < 4);

  counters[index].count = 0;
  UpdateEx(index);

  U32H_ref(0x1070) |= BFLIP32(counters[index].interrupt);
  if ( (counters[index].mode & 0x40) == 0 ) { // only 1 interrupt
    counters[index].Cycle = 0xffffffff;
  }
}

void RootCounter::Set()
{
  nextCounter = 0x7fffffff;
  nextsCounter = R3000ARegs().Cycle;

  for (int i = 0; i < counter_num; i++) {
    int count;
    if (counters[i].Cycle == 0xffffffff) continue;

    count = counters[i].Cycle - (R3000ARegs().Cycle - counters[i].sCycle);
    if (count < 0) {
      nextCounter = 0;
      break;
    }
    if (count < static_cast<int>(nextCounter)) {
      nextCounter = count;
    }
  }
}


void RootCounter::UpdateVSyncRate()
{
  counters[3].rate = (PSXCLK / 60);   // 60 Hz
}


void RootCounter::Init()
{
  memset(counters, 0, sizeof(counters));

  counters[0].rate = 1; counters[0].interrupt = 0x10;
  counters[1].rate = 1; counters[1].interrupt = 0x20;
  counters[2].rate = 1; counters[2].interrupt = 0x40;

  counters[3].interrupt = 1;
  counters[3].mode = 0x58;    // the VSync counter mode
  counters[3].target = 1;
  UpdateVSyncRate();

  // cnt = 4; // if SPU_async == NULL

  UpdateEx(0); UpdateEx(1); UpdateEx(2); UpdateEx(3);
  Set();
  last = 0;

  wxMessageOutputDebug().Printf(wxT("Initialized PSX root counter."));
}


void RootCounter::Update()
{
  uint32_t cycle = R3000ARegs().Cycle;
  if ( (cycle - counters[3].sCycle) >= counters[3].Cycle ) {
    UpdateEx(3);
    U32H_ref(0x1070) |= BFLIP32(1);
  }
  if ( (cycle - counters[0].sCycle) >= counters[0].Cycle ) {
    Reset(0);
  }
  if ( (cycle - counters[1].sCycle) >= counters[1].Cycle ) {
    Reset(1);
  }
  if ( (cycle - counters[2].sCycle) >= counters[2].Cycle ) {
    Reset(2);
  }

  Set();
}


void RootCounter::WriteCount(unsigned int index, unsigned int value)
{
  counters[index].count = value;
  UpdateEx(index);
  Set();
}


void RootCounter::WriteMode(unsigned int index, unsigned int value)
{
  counters[index].mode = value;
  counters[index].count = 0;

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

  UpdateEx(index);
  Set();
}


void RootCounter::WriteTarget(unsigned int index, unsigned int value)
{
  counters[index].target = value;
  UpdateEx(index);
  Set();
}


unsigned int RootCounter::ReadCount(unsigned int index)
{
  unsigned int ret;

  if (counters[index].mode & 0x08) {  // count to value in target
    ret = counters[index].count + BIAS*((R3000ARegs().Cycle - counters[index].sCycle) / counters[index].rate);
  } else {    // count to 0xffff
    ret = counters[index].count + BIAS*(R3000ARegs().Cycle / counters[index].rate);
  }

  return ret & 0xffff;
}


unsigned int RootCounter::SPURun()
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
  if (R3000ARegs().Cycle < last) {
    cycles = 0xffffffff - last;
    cycles += R3000ARegs().Cycle + 1;
  } else {
    cycles = R3000ARegs().Cycle - last;
  }

  const uint32_t clk_p_hz = PSXCLK / 44100 / 2;
  if (cycles >= clk_p_hz) {
    uint32_t step_count = cycles / clk_p_hz;
    uint32_t pool = cycles % clk_p_hz;
    bool ret = Spu().Step(step_count);
    last = R3000ARegs().Cycle - pool;
    if (ret == false) {
      // wxMessageOutputDebug().Printf(wxT("RootCounter: counter = %d"), R3000ARegs().Cycle);
      return 0;
    }
  }

  return 1;
}

void RootCounter::DeadLoopSkip()
{
  // wxMessageOutputDebug().Printf(wxT("CounterDeadLoopSkip"));

  int32_t min, lmin;
  uint32_t cycle = R3000ARegs().Cycle;

  lmin = 0x7fffffff;

  for (int i = 0; i < 4; i++) {
    if (counters[i].Cycle != 0xffffffff) {
      min = counters[i].Cycle;
      min -= cycle - counters[i].sCycle;
      // wxASSERT(min >= 0);
      if (min < lmin) {
        lmin = min;
      }
    }
  }

  if (lmin > 0) {
    R3000ARegs().Cycle = cycle + lmin;
  }
}


}   // namespace PSX
