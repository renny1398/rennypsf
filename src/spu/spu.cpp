#include "spu/spu.h"
#include <wx/msgout.h>
#include <wx/hashmap.h>
#include <cstring>

#include "psx/hardware.h"
#include "SoundManager.h"

namespace {

inline int32_t CLIP(int32_t x) {
  if (x > 32767) return 32767;
  if (x < -32768) return -32768;
  return x;
}

int32_t poo;

}   // namespace


wxDEFINE_EVENT(wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);



namespace SPU {


////////////////////////////////////////////////////////////////////////
// SPU Request class implement
////////////////////////////////////////////////////////////////////////


namespace {
WX_DECLARE_HASH_MAP(int, SPURequest*, wxIntegerHash, wxIntegerEqual, RequestMap);
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
    for (int i = 0; i < core_count; i++) {
      SPUCore& core = p_spu->core(i);
      core.Step();
    }
    p_spu->NotifyObservers();
    p_spu->ResetStepStatus();
  }
}


const SPURequest* SPUNoteOnRequest::CreateRequest(int ch) {
  if (24 <= ch) {
    wxMessageOutputDebug().Printf(wxT("Warning: invalid channel number %d."), ch);
    return NULL;
  }

  static RequestMap pool(1);
  RequestMap::const_iterator itr = pool.find(ch);
  if (itr == pool.end()) {
    SPURequest* req = new SPUNoteOnRequest(ch);
    pool.insert(RequestMap::value_type(ch, req));
    return req;
  }
  return itr->second;
}


void SPUNoteOnRequest::Execute(SPUBase* p_spu) const {
  // p_spu->Voice(ch_).tone->ConvertData();
  p_spu->Voice(ch_).Step();
}


const SPURequest* SPUNoteOffRequest::CreateRequest(int ch) {
  static RequestMap pool(1);
  RequestMap::const_iterator itr = pool.find(ch);
  if (itr == pool.end()) {
    SPURequest* req = new SPUNoteOffRequest(ch);
    pool.insert(RequestMap::value_type(ch, req));
    return req;
  }
  return itr->second;
}


void SPUNoteOffRequest::Execute(SPUBase* p_spu) const {
   p_spu->Voice(ch_).Step();
}


const SPURequest* SPUSetOffsetRequest::CreateRequest(int ch) {
  static RequestMap pool(1);
  RequestMap::const_iterator itr = pool.find(ch);
  if (itr == pool.end()) {
    SPURequest* req = new SPUSetOffsetRequest(ch);
    pool.insert(RequestMap::value_type(ch, req));
    return req;
  }
  return itr->second;
}


void SPUSetOffsetRequest::Execute(SPUBase* p_spu) const {
  // p_spu->Voice(ch_).tone->ConvertData();
}


////////////////////////////////////////////////////////////////////////
// SPU Core class implement
////////////////////////////////////////////////////////////////////////


void SPUCore::Step() {
  voice_manager_.StepForAll();
}


////////////////////////////////////////////////////////////////////////
// SPU class implement
////////////////////////////////////////////////////////////////////////


SPUBase::SPUBase(PSX::Composite* composite)
  : Component(composite), soundbank_(), reverb_(this) {
  cores_.assign(1, SPUCore(this));
  // new(&cores_[0]) SPUCore(this);

  mem8_.reset(new uint8_t[0x100000 * 2]);  // 1MB or 2MB
  ::memset(mem8_.get(), 0, 0x100000 * 2);
  p_mem16_ = (uint16_t*)mem8_.get();

  Init();
}


