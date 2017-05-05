#pragma once
#include "common.h"
#include "hardware.h"

class SoundBlock;
class RcntTest;

namespace psx {

extern const int PSXCLK;

struct RootCounter
{
  static const unsigned int kCounterStopped = 0x0001;
  static const unsigned int kCountToTarget  = 0x0008;
  static const unsigned int kIrqOnTarget    = 0x0010;
  static const unsigned int kIrqOnOverflow  = 0x0020;
  static const unsigned int kIrqRegenerate  = 0x0040;
  static const unsigned int kTargetReached  = 0x0050;
  static const unsigned int kDirectSysClock = 0x0100;
  static const unsigned int kSysClockDivBy8 = 0x0200;
  static const unsigned int kIrqRequest     = 0x0400;
  static const unsigned int kCountEqTarget  = 0x0800;
  static const unsigned int kOverflow       = 0x1000;

public:
  RootCounter();

  // update internal variables, and then get.
  unsigned int ReadCount(unsigned int cycle32) const;
  unsigned int ReadMode(unsigned int cycle32) const;
  unsigned int ReadTarget() const;

  void WriteCount(unsigned int count, unsigned int cycle32);
  void WriteMode(unsigned int mode, unsigned int cycle32);
  void WriteTarget(unsigned int target, unsigned int cycle32);

  // get without updating.
  unsigned int count(const unsigned int cycle32) const;
  unsigned int mode() const;
  unsigned int target() const;

  bool irq() const;
  void reset_irq();

protected:
  void UpdateCycle(const unsigned int cycle32);
  void UpdateCycle(unsigned int count, const unsigned int cycle32);
  void Update(const unsigned int cycle32);

private:
  unsigned int mode_, target_;
  unsigned int cycle_start_, cycle_;    // sCycle: start of cycle, Cycle: end_cycle - start_cycle
  unsigned int rate_;
  bool counts_to_target_; // false: count to overflow, true: count to target
  bool irq_;

  friend class RootCounterManager;
};

class RootCounterManager : public Component, private IRQAccessor {

 public:
  RootCounterManager(PSX* composite);

  void Init();
  // void Reset();

  // Cycle functions
  unsigned int cycle32() const;
  void IncreaseCycle();

  unsigned int interrupt(unsigned int index) const;

  void Update();
  void Update(unsigned int index);
  void UpdateVSyncRate();

  int SPURun();
  void DeadLoopSkip();

  unsigned int ReadCountEx(unsigned int index) const;
  unsigned int ReadModeEx(unsigned int index) const;
  unsigned int ReadTargetEx(unsigned int index) const;

  void WriteCountEx(unsigned int index, unsigned int value);
  void WriteModeEx(unsigned int index, unsigned int value);
  void WriteTargetEx(unsigned int index, unsigned int value);

 public:
  static const unsigned int kCounterPixel = 0;
  static const unsigned int kCounterHorRetrace = 1;
  static const unsigned int kCounterSystemClock = 2;
  static const unsigned int kCounterVblank = 3;

  /* counter 0: pixelclock
   * counter 1: horizontal retrace
   * counter 2: 1/8 system clock
   * counter 3: vertical retrace
   */

  RootCounter& operator()(int i) {
    return counters[i];
  }

 private:
  RootCounter counters[5];
  const unsigned int interrupt_[4];

  unsigned int cycle_;

  mutable unsigned int clks_to_update_min_, nextsCounter;
  mutable signed   int nextiCounter;
  unsigned int last_spusync_cycle_;

  friend class mips::Processor;
  friend class ::RcntTest;
};

}   // namespace PSX
