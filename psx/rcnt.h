#pragma once
#include <stdint.h>


namespace PSX {
namespace RootCounter {

void Init();
void Update();
void UpdateVSyncRate();
void WriteCount(unsigned long index, unsigned long value);
void WriteMode(unsigned long index, unsigned long value);
void WriteTarget(unsigned long index, unsigned long value);
unsigned long ReadCount(unsigned long index);

struct Counter
{
    unsigned long count, mode, target;
    unsigned long sCycle, Cycle;    // sCycle: start of cycle, Cycle: end_cycle - start_cycle
    unsigned long rate, interrupt;
};
extern Counter counters[5];
extern unsigned long nextCounter, nextsCounter;


unsigned long SPURun();
void DeadLoopSkip();

}   // namespace RootCounter
}   // namespace PSX
