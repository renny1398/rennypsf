#pragma once
#include <stdint.h>


namespace PSX {
namespace RootCounter {

void Init();
void Update();
void UpdateVSyncRate();
void WriteCount(unsigned int index, unsigned int value);
void WriteMode(unsigned int index, unsigned int value);
void WriteTarget(unsigned int index, unsigned int value);
unsigned int ReadCount(unsigned int index);

struct Counter
{
    unsigned int count, mode, target;
    unsigned int sCycle, Cycle;    // sCycle: start of cycle, Cycle: end_cycle - start_cycle
    unsigned int rate, interrupt;
};
extern Counter counters[5];
extern unsigned int nextCounter, nextsCounter;


unsigned int SPURun();
void DeadLoopSkip();

}   // namespace RootCounter
}   // namespace PSX
