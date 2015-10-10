#pragma once
#include <stdint.h>
#include <deque>
#include <set>
#include <pthread.h>

#include <wx/event.h>
#include <wx/vector.h>
#include <wx/hashmap.h>
#include <wx/thread.h>

#include <wx/scopedarray.h>


class wxEvtHandler;

namespace SPU {


typedef unsigned int SPUAddr;


class SPUInstrument;
class SPUVoice;

class SamplingToneIterator
{
public:
  SamplingToneIterator(SPUInstrument* pTone = 0, SPUVoice* pChannel = 0);
  SamplingToneIterator(const SamplingToneIterator& itr);

  SamplingToneIterator& operator =(const SamplingToneIterator& itr);

  bool HasNext() const;
  int Next();

  SamplingToneIterator& operator +=(int n);

  SPUInstrument* GetTone() const;
  // void SetPreferredLoop(uint32_t index);
  int32_t GetLoopOffset() const;

protected:
  void clone(SamplingToneIterator* itrTone) const;

private:
  SPUInstrument* pTone_;
  SPUVoice* pChannel_;
  mutable uint32_t index_;

  void* operator new(std::size_t);
};



class SoundBank;

class SPUInstrument
{
public:
  SPUInstrument(SoundBank *soundbank, uint8_t* pADPCM);

  SoundBank& soundbank() { return soundbank_; }
  const SoundBank& soundbank() const { return soundbank_; }

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

  void Mute() { muted_ = true; }
  void Unmute() { muted_ = false; }
  bool IsMuted() const { return muted_; }

  // iterators
  SamplingToneIterator Iterator(SPUVoice *pChannel) const;
  // const SamplingToneIterator& Begin() const;
  // const SamplingToneIterator& End() const;

protected:
  void ADPCM2LPCM();

  bool hasFinishedConv() const;


private:
  SoundBank& soundbank_;

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

  bool muted_;

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

  void PostTransform(SPUInstrument* tone, int sampling_rate);

private:
  static void* mainLoop(void *);

  int sampling_rate_;

  std::deque<SPUInstrument*> queue_;
  pthread_t thread_;
  pthread_mutex_t mutexQueue_;
  pthread_cond_t condQueue_;
  bool requiresShutdown_;
};







WX_DECLARE_HASH_MAP(uint32_t, SPUInstrument*, wxIntegerHash, wxIntegerEqual, SamplingToneMap);


class SPUBase;

class SoundBank : public wxEvtHandler
{
public:
  SoundBank(SPUBase* pSPU);
  ~SoundBank();

  void Init();
  void Reset();
  void Shutdown();

  // const SamplineTone* GetSamplingTone(uint8_t* pADPCM) const;
  SPUInstrument* GetSamplingTone(uint32_t addr) const;
  SPUInstrument* GetSamplingTone(uint32_t addr);


  SPUBase* GetSPU() const;
  bool ContainsAddr(uint32_t addr) const;

  void FourierTransform(SPUInstrument* tone);

  void AddListener(wxEvtHandler* listener);
  void RemoveListener(wxEvtHandler* listener);

  void NotifyOnAdd(SPUInstrument* tone) const;
  void NotifyOnModify(SPUInstrument* tone) const;
  void NotifyOnRemove(SPUInstrument* tone) const;

protected:
  void OnMuteTone(wxCommandEvent& event);
  void OnUnmuteTone(wxCommandEvent& event);

private:
  SPUBase* pSPU_;

  mutable SamplingToneMap tones_;

  // wxThread *thread;
  // int* fftBuffer_;
  FourierTransformer fft_;

  std::set<wxEvtHandler*> listeners_;
};



////////////////////////////////////////////////////////////////////////
// New Soundbank and Instrument
////////////////////////////////////////////////////////////////////////

}
#include "common/soundbank.h"
namespace SPU {


class SPUInstrument_New;

class PCM_Converter : public wxThread {
public:
  PCM_Converter(SPUInstrument_New*, uint8_t* p_adpcm);
protected:
  wxThread::ExitCode Entry();
  void OnExit();
private:
  SPUInstrument_New* const p_inst_;
  const uint8_t* const p_adpcm_;
};


class SPUInstrument_New : public Instrument {
public:
  SPUInstrument_New(const SPUBase& spu, SPUAddr addr, SPUAddr loop);
  ~SPUInstrument_New();

  int id() const;
  int at(int i) const;
  unsigned int length() const;
  int loop() const;

  SPUAddr addr() const { return addr_; }

protected:
  void MeasureLength(const SPUBase&);

private:
  SPUAddr addr_;
  wxVector<int> data_;
  unsigned int length_;
  int loop_;

  SPUAddr external_loop_addr_;

  unsigned int read_size_;
  PCM_Converter* thread_;
  mutable wxMutex read_mutex_;
  mutable wxCondition read_cond_;

  friend class PCM_Converter;
};







} // namespace SPU

/*
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_ADD_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_MODIFY_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_REMOVE_TONE, wxCommandEvent);
*/
