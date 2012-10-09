#include "PSXCounters.h"
#include "R3000A.h"
#include "PSXMemory.h"
#include <cstring>
#include <wx/msgout.h>

const int BIAS = 2;
const int PSXCLK = 33868800;	/* 33.8688 Mhz */

static const int counter_num = 4;
static uint32_t last = 0;







namespace PSXRootCounter
{

Counter counters[5];
unsigned long nextCounter;
unsigned long nextsCounter;

unsigned long &g_nextCounter = nextCounter;
unsigned long &g_nextsCounter = nextsCounter;


void _update(unsigned long index)
{
    counters[index].sCycle = R3000A::cycle;
    if ( (!(counters[index].mode & 1) || (index != 2)) && counters[index].mode & 0x30 ) {
        if (counters[index].mode & 0x10) {  // interrupt on target
            counters[index].Cycle = (counters[index].target - counters[index].count) * counters[index].rate / BIAS;
        } else {    // interrupt on 0xffff
            counters[index].Cycle = (0xffff - counters[index].count) * counters[index].rate / BIAS;
        }
    } else {
        counters[index].Cycle = ULONG_MAX;
    }

}

void Reset(unsigned long index)
{
    counters[index].count = 0;
    _update(index);

    psxHu32ref(0x1070) |= BFLIP32(counters[index].interrupt);
    if ( (counters[index].mode & 0x40) == 0 ) { // only 1 interrupt
        counters[index].Cycle = ULONG_MAX;
    }
}

void Set()
{
    g_nextCounter = LONG_MAX;
    g_nextsCounter = R3000A::cycle;

    for (int i = 0; i < counter_num; i++) {
        long count;
        if (counters[i].Cycle == ULONG_MAX) continue;

        count = counters[i].Cycle - (R3000A::cycle - counters[i].sCycle);
        if (count < 0) {
            g_nextCounter = 0;
            break;
        }
        if (count < static_cast<long>(g_nextCounter)) {
            g_nextCounter = count;
        }
    }
}


void UpdateVSyncRate()
{
    counters[3].rate = (PSXCLK / 60);   // 60 Hz
}


void Init()
{
    memset(counters, 0, sizeof(counters));

    counters[0].rate = 1; counters[0].interrupt = 0x10;
    counters[1].rate = 1; counters[1].interrupt = 0x20;
    counters[2].rate = 1; counters[1].interrupt = 0x40;

    counters[3].interrupt = 1;
    counters[3].mode = 0x58;    // the VSync counter mode
    counters[3].target = 1;
    UpdateVSyncRate();

    // cnt = 4; // if SPU_async == NULL

    _update(0); _update(1); _update(2); _update(3);
    Set();
    last = 0;
}


void Update()
{
    uint32_t cycle = R3000A::cycle;
    if ( (cycle- counters[3].sCycle) >= counters[3].Cycle ) {
        _update(3);
        psxHu32ref(0x1070) |= BFLIP32(1);
    }
    if ( (cycle- counters[0].sCycle) >= counters[0].Cycle ) {
        Reset(0);
    }
    if ( (cycle- counters[1].sCycle) >= counters[1].Cycle ) {
        Reset(1);
    }
    if ( (cycle- counters[2].sCycle) >= counters[2].Cycle ) {
        Reset(2);
    }

    Set();
}


void WriteCount(unsigned long index, unsigned long value)
{
    counters[index].count = value;
    _update(index);
    Set();
}


void WriteMode(unsigned long index, unsigned long value)
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

    _update(index);
    Set();
}


void WriteTarget(unsigned long index, unsigned long value)
{
    counters[index].target = value;
    _update(index);
    Set();
}


unsigned long ReadCount(unsigned long index)
{
    unsigned long ret;

    if (counters[index].mode & 0x08) {  // count to value in target
        ret = counters[index].count + BIAS*((R3000A::cycle - counters[index].sCycle) / counters[index].rate);
    } else {    // count to 0xffff
        ret = counters[index].count + BIAS*(R3000A::cycle / counters[index].rate);
    }

    return ret & 0xffff;
}


unsigned long SPURun()
{
    unsigned long cycles;
    if (R3000A::cycle < last) {
        cycles = UINT_FAST32_MAX - last;
        cycles += R3000A::cycle + 1;
    } else {
        cycles = R3000A::cycle - last;
    }

    if (cycles >= 16) {
        wxMessageOutputDebug().Printf("TODO: implement SPUasync");
        last = R3000A::cycle;
    }
    return 1;
}

void DeadLoopSkip()
{
    long min, lmin;
    uint32_t cycle = R3000A::cycle;

    lmin = LONG_MAX;

    for (int i = 0; i < 4; i++) {
        if (counters[i].Cycle != ULONG_MAX) {
            min = counters[i].Cycle;
            min -= cycle - counters[i].sCycle;
            wxASSERT(min >= 0);
            if (min < lmin) {
                lmin = min;
            }
        }
    }

    if (lmin > 0) {
        R3000A::cycle = cycle + lmin;
    }
}

}   // namespace PSXRootCounter;
