#include "soundbank.h"
#include "spu.h"
#include "../psx/psx.h"

#include <cmath>
#include <stdexcept>


namespace SPU {


SamplingTone::SamplingTone(const SoundBank *pSB, uint8_t *pADPCM) :
    pSB_(pSB), pADPCM_(pADPCM), indexLoop_(0xffffffff), processedBlockNumber_(0),
    freq_(0.0), begin_(this), prev1_(0), prev2_(0)
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
    wxASSERT(len % 28 == 0);
    return len;
}


uint32_t SamplingTone::GetLoopIndex() const
{
    wxASSERT(indexLoop_ % 28 == 0);
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

    wxASSERT(len == processedBlockNumber_ * 28);
    for (int i = len; i <= index; i += 28) {
        ADPCM2LPCM();
        if (hasFinishedConv()) break;
    }

    return LPCM_.at(index);
}


const SoundBank* SamplingTone::GetSoundBank() const
{
    return pSB_;
}


void SamplingTone::ADPCM2LPCM() const
{
    wxASSERT(hasFinishedConv() == false);

    static const int xa_adpcm_table[5][2] = {
        {   0,   0 },
        {  60,   0 },
        { 115, -52 },
        {  98, -55 },
        { 122, -60 }
    };

    SPU* pSpu = pSB_->GetSPU();

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
        pSB_->NotifyOnModify(const_cast<SamplingTone*>(this));
    } else if ( flags & 1 ) {
        // the end of this tone
        processedBlockNumber_ = 0xffffffff;
        // wxMessageOutputDebug().Printf(wxT("Tone end"));
        pSB_->NotifyOnModify(const_cast<SamplingTone*>(this));
        /*
        if (flags != 3 || indexLoop_ == 0) { // DQ4
            start = (uint8_t*)0xffffffff;
            ;
        } else {
            wxASSERT( (flags & 4) == 0 );
            start = pLoop;
        }
        */
        Spu.SoundBank_.FourierTransform(const_cast<SamplingTone*>(this));
    }

    prev1_ = prev1;
    prev2_ = prev2;
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

/*
void SamplingToneIterator::SetPreferredLoop(uint32_t index)
{
//    wxMessageOutputDebug().Printf(wxT("force loop is on at %d (start = %d)"), (uint32_t)val << 3, );
    indexPreferredLoop_ = (index - pTone_->GetSPUOffset()) * 28 / 16;
    // experimental
    pTone_->indexLoop_ = (index - pTone_->GetSPUOffset()) * 28 / 16;
}
*/

bool SamplingToneIterator::HasNext() const
{
    if (pTone_ == 0) return false;

    if (index_ < pTone_->GetLength()) {
        return true;
    }
    if (pTone_->hasFinishedConv() == false) {
        return true;
    }

    uint32_t indexLoop;
    const uint32_t offsetExternalLoop = pChannel_->offsetExternalLoop;
    if (offsetExternalLoop < 0x80000000) {
        indexLoop = (offsetExternalLoop - pTone_->GetSPUOffset()) * 28 / 16;
    } else {
        indexLoop = pTone_->GetLoopIndex();
        if (0x80000000 <= indexLoop) {
            return false;
        }
    }
    uint32_t lenLoop = pTone_->GetLength() - indexLoop;
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


void FourierTransformer::PostTransform(SamplingTone* tone)
{
    pthread_mutex_lock(&mutexQueue_);
    queue_.push_back(tone);
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
        pthread_mutex_lock(&ft->mutexQueue_);
        if (ft->queue_.empty() == false) {
            tone = ft->queue_.front();
            ft->queue_.pop_front();
        } else {
            tone = 0;
        }
        pthread_mutex_unlock(&ft->mutexQueue_);

        if (tone != 0) {
            // const int32_t *p = tone->GetData();
            int n = tone->GetLength();
            n = (int) pow( 2.0, floor(log(n) / log(2.0)) );

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

            for (int i = 0; i < n; i++) {
                a[i] = (double)tone->At(i) / 32767.0;
            }

            cdft(n, -1, a, ip, w);

            double valMax2 = 0.0;
            int indexMax = 0;
            for (int i = 0; i < n/2; i++) {
                const double re = a[2*i];
                const double im = a[2*i+1];
                const double val2 = re*re + im*im;
                if (valMax2 < val2) {
                    valMax2 = val2;
                    indexMax = i;
                }
            }

            SPU* pSPU = tone->GetSoundBank()->GetSPU();
            double freq = static_cast<double>(indexMax * pSPU->GetDefaultSamplingRate()) / n;
            tone->SetFreq(freq);
            Spu.SoundBank_.NotifyOnModify(tone);
            wxMessageOutputDebug().Printf(wxT("Finished FFT (offset = %d, freq = %d)"), tone->GetSPUOffset(), indexMax);
        }

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




SoundBank::SoundBank(SPU *pSPU) : pSPU_(pSPU) {}


void SoundBank::NotifyOnAdd(SamplingTone *tone) const
{
    for (std::set<SoundBankListener*>::const_iterator it = listeners_.begin(); it != listeners_.end(); ++it) {
        const_cast<SoundBankListener*>(*it)->onAdd(const_cast<SoundBank*>(this), tone);
    }
}


void SoundBank::NotifyOnModify(SamplingTone *tone) const
{
    for (std::set<SoundBankListener*>::const_iterator it = listeners_.begin(); it != listeners_.end(); ++it) {
        const_cast<SoundBankListener*>(*it)->onModify(const_cast<SoundBank*>(this), tone);
    }
}


void SoundBank::NotifyOnRemove(SamplingTone *tone) const
{
    for (std::set<SoundBankListener*>::const_iterator it = listeners_.begin(); it != listeners_.end(); ++it) {
        const_cast<SoundBankListener*>(*it)->onRemove(const_cast<SoundBank*>(this), tone);
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
    for (SamplingToneMap::Iterator it = tones_.begin(); it != tones_.end(); ) {
        SamplingTone* tone = it.m_node->m_value.second;
        NotifyOnRemove(tone);
        delete tone;
    }
    tones_.clear();
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
    fft_.PostTransform(tone);
}


void SoundBank::AddListener(SoundBankListener *listener)
{
    listeners_.insert(listener);
}


void SoundBank::RemoveListener(SoundBankListener *listener)
{
    listeners_.erase(listener);
}


void SoundBank::RemoveAllListeners()
{
    listeners_.clear();
}


}   // namespace SPU
