#pragma once
#include <stdint.h>
#include <deque>
#include <set>
#include <pthread.h>

#include <wx/event.h>
#include <wx/vector.h>
#include <wx/hashmap.h>
#include <wx/thread.h>

#include <wx/event.h>


class wxEvtHandler;

namespace SPU {


typedef unsigned int SPUAddr;


class SamplingTone;
class ChannelInfo;

class SamplingToneIterator
{
public:
    SamplingToneIterator(SamplingTone* pTone = 0, ChannelInfo* pChannel = 0);
    SamplingToneIterator(const SamplingToneIterator& itr);

    SamplingToneIterator& operator =(const SamplingToneIterator& itr);

    bool HasNext() const;
    int Next();

    SamplingToneIterator& operator +=(int n);

    SamplingTone* GetTone() const;
    // void SetPreferredLoop(uint32_t index);
    int32_t GetLoopOffset() const;

protected:
    void clone(SamplingToneIterator* itrTone) const;

private:
    SamplingTone* pTone_;
    ChannelInfo* pChannel_;
    mutable uint32_t index_;

    void* operator new(std::size_t);
};



class SoundBank;

class SamplingTone
{
public:
    SamplingTone(const SoundBank *pSB, uint8_t* pADPCM);

    const int32_t* GetData() const;
    const uint8_t* GetADPCM() const;
    SPUAddr GetAddr() const;
    uint32_t GetLength() const;
    uint32_t GetLoopOffset() const;
    SPUAddr GetExternalLoopAddr() const;
    void SetExternalLoopAddr(SPUAddr addr);
    SPUAddr GetEndAddr() const;

    double GetFreq() const;
    void SetFreq(double f);

    int At(int index) const;

    void ConvertData();

    const SoundBank *GetSoundBank() const;

    // iterators
    SamplingToneIterator Iterator(ChannelInfo *pChannel) const;
    // const SamplingToneIterator& Begin() const;
    // const SamplingToneIterator& End() const;

protected:
    void ADPCM2LPCM();

    bool hasFinishedConv() const;


private:
    const SoundBank* pSB_;

    const uint8_t* const pADPCM_;
    mutable wxVector<int32_t> LPCM_;
    const uint8_t* current_pointer_;

    mutable uint32_t loop_offset_;
    uint32_t external_loop_addr_;
    bool forcesStop_;
    // bool hasFinishedConv_;
    mutable uint32_t processedBlockNumber_;
    SPUAddr end_addr_;
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

    void PostTransform(SamplingTone* tone, int sampling_rate);

private:
    static void* mainLoop(void *);

    int sampling_rate_;

    std::deque<SamplingTone*> queue_;
    pthread_t thread_;
    pthread_mutex_t mutexQueue_;
    pthread_cond_t condQueue_;
    bool requiresShutdown_;
};







WX_DECLARE_HASH_MAP(uint32_t, SamplingTone*, wxIntegerHash, wxIntegerEqual, SamplingToneMap);


class SPU;

class SoundBank
{
public:
    SoundBank(SPU* pSPU);
    ~SoundBank();

    void Init();
    void Reset();
    void Shutdown();

    // const SamplineTone* GetSamplingTone(uint8_t* pADPCM) const;
    SamplingTone *GetSamplingTone(uint32_t addr) const;


    SPU* GetSPU() const;
    bool ContainsAddr(uint32_t addr) const;

    void FourierTransform(SamplingTone* tone);

    void AddListener(wxEvtHandler* listener);
    void RemoveListener(wxEvtHandler* listener);

    void NotifyOnAdd(SamplingTone* tone) const;
    void NotifyOnModify(SamplingTone* tone) const;
    void NotifyOnRemove(SamplingTone* tone) const;

private:
    SPU* pSPU_;

    mutable SamplingToneMap tones_;

    // wxThread *thread;
    // int* fftBuffer_;
    FourierTransformer fft_;

    std::set<wxEvtHandler*> listeners_;
};




} // namespace SPU


wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_ADD_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_MODIFY_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_REMOVE_TONE, wxCommandEvent);

