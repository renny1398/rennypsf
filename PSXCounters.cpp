#include "PSXCounters.h"
#include "R3000A.h"
#include "PSXMemory.h"
#include <cstring>

#define BIAS	2
#define PSXCLK	33868800	/* 33.8688 Mhz */

static const int counter_num = 4;
static uint32_t last = 0;

namespace PSXRootCounter
{

using namespace PSXCounter;

static void _update(uint_fast32_t index)
{
    counters[index].sCycle = R3000A::cycle;
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

static void Reset(uint_fast32_t index)
{
    counters[index].count = 0;
    _update(index);

    psxHu32ref(0x1070) |= BFLIP32(counters[index].interrupt);
    if ( (counters[index].mode & 0x40) == 0 ) { // only 1 interrupt
        counters[index].Cycle = 0xffffffff;
    }
}

static void Set()
{
    nextCounter = 0x7fffffff;
    nextsCounter = R3000A::cycle;

    for (int i = 0; i < count_num; i++) {
        int_fast32_t count;
        if (counters[i].Cycle == 0xffffffff) continue;

        count = counters[i].Cycle - (R3000A::cycle - counters[i].sCycle);
        if (count < 0) {
            nextCounter = 0;
            break;
        }
        if (count < static_cast<int_fast32_t>(nextCounter)) {
            nextCounter = count;
        }
    }
}

}   // namespace PSXRootCounter;


void PSXRootCounter::Init()
{
    memset(counters, 0, sizeof(conters));

    counters[0].rate = 1; counters[0].interrupt = 0x10;
    counters[1].rate = 1; counters[1].interrupt = 0x20;
    counters[2].rate = 1; counters[1].interrupt = 0x40;

    counters[3].interrupt = 1;
    counters[3].mode = 0x58;    // the VSync counter mode
    counters[3].target = 1;
    UpdateVSyncRate();

    // cnt = 4; // SPU_async == NULL

    _update(0); _update(1); _update(2); _update(3);
    Set();
    last = 0;
}


uint_fast32_t PSXCounter::SPURun()
{
    uint_fast32_t cycles;
    if (R3000A::cycle < last) {
        cycles = UINT_FAST32_MAX - last;
        cycles += R3000A::cycle + 1;
    } else {
        cycles = R3000A::cycle - last;
    }

    if (cycles >= 16) {
        // SPUasync
        last = R3000A::cycle;
    }
    return 1;
}


PSXCounter::Counter PSXCounter::counters[5];
