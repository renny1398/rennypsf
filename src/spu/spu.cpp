#include "spu.h"
#include <wx/msgout.h>
#include <cstring>

#include "../psx/hardware.h"
#include "../SoundManager.h"

namespace {

inline int32_t CLIP(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return x;
}

static int32_t poo;

}   // namespace


wxDEFINE_EVENT(wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);



namespace SPU {


ChannelInfo::ChannelInfo(SPU *pSPU) :
  spu_(*pSPU),
  lpcm_buffer_l(pSPU->NSSIZE), lpcm_buffer_r(pSPU->NSSIZE),
  pInterpolation(new GaussianInterpolation(*this)) {
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

  ADSRX.Start();
  spu_.Reverb().StartReverb(this);

  itrTone = tone->Iterator(this);
  isStopped = false;
  is_on_ = true;

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
  // if (bNew) StartSound();
  if (IsOn() == false) return;

  if (iActFreq != iUsedFreq) {
    VoiceChangeFrequency();
  }

  int fa;
  while (pInterpolation->spos >= 0x10000) {
    if (itrTone.HasNext() == false) {
      is_on_ = false;
      // isStopped = true;
      ADSRX.lVolume = 0;
      ADSRX.EnvelopeVol = 0;
      set_envelope(0);
      return;
    }
    fa = itrTone.Next();

    pInterpolation->StoreValue(fa);
    pInterpolation->spos -= 0x10000;
  }

  if (bNoise) {
    // TODO: Noise
    fa = 0;
  } else {
    fa = pInterpolation->GetValue();
  }

  // sval = (MixADSR() * fa) / 1023;
  int prev_envvol = ADSRX.EnvelopeVol;
  int curr_envvol = MixADSR();
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
  : pSPU_(pSPU), channelNumber_(channelNumber)
{
  for (int i = 0; i < channelNumber; i++) {
    // ChannelInfo* channel = new ChannelInfo(pSPU);
    // channel->ch = i;
    channels_.push_back(ChannelInfo(pSPU));
  }
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
    ch.isStopped = true;

    pSPU_->ChangeProcessState(SPU::STATE_NOTE_OFF, i);

    // ch.NotifyOnNoteOff();
  }
}


void ChannelArray::Notify() const {
  output()->OnUpdate(this);
}





void SPUBase::NotifyOnUpdateStartAddress(int ch) const {
  pthread_mutex_lock(&process_mutex_);
  process_state_ = SPU::STATE_SET_OFFSET;
  processing_channel_ = ch;
  pthread_cond_broadcast(&process_cond_);
  pthread_mutex_unlock(&process_mutex_);
}


void SPUBase::NotifyOnChangeLoopIndex(ChannelInfo* /*pChannel*/) const
{
  /*
  SamplingTone* tone = pChannel->tone;
  ToneInfo tone_info;
  tone_info.number = tone->GetAddr();
  tone_info.length = tone->GetLength();
  tone_info.loop = (tone->GetExternalLoopAddr() - tone->GetAddr()) * 28 / 16;
  tone_info.pitch = tone->GetFreq();
  tone_info.data = tone->GetData();
  sound_driver_->OnChangeTone(tone_info);
  */
}


void SPUBase::ChangeProcessState(ProcessState state, int ch) {
  pthread_mutex_lock(&process_mutex_);
  process_state_ = state;
  processing_channel_ = ch;
  pthread_cond_broadcast(&process_cond_);
  pthread_mutex_unlock(&process_mutex_);

  pthread_mutex_lock(&wait_start_mutex_);
  while (process_state_ != STATE_START_PROCESS) {
    pthread_cond_wait(&process_cond_, &wait_start_mutex_);
  }
  pthread_mutex_unlock(&wait_start_mutex_);
}




SamplingTone* SPU::GetSamplingTone(uint32_t addr) const
{
  return const_cast<SoundBank*>(&SoundBank_)->GetSamplingTone(addr);
}


void SPU::NotifyOnAddTone(const SamplingTone& /*tone*/) const {
  /*
  ToneInfo tone_info;
  tone_info.number = tone.GetAddr();
  tone_info.length = tone.GetLength();
  tone_info.loop = tone.GetLoopOffset();
  tone_info.pitch = tone.GetFreq();
  tone_info.data = tone.GetData();
  sound_driver_->OnAddTone(tone_info);
  */
}


void SPU::NotifyOnChangeTone(const SamplingTone& /*tone*/) const {
/*
  ToneInfo tone_info;
  tone_info.number = tone.GetAddr();
  tone_info.length = tone.GetLength();
  tone_info.loop = tone.GetLoopOffset();
  tone_info.pitch = tone.GetFreq();
  tone_info.data = tone.GetData();
  sound_driver_->OnChangeTone(tone_info);
  */
}


void SPU::NotifyOnRemoveTone(const SamplingTone& /*tone*/) const {
/*
  ToneInfo tone_info;
  tone_info.number = tone.GetAddr();
  sound_driver_->OnRemoveTone(tone_info);
  */
}


void SPU::SetupStreams()
{
  m_pSpuBuffer = new uint8_t[32768];

  SoundBank_.Reset();
  reverb_.Reset();

  for (int i = 0; i < 24; i++) {
    ChannelInfo ch = Channels.At(i);
    ch.ADSRX.SustainLevel = 0xf << 27;
    // Channels[i].iIrqDone = 0;
    ch.tone = 0;
    ch.itrTone = SamplingToneIterator();
  }
}

void SPU::RemoveStreams()
{
  delete [] m_pSpuBuffer;
  m_pSpuBuffer = 0;
}



SPUThread::SPUThread(SPU *pSPU)
  : wxThread(wxTHREAD_JOINABLE), pSPU_(pSPU)
{
}


void* SPUThread::Entry()
{
  wxASSERT(pSPU_ != 0);
  SPU* pSPU = pSPU_;
  numSamples_ = 0;

  for (int i = 0; i < 24; i++) {
    pSPU->Channels.At(i).is_ready = false;
  }

  wxMessageOutputDebug().Printf(wxT("Started SPU thread."));

  pthread_mutex_t& process_mutex = pSPU->process_mutex_;
  pthread_mutex_t& wait_start_mutex = pSPU->wait_start_mutex_;
  pthread_cond_t& process_cond = pSPU->process_cond_;
  pthread_mutex_t& dma_writable_mutex = pSPU->dma_writable_mutex_;
  int& processing_channel = pSPU->processing_channel_;

  pthread_mutex_lock(&process_mutex);

  do {
    pthread_mutex_lock(&wait_start_mutex);
    SPU::ProcessState process_state = pSPU->process_state_;
    pSPU->process_state_ = SPU::STATE_START_PROCESS;
    pthread_cond_broadcast(&process_cond);
    pthread_mutex_unlock(&wait_start_mutex);

    switch (process_state) {
    case SPU::STATE_PSX_IS_READY:
      pthread_mutex_lock(&dma_writable_mutex);
      numSamples_ = processing_channel;
      while (0 < numSamples_) {
        --numSamples_;
        unsigned int ch_count = pSPU->Channels.channel_count();
        for (unsigned int i = 0; i < ch_count; i++) {
          ChannelInfo& channel = pSPU->Channels.At(i);
          if (channel.is_ready) continue;
          channel.Update();
        }
        pSPU->Channels.Notify();
        if (pSPU->NSSIZE <= ++pSPU->ns) {
          pSPU->ns = 0;
        }

        for (unsigned int i = 0; i < ch_count; i++) {
          pSPU->Channels.At(i).is_ready = false;
        }
      }
      wxASSERT(numSamples_ == 0);
      pthread_mutex_unlock(&dma_writable_mutex);
      break;

    case SPU::STATE_NOTE_ON:
      pSPU->Channels.At(processing_channel).tone->ConvertData();
      pSPU->Channels.At(processing_channel).Update();
      break;

    case SPU::STATE_NOTE_OFF:
      pSPU->Channels.At(processing_channel).Update();
      break;

    case SPU::STATE_SET_OFFSET:
      // pSPU->Channels[processing_channel].tone->ConvertData();
      break;

    default:
      break;
    }

    if (process_state == SPU::STATE_SHUTDOWN) break;

    pthread_cond_wait(&process_cond, &process_mutex);
  } while (true);

  pthread_mutex_unlock(&process_mutex);

  wxMessageOutputDebug().Printf(wxT("Terminated SPU thread."));
  return 0;
}



SPU::SPU(PSX::Composite *composite)
  : SPUBase(composite),
    Memory(reinterpret_cast<uint8_t*>(m_spuMem)), Channels(this, 24),
    reverb_(this), SoundBank_(this)
{
  Init();
}


void SPU::Init()
{
  // m_iXAPitch = 1;
  // m_iUseTimer = 2;
  // m_iDebugMode = 0;
  // m_iRecordMode = 0;
  m_iUseReverb = 2;
  useInterpolation = GAUSS_INTERPOLATION;
  // m_iDisStereo = 0;
  // m_iUseDBufIrq = 0;

  process_mutex_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  wait_start_mutex_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  process_cond_ = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
  dma_writable_mutex_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;

  //::memset(&Channels, 0, sizeof(Channels));
  ChannelInfo::InitADSR();

  wxMessageOutputDebug().Printf(wxT("Initialized SPU."));
}

void SPU::Open()
{
  // m_iUseXA = 1;
  // m_iVolume = 3;
  reverb_.iReverbOff = -1;
  // spuIrq = 0;
  dma_current_addr_ = 0xffffffff;
  m_bEndThread = 0;
  m_bThreadEnded = 0;
  m_pMixIrq = 0;

  // pSpuIrq = 0;
  // m_iSPUIRQWait = 1;

  ns = 0;

  SetupStreams();

  poo = 0;
  // m_pS = (short*)m_pSpuBuffer;

  if (thread_ == 0) {
    thread_ = new SPUThread(this);
    thread_->Create();
    process_state_ = STATE_NONE;
    thread_->Run();
  }

  wxMessageOutputDebug().Printf(wxT("Reset SPU."));
}

void SPU::Close()
{
  RemoveStreams();
  isPlaying_ = false;
}


void SPU::Shutdown()
{
  Close();
  thread_->Wait();
  wxMessageOutputDebug().Printf(wxT("Shut down SPU."));
}


bool SPU::IsRunning() const {
  return isPlaying_;
}


void SPU::Async(uint32_t cycles)
{
  int32_t do_samples;

  const int SPEED = 384;
  // 384 = PSXCLK / (44100*2)
  poo += cycles;
  do_samples = poo / SPEED;
  if (do_samples == 0) return;
  poo -= do_samples * SPEED;

  ChangeProcessState(STATE_PSX_IS_READY, do_samples);
}



// SPU SPU::Spu;

}   // namespace SPU

// SPU::SPU& Spu = SPU::SPU::Spu;
