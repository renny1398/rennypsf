#pragma once
#include "common.h"


namespace PSX {

  class RootCounter : public Component {

   public:
    RootCounter(Composite* composite)
      : Component(composite) {}

    void Init();
    void Update();
    void UpdateVSyncRate();
    void WriteCount(unsigned int index, unsigned int value);
    void WriteMode(unsigned int index, unsigned int value);
    void WriteTarget(unsigned int index, unsigned int value);
    unsigned int ReadCount(unsigned int index);

    unsigned int SPURun();
    void DeadLoopSkip();

   protected:
    void UpdateEx(u32 index);
    void Reset(unsigned int index);
    void Set();

   public:
    struct Counter
    {
      unsigned int count, mode, target;
      unsigned int sCycle, Cycle;    // sCycle: start of cycle, Cycle: end_cycle - start_cycle
      unsigned int rate, interrupt;
    };
    Counter counters[5];
   private:
    unsigned int nextCounter, nextsCounter;

    friend class R3000A::Processor;
  };
}   // namespace PSX
