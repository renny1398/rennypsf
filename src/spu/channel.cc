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


ChannelInfo::ChannelInfo(SPU *pSPU) :
  spu_(*pSPU),
  lpcm_buffer_l(pSPU->NSSIZE), lpcm_buffer_r(pSPU->NSSIZE),
  pInterpolation(new GaussianInterpolation) {
  tone = NULL;
  is_on_ = false;
  is_ready = false;
}


ChannelInfo::ChannelInfo(const ChannelInfo &info)
  : ::Sample16(), spu_(info.spu_),
    lpcm_buffer_l(spu_.NSSIZE), lpcm_buffer_r(spu_.NSSIZE),
    pInterpolation(new GaussianInterpolation) {
  tone = NULL;
  is_on_ = false;
  is_ready = false;
}


void ChannelInfo::NotifyOnNoteOn() const {
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


void ChannelInfo::NotifyOnNoteOff() const {
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


void ChannelInfo::NotifyOnChangePitch() const {
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

void ChannelInfo::NotifyOnChangeVelocity() const {
/*
  NoteInfo note;
  note.ch = ch;
  note.is_on = is_on_;
  note.tone_number_ = -1;
  note.velocity = static_cast<float>(ADSRX.EnvelopeVol) / 1024.0;

  // Spu().sound_driver_->OnChangeVelocity(event);
  */
}


void ChannelInfo::StartSound()
{
  // wxMutexLocker locker(on_mutex_);

  On();
  spu_.Reverb().StartReverb(this);

  itrTone = tone->Iterator(this);

  pInterpolation->Start();

  useExternalLoop = false;

  // Spu().ChangeProcessState(SPU::STATE_NOTE_ON, ch);
  NotifyOnNoteOn();

  // TODO: Change newChannelFlags
}


void ChannelInfo::VoiceChangeFrequency()
{
  wxASSERT(iActFreq != iUsedFreq);
  iUsedFreq = iActFreq;
  pInterpolation->SetSinc(iRawPitch);
}

// FModChangeFrequency



void ChannelInfo::Update()
{
  is_ready = true;

  if (IsMuted()) return;
  // if (IsOn() == false) return;

  if (iActFreq != iUsedFreq) {
    VoiceChangeFrequency();
  }

  int fa;
  while (pInterpolation->spos >= 0x10000) {
    if (itrTone.HasNext() == false) {
      Off();
      ADSRX.lVolume = 0;
      ADSRX.EnvelopeVol = 0;
      set_envelope(0);
      return;
    }
    fa = itrTone.Next();

    if (bFMod != 2) {
      if ( ( spu_.Sp0& 0x4000) == 0 ) fa = 0;
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
  int prev_envvol = ADSRX.EnvelopeVol;
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
    lpcm_buffer_l[spu_.ns] = CLIP(left);
    lpcm_buffer_r[spu_.ns] = CLIP(right);

    /*
      spu_.Reverb().StoreReverb(*this);
      spu_.Reverb().Mix();
      left = spu_.Reverb().GetLeft();
      right = spu_.Reverb().GetRight();
      //left /= 3;
      // right /= 3;
      CLIP(left);  CLIP(right);
*/

    set_envelope(ADSRX.EnvelopeVol);
    Set16(sval);
    set_volume(iLeftVolume, iRightVolume);
    set_volume_max(0x4000);
  }
  pInterpolation->spos += pInterpolation->GetSinc();
  // wxMessageOutputDebug().Printf("spos = 0x%08x", pInterpolation->spos);
}




ChannelArray::ChannelArray(SPU *pSPU, int channelNumber)
  : pSPU_(pSPU), channels_(channelNumber, ChannelInfo(pSPU)), channelNumber_(channelNumber)
{
  wxASSERT(pSPU != NULL);
}


bool ChannelArray::ExistsNew() const {
  return (flagNewChannels_ != 0);
}


void ChannelArray::SoundNew(uint32_t flags, int start)
{
  wxASSERT(flags < 0x01000000);
  flagNewChannels_ |= flags << start;
  for (int i = start; flags != 0; i++, flags >>= 1) {
    if (!(flags & 1) || (channels_.at(i).tone->GetADPCM() == 0)) continue;
    channels_.at(i).StartSound();
  }
}


void ChannelArray::VoiceOff(uint32_t flags, int start)
{
  wxASSERT(flags < 0x01000000);
  for (int i = start; flags != 0; i++, flags >>= 1) {
    if ((flags & 1) == 0) continue;
    ChannelInfo& ch = channels_.at(i);
    ch.Off();

    pSPU_->ChangeProcessState(SPU::STATE_NOTE_OFF, i);

    // ch.NotifyOnNoteOff();
  }
}


void ChannelArray::Notify() const {
  output()->OnUpdate(this);
}



} // namespace SPU
