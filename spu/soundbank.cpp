#include "soundbank.h"
#include "spu.h"
#include "../psx/psx.h"

#include <cmath>
#include <stdexcept>


wxDEFINE_EVENT(wxEVENT_SPU_ADD_TONE, wxCommandEvent);
wxDEFINE_EVENT(wxEVENT_SPU_MODIFY_TONE, wxCommandEvent);
wxDEFINE_EVENT(wxEVENT_SPU_REMOVE_TONE, wxCommandEvent);


namespace SPU {


SamplingTone::SamplingTone(const SoundBank *pSB, uint8_t *pADPCM) :
    pSB_(pSB), pADPCM_(pADPCM), indexLoop_(0xffffffff), forcesStop_(false),
    processedBlockNumber_(0), freq_(0.0), begin_(this), prev1_(0), prev2_(0)
{
    wxASSERT(pADPCM_ != 0);
}


const uint8_t* SamplingTone::GetADPCM() const
{
    return pADPCM_;
}


const int32_t* SamplingTone::GetData() const
{
    return &LPCM_[0];
}


uint32_t SamplingTone::GetSPUOffset() const
{
    return ( pADPCM_ - pSB_->GetSPU()->GetSoundBuffer() );
}


uint32_t SamplingTone::GetLength() const
{
    const uint32_t len = LPCM_.size();
    // wxASSERT(len % 28 == 0);
    return len;
}


uint32_t SamplingTone::GetLoopIndex() const
{
    wxASSERT(indexLoop_ % 28 == 0 || indexLoop_ == 0xffffffff);
    return indexLoop_;
}


double SamplingTone::GetFreq() const
{
    return freq_;
}


void SamplingTone::SetFreq(double f)
{
    freq_ = f;
}


bool SamplingTone::hasFinishedConv() const
{
    return (0x80000000 <= processedBlockNumber_);
}


int SamplingTone::At(int index) const
{
    int len = GetLength();

    if (index < len) {
        return LPCM_.at(index);
    }

    if (hasFinishedConv()) {
        wxMessageOutputDebug().Printf(wxT("WARNING: hasFinishedConv"));
    }

    wxASSERT(static_cast<uint32_t>(len) == processedBlockNumber_ * 28);
    for (int i = len; i <= index; i += 28) {
        const_cast<SamplingTone*>(this)->ADPCM2LPCM();
        if (hasFinishedConv()) break;
    }

    return LPCM_.at(index);
}


const SoundBank* SamplingTone::GetSoundBank() const
{
    return pSB_;
}


void SamplingTone::ADPCM2LPCM()
{
    wxASSERT(hasFinishedConv() == false);

    static const int xa_adpcm_table[5][2] = {
        {   0,   0 },
        {  60,   0 },
        { 115, -52 },
        {  98, -55 },
        { 122, -60 }
    };

    // SPU* pSpu = pSB_->GetSPU();

    int prev1 = prev1_;
    int prev2 = prev2_;
    // wxMessageOutputDebug().Printf(wxT("pADPCM = %p, block number = %d"), pADPCM_, processedBlockNumber_);
    const uint8_t* p = pADPCM_ + processedBlockNumber_ * 16;
    // wxMessageOutputDebug().Printf(wxT("p = %p"), p);
    int predict_nr = *p++;
    int shift_factor = predict_nr & 0xf;
    int flags = *p++;
    predict_nr >>= 4;

    int d, s, fa;

    for (int i = 0; i < 14; i++) {
        d = *p++;

        s = (d & 0xf) << 12;
        if (s & 0x8000) s |= 0xffff0000;
        fa = (s >> shift_factor);
        fa += ((prev1 * xa_adpcm_table[predict_nr][0]) >> 6) + ((prev2 * xa_adpcm_table[predict_nr][1]) >> 6);
        prev2 = prev1; prev1 = fa;

        LPCM_.push_back(fa);

        s = (d & 0xf0) << 8;
        if (s & 0x8000) s |= 0xffff0000;
        fa = (s >> shift_factor);
        fa += ((prev1 * xa_adpcm_table[predict_nr][0]) >> 6) + ((prev2 * xa_adpcm_table[predict_nr][1]) >> 6);
        prev2 = prev1; prev1 = fa;

        LPCM_.push_back(fa);
    }
    ++processedBlockNumber_;
    wxASSERT(p == pADPCM_ + processedBlockNumber_ * 16);

    /*
    const uint8_t* pLoop = indexLoop_ ? pADPCM_ + (indexLoop_ / 28 * 16) : 0;
    if (pSpu->Sp0 & 0x40) {
        if ( (p-16 < pSpu->m_pSpuIrq && pSpu->m_pSpuIrq <= p) || ((flags & 1) && (pLoop-16 < pSpu->m_pSpuIrq && pSpu->m_pSpuIrq <= pLoop)) ) {
            // iIrqDone = 1;
            PSX::u32Href(0x1070) |= BFLIP32(0x200);
        }
    }
    */

    if ( flags & 4 ) {
        // pLoop = start - 16;
        indexLoop_ = (processedBlockNumber_ - 1) * 28;
        pSB_->NotifyOnModify(this);
    } else if ( flags & 1 ) {
        // the end of this tone
        pSB_->NotifyOnModify(this);

        if (flags != 3) {
            forcesStop_ = true;
        }

        processedBlockNumber_ = 0xffffffff;
    }

    prev1_ = prev1;
    prev2_ = prev2;
}


void SamplingTone::ConvertData()
{
     while (processedBlockNumber_ < 0x80000000) {
        ADPCM2LPCM();
    }
     Spu.SoundBank_.FourierTransform(this);
}


SamplingToneIterator SamplingTone::Iterator(ChannelInfo *pChannel) const
{
    // clone the 'begin' iterator
    return SamplingToneIterator(const_cast<SamplingTone*>(this), pChannel);
}



SamplingToneIterator::SamplingToneIterator(SamplingTone *pTone, ChannelInfo *pChannel) :
    pTone_(pTone), pChannel_(pChannel), index_(0)
{}


void SamplingToneIterator::clone(SamplingToneIterator *itrTone) const
{
    itrTone->pTone_ = pTone_;
    itrTone->pChannel_ = pChannel_;
    itrTone->index_ = index_;
}


SamplingToneIterator::SamplingToneIterator(const SamplingToneIterator &itr)
{
    itr.clone(this);
}


SamplingToneIterator& SamplingToneIterator::operator=(const SamplingToneIterator& itr)
{
    itr.clone(this);
    return *this;
}


SamplingTone* SamplingToneIterator::GetTone() const
{
    return pTone_;
}


uint32_t SamplingToneIterator::GetLoopIndex() const
{
    if (pTone_ == 0 || pTone_->forcesStop_ == true) {
        return 0xffffffff;
    }

    uint32_t indexLoop;
    const uint32_t offsetExternalLoop = pChannel_->offsetExternalLoop;
    const uint32_t inLoop = pTone_->GetLoopIndex();
    const uint32_t exLoop = (offsetExternalLoop - pTone_->GetSPUOffset()) * 28 / 16;

    if (pChannel_->useExternalLoop) {
        indexLoop = exLoop;
    } else if (0x80000000 <= inLoop) {
        if (0x80000000 <= offsetExternalLoop) {
            return 0xffffffff;
        }
        indexLoop = exLoop;
    } else {
        indexLoop = inLoop;
    }

    return indexLoop;
}




bool SamplingToneIterator::HasNext() const
{
    if (pTone_ == 0) {
        return false;
    }

    const uint32_t len = pTone_->GetLength();
    if (index_ < len) {
        return true;
    }
    if (pTone_->hasFinishedConv() == false) {
        return true;
    }

    uint32_t indexLoop = GetLoopIndex();
    if (0x80000000 <= indexLoop) {
        return false;
    }

    uint32_t lenLoop = len - indexLoop;
    if (0x80000000 <= lenLoop) {
        return false;
    }

    // wxMessageOutputDebug().Printf(wxT("Loop index = %d, Loop length = %d"), indexLoop, lenLoop);
    while (pTone_->GetLength() <= index_) {
        index_ -= lenLoop;
    }
    return true;
}


int SamplingToneIterator::Next()
{
    return pTone_->At(index_++);
}


SamplingToneIterator& SamplingToneIterator::operator +=(int n) {
    index_ += n;
    return *this;
}


////////////////////////////////////////////////////////////////////////
// Fourier Transformer
////////////////////////////////////////////////////////////////////////


FourierTransformer::FourierTransformer()
{
    thread_ = 0;
    mutexQueue_ = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
    condQueue_ = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    requiresShutdown_ = false;
}


FourierTransformer::~FourierTransformer()
{
    if (thread_) {
        requiresShutdown_ = true;
        pthread_cond_broadcast(&condQueue_);
        pthread_join(thread_, 0);
    }
}


void FourierTransformer::PostTransform(SamplingTone* tone, int sampling_rate)
{
    if (0.0 < tone->GetFreq()) return;

    pthread_mutex_lock(&mutexQueue_);
    queue_.push_back(tone);
    sampling_rate_ = sampling_rate;
    pthread_cond_broadcast(&condQueue_);
    pthread_mutex_unlock(&mutexQueue_);
    if (thread_ == 0) {
        pthread_create(&thread_, 0, mainLoop, this);
    }
    wxMessageOutputDebug().Printf(wxT("Post FFT (offset = %d)"), tone->GetSPUOffset());
}


extern "C" void cdft(int, int, double *, int *, double *);

void* FourierTransformer::mainLoop(void *param)
{
    FourierTransformer *ft = (FourierTransformer*)param;
    SamplingTone* tone = 0;

    double* a = 0;
    int* ip = 0;
    double* w = 0;
    int np = 0;

    do {
        do {
            pthread_mutex_lock(&ft->mutexQueue_);
            if (ft->queue_.empty()) {
                pthread_mutex_unlock(&ft->mutexQueue_);
                break;
            }
            tone = ft->queue_.front();
            ft->queue_.pop_front();
            pthread_mutex_unlock(&ft->mutexQueue_);

            // const int32_t *p = tone->GetData();
            int len = tone->GetLength();
            int n = (int) pow( 2.0, floor(log(len) / log(2.0)) );

            if (np < n) {
                if (np) {
                    delete [] w;
                    delete [] ip;
                    delete [] a;
                }
                np = n;
                a = new double[np*2];
                ip = new int[2+(int)sqrt(np)];
                w = new double[np];
                ip[0] = 0;
            }

            const int dn = len - n;
            for (int i = 0; i < n; i++) {
                a[i] = (double)tone->At(dn+i);
            }

            cdft(n, -1, a, ip, w);

            double valMax = 0.0;
            int indexMax = 0;
            for (int i = 0; i < n/4; i++) {
                const double re = a[2*i];
                const double im = a[2*i+1];
                const double val = re*re + im*im;
                if (valMax < val) {
                    valMax = val;
                    indexMax = i;
                }
            }
/*
            while (0 < indexMax) {
                const int n = indexMax / 2;
                const double re = a[2*n];
                const double im = a[2*n+1];
                const double val = re*re + im*im;
                if (val <= 7*valMax/8) break;
                valMax = val;
                indexMax = n;
            }
*/
            double freq = static_cast<double>(ft->sampling_rate_) / n;
            tone->SetFreq(freq);
            Spu.SoundBank_.NotifyOnModify(tone);
            wxMessageOutputDebug().Printf(wxT("Finished FFT (offset = %d, freq = %d)"), tone->GetSPUOffset(), indexMax);
        } while (true);

        if (ft->requiresShutdown_) break;

        pthread_mutex_lock(&ft->mutexQueue_);
        pthread_cond_wait(&ft->condQueue_, &ft->mutexQueue_);
        pthread_mutex_unlock(&ft->mutexQueue_);

    } while (ft->requiresShutdown_ == false);

    if (np) {
        delete [] w;
        delete [] ip;
        delete [] a;
    }

    return 0;
}


SoundBank::SoundBank(SPU *pSPU) : pSPU_(pSPU)
{
    Init();
}


SoundBank::~SoundBank() {}


void SoundBank::Init() {

}

void SoundBank::Shutdown() {

}


void SoundBank::NotifyOnAdd(SamplingTone *tone) const
{
    wxCommandEvent event(wxEVENT_SPU_ADD_TONE, wxID_ANY);
    event.SetClientData(tone);

    std::set<wxEvtHandler*>::const_iterator itrEnd = listeners_.end();
    for (std::set<wxEvtHandler*>::const_iterator itr = listeners_.begin(); itr != itrEnd; ++itr) {
        (*itr)->AddPendingEvent(event);
    }
}


void SoundBank::NotifyOnModify(SamplingTone *tone) const
{
    wxCommandEvent event(wxEVENT_SPU_MODIFY_TONE, wxID_ANY);
    event.SetClientData(tone);

    std::set<wxEvtHandler*>::const_iterator itrEnd = listeners_.end();
    for (std::set<wxEvtHandler*>::const_iterator itr = listeners_.begin(); itr != itrEnd; ++itr) {
        (*itr)->AddPendingEvent(event);
    }
}


void SoundBank::NotifyOnRemove(SamplingTone *tone) const
{
    wxCommandEvent event(wxEVENT_SPU_REMOVE_TONE, wxID_ANY);
    event.SetClientData(tone);

    std::set<wxEvtHandler*>::const_iterator itrEnd = listeners_.end();
    for (std::set<wxEvtHandler*>::const_iterator itr = listeners_.begin(); itr != itrEnd; ++itr) {
        (*itr)->ProcessEvent(event);
    }
}


SamplingTone* SoundBank::GetSamplingTone(uint32_t addr) const
{
    SamplingToneMap::Iterator it = tones_.find(addr);
    if (it != tones_.end()) {
        return it.m_node->m_value.second;
    }
    SamplingTone* tone = new SamplingTone(this, pSPU_->GetSoundBuffer() + addr);
    // tones_.insert(addr, tone);
    tones_[addr] = tone;
    const_cast<SoundBank*>(this)->NotifyOnAdd(tone);
    return tone;
}



void SoundBank::Reset()
{
    while (tones_.empty() == false) {
        uint32_t offset = (tones_.begin()).m_node->m_value.first;
        SamplingTone* tone = (tones_.begin()).m_node->m_value.second;
        NotifyOnRemove(tone);
        tones_.erase(offset);
        delete tone;
    }
}


SPU* SoundBank::GetSPU() const
{
    return pSPU_;
}


bool SoundBank::ContainsAddr(uint32_t addr) const
{
    return (tones_.find(addr) != tones_.end());
}


void SoundBank::FourierTransform(SamplingTone *tone)
{
    fft_.PostTransform(tone, GetSPU()->GetDefaultSamplingRate());
}


void SoundBank::AddListener(wxEvtHandler* listener)
{
    listeners_.insert(listener);
}

void SoundBank::RemoveListener(wxEvtHandler* listener)
{
    listeners_.erase(listener);
}


}   // namespace SPU
