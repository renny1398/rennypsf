#include "spu/channel.h"
#include <cstring>
#include <wx/msgout.h>



namespace {

uint32_t rateTable[160];
const unsigned int TableDisp[] = {
  -0x18+0+32, -0x18+4+32, -0x18+6+32, -0x18+8+32,
  -0x18+9+32, -0x18+10+32, -0x18+11+32, -0x18+12+32,
  -0x1b+0+32, -0x1b+4+32, -0x1b+6+32, -0x1b+8+32,
  -0x1b+9+32, -0x1b+10+32, -0x1b+11+32, -0x1b+12+32
};


int AdvanceEnvelopeOff(SPU::ChannelInfo*) { return 0; }


int AdvanceEnvelopeOnAttack(SPU::ChannelInfo* ch) {
  int32_t envVol = ch->ADSRX.EnvelopeVol;
  uint32_t disp = -0x10 + 32;
  if (ch->ADSRX.AttackModeExp && envVol >= 0x60000000) {
    disp = -0x18 + 32;
  }
  envVol += rateTable[ch->ADSRX.AttackRate + disp];
  if (static_cast<uint32_t>(envVol) > 0x7fffffff) {
    envVol = 0x7fffffff;
    ch->ADSRX.State = SPU::kEnvelopeStateDecay;
  }
  ch->ADSRX.EnvelopeVol = envVol;
  ch->ADSRX.lVolume = (envVol >>= 21);
  return envVol;
}


int AdvanceEnvelopeOnDecay(SPU::ChannelInfo* ch) {
  int32_t envVol = ch->ADSRX.EnvelopeVol;
  uint32_t disp = TableDisp[(envVol >> 28) & 0x7];
  envVol -= rateTable[ch->ADSRX.DecayRate + disp];
  if (envVol < 0) envVol = 0;
  if (envVol <= ch->ADSRX.SustainLevel) {
    ch->ADSRX.State = SPU::kEnvelopeStateSustain;
  }
  ch->ADSRX.EnvelopeVol = envVol;
  ch->ADSRX.lVolume = (envVol >>= 21);
  return envVol;
}


int AdvanceEnvelopeOnSustain(SPU::ChannelInfo* ch) {
  int32_t envVol = ch->ADSRX.EnvelopeVol;
  uint32_t disp;
  if (ch->ADSRX.SustainIncrease) {
    disp = -0x10 + 32;
    if (ch->ADSRX.SustainModeExp) {
      if (envVol >= 0x60000000) {
        disp = -0x18 + 32;
      }
    }
    envVol += rateTable[ch->ADSRX.SustainRate + disp];
    if (static_cast<uint32_t>(envVol) > 0x7fffffff) {
      envVol = 0x7fffffff;
    }
  } else {
    disp = (ch->ADSRX.SustainModeExp) ? TableDisp[((envVol >> 28) & 0x7) + 8] : -0x0f + 32;
    envVol -= rateTable[ch->ADSRX.SustainRate + disp];
    if (envVol < 0) envVol = 0;
  }
  ch->ADSRX.EnvelopeVol = envVol;
  ch->ADSRX.lVolume = (envVol >>= 21);
  return envVol;
}


int AdvanceEnvelopeOnRelease(SPU::ChannelInfo* ch) {
  int32_t envVol = ch->ADSRX.EnvelopeVol;
  uint32_t disp = (ch->ADSRX.ReleaseModeExp) ? TableDisp[(envVol >> 28) & 0x7] : -0x0c + 32;
  envVol -= rateTable[ch->ADSRX.ReleaseRate + disp];
  if (envVol <= 0) {
    envVol = 0;
    ch->ADSRX.State = SPU::kEnvelopeStateOff;
  }
  ch->ADSRX.EnvelopeVol = envVol;
  ch->ADSRX.lVolume = (envVol >>= 21);
  return envVol;
}


int (*const advance_envelope_LUT[SPU::kEnvelopeStateCount])(SPU::ChannelInfo*) = {
    AdvanceEnvelopeOff,
    AdvanceEnvelopeOnAttack,
    AdvanceEnvelopeOnDecay,
    AdvanceEnvelopeOnSustain,
    AdvanceEnvelopeOnRelease
};

}



namespace SPU {


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


void ChannelInfo::On() {
  is_on_ = true;
  ADSRX.Start();
}


void ChannelInfo::Off() {
  is_on_ = false;
  ADSRX.State = kEnvelopeStateRelease;
}


void ADSRInfoEx::Start() {
  lVolume = 1;
  State = kEnvelopeStateAttack;
  EnvelopeVol = 0;
}


int ChannelInfo::AdvanceEnvelope() {
  return advance_envelope_LUT[ADSRX.State](this);
}

}   // namespace SPU
