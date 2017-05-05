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


SPUVoice::SPUVoice()
  : p_spu_(nullptr), ch(0xffffffff), iUsedFreq(0), iRawPitch(0),
    is_ready_(false), ready_cond_(ready_mutex_)
{}


SPUVoice::SPUVoice(SPUBase *pSPU, int ch)
  : p_spu_(pSPU), ch(ch),
    lpcm_buffer_l(pSPU->NSSIZE), lpcm_buffer_r(pSPU->NSSIZE),
    pInterpolation(new GaussianInterpolation),
    is_ready_(false), ready_cond_(ready_mutex_) {
  tone = nullptr;
  is_on_ = false;
}


SPUVoice::SPUVoice(const SPUVoice &info)
  : p_spu_(info.p_spu_), ch(info.ch),
    lpcm_buffer_l(p_spu_->NSSIZE), lpcm_buffer_r(p_spu_->NSSIZE),
    pInterpolation(new GaussianInterpolation),
    is_ready_(false), ready_cond_(ready_mutex_) {
  tone = nullptr;
  is_on_ = false;
}

/*
bool SPUVoice::IsReady() const {
  return is_ready_;
}
*/

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
    p_inst = dynamic_cast<SPUInstrument_New*>(&Spu().soundbank().instrument(SPUInstrument_New::CalculateId(addr, useExternalLoop ? addrExternalLoop : 0xffffffff)));
    if (p_inst == 0) {
      p_inst = new SPUInstrument_New(Spu(), addr, useExternalLoop ? addrExternalLoop : 0xffffffff);
      Spu().soundbank().set_instrument(p_inst);
      rennyLogDebug("SPUInterument", "Created a new instrument. (id = 0x%08x, length = %d, loop = %d)",
                    p_inst->id(), p_inst->length(), p_inst->loop());
    }
    tone = p_inst;
  }
  itrTone = p_inst->Iterator(true);

  pInterpolation->Start();

  useExternalLoop = false;

  VoiceOn();
  p_spu_->Reverb().StartReverb(this);

  // Spu().ChangeProcessState(SPU::STATE_NOTE_ON, ch);
  const SPURequest* req = SPUNoteOnRequest::CreateRequest(ch);
  if (req == 0) {
    VoiceOffAndStop();
    return;
  }
  p_spu_->PutRequest(req);

  NotifyOnNoteOn();

  // TODO: Change newChannelFlags
}


void SPUVoice::VoiceChangeFrequency()
{
  rennyAssert(iActFreq != iUsedFreq);
  iUsedFreq = iActFreq;
  pInterpolation->SetSinc(iRawPitch * Spu().GetDefaultSamplingRate() / Spu().GetCurrentSamplingRate());
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
      if ( ( p_spu_->core(0).ctrl_ & 0x4000 ) == 0 ) fa = 0;
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
    lpcm_buffer_l[p_spu_->ns] = CLIP(left);
    lpcm_buffer_r[p_spu_->ns] = CLIP(right);
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


SPUVoiceManager::SPUVoiceManager() : pSPU_(nullptr) {}


SPUVoiceManager::SPUVoiceManager(SPUBase *pSPU, int channelNumber)
  : pSPU_(pSPU), channels_(channelNumber, SPUVoice())
{
  rennyAssert(pSPU != nullptr);
  for (int i = 0; i < channelNumber; i++) {
    SPUVoice* p_voice = &channels_.at(i);
    new(p_voice) SPUVoice(pSPU, i);
  }
}


bool SPUVoiceManager::ExistsNew() const {
  return (flagNewChannels_ != 0);
}


void SPUVoiceManager::SoundNew(uint32_t flags, int start)
{
  rennyAssert(flags < 0x01000000); // WARNING: this is for PSX.
  flagNewChannels_ |= flags << start;
  for (int i = start; flags != 0; i++, flags >>= 1) {
    if ((flags & 1) == 0) continue;
    channels_.at(i).StartSound();
  }
}


void SPUVoiceManager::VoiceOff(uint32_t flags, int start)
{
  rennyAssert(flags < 0x01000000); // WARNING: this is for PSX.
  for (int i = start; flags != 0; i++, flags >>= 1) {
    if ((flags & 1) == 0) continue;
    SPUVoice& ch = channels_.at(i);
    ch.VoiceOff();

    // pSPU_->ChangeProcessState(SPUBase::STATE_NOTE_OFF, i);
    const SPURequest* req = SPUNoteOffRequest::CreateRequest(i);
    pSPU_->PutRequest(req);

    // ch.NotifyOnNoteOff();
  }
}


void SPUVoiceManager::StepForAll() {
  for (auto& v : channels_) {
    if (v.ch >= 24) break;
    v.Advance();
  }
}

// deprecated
void SPUVoiceManager::ResetStepStatus() {}

} // namespace SPU
