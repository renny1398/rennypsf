#include "psf/spu/channel.h"
#include "psf/spu/spu.h"

#include "common/SoundFormat.h"
#include "common/SoundManager.h"
#include "common/debug.h"


namespace {
inline int32_t CLIP(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return x;
}
} // namespace

namespace SPU {

/*
SPUVoice::SPUVoice()
  : p_core_(nullptr),
    pInterpolation(nullptr), iUsedFreq(0), iRawPitch(0),
    is_ready_(false), ready_cond_(ready_mutex_)
{}
*/

SPUVoice::SPUVoice(SPUCore* p_core)
  : p_core_(p_core),
    lpcm_buffer_l(45), lpcm_buffer_r(45),
    pInterpolation(new GaussianInterpolation),
    is_ready_(false), ready_cond_(ready_mutex_) {
  tone = nullptr;
  is_on_ = false;
}

SPUVoice::SPUVoice(const SPUVoice &info)
  : SPUVoice(info.p_core_) {}

SPUVoice::~SPUVoice() {
  if (pInterpolation) {
    delete pInterpolation;
  }
}

SPUBase* SPUVoice::p_spu() { return p_core_->p_spu(); }
const SPUBase* SPUVoice::p_spu() const { return p_core_->p_spu(); }

void SPUVoice::SetReady() const {
  ready_mutex_.Lock();
  is_ready_ = true;
  ready_cond_.Broadcast();
  ready_mutex_.Unlock();
}

void SPUVoice::SetUnready() const {
  ready_mutex_.Lock();
  is_ready_ = false;
  ready_cond_.Broadcast();
  ready_mutex_.Unlock();
}


void SPUVoice::NotifyOnNoteOn() const {
/*
  NoteInfo note;
  note.ch = ch;
  note.is_on = true;
  note.tone_number_ = tone->GetAddr();
  note.pitch = Pitch;
  note.rate = iActFreq;
  note.velocity = ADSRX.EnvelopeVol / 1024.0;

  Spu().sound_driver_->OnNoteOn(note);
  */
}


void SPUVoice::NotifyOnNoteOff() const {
/*
  NoteInfo note;
  note.ch = ch;
  note.is_on = false;
  note.tone_number_ = -1;
  note.pitch = 0.0;
  note.rate = 0;
  // note.velocity = ADSRX.EnvelopeVol / 1024.0;

  Spu().sound_driver_->OnNoteOff(note);
  */
}


void SPUVoice::NotifyOnChangePitch() const {
/*
  NoteInfo note;
  note.ch = ch;
  note.is_on = is_on_;
  note.tone_number_ = -1;
  note.pitch = Pitch;
  note.rate = iActFreq;
  note.velocity = static_cast<float>(ADSRX.EnvelopeVol) / 1024.0;

  Spu().sound_driver_->OnChangePitch(note);
  */
}

void SPUVoice::NotifyOnChangeVelocity() const {
/*
  NoteInfo note;
  note.ch = ch;
  note.is_on = is_on_;
  note.tone_number_ = -1;
  note.velocity = static_cast<float>(ADSRX.EnvelopeVol) / 1024.0;

  // Spu().sound_driver_->OnChangeVelocity(event);
  */
}


void SPUVoice::StartSound()
{
  // TODO: Mutex Lock

  SPUInstrument_New* p_inst = tone;
  if (p_inst == 0 || addr != p_inst->addr() || 0x80000000 <= addr) {
    p_inst = dynamic_cast<SPUInstrument_New*>(&p_spu()->soundbank().instrument(SPUInstrument_New::CalculateId(addr, useExternalLoop ? addrExternalLoop : 0xffffffff)));
    if (p_inst == 0) {
      p_inst = new SPUInstrument_New(*p_spu(), addr, useExternalLoop ? addrExternalLoop : 0xffffffff);
      p_spu()->soundbank().set_instrument(p_inst);
      rennyLogDebug("SPUInterument", "Created a new instrument. (id = 0x%08x, length = %d, loop = %d)",
                    p_inst->id(), p_inst->length(), p_inst->loop());
    }
    tone = p_inst;
  }
  itrTone = p_inst->Iterator(true);

  pInterpolation->Start();

  useExternalLoop = false;

  VoiceOn();
  p_spu()->Reverb().StartReverb(this);

  // Spu().ChangeProcessState(SPU::STATE_NOTE_ON, ch);
  const SPURequest* req = SPUNoteOnRequest::CreateRequest(this);
  if (req == 0) {
    VoiceOffAndStop();
    return;
  }
  p_spu()->PutRequest(req);

  NotifyOnNoteOn();

  // TODO: Change newChannelFlags
}


void SPUVoice::VoiceChangeFrequency()
{
  rennyAssert(iActFreq != iUsedFreq);
  iUsedFreq = iActFreq;
  pInterpolation->SetSinc(iRawPitch * p_spu()->GetDefaultSamplingRate() / p_spu()->GetCurrentSamplingRate());
}

// FModChangeFrequency


void SPUVoice::Advance() {

  if (is_ready_) return;
  set_envelope(0);

  // if (IsMuted()) return;
  if (ADSR.IsOff()) {
    SetReady();
    return;
  }

  if (iActFreq != iUsedFreq) {
    VoiceChangeFrequency();
  }

  int fa;
  while (pInterpolation->GetSincPosition() >= 0x10000) {
    if (itrTone.HasNext() == false) {
      VoiceOffAndStop();
      SetReady();
      return;
    }
    fa = itrTone.Next();

    if (bFMod != 2) {
      if ( ( p_core_->ctrl_ & 0x4000 ) == 0 ) fa = 0;
      pInterpolation->StoreValue(fa);
    }
    pInterpolation->SubSincPosition(0x10000);
  }

  if (bNoise) {
    // TODO: Noise
    fa = 0;
  } else if (bFMod != 2) {
    fa = pInterpolation->GetValue();
  }

  // sval = (MixADSR() * fa) / 1023;
  int prev_envvol = ADSR.envelope_volume();
  int curr_envvol = AdvanceEnvelope();
  sval = (curr_envvol * fa) / 1023;
  if (prev_envvol != curr_envvol) {
    NotifyOnChangeVelocity();
  }

  if (bFMod == 2) {
    // TODO: FM
  } else {
    int left = (sval * iLeftVolume) / 0x4000;
    int right = (sval * iRightVolume) / 0x4000;
    lpcm_buffer_l[p_spu()->ns] = CLIP(left);
    lpcm_buffer_r[p_spu()->ns] = CLIP(right);
  }
  pInterpolation->AdvanceSincPosition();
  // wxMessageOutputDebug().Printf("spos = 0x%08x", pInterpolation->spos);
  SetReady();
}


bool SPUVoice::Get(SampleSequence* dest) const {
  if (dest == nullptr) return false;

  ready_mutex_.Lock();
  while (is_ready_ == false) {
    if (ready_cond_.WaitTimeout(1000000) == wxCOND_TIMEOUT) {
      ready_mutex_.Unlock();
      rennyLogWarning("SPUVoice", "Get() is timeout.");
      return false;
    }
  }
  ready_mutex_.Unlock();

  const float fvol_left  = static_cast<float>(iLeftVolume) / 0x4000;
  const float fvol_right = static_cast<float>(iRightVolume) / 0x4000;

  dest->set_volume(fvol_left, fvol_right);
  dest->Push16i(sval, ADSR.envelope_volume());

  SetUnready();
  return true;
}

////////////////////////////////////////////////////////////////////////
// SPUCore voice manager functions
////////////////////////////////////////////////////////////////////////

SPUCoreVoiceManager::SPUCoreVoiceManager(SPUCore* p_core, int voice_count)
  : p_core_(p_core), voices_(voice_count, SPUVoice(p_core)), new_flags_(0) {
  rennyAssert(p_core != nullptr);
}

SPUVoice& SPUCoreVoiceManager::VoiceRef(int ch) {
  return voices_.at(ch);
}

unsigned int SPUCoreVoiceManager::GetVoiceCount() const {
  return voices_.size();
}

int SPUCoreVoiceManager::GetVoiceIndex(SPUVoice* p_voice) const {
  int i = 0;
  for (const auto& v : voices_) {
    if (&v == p_voice) {
      return i;
    }
    ++i;
  }
  return -1;
}

bool SPUCoreVoiceManager::ExistsNew() const {
  return (new_flags_ != 0);
}

void SPUCoreVoiceManager::SoundNew(uint32_t flags, int start) {
  rennyAssert(start < 32);
  new_flags_ |= flags << start;
  for (int i = start; flags != 0; ++i, flags >>= 1) {
    if ((flags & 1) == 0) continue;
    VoiceRef(i).StartSound();
  }
}

void SPUCoreVoiceManager::VoiceOff(uint32_t flags, int start) {
  rennyAssert(start < 32);
  for (int i = start; flags != 0; i++, flags >>= 1) {
    if ((flags & 1) == 0) continue;
    auto& ref_voice = VoiceRef(i);
    ref_voice.VoiceOff();
    const SPURequest* req = SPUNoteOffRequest::CreateRequest(&ref_voice);
    p_core_->p_spu()->PutRequest(req);
  }
}

void SPUCoreVoiceManager::Advance() {
  for (auto& v : voices_) {
    v.Advance();
  }
}


////////////////////////////////////////////////////////////////////////
// SPU voice manager functions
////////////////////////////////////////////////////////////////////////

SPUVoiceManager::SPUVoiceManager() : p_spu_(nullptr) {}

SPUVoiceManager::SPUVoiceManager(SPUBase *p_spu)
  : p_spu_(p_spu) {
  rennyAssert(p_spu != nullptr);
}

SPUVoice& SPUVoiceManager::VoiceRef(int ch) {
  auto core_num = p_spu_->core_count();
  for (decltype(core_num) i = 0; i < core_num; ++i) {
    SPUCore& core = p_spu_->core(i);
    if (ch < static_cast<int>(core.Voices().GetVoiceCount())) {
      return core.Voice(ch);
    }
    ch -= core.Voices().GetVoiceCount();
  }
  throw std::out_of_range(nullptr);
}

unsigned int SPUVoiceManager::voice_count() const {
  unsigned int ret = 0;
  auto core_num = p_spu_->core_count();
  for (decltype(core_num) i = 0; i < core_num; ++i) {
    ret += p_spu_->core(i).Voices().GetVoiceCount();
  }
  return ret;
}

bool SPUVoiceManager::ExistsNew() const {
  auto core_num = p_spu_->core_count();
  for (decltype(core_num) i = 0; i < core_num; ++i) {
    if (p_spu_->core(i).Voices().ExistsNew()) {
      return true;
    }
  }
  return false;
}

/*
void SPUVoiceManager::SoundNew(uint32_t flags, int start) {
}

void SPUVoiceManager::VoiceOff(uint32_t flags, int start) {
}
*/

void SPUVoiceManager::Advance() {
  auto core_num = p_spu_->core_count();
  for (decltype(core_num) i = 0; i < core_num; ++i) {
    p_spu_->core(i).Voices().Advance();
  }
}

} // namespace SPU
