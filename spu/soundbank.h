#pragma once
#include <stdint.h>
#include <wx/vector.h>
#include <wx/hashmap.h>
#include <wx/thread.h>


namespace SPU {


class SamplingTone;

class SamplingToneIterator
{
public:
    SamplingToneIterator(SamplingTone* pTone = 0);

    bool HasNext() const;
    int Next();

    void SetPreferredLoop(uint32_t index);

private:
    SamplingTone* pTone_;
    mutable uint32_t index_;
    uint32_t indexPreferredLoop_;

    void* operator new(std::size_t);
};


class SoundBank;

class SamplingTone
{
public:
    SamplingTone(SoundBank* pSB);

    const uint32_t* GetData() const;
    uint32_t GetLength() const;
    uint32_t GetLoopIndex() const;
    int GetFreq() const;

    int At(int index) const;

    // iterators
    SamplingToneIterator Iterator() const;
    // const SamplingToneIterator& Begin() const;
    // const SamplingToneIterator& End() const;

protected:
    void ADPCM2LPCM() const;

    bool hasFinishedConv() const;

    // for iterator


private:
    SoundBank* pSB_;

    uint8_t* pADPCM_;
    mutable wxVector<int32_t> LPCM_;

    mutable uint32_t indexLoop_;
    // bool hasFinishedConv_;
    mutable uint32_t processedBlockNumber_;

    SamplingToneIterator begin_;

    mutable int prev1_, prev2_;

    friend class SoundBank;
    friend class SamplingToneIterator;
};





WX_DECLARE_HASH_MAP(uint32_t, SamplingTone*, wxIntegerHash, wxIntegerEqual, SamplingToneMap);

class SPU;

class SoundBank
{
public:
    SoundBank(SPU* pSPU);

    void Reset();

    SPU* GetSPU() const;
    bool ContainsAddr(int32_t addr);

protected:

private:
    SPU* pSPU_;

    SamplingToneMap tones_;

    // wxThread *thread;
    int* fftBuffer_;
};














} // namespace SPU
