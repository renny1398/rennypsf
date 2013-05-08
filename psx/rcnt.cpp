#include "rcnt.h"
#include "r3000a.h"
#include "memory.h"
#include "../spu/spu.h"
#include <cstring>
#include <wx/msgout.h>

const int BIAS = 2;
const int PSXCLK = 33868800;	/* 33.8688 Mhz */

static const int counter_num = 4;
static uint32_t last = 0;

namespace PSX {
namespace RootCounter {


Counter counters[5];
unsigned int nextCounter;
unsigned int nextsCounter;

unsigned int &g_nextCounter = nextCounter;
unsigned int &g_nextsCounter = nextsCounter;


void update(unsigned int index)
{
    counters[index].sCycle = R3000a.Cycle;
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

void Reset(unsigned int index)
{
    wxASSERT(0 <= index && index < 4);

    counters[index].count = 0;
    update(index);

    u32Href(0x1070) |= BFLIP32(counters[index].interrupt);
    if ( (counters[index].mode & 0x40) == 0 ) { // only 1 interrupt
        counters[index].Cycle = 0xffffffff;
    }
}

void set()
{
    g_nextCounter = 0x7fffffff;
    g_nextsCounter = R3000a.Cycle;

    for (int i = 0; i < counter_num; i++) {
        int count;
        if (counters[i].Cycle == 0xffffffff) continue;

        count = counters[i].Cycle - (R3000a.Cycle - counters[i].sCycle);
        if (count < 0) {
            g_nextCounter = 0;
            break;
        }
        if (count < static_cast<int>(g_nextCounter)) {
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
    counters[2].rate = 1; counters[2].interrupt = 0x40;

    counters[3].interrupt = 1;
    counters[3].mode = 0x58;    // the VSync counter mode
    counters[3].target = 1;
    UpdateVSyncRate();

    // cnt = 4; // if SPU_async == NULL

    update(0); update(1); update(2); update(3);
    set();
    last = 0;

    wxMessageOutputDebug().Printf(wxT("Initialized PSX root counter."));
}


void Update()
{
    uint32_t cycle = R3000a.Cycle;
    if ( (cycle - counters[3].sCycle) >= counters[3].Cycle ) {
        update(3);
        u32Href(0x1070) |= BFLIP32(1);
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

    set();
}


void WriteCount(unsigned int index, unsigned int value)
{
    counters[index].count = value;
    update(index);
    set();
}


void WriteMode(unsigned int index, unsigned int value)
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

    update(index);
    set();
}


void WriteTarget(unsigned int index, unsigned int value)
{
    counters[index].target = value;
    update(index);
    set();
}


unsigned int ReadCount(unsigned int index)
{
    unsigned int ret;

    if (counters[index].mode & 0x08) {  // count to value in target
        ret = counters[index].count + BIAS*((R3000a.Cycle - counters[index].sCycle) / counters[index].rate);
    } else {    // count to 0xffff
        ret = counters[index].count + BIAS*(R3000a.Cycle / counters[index].rate);
    }

    return ret & 0xffff;
}


unsigned int SPURun()
{
    uint32_t cycles;
    if (R3000a.Cycle < last) {
        cycles = 0xffffffff - last;
        cycles += R3000a.Cycle + 1;
    } else {
        cycles = R3000a.Cycle - last;
    }

    if (cycles >= 16) {
        Spu.Async(cycles);
        last = R3000a.Cycle;
    }
    return 1;
}

void DeadLoopSkip()
{
    // wxMessageOutputDebug().Printf(wxT("CounterDeadLoopSkip"));

    int32_t min, lmin;
    uint32_t cycle = R3000a.Cycle;

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
        R3000a.Cycle = cycle + lmin;
    }
}

}   // namespace RootCounter;
}   // namespace PSX
