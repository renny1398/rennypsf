#pragma once
#include <stdint.h>
#include <deque>
#include <set>
#include <pthread.h>

#include <wx/vector.h>
#include <wx/hashmap.h>
#include <wx/thread.h>


namespace SPU {


class SamplingTone;

class SamplingToneIterator
{
public:
    SamplingToneIterator(SamplingTone* pTone = 0);
    SamplingToneIterator(const SamplingToneIterator& itr);

    SamplingToneIterator& operator =(const SamplingToneIterator& itr);

    bool HasNext() const;
    int Next();

    SamplingTone* GetTone() const;
    void SetPreferredLoop(uint32_t index);

protected:
    void clone(SamplingToneIterator* itrTone) const;

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
    SamplingTone(const SoundBank *pSB, uint8_t* pADPCM);

    const int32_t* GetData() const;
    const uint8_t* GetADPCM() const;
    uint32_t GetSPUOffset() const;
    uint32_t GetLength() const;
    uint32_t GetLoopIndex() const;
    double GetFreq() const;
    void SetFreq(double f);

    int At(int index) const;

    const SoundBank *GetSoundBank() const;

    // iterators
    SamplingToneIterator Iterator() const;
    // const SamplingToneIterator& Begin() const;
    // const SamplingToneIterator& End() const;

protected:
    void ADPCM2LPCM() const;

    bool hasFinishedConv() const;




private:
    const SoundBank* pSB_;

    const uint8_t* pADPCM_;
    mutable wxVector<int32_t> LPCM_;

    mutable uint32_t indexLoop_;
    // bool hasFinishedConv_;
    mutable uint32_t processedBlockNumber_;
    double freq_;

    SamplingToneIterator begin_;

    mutable int prev1_, prev2_;

    friend class SoundBank;
    friend class SamplingToneIterator;
};




////////////////////////////////////////////////////////////////////////
// Fourier Transformer
////////////////////////////////////////////////////////////////////////


class FourierTransformer
{
public:
    FourierTransformer();
    ~FourierTransformer();

    void PostTransform(SamplingTone* tone);

private:
    static void* mainLoop(void *);

    std::deque<SamplingTone*> queue_;
    pthread_t thread_;
    pthread_mutex_t mutexQueue_;
    pthread_cond_t condQueue_;
    bool requiresShutdown_;
};







WX_DECLARE_HASH_MAP(uint32_t, SamplingTone*, wxIntegerHash, wxIntegerEqual, SamplingToneMap);

class SoundBankListener
{
    friend class SoundBank;

public:
    virtual ~SoundBankListener() {}

protected:
    // virtual void onUpdate(SoundBank* soundBank) = 0;

    virtual void onAdd(SoundBank*, SamplingTone*) {}
    virtual void onModify(SoundBank*, SamplingTone*) {}
    virtual void onRemove(SoundBank*, SamplingTone*) {}
};


class SPU;

class SoundBank
{
public:
    SoundBank(SPU* pSPU);

    // const SamplineTone* GetSamplingTone(uint8_t* pADPCM) const;
    SamplingTone *GetSamplingTone(uint32_t addr) const;

    void Reset();

    SPU* GetSPU() const;
    bool ContainsAddr(uint32_t addr) const;

    void FourierTransform(SamplingTone* tone);

    void AddListener(SoundBankListener* listener);
    void RemoveListener(SoundBankListener* listener);
    void RemoveAllListeners();

    void NotifyOnAdd(SamplingTone* tone) const;
    void NotifyOnModify(SamplingTone* tone) const;
    void NotifyOnRemove(SamplingTone* tone) const;

protected:

private:
    SPU* pSPU_;

    mutable SamplingToneMap tones_;

    // wxThread *thread;
    // int* fftBuffer_;
    FourierTransformer fft_;

    std::set<SoundBankListener*> listeners_;
};











} // namespace SPU
