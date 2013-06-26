#include "spu.h"
#include <wx/msgout.h>
#include <cstring>

#include "../psx/hardware.h"
#include "../app.h"

namespace {

inline void CLIP(int32_t& x) {
    if (x > 32767) x = 32767;
    if (x < -32767) x = -32767;
}

static int32_t poo;

}   // namespace


wxDEFINE_EVENT(wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);



namespace SPU {

wxDEFINE_SCOPED_PTR(InterpolationBase, InterpolationPtr)

ChannelInfo::ChannelInfo(SPU *pSPU) :
    pSPU_(pSPU), pInterpolation(new GaussianInterpolation(*this))
    // TODO: how to generate GaussianInterpolation
{
    is_ready = false;
}


void ChannelInfo::AddListener(ChannelInfoListener *listener) {
    listeners_.insert(listener);
}


void ChannelInfo::RemoveListener(ChannelInfoListener *listener) {
    listeners_.erase(listener);
}


void ChannelInfo::NotifyOnNoteOn() const {
    std::set<ChannelInfoListener*>::const_iterator itr = listeners_.begin();
    const std::set<ChannelInfoListener*>::const_iterator itrEnd = listeners_.end();
    while (itr != itrEnd) {
        (*itr)->OnNoteOn(*this);
        ++itr;
    }
}


void ChannelInfo::NotifyOnNoteOff() const {
    std::set<ChannelInfoListener*>::const_iterator itr = listeners_.begin();
    const std::set<ChannelInfoListener*>::const_iterator itrEnd = listeners_.end();
    while (itr != itrEnd) {
        (*itr)->OnNoteOff(*this);
        ++itr;
    }
}



void ChannelInfo::StartSound()
{
    wxASSERT(bNew == true);

    ADSRX.Start();
    Spu.Reverb.StartReverb(*this);

    // s_1 = 0;
    // s_2 = 0;
    // iSBPos = 28;

    itrTone = tone->Iterator(this);
    bNew = false;
    isStopped = false;
    isOn = true;

    pInterpolation->Start();

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

    if (is_muted) return;
    if (bNew) StartSound();
    if (isOn == false) return;

    if (iActFreq != iUsedFreq) {
        VoiceChangeFrequency();
    }

    int fa;
    while (pInterpolation->spos >= 0x10000) {
        if (itrTone.HasNext() == false) {
            isOn = false;
            // isStopped = true;
            ADSRX.lVolume = 0;
            ADSRX.EnvelopeVol = 0;
            wxGetApp().GetSoundManager()->SetEnvelopeVolume(ch, ADSRX.lVolume);
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

    sval = (MixADSR() * fa) / 1023;

    if (bFMod == 2) {
        // TODO: FM
    } else {
        int left = (sval * iLeftVolume) / 0x4000;
        int right = (sval * iRightVolume) / 0x4000;
/*
        Spu.StoreReverb(*this, 0);
        left += Spu.MixReverbLeft(0)/3;
        right += Spu.MixReverbRight()/3;
        left /= 3;
        right /= 3;
        CLIP(left);  CLIP(right);
 */
        wxGetApp().GetSoundManager()->SetEnvelopeVolume(ch, ADSRX.lVolume);
        wxGetApp().GetSoundManager()->WriteStereo(ch, left, right);
    }
    pInterpolation->spos += pInterpolation->GetSinc();
    // wxMessageOutputDebug().Printf("spos = 0x%08x", pInterpolation->spos);
}




int ChannelArray::GetChannelNumber() const
{
    return channelNumber_;
}




ChannelArray::ChannelArray(SPU *pSPU, int channelNumber)
    : pSPU_(pSPU), channels_(new ChannelInfo[channelNumber]), channelNumber_(channelNumber)
{
    for (int i = 0; i < channelNumber; i++) {
        channels_[i].pSPU_ = pSPU;
        channels_[i].ch = i;
        channels_[i].is_muted = false;
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
        if (!(flags & 1) || (channels_[i].tone->GetADPCM() == 0)) continue;
        channels_[i].bNew = true;
        // channels_[i].isStopped = false;
        channels_[i].useExternalLoop = false;

        pSPU_->ChangeProcessState(SPU::STATE_NOTE_ON, i);

        channels_[i].NotifyOnNoteOn();
        pSPU_->NotifyOnChangeLoopIndex(&channels_[i]);
    }
}


void ChannelArray::VoiceOff(uint32_t flags, int start)
{
    wxASSERT(flags < 0x01000000);
    for (int i = start; flags != 0; i++, flags >>= 1) {
        if ((flags & 1) == 0) continue;
        channels_[i].isStopped = true;

        pSPU_->ChangeProcessState(SPU::STATE_NOTE_OFF, i);

        channels_[i].NotifyOnNoteOff();
    }
}



ChannelInfo& ChannelArray::operator [](int i) {
    return channels_[i];
}




void SPUBase::AddListener(wxEvtHandler *listener)
{
    listeners_.insert(listener);
}


void SPUBase::RemoveListener(wxEvtHandler *listener)
{
    listeners_.erase(listener);
}


void SPUBase::NotifyOnUpdateStartAddress(int ch) const {
    pthread_mutex_lock(&process_mutex_);
    process_state_ = SPU::STATE_SET_OFFSET;
    processing_channel_ = ch;
    pthread_cond_broadcast(&process_cond_);
    pthread_mutex_unlock(&process_mutex_);
}


void SPUBase::NotifyOnChangeLoopIndex(ChannelInfo *pChannel) const
{
    wxCommandEvent event(wxEVENT_SPU_CHANNEL_CHANGE_LOOP);
    event.SetClientData(pChannel);

    const SPUListener::const_iterator itrEnd = listeners_.end();
    for (SPUListener::const_iterator itr = listeners_.begin(); itr != itrEnd; ++itr) {
        (*itr)->AddPendingEvent(event);
    }
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
    return SoundBank_.GetSamplingTone(addr);
}




void SPU::SetupStreams()
{
    m_pSpuBuffer = new uint8_t[32768];

    SoundBank_.Reset();
    Reverb.Reset();

    for (int i = 0; i < 24; i++) {
        Channels[i].ADSRX.SustainLevel = 0xf << 27;
        // Channels[i].iIrqDone = 0;
        Channels[i].tone = 0;
        Channels[i].itrTone = SamplingToneIterator();
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
        pSPU->Channels[i].is_ready = false;
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
                for (int i = 0; i < 24; i++) {
                    ChannelInfo& channel = pSPU->Channels[i];
                    if (channel.is_ready) continue;
                    channel.Update();
                }
                wxGetApp().GetSoundManager()->Flush();

                for (int i = 0; i < 24; i++) {
                    pSPU->Channels[i].is_ready = false;
                }
            }
            wxASSERT(numSamples_ == 0);
            pthread_mutex_unlock(&dma_writable_mutex);
            break;

        case SPU::STATE_NOTE_ON:
            pSPU->Channels[processing_channel].Update();
            break;

        case SPU::STATE_NOTE_OFF:
            pSPU->Channels[processing_channel].Update();
            break;

        case SPU::STATE_SET_OFFSET:
            pSPU->Channels[processing_channel].tone->ConvertData();
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



SPU::SPU() :
    Memory(reinterpret_cast<uint8_t*>(m_spuMem)), Channels(this, 24),
    Reverb(this), SoundBank_(this)
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
    Reverb.iReverbOff = -1;
    // spuIrq = 0;
    Addr = 0xffffffff;
    m_bEndThread = 0;
    m_bThreadEnded = 0;
    m_pMixIrq = 0;

    // pSpuIrq = 0;
    // m_iSPUIRQWait = 1;

    SetupStreams();

    poo = 0;
    m_pS = (short*)m_pSpuBuffer;

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

    // 384 = PSXCLK / (44100*2)
    poo += cycles;
    do_samples = poo / 384;
    if (do_samples == 0) return;
    poo -= do_samples * 384;

    ChangeProcessState(STATE_PSX_IS_READY, do_samples);
}



SPU SPU::Spu;

}   // namespace SPU

SPU::SPU& Spu = SPU::SPU::Spu;
