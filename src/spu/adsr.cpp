#include "spu/channel.h"
#include <cstring>
#include <wx/msgout.h>



namespace {

uint32_t rateTable[160];
const unsigned int TableDisp[] = {
    -0x18+0+32, -0x18+4+32, -0x18+6+32, -0x18+8+32,
    -0x18+9+32, -0x18+10+32, -0x18+11+32, -0x18+12+32};

const unsigned int TableDisp_Sustain[] = {
    -0x1b+0+32, -0x1b+4+32, -0x1b+6+32, -0x1b+8+32,
    -0x1b+9+32, -0x1b+10+32, -0x1b+11+32, -0x1b+12+32
};

}


namespace SPU {


EnvelopeInfo::EnvelopeInfo() {
  // Attack
  attack_mode_exp_ = false;
  attack_rate_ = 0;
  // Decay
  decay_rate_ = 0;
  // Sustain
  sustain_level_ = 0xf << 27;
  sustain_mode_exp_ = false;
  sustain_increase_ = 0;
  sustain_rate_ = 0;
  // Release
  release_mode_exp_ = false;
  release_rate_ = 0;
}

EnvelopeInfo::~EnvelopeInfo() {}


bool EnvelopeInfo::attack_mode_exp() const {
  return attack_mode_exp_;
}

char EnvelopeInfo::attack_rate() const {
  return attack_rate_;
}

char EnvelopeInfo::decay_rate() const {
  return decay_rate_;
}

int EnvelopeInfo::sustain_level() const {
  return sustain_level_;
}

bool EnvelopeInfo::sustain_mode_exp() const {
  return sustain_mode_exp_;
}

int EnvelopeInfo::sustain_increase() const {
  return sustain_increase_;
}

char EnvelopeInfo::sustain_rate() const {
  return sustain_rate_;
}

bool EnvelopeInfo::release_mode_exp() const {
  return release_mode_exp_;
}

char EnvelopeInfo::release_rate() const {
  return release_rate_;
}


void EnvelopePassive::set_attack_mode_exp(bool exp) {
  attack_mode_exp_ = exp;
}

void EnvelopePassive::set_attack_rate(char rate) {
  attack_rate_ = rate;
}

void EnvelopePassive::set_decay_rate(char rate) {
  decay_rate_ = rate;
}

void EnvelopePassive::set_sustain_level(int level) {
  sustain_level_ = level;
}

void EnvelopePassive::set_sustain_mode_exp(bool exp) {
  sustain_mode_exp_ = exp;
}

void EnvelopePassive::set_sustain_increase(int inc) {
  sustain_increase_ = inc;
}

void EnvelopePassive::set_sustain_rate(char rate) {
  sustain_rate_ = rate;
}

void EnvelopePassive::set_release_mode_exp(bool exp) {
  release_mode_exp_ = exp;
}

void EnvelopePassive::set_release_rate(char rate) {
  release_rate_ = rate;
}


int EnvelopeActive::envelope_volume() const {
  return EnvelopeVol;
}

void EnvelopeActive::set_envelope_volume(int vol) {
  EnvelopeVol = vol;
}

long EnvelopeActive::volume() const {
  return lVolume;
}

void EnvelopeActive::set_volume(long vol) {
  lVolume = vol;
}



void EnvelopeState::SetState(SPUVoice* ch, const EnvelopeState* state) {
  ch->ADSR.set_envelope_state(state);
}


class EnvelopeOff : public EnvelopeState {
public:
  static const EnvelopeOff* Instance();
  int Advance(SPUVoice*) const;
};

class EnvelopeAttack : public EnvelopeState {
public:
  static const EnvelopeAttack* Instance();
  int Advance(SPUVoice* ch) const;
};

class EnvelopeDecay : public EnvelopeState {
public:
  static const EnvelopeDecay* Instance();
  int Advance(SPUVoice* ch) const;
};

class EnvelopeSustain : public EnvelopeState {
public:
  static const EnvelopeSustain* Instance();
  int Advance(SPUVoice* ch) const ;
};

class EnvelopeRelease : public EnvelopeState {
public:
  static const EnvelopeRelease* Instance();
  int Advance(SPUVoice* ch) const;
};


const EnvelopeOff* EnvelopeOff::Instance() {
  static EnvelopeOff instance;
  return &instance;
}

int EnvelopeOff::Advance(SPUVoice*) const {
  return 0;
}


const EnvelopeAttack* EnvelopeAttack::Instance() {
    static EnvelopeAttack instance;
    return &instance;
  }

int EnvelopeAttack::Advance(SPUVoice* ch) const {
  EnvelopeActive& adsr = ch->ADSR;
  int32_t envVol = adsr.envelope_volume();
  int32_t rate = adsr.attack_rate();
  if (envVol == 0 && rate == 0) {
    wxMessageOutputDebug().Printf(wxT("Warning: attack rate is zero!!!"));
    SetState(ch, EnvelopeOff::Instance());
    return 0;
  }
  uint32_t disp = -0x10 + 32;
  if (adsr.attack_mode_exp() && envVol >= 0x60000000) {
    disp = -0x18 + 32;
  }
  envVol += rateTable[rate + disp];
  if (static_cast<uint32_t>(envVol) > 0x7fffffff) {
    envVol = 0x7fffffff;
    SetState(ch, EnvelopeDecay::Instance());
  }
  adsr.set_envelope_volume(envVol);
  adsr.set_volume(envVol >>= 21);
  return envVol;
}


const EnvelopeDecay* EnvelopeDecay::Instance() {
  static EnvelopeDecay instance;
  return &instance;
}

int EnvelopeDecay::Advance(SPUVoice* ch) const {
  EnvelopeActive& adsr = ch->ADSR;
  int32_t envVol = adsr.envelope_volume();
  uint32_t disp = TableDisp[(envVol >> 28) & 0x7];
  envVol -= rateTable[adsr.decay_rate() + disp];
  if (envVol < 0) envVol = 0;
  if (envVol <= adsr.sustain_level()) {
    SetState(ch, EnvelopeSustain::Instance());
  }
  adsr.set_envelope_volume(envVol);
  adsr.set_volume(envVol >>= 21);
  return envVol;
}


const EnvelopeSustain* EnvelopeSustain::Instance() {
  static EnvelopeSustain instance;
  return &instance;
}

int EnvelopeSustain::Advance(SPUVoice* ch) const {
  EnvelopeActive& adsr = ch->ADSR;
  int32_t envVol = adsr.envelope_volume();
  uint32_t disp;
  if (adsr.sustain_increase()) {
    disp = -0x10 + 32;
    if (adsr.sustain_mode_exp()) {
      if (envVol >= 0x60000000) {
        disp = -0x18 + 32;
      }
    }
    envVol += rateTable[adsr.sustain_rate() + disp];
    if (static_cast<uint32_t>(envVol) > 0x7fffffff) {
      envVol = 0x7fffffff;
    }
  } else {
    disp = (adsr.sustain_mode_exp()) ? TableDisp_Sustain[(envVol >> 28) & 0x7] : -0x0f + 32;
    envVol -= rateTable[adsr.sustain_rate() + disp];
    if (envVol < 0) envVol = 0;
  }
  adsr.set_envelope_volume(envVol);
  adsr.set_volume(envVol >>= 21);
  return envVol;
}


const EnvelopeRelease* EnvelopeRelease::Instance() {
  static EnvelopeRelease instance;
  return &instance;
}

int EnvelopeRelease::Advance(SPUVoice* ch) const {
  EnvelopeActive& adsr = ch->ADSR;
  int32_t envVol = adsr.envelope_volume();
  uint32_t disp = (adsr.release_mode_exp()) ? TableDisp[(envVol >> 28) & 0x7] : -0x0c + 32;
  if (adsr.release_rate() + disp >= 160) {
    wxMessageOutputDebug().Printf(wxT("WARNING: rateTable index is too large. (release_rate = %d, disp = %d)"), adsr.release_rate(), disp);
    envVol = 0;
    SetState(ch, EnvelopeOff::Instance());
    return envVol;
  }
  envVol -= rateTable[adsr.release_rate() + disp];
  if (envVol <= 0) {
    envVol = 0;
    SetState(ch, EnvelopeOff::Instance());
  }
  adsr.set_envelope_volume(envVol);
  adsr.set_volume(envVol >>= 21);
  return envVol;
}



EnvelopeActive::EnvelopeActive() : State(EnvelopeOff::Instance()) {
  EnvelopeVol = 0;
  lVolume = 0L;
}


void SPUVoice::InitADSR()
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


void SPUVoice::VoiceOn() {
  is_on_ = true;
  ADSR.Set(ADSRX);
  // ADSRX.Start();
}


void SPUVoice::VoiceOff() {
  is_on_ = false;
  ADSR.Release();
}


void SPUVoice::VoiceOffAndStop() {
  is_on_ = false;
  ADSR.Stop();
  set_envelope(0);
}

void EnvelopeActive::Set(const EnvelopePassive& passive) {
  // Attack
  attack_mode_exp_ = passive.attack_mode_exp();
  attack_rate_ = passive.attack_rate();
  if (attack_rate_ == 0) {
    wxMessageOutputDebug().Printf(wxT("Warning: attack rate is zero!"));
  }
  // Decay
  decay_rate_ = passive.decay_rate();
  // Sustain
  sustain_level_ = passive.sustain_level();
  sustain_mode_exp_ = passive.sustain_mode_exp();
  sustain_increase_ = passive.sustain_increase();
  sustain_rate_ = passive.sustain_rate();
  // Release
  release_mode_exp_ = passive.release_mode_exp();
  release_rate_ = passive.release_rate();
  // Volume
  lVolume = 1;
  EnvelopeVol = 0;
  // State
  State = EnvelopeAttack::Instance();
}


void EnvelopeActive::Release() {
  if (State != EnvelopeOff::Instance()) {
    State = EnvelopeRelease::Instance();
  }
}


void EnvelopeActive::Stop() {
  State = EnvelopeOff::Instance();
  lVolume = 0;
  EnvelopeVol = 0;
}


bool EnvelopeActive::IsAttack() const {
  return (State == EnvelopeAttack::Instance());
}

bool EnvelopeActive::IsDecay() const {
  return (State == EnvelopeDecay::Instance());
}

bool EnvelopeActive::IsSustain() const {
  return (State == EnvelopeSustain::Instance());
}

bool EnvelopeActive::IsRelease() const {
  return (State == EnvelopeRelease::Instance());
}

bool EnvelopeActive::IsOff() const {
  return (State == EnvelopeOff::Instance());
}


void EnvelopeActive::set_envelope_state(const EnvelopeState* state) {
  State = state;
}


int EnvelopeActive::AdvanceEnvelope(SPUVoice* ch) {
  return State->Advance(ch);
}


int SPUVoice::AdvanceEnvelope() {
  return ADSR.AdvanceEnvelope(this);
}

}   // namespace SPU
