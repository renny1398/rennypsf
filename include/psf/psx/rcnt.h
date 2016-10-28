#pragma once
#include "common.h"

class SoundBlock;

namespace PSX {

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

  unsigned int count_, mode_, target_;

public:
  unsigned int count() const;
  unsigned int mode() const;
  unsigned int target() const;


protected:
  unsigned int count_start_clk, rest_of_count_clk;    // sCycle: start of cycle, Cycle: end_cycle - start_cycle
  unsigned int rate, interrupt;

  friend class RootCounterManager;
};

class RootCounterManager : public Component {

 public:
  RootCounterManager(Composite* composite)
    : Component(composite) {}

  void Init();
  void Update();
  void UpdateVSyncRate();
  void WriteCount(unsigned int index, unsigned int value);
  void WriteMode(unsigned int index, unsigned int value);
  void WriteTarget(unsigned int index, unsigned int value);
  unsigned int ReadCount(unsigned int index);

  int SPURun(SoundBlock*);
  void DeadLoopSkip();

 protected:
  void UpdateCycle(u32 index);
  void Reset(unsigned int index);
  void SetNextCounter();



 public: // NEW
  unsigned int ReadCountEx(unsigned int index) const;
  unsigned int ReadModeEx(unsigned int index) const;
  unsigned int ReadTargetEx(unsigned int index) const;

  void WriteCountEx(unsigned int index, unsigned int value);
  void WriteModeEx(unsigned int index, unsigned int value);
  void WriteTargetEx(unsigned int index, unsigned int value);

  void RunCountersEx();

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
  RootCounter counters[5];

private:
  unsigned int clks_to_update_min_, nextsCounter;
  signed   int nextiCounter;

  friend class R3000A::Processor;
};

}   // namespace PSX
