#include "spu/channel.h"
#include "spu/spu.h"
#include "SoundManager.h"
#include <wx/debug.h>


namespace {
inline int32_t CLIP(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return x;
}
} // namespace

namespace SPU {


SPUVoice::SPUVoice() : p_spu_(NULL), ch(0xffffffff), iUsedFreq(0), iRawPitch(0) {}


SPUVoice::SPUVoice(SPUBase *pSPU, int ch) :
  p_spu_(pSPU), ch(ch),
  lpcm_buffer_l(pSPU->NSSIZE), lpcm_buffer_r(pSPU->NSSIZE),
  pInterpolation(new GaussianInterpolation) {
  tone = NULL;
  is_on_ = false;
  is_ready = false;
}


SPUVoice::SPUVoice(const SPUVoice &info)
  : ::Sample16(), p_spu_(info.p_spu_), ch(info.ch),
    lpcm_buffer_l(p_spu_->NSSIZE), lpcm_buffer_r(p_spu_->NSSIZE),
    pInterpolation(new GaussianInterpolation) {
  tone = NULL;
  is_on_ = false;
  is_ready = false;
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
      wxMessageOutputDebug().Printf(wxT("Created a new instrument. (id = %d, length = %d, loop = %d)"),
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
  wxASSERT(iActFreq != iUsedFreq);
  iUsedFreq = iActFreq;
  pInterpolation->SetSinc(iRawPitch);
}

// FModChangeFrequency



void SPUVoice::Step()
{
  set_envelope(0);
  Set16(0);
  is_ready = true;

  if (IsMuted()) return;
  if (ADSR.IsOff()) {
    return;
  }

  if (iActFreq != iUsedFreq) {
    VoiceChangeFrequency();
  }

  int fa;
  while (pInterpolation->spos >= 0x10000) {
    if (itrTone.HasNext() == false) {
      VoiceOffAndStop();
      return;
    }
    fa = itrTone.Next();

    if (bFMod != 2) {
      if ( ( p_spu_->core(0).ctrl_ & 0x4000 ) == 0 ) fa = 0;
      pInterpolation->StoreValue(fa);
    }
    pInterpolation->spos -= 0x10000;
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

    set_envelope(ADSR.envelope_volume());
    Set16(sval);
    set_volume(iLeftVolume, iRightVolume);
    set_volume_max(0x4000);
  }
  pInterpolation->spos += pInterpolation->GetSinc();
  // wxMessageOutputDebug().Printf("spos = 0x%08x", pInterpolation->spos);
}


SPUVoiceManager::SPUVoiceManager() : pSPU_(NULL), channelNumber_(0) {}


SPUVoiceManager::SPUVoiceManager(SPUBase *pSPU, int channelNumber)
  : pSPU_(pSPU), channels_(channelNumber, SPUVoice()), channelNumber_(channelNumber)
{
  wxASSERT(pSPU != NULL);
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
  wxASSERT(flags < 0x01000000); // WARNING: this is for PSX.
  flagNewChannels_ |= flags << start;
  for (int i = start; flags != 0; i++, flags >>= 1) {
    if ((flags & 1) == 0) continue;
    channels_.at(i).StartSound();
  }
}


void SPUVoiceManager::VoiceOff(uint32_t flags, int start)
{
  wxASSERT(flags < 0x01000000); // WARNING: this is for PSX.
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
  const wxVector<SPUVoice>::iterator itr_end = channels_.end();
  for (wxVector<SPUVoice>::iterator itr = channels_.begin(); itr != itr_end; ++itr) {
    if (itr->ch >= 24) break;
    itr->Step();
  }
}


void SPUVoiceManager::ResetStepStatus() {
  const wxVector<SPUVoice>::iterator itr_end = channels_.end();
  for (wxVector<SPUVoice>::iterator itr = channels_.begin(); itr != itr_end; ++itr) {
    itr->is_ready = false;
  }
}


void SPUVoiceManager::Notify() const {
  const wxSharedPtr<SoundDeviceDriver>& sdd = output();
  if (sdd.get() == 0) {
    // wxMessageOutputDebug().Printf(wxT("Warning: no sound device."));
    return;
  }
  sdd->OnUpdate(this);
}



} // namespace SPU
