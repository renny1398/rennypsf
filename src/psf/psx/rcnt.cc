#include "psf/psx/rcnt.h"
#include "psf/psx/r3000a.h"
#include "psf/psx/memory.h"
#include "psf/spu/spu.h"

#include "common/SoundFormat.h"
#include "common/debug.h"
#include <cstring>


namespace {
const int BIAS = 1; // 2;
} // namespace

namespace psx {

const int PSXCLK = 33868800;	/* 33.8688 Mhz */

RootCounter::RootCounter()
  : mode_(0), target_(0),
    cycle_start_(0), cycle_(0), rate_(1),
    counts_to_target_(false), irq_(false) {}

void RootCounter::UpdateCycle(const unsigned int cycle32) {
  if (count(cycle32) < target_) {
    cycle_ = target_ * rate_ / BIAS;
    counts_to_target_ = true;
  } else {
    cycle_ = 0xffff * rate_ / BIAS;
    counts_to_target_ = false;
  }
}

void RootCounter::UpdateCycle(unsigned int count, const unsigned int cycle32) {
  count &= 0xffff;
  cycle_start_ = cycle32 - (count * rate_ / BIAS);
  if (count < target_) {
    cycle_ = target_ * rate_ / BIAS;
    counts_to_target_ = true;
  } else {
    cycle_ = 0xffff * rate_ / BIAS;
    counts_to_target_ = false;
  }
}

unsigned int RootCounter::count(const unsigned int cycle32) const {
  return ((cycle32 - cycle_start_) * BIAS / rate_) & 0xffff;
}

unsigned int RootCounter::mode() const {
  return mode_;
}

unsigned int RootCounter::target() const {
  return target_;
}

bool RootCounter::irq() const {
  return irq_;
}

void RootCounter::reset_irq() {
  irq_ = false;
}

void RootCounter::Update(const unsigned int cycle32) {
  if (cycle32 - cycle_start_ >= cycle_) {
    // Reset
    unsigned int count;
    if (counts_to_target_) {
      if (mode_ & kCountToTarget) {
        count = (cycle32 - cycle_start_) * BIAS / rate_ - target_;
        UpdateCycle(count, cycle32);
      } else {
        RootCounter::UpdateCycle(cycle32);
      }
      if ((mode_ & kIrqOnTarget) &&
          ((mode_ & kIrqRegenerate) || (mode_ & kCountEqTarget) == 0)) {
        irq_ = true;
      }
      mode_ |= kCountEqTarget;
    } else {
      count = (cycle32 - cycle_start_) * BIAS / rate_ - 0xffff;
      UpdateCycle(count, cycle32);
      if ((mode_ & kIrqOnOverflow) &&
          ((mode_ & kIrqRegenerate) || (mode_ & kOverflow) == 0)) {
        irq_ = true;
      }
      mode_ |= kOverflow;
    }
    mode_ |= kIrqRequest;
  }
}

unsigned int RootCounter::ReadCount(unsigned int cycle32) const {
  const_cast<RootCounter*>(this)->Update(cycle32);
  return count(cycle32);
}

unsigned int RootCounter::ReadMode(unsigned int cycle32) const {
  const_cast<RootCounter*>(this)->Update(cycle32);
  return mode();
}

unsigned int RootCounter::ReadTarget() const {
  return target();
}

void RootCounter::WriteCount(unsigned int count, unsigned int cycle32) {
  Update(cycle32);
  UpdateCycle(count, cycle32);
}

void RootCounter::WriteMode(unsigned int mode, unsigned int cycle32) {
  Update(cycle32);
  mode_ = mode;
  reset_irq();
}

void RootCounter::WriteTarget(unsigned int target, unsigned int cycle32) {
  Update(cycle32);
  target_ = target;
  UpdateCycle(count(cycle32), cycle32);
}

RootCounterManager::RootCounterManager(PSX* composite)
  : Component(composite), IRQAccessor(composite),
    interrupt_{ 0x10, 0x20, 0x40, 0x01 }, cycle_(0),
    last_spusync_cycle_(0) {}

unsigned int RootCounterManager::cycle32() const {
  return cycle_;
}

void RootCounterManager::IncreaseCycle() {
  ++cycle_;
  SPURun();
}

void RootCounterManager::UpdateVSyncRate() {
  counters[3].rate_ = (PSXCLK / 60);   // 60 Hz
}

void RootCounterManager::Init()
{
  memset(counters, 0, sizeof(counters));

  cycle_ = 0;

  // pixelclock
  counters[0].rate_ = 1;
  // horizontal retrace
  counters[1].rate_ = 1;
  // 1/8 system clock
  counters[2].rate_ = 1;
  // vertical retrace
  counters[3].mode_ = RootCounter::kTargetReached | RootCounter::kCountToTarget;
  counters[3].target_ = 1;
  UpdateVSyncRate();

  for (auto i = 0; i < 4; ++i) {
    counters[i].UpdateCycle(0, cycle_);
  }
  last_spusync_cycle_ = 0;
  rennyLogDebug("PSXRootCounter", "Initialized PSX root counter.");
}

unsigned int RootCounterManager::interrupt(unsigned int index) const {
  return interrupt_[index];
}

void RootCounterManager::Update(unsigned int index) {
  rennyAssert(index < 4);
  counters[index].Update(cycle_);
  if (counters[index].irq()) {
    set_irq_data(interrupt_[index]);
    counters[index].reset_irq();
  }
}

void RootCounterManager::Update() {
  for (unsigned int i = 0; i < 4; ++i) {
    Update(i);
  }
}

unsigned int RootCounterManager::ReadCountEx(unsigned int index) const {
  // return counters[index].count_;
  return counters[index].ReadCount(cycle_);
}

unsigned int RootCounterManager::ReadModeEx(unsigned int index) const {
  return counters[index].ReadMode(cycle_);
}

unsigned int RootCounterManager::ReadTargetEx(unsigned int index) const {
  return counters[index].ReadTarget();
}


void RootCounterManager::WriteCountEx(unsigned int index, unsigned int value) {
  counters[index].WriteCount(value, cycle_);
}

void RootCounterManager::WriteModeEx(unsigned int index, unsigned int value) {
  counters[index].WriteMode(value, cycle_);
  if (index == 0) {   // pixel clock
    switch (value & 0x300) {
    case 0x100: // pixel clock
      counters[index].rate_ = (counters[3].rate_ / 386) / 262; // seems ok
      break;
    default:
      counters[index].rate_ = 1;
    }
  }
  else if (index == 1) {  // horizontal clock
    switch (value & 0x300) {
    case 0x100: // horizontal clock
      counters[index].rate_ = (counters[3].rate_) / 262; // seems ok
      break;
    default:
      counters[index].rate_ = 1;
    }
  }
  else if (index == 2) {  // 1/8 system clock
    switch (value & 0x300) {
    case 0x200: // 1/8 * system clock
      counters[index].rate_ = 8; // 1/8 speed
      break;
    default:
      counters[index].rate_ = 1; // normal speed
    }
  }
  counters[index].UpdateCycle(0, cycle_);
}

void RootCounterManager::WriteTargetEx(unsigned int index, unsigned int value) {
  counters[index].WriteTarget(value, cycle_);
}

int RootCounterManager::SPURun() {
  uint32_t cycles = cycle_ - last_spusync_cycle_;
  const uint32_t clk_p_hz = PSXCLK / Spu().GetCurrentSamplingRate();
  if (cycles >= clk_p_hz) {
    uint32_t step_count = cycles / clk_p_hz;
    uint32_t pool = cycles % clk_p_hz;
    bool ret = false;
    if (last_spusync_cycle_ == 0) {
      ret = Spu().Advance(step_count);
    } else {
      for (uint32_t i = 0; i < step_count; ++i) {
        ret = Spu().GetAsync(nullptr);
        if (ret == false) break;
      }
    }
    last_spusync_cycle_ = cycle_ - pool;
    if (ret == false) {
      // wxMessageOutputDebug().Printf(wxT("RootCounter: counter = %d"), R3000ARegs().Cycle);
      return -1;
    }
    return 1;
  }
  return 0;
}

void RootCounterManager::DeadLoopSkip()
{
  // wxMessageOutputDebug().Printf(wxT("CounterDeadLoopSkip"));

  int32_t min, lmin;
  uint32_t cycle = cycle_;

  lmin = 0x7fffffff;

  for (int i = 0; i < 4; i++) {
    if (counters[i].cycle_ != 0xffffffff) {
      min = counters[i].cycle_;
      min -= cycle - counters[i].cycle_start_;
      // rennyAssert(min >= 0);
      if (min < lmin) {
        lmin = min;
      }
    }
  }

  if (lmin > 0) {
    cycle_ = cycle + lmin;
  }
}

}   // namespace psx
