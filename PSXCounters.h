#pragma once
#include <stdint.h>


namespace PSXRootCounter
{

void Init();
void Update();
void WriteCount(uint_fast32_t index, uint_fast32_t value);
void WriteMode(uint_fast32_t index, uint_fast32_t value);
void WriteTarget(uint_fast32_t index, uint_fast32_t value);
uint_fast32_t ReadCount(uint_fast32_t index);

}

namespace PSXCounter
{

struct Counter
{
    uint_fast32_t count, mode, target;
    uint_fast32_t sCycle, Cycle, rate, interrupt;
};
extern Counter counters[5];
uint_fast32_t nextCounter, nextsCounter;

void UpdateVSyncRate();

int SPURun();
void DeadLoopSkip();

}
