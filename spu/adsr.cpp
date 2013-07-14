#include "spu.h"
#include <cstring>
#include <wx/msgout.h>


namespace SPU {

////////////////////////////////////////////////////////////////////////
// ADSR
////////////////////////////////////////////////////////////////////////

uint32_t ChannelInfo::rateTable[160];

void ChannelInfo::InitADSR()
{
    // initialize RateTable
    // RateTable + 32 = { 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, ...}
    ::memset(rateTable, 0, sizeof(uint32_t) * 32);
    uint32_t r = 3, rs = 1, rd = 0;
    for (int i = 32; i < 160; i++) {
        if (r < 0x3fffffff) {
            r += rs;
            if (++rd == 5) {
                rd = 1;
                rs *= 2;
            }
        }
        if (r > 0x3fffffff) {
            r = 0x3fffffff;
        }
        rateTable[i] = r;
    }
}


void ADSRInfoEx::Start()
{
    lVolume = 1;
    State = ADSR_STATE_ATTACK;
    EnvelopeVol = 0;
}


static const unsigned long TableDisp[] = {
    -0x18+0+32, -0x18+4+32, -0x18+6+32, -0x18+8+32,
    -0x18+9+32, -0x18+10+32, -0x18+11+32, -0x18+12+32,
    -0x1b+0+32, -0x1b+4+32, -0x1b+6+32, -0x1b+8+32,
    -0x1b+9+32, -0x1b+10+32, -0x1b+11+32, -0x1b+12+32
};

int ChannelInfo::MixADSR()
{
    uint32_t disp;
    int32_t envVol = ADSRX.EnvelopeVol;

    if (isStopped) {
        // do Release
        disp = (ADSRX.ReleaseModeExp) ? TableDisp[(envVol >> 28) & 0x7] : -0x0c + 32;
        envVol -= rateTable[ADSRX.ReleaseRate + disp];
        if (envVol <= 0) {
            envVol = 0;
            is_on_ = false;
        }
        ADSRX.EnvelopeVol = envVol;
        ADSRX.lVolume = (envVol >>= 21);
        return envVol;
    }

    switch (ADSRX.State) {
    case ADSR_STATE_ATTACK:
        disp = -0x10 + 32;
        if (ADSRX.AttackModeExp && envVol >= 0x60000000) {
            disp = -0x18 + 32;
        }
        envVol += rateTable[ADSRX.AttackRate + disp];
        if (static_cast<uint32_t>(envVol) > 0x7fffffff) {
            envVol = 0x7fffffff;
            ADSRX.State = ADSR_STATE_DECAY;
        }
        ADSRX.EnvelopeVol = envVol;
        ADSRX.lVolume = (envVol >>= 21);
        return envVol;

    case ADSR_STATE_DECAY:
        disp = TableDisp[(envVol >> 28) & 0x7];
        envVol -= rateTable[ADSRX.DecayRate + disp];
        if (envVol < 0) envVol = 0;
        if (envVol <= ADSRX.SustainLevel) {
            ADSRX.State = ADSR_STATE_SUSTAIN;
        }
        ADSRX.EnvelopeVol = envVol;
        ADSRX.lVolume = (envVol >>= 21);
        return envVol;

    case ADSR_STATE_SUSTAIN:
        if (ADSRX.SustainIncrease) {
            disp = -0x10 + 32;
            if (ADSRX.SustainModeExp) {
                if (envVol >= 0x60000000) {
                    disp = -0x18 + 32;
                }
            }
            envVol += rateTable[ADSRX.SustainRate + disp];
            if (static_cast<uint32_t>(envVol) > 0x7fffffff) {
                envVol = 0x7fffffff;
            }
        } else {
            disp = (ADSRX.SustainModeExp) ? TableDisp[((envVol >> 28) & 0x7) + 8] : -0x0f + 32;
            envVol -= rateTable[ADSRX.SustainRate + disp];
            if (envVol < 0) envVol = 0;
        }
        ADSRX.EnvelopeVol = envVol;
        ADSRX.lVolume = (envVol >>= 21);
        return envVol;

    case ADSR_STATE_RELEASE:
        return 0;
    }
    wxMessageOutputDebug().Printf(wxT("WARNING: SPUChannel state is wrong."));
    return 0;
}


}   // namespace SPU
