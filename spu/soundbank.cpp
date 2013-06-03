#include "soundbank.h"
#include "spu.h"
#include "../psx/psx.h"


namespace SPU {


SamplingTone::SamplingTone(const SoundBank *pSB, uint8_t *pADPCM) :
    pSB_(pSB), pADPCM_(pADPCM), indexLoop_(0xffffffff), processedBlockNumber_(0),
    begin_(this), prev1_(0), prev2_(0)
{
    wxMessageOutputDebug().Printf(wxT("Sampling tone addr = %08x"), pADPCM_);
}


const uint8_t* SamplingTone::GetADPCM() const
{
    return pADPCM_;
}


uint32_t SamplingTone::GetLength() const
{
    return LPCM_.size();
}


uint32_t SamplingTone::GetLoopIndex() const
{
    return indexLoop_;
}


bool SamplingTone::hasFinishedConv() const
{
    return (0x80000000 <= processedBlockNumber_);
}


int SamplingTone::At(int index) const
{
    int len = LPCM_.size();
    wxASSERT(len % 28 == 0);

    if (index < len) {
        return LPCM_.at(index);
    }

    wxASSERT(len == processedBlockNumber_ * 28);
    for (int i = len; i <= index; i += 28) {
        ADPCM2LPCM();
        if (hasFinishedConv()) break;
    }

    return LPCM_.at(index);
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
    uint8_t* start = pADPCM_ + processedBlockNumber_ * 16;
    int predict_nr = *start++;
    int shift_factor = predict_nr & 0xf;
    int flags = *start++;
    predict_nr >>= 4;

    if ( flags & 4 ) {
        // pLoop = start - 16;
        indexLoop_ = processedBlockNumber_ * 28;
    }

    int d, s, fa;

    for (int i = 0; i < 28; start++) {
        d = *start;

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
    wxASSERT(start == pADPCM_ + processedBlockNumber_ * 16);


    uint8_t* pLoop = indexLoop_ ? pADPCM_ + (indexLoop_ / 28 * 16) : 0;
    if (pSpu->Sp0 & 0x40) {
        if ( (start-16 < pSpu->m_pSpuIrq && pSpu->m_pSpuIrq <= start) || ((flags & 1) && (pLoop-16 < pSpu->m_pSpuIrq && pSpu->m_pSpuIrq <= pLoop)) ) {
            // iIrqDone = 1;
            PSX::u32Href(0x1070) |= BFLIP32(0x200);
        }
    }


    if (flags & 1) {
        // the end of this tone
        processedBlockNumber_ = -1;
        /*
        if (flags != 3 || indexLoop_ == 0) { // DQ4
            start = (uint8_t*)0xffffffff;
            ;
        } else {
            wxASSERT( (flags & 4) == 0 );
            start = pLoop;
        }
        */
    }

    prev1_ = prev1;
    prev2_ = prev2;

}










SamplingToneIterator SamplingTone::Iterator() const
{
    // clone the 'begin' iterator
    return begin_;
}





SamplingToneIterator::SamplingToneIterator(SamplingTone *pTone) :
    pTone_(pTone), index_(0), indexPreferredLoop_(0xffffffff)
{
    index_ = 0;
}


void SamplingToneIterator::SetPreferredLoop(uint32_t index)
{
    indexPreferredLoop_ = index;
}


bool SamplingToneIterator::HasNext() const
{
    if (pTone_ == 0) return false;

    if (index_ < pTone_->GetLength()) {
        return true;
    }
    uint32_t indexLoop = (indexPreferredLoop_ < 0x80000000) ? indexPreferredLoop_ : pTone_->GetLoopIndex();
    if (0x80000000 <= indexLoop) {
        return false;
    }
    uint32_t lenLoop = pTone_->GetLength() - indexLoop;
    while (index_ < pTone_->GetLength()) {
        index_ -= lenLoop;
    }
    return true;
}


int SamplingToneIterator::Next()
{
    return pTone_->At(index_++);
}





SoundBank::SoundBank(SPU *pSPU) : pSPU_(pSPU) {}


SamplingTone* SoundBank::GetSamplingTone(uint32_t addr) const
{
    SamplingToneMap::Iterator it = tones_.find(addr);
    if (it != tones_.end()) {
        return it.m_node->m_value.second;
    }
    SamplingTone* tone = new SamplingTone(this, pSPU_->GetSoundBuffer() + addr);
    // tones_.insert(addr, tone);
    tones_[addr] = tone;
    return tone;
}



void SoundBank::Reset()
{
    for (SamplingToneMap::Iterator it = tones_.begin(); it != tones_.end(); ) {
        delete it.m_node->m_value.second;
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









}   // namespace SPU