void SPUBase::NotifyOnUpdateStartAddress(int ch) const {
/*
  pthread_mutex_lock(&process_mutex_);
  process_state_ = SPUBase::STATE_SET_OFFSET;
  processing_channel_ = ch;
  pthread_cond_broadcast(&process_cond_);
  pthread_mutex_unlock(&process_mutex_);
*/
  const SPURequest* req = SPUSetOffsetRequest::CreateRequest(ch);
  thread_->PutRequest(req);
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


bool SPUBase::Step(int step_count) {
  if (thread_ == 0 || thread_->IsRunning() == false) {
    return false;
  }
  thread_->WaitForLastStep();
  const SPURequest* req = SPUStepRequest::CreateRequest(step_count);
  thread_->PutRequest(req);
  return true;
}


void SPUBase::NotifyObservers() {
  const wxVector<SPUCore>::iterator itr_end = cores_.end();
  for (wxVector<SPUCore>::iterator itr = cores_.begin(); itr != itr_end; ++itr) {
    itr->Voices().Notify();
  }
}


void SPUBase::ResetStepStatus() {
  const wxVector<SPUCore>::iterator itr_end = cores_.end();
  for (wxVector<SPUCore>::iterator itr = cores_.begin(); itr != itr_end; ++itr) {
    itr->Voices().ResetStepStatus();
  }
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
  m_pSpuBuffer = new uint8_t[32768];

  soundbank_.Clear();
  reverb_.Reset();

  for (int i = 0; i < 24; i++) {
    SPUVoice& ch = Voice(i);
    // ch.ADSRX.SustainLevel = 0xf << 27;
    // Channels[i].iIrqDone = 0;
    ch.tone = 0;
    ch.itrTone = InstrumentDataIterator();
  }
}

void SPUBase::RemoveStreams()
{
  delete [] m_pSpuBuffer;
  m_pSpuBuffer = 0;
}


void SPUBase::PutRequest(const SPURequest *req) {
  if (thread_ == NULL) return;
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
      wxMessageOutputDebug().Printf(wxT("SPUThread::WaitForLastStep(): Warning: waiting time is out."));
    }
  }
}


wxThread::ExitCode SPUThread::Entry()
{
  wxASSERT(pSPU_ != 0);
  SPUBase* p_spu = pSPU_;
  // numSamples_ = 0;

  for (int i = 0; i < 24; i++) {
    p_spu->Voice(i).is_ready = false;
  }

  wxMessageOutputDebug().Printf(wxT("Started SPU thread."));

  do {
    queue_mutex_.Lock();
    while (req_queue_.empty()) {
      queue_cond_.Wait();
    }
    queue_mutex_.Unlock();

    const SPURequest* const p_req = req_queue_.front();
    if (p_req == NULL) {
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
  wxMessageOutputDebug().Printf(wxT("Terminated SPU thread."));
}


void SPUBase::Init()
{
  // m_iXAPitch = 1;
  // m_iUseTimer = 2;
  // m_iDebugMode = 0;
  // m_iRecordMode = 0;
  useInterpolation = GAUSS_INTERPOLATION;
  // m_iDisStereo = 0;
  // m_iUseDBufIrq = 0;

/*
  process_mutex_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  wait_start_mutex_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
  process_cond_ = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
  dma_writable_mutex_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
*/
  //::memset(&Channels, 0, sizeof(Channels));
  SPUVoice::InitADSR();

  thread_ = 0;

  wxMessageOutputDebug().Printf(wxT("Initialized SPU."));
}

void SPUBase::Open()
{
  // m_iUseXA = 1;
  // m_iVolume = 3;
  reverb_.iReverbOff = -1;
  // spuIrq = 0;
  core(0).addr_ = 0xffffffff;
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
    // process_state_ = STATE_NONE;
    thread_->Run();
  }

  wxMessageOutputDebug().Printf(wxT("Reset SPU."));
}

void SPUBase::Close()
{
  if (thread_ != 0 && thread_->IsRunning()) {
    thread_->PutRequest(NULL);
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
  wxMessageOutputDebug().Printf(wxT("Shut down SPU."));
  wxMessageOutputDebug().Printf(wxT("thread = %p"), thread_);
}


bool SPUBase::IsRunning() const {
  return isPlaying_;
}


/*
void SPUBase::Async(uint32_t cycles)
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
*/


SPU2::SPU2(PSX::Composite *composite)
  : SPUBase(composite) {}


}   // namespace SPU
