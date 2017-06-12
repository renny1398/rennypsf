#include "psf/spu/spu.h"
#include "psf/psx/psx.h"
#include "common/debug.h"
#include <wx/hashmap.h>
#include <cstring>

#include "psf/psx/hardware.h"
#include "common/SoundManager.h"


namespace {
/*
inline int32_t CLIP(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return x;
}
*/
int32_t poo;

}   // namespace


wxDEFINE_EVENT(wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);



namespace SPU {


////////////////////////////////////////////////////////////////////////
// SPU Request class implement
////////////////////////////////////////////////////////////////////////


namespace {
WX_DECLARE_HASH_MAP(long, SPURequest*, wxIntegerHash, wxIntegerEqual, RequestMap);
}


const SPURequest* SPUStepRequest::CreateRequest(int step_count) {
  static RequestMap pool(1);
  RequestMap::const_iterator itr = pool.find(step_count);
  if (itr == pool.end()) {
    SPURequest* req = new SPUStepRequest(step_count);
    pool.insert(RequestMap::value_type(step_count, req));
    return req;
  }
  return itr->second;
}


void SPUStepRequest::Execute(SPUBase* p_spu) const {
  int step = step_count_;
  while (step--) {
    const int core_count = p_spu->core_count();
    REVERBInfo& rvb = p_spu->Reverb();
    for (int i = 0; i < core_count; i++) {
      SPUCore& core = p_spu->core(i);
      core.Advance();

      // rvb.ClearReverb();
      for (unsigned int j = 0; j < 24; j++) {
        SPUVoice& ch = core.Voice(j);
        if (ch.bRVBActive == true) {
          rvb.StoreReverb(ch);
        }
      }
      rvb.Mix();
    }
  }
}


const SPURequest* SPUNoteOnRequest::CreateRequest(SPUVoice* p_voice) {
  static RequestMap pool(1);
  int pool_index = reinterpret_cast<long>(p_voice);  // WARNING
  RequestMap::const_iterator itr = pool.find(pool_index);
  if (itr == pool.end()) {
    SPURequest* req = new SPUNoteOnRequest(p_voice);
    pool.insert(RequestMap::value_type(pool_index, req));
    return req;
  }
  return itr->second;
}


void SPUNoteOnRequest::Execute(SPUBase*) const {
  // p_spu->Voice(ch_).tone->ConvertData();
  p_voice_->Advance();
}


const SPURequest* SPUNoteOffRequest::CreateRequest(SPUVoice* p_voice) {
  static RequestMap pool(1);
  int pool_index = reinterpret_cast<long>(p_voice);  // WARNING
  RequestMap::const_iterator itr = pool.find(pool_index);
  if (itr == pool.end()) {
    SPURequest* req = new SPUNoteOffRequest(p_voice);
    pool.insert(RequestMap::value_type(pool_index, req));
    return req;
  }
  return itr->second;
}

void SPUNoteOffRequest::Execute(SPUBase*) const {
  p_voice_->Advance();
}


const SPURequest* SPUSetOffsetRequest::CreateRequest(SPUVoice* p_voice) {
  static RequestMap pool(1);
  int pool_index = reinterpret_cast<long>(p_voice);  // WARNING
  RequestMap::const_iterator itr = pool.find(pool_index);
  if (itr == pool.end()) {
    SPURequest* req = new SPUSetOffsetRequest(p_voice);
    pool.insert(RequestMap::value_type(pool_index, req));
    return req;
  }
  return itr->second;
}


void SPUSetOffsetRequest::Execute(SPUBase*) const {
  // p_spu->Voice(ch_).tone->ConvertData();
}


////////////////////////////////////////////////////////////////////////
// SPU Core class implement
////////////////////////////////////////////////////////////////////////

SPUCore::SPUCore()
  : p_spu_(nullptr), voice_manager_(this, 0),
    dma_delay_(0), dma_callback_routine_(0), dma_flag_(0) {}

SPUCore::SPUCore(SPUBase* spu)
  : p_spu_(spu), voice_manager_(this, 24),
    dma_delay_(0), dma_callback_routine_(0), dma_flag_(0) {
  rennyAssert(spu != nullptr);
}

void SPUCore::Advance() {
  DecreaseDMADelay();
  voice_manager_.Advance();
}


////////////////////////////////////////////////////////////////////////
// SPU class implement
////////////////////////////////////////////////////////////////////////

SPUBase::SPUBase(psx::PSX* composite)
  : Component(composite), UserMemoryAccessor(composite),
    Get(&SPUBase::GetAsync),
    p_psx_(composite),
    soundbank_(), reverb_(this),
    cores_(composite->version(), SPUCore()),
    voice_manager_(this) {

  uint32_t core_num = composite->version();
  rennyAssert(core_num == 1 || core_num == 2);
  // if (core_num < 1 || 2 < core_num) {
  //   core_num = 1;
  // }

  for (uint32_t i = 0; i < core_num; ++i) {
    new(&cores_.at(i)) SPUCore(this);
  }

  switch (core_num) {
  case 2:
    default_sampling_rate_ = 48000;
    output_sampling_rate_ = 48000;
    break;
  default:
    default_sampling_rate_ = 44100;
    output_sampling_rate_ = 44100;
  }


  mem8_.reset(new uint8_t[0x100000 * core_num]);  // 1MB or 2MB
  ::memset(mem8_.get(), 0, 0x100000 * core_num);
  p_mem16_ = (uint16_t*)mem8_.get();

  Init();
}


uint32_t SPUBase::GetDefaultSamplingRate() const {
  return default_sampling_rate_;
}

uint32_t SPUBase::GetCurrentSamplingRate() const {
  return output_sampling_rate_;
}

void SPUBase::ChangeOutputSamplingRate(uint32_t rate) {
  output_sampling_rate_ = rate;
}



void SPUBase::NotifyOnUpdateStartAddress(int ch) const {
/*
  pthread_mutex_lock(&process_mutex_);
  process_state_ = SPUBase::STATE_SET_OFFSET;
  processing_channel_ = ch;
  pthread_cond_broadcast(&process_cond_);
  pthread_mutex_unlock(&process_mutex_);
*/
  // const SPURequest* req = SPUSetOffsetRequest::CreateRequest(ch);
  // thread_->PutRequest(req);
}


void SPUBase::NotifyOnChangeLoopIndex(SPUVoice* /*pChannel*/) const
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


/*
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
*/


bool SPUBase::Advance(int step_count) {
  const SPURequest* req = SPUStepRequest::CreateRequest(step_count);
  thread_->PutRequest(req);
  return true;
}


void SPUBase::NotifyObservers() {
/*  const wxVector<SPUCore>::iterator itr_end = cores_.end();
  for (wxVector<SPUCore>::iterator itr = cores_.begin(); itr != itr_end; ++itr) {
    itr->Voices().NotifyDevice();
  }*/
}


bool SPUBase::IsAsync() const {
  return thread_ != nullptr;
}

/*
bool SPUBase::GetSync(SoundBlock* dest) {

  const wxVector<SPUCore>::iterator core_it_end = cores_.end();
  for (wxVector<SPUCore>::iterator core_it = cores_.begin(); core_it != core_it_end; ++core_it) {

    for (unsigned int j = 0; j < 24; j++) {
      SPUVoice& ch = core_it->Voice(j);
      ch.Step();
      ch.Get(&dest->Ch(j));
    }

    for (unsigned int j = 0; j < 24; j++) {
      SPUVoice& ch = core_it->Voice(j);
      if (ch.bRVBActive == true) {
        Reverb().StoreReverb(ch);
      }
    }
    Reverb().Mix();

    Sample& rvb_left = dest->ReverbCh(0);
    Sample& rvb_right = dest->ReverbCh(1);
    rvb_left.Set16(Reverb().GetLeft());
    rvb_right.Set16(Reverb().GetRight());
  }

  return true;
}
*/

bool SPUBase::GetAsync(SoundBlock* dest) {
  if (thread_ == 0 || thread_->IsRunning() == false) return false;
  if (dest == nullptr) { dest = out_; }
  // thread_->WaitForLastStep();
  for (auto& core : cores_) {
    for (unsigned int i = 0; i < 24; i++) {
      SPUVoice& ch = core.Voice(i);
      ch.Get(&dest->Ch(i));
    }
    SampleSequence& rvb_left = dest->ReverbCh(0);
    SampleSequence& rvb_right = dest->ReverbCh(1);
    rvb_left.Push16i(Reverb().GetLeft());
    rvb_right.Push16i(Reverb().GetRight());
    const SPURequest* req = SPUStepRequest::CreateRequest(1);
    thread_->PutRequest(req);
  }
  return true;
}


SPUInstrument_New *SPUBase::GetSamplingTone(uint32_t addr) const
{
  Soundbank& soundbank = const_cast<Soundbank&>(soundbank_);
  SPUInstrument_New& p_inst = dynamic_cast<SPUInstrument_New&>(soundbank.instrument(addr / 16));
  return &p_inst;
}


void SPUBase::NotifyOnAddTone(const SPUInstrument_New & /*tone*/) const {
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


void SPUBase::NotifyOnChangeTone(const SPUInstrument_New & /*tone*/) const {
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


void SPUBase::NotifyOnRemoveTone(const SPUInstrument_New & /*tone*/) const {
/*
  ToneInfo tone_info;
  tone_info.number = tone.GetAddr();
  sound_driver_->OnRemoveTone(tone_info);
  */
}


void SPUBase::SetupStreams()
{
  // m_pSpuBuffer = new uint8_t[32768];

  ::memset(mem8_.get(), 0, 0x100000 * cores_.size());

  soundbank_.Clear();
  reverb_.Reset();

  for (int i = 0; i < 24; i++) {
    SPUVoice& ch = Voice(i);
    // ch.ADSRX.SustainLevel = 0xf << 27;
    // Channels[i].iIrqDone = 0;
    ch.tone = 0;
    ch.itrTone = InstrumentDataIterator();
    ch.sval = 0;
    ch.hasReverb = false;
    ch.bRVBActive = false;
  }
}

void SPUBase::RemoveStreams()
{
  // delete [] m_pSpuBuffer;
  // m_pSpuBuffer = 0;
}


void SPUBase::PutRequest(const SPURequest *req) {
  if (thread_ == nullptr) return;
  thread_->PutRequest(req);
}


SPUThread::SPUThread(SPUBase *pSPU)
  : wxThread(wxTHREAD_JOINABLE), pSPU_(pSPU), queue_cond_(queue_mutex_)
{
}


void SPUThread::PutRequest(const SPURequest *req) {
  wxMutexLocker locker(queue_mutex_);
  req_queue_.push_back(req);
  queue_cond_.Broadcast();
}


void SPUThread::WaitForLastStep() {
  wxMutexLocker locker(queue_mutex_);
  while (req_queue_.empty() == false) {
    if (queue_cond_.WaitTimeout(1000) == wxCOND_TIMEOUT) {
      rennyLogWarning("SPUThread", "WaitForLastStep(): waiting time is out.");
    }
  }
}


wxThread::ExitCode SPUThread::Entry()
{
  rennyAssert(pSPU_ != 0);
  SPUBase* p_spu = pSPU_;
  // numSamples_ = 0;

  /*
  for (int i = 0; i < 24; i++) {
    p_spu->Voice(i).SetUnready();
  }
  */

  rennyLogDebug("SPUThread", "Started SPU thread.");

  do {
    queue_mutex_.Lock();
    while (req_queue_.empty()) {
      queue_cond_.Wait();
    }
    queue_mutex_.Unlock();

    const SPURequest* const p_req = req_queue_.front();
    if (p_req == nullptr) {
      queue_mutex_.Lock();
      req_queue_.pop_front();
      queue_cond_.Broadcast();
      queue_mutex_.Unlock();
      return 0;
    }
    p_req->Execute(p_spu);

    queue_mutex_.Lock();
    req_queue_.pop_front();
    queue_cond_.Broadcast();
    queue_mutex_.Unlock();
  } while (true);
}


void SPUThread::OnExit() {
  rennyLogDebug("SPUThread", "Terminated SPU thread.");
}


void SPUBase::Init()
{
  useInterpolation = GAUSS_INTERPOLATION;
  SPUVoice::InitADSR();

  thread_ = 0;

  rennyLogDebug("SPU", "Initialized SPU.");
}



void SPUBase::Open()
{
  reverb_.iReverbOff = -1;
  core(0).addr_ = 0xffffffff;
  m_pMixIrq = 0;

  ns = 0;

  SetupStreams();

  poo = 0;

  if (thread_ == 0) {
    thread_ = new SPUThread(this);
    thread_->Create();
    thread_->Run();
    rennyLogDebug("SPU", "Create a thread.");
  }

  rennyLogDebug("SPU", "Reset SPU.");
}

void SPUBase::Close()
{
  if (thread_ != 0 && thread_->IsRunning()) {
    thread_->PutRequest(nullptr);
    thread_->Wait();
    delete thread_;
    thread_ = 0;
  }
  RemoveStreams();
  isPlaying_ = false;
}


void SPUBase::Shutdown()
{
  Close();
  mem8_.reset();
  rennyLogDebug("SPU", "Shut down SPU.");
}


bool SPUBase::IsRunning() const {
  return isPlaying_;
}

/*
SPU2::SPU2(psx::PSX *composite)
  : SPUBase(composite), cores_(this) {}
*/

}   // namespace SPU
