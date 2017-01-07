#pragma once
#include <stdint.h>
#include <pthread.h>
#include <deque>

#include <wx/vector.h>
#include <wx/thread.h>

#include "channel.h"
#include "../psx/common.h"
#include "../psx/memory.h"
#include "reverb.h"
#include "interpolation.h"
#include "soundbank.h"
#include "common/SoundFormat.h"


class PSF;
class PSF1;
class PSF2;

class wxEvtHandler;
class SoundDevice;

namespace SPU
{

typedef uint32_t SPUAddr;


////////////////////////////////////////////////////////////////////////
// SPU Request class
////////////////////////////////////////////////////////////////////////

class SPURequest {
protected:
  virtual ~SPURequest() {}
public:
  virtual void Execute(SPUBase*) const = 0;

};


class SPUStepRequest : public SPURequest {
protected:
  SPUStepRequest(int step_count) : step_count_(step_count) {}
public:
  void Execute(SPUBase* spu) const;
  static const SPURequest* CreateRequest(int step_count);
private:
  int step_count_;
};


class SPUNoteOnRequest : public SPURequest {
protected:
  SPUNoteOnRequest(int ch) : ch_(ch) {}
public:
  void Execute(SPUBase*) const;
  static const SPURequest* CreateRequest(int ch);
private:
  int ch_;
};


class SPUNoteOffRequest : public SPURequest {
protected:
  SPUNoteOffRequest(int ch) : ch_(ch) {}
public:
  void Execute(SPUBase*) const;
  static const SPURequest* CreateRequest(int ch);
private:
  int ch_;
};


class SPUSetOffsetRequest : public SPURequest {
protected:
  SPUSetOffsetRequest(int ch) : ch_(ch) {}
public:
  void Execute(SPUBase*) const;
  static const SPURequest* CreateRequest(int ch);
private:
  int ch_;
};



////////////////////////////////////////////////////////////////////////
// SPU Thread
////////////////////////////////////////////////////////////////////////

class SPUBase;

class SPUThread : public wxThread
{
public:
  SPUThread(SPUBase* pSPU);

  void PutRequest(const SPURequest* req);
  void WaitForLastStep();

protected:
  ExitCode Entry();
  void OnExit();

private:
  SPUBase* pSPU_;
  int numSamples_;

  std::deque<const SPURequest*> req_queue_;
  wxMutex queue_mutex_;
  wxCondition queue_cond_;

  friend class SPUBase;
};



////////////////////////////////////////////////////////////////////////
// SPU abstract class
////////////////////////////////////////////////////////////////////////


// WX_DECLARE_HASH_SET(wxEvtHandler*, wxPointerHash, wxPointerEqual, SPUListener);


/**
 * SPU Core
 */

struct SPUCore {

  SPUCore() {}
  SPUCore(SPUBase* spu) : voice_manager_(spu, 24 + 2 /* 2 for Reverb */) {}

  static const int kStateFlagDMACompleted = 0x80;

  unsigned short ctrl_; // Sp0
  unsigned short stat_;
  unsigned int irq_;  // IRQ address
  mutable unsigned int addr_; // DMA current pointer
  // unsigned int rvb_addr_;
  // unsigned int rvb_addr_end_;

  void Step();

  SPUVoice& Voice(int ch) { return voice_manager_.At(ch); }
  SPUVoiceManager& Voices() { return voice_manager_; }

private:
  // MAIN infos struct for each channel
  SPUVoiceManager voice_manager_;
};



class SPUBase : public PSX::Component, private PSX::UserMemoryAccessor
{
public:
  SPUBase(PSX::Composite* composite);
  virtual ~SPUBase() {}

  void Init();
  void Open();
  void Close();
  void Shutdown();
  bool IsRunning() const;

  bool Step(int step_count);
  void ResetStepStatus();

  bool IsAsync() const;
  bool (SPUBase::*Get)(SoundBlock* dest);
  bool GetSync(SoundBlock* dest);
  bool GetAsync(SoundBlock* dest);

  void NotifyObservers();

  uint32_t GetDefaultSamplingRate() const;
  uint32_t GetCurrentSamplingRate() const;
  void ChangeOutputSamplingRate(uint32_t rate);

  SPUVoice& Voice(int ch);

  // Reverb
  REVERBInfo& Reverb() { return reverb_; }
  const REVERBInfo& Reverb() const { return reverb_; }

  Soundbank& soundbank() { return soundbank_; }

  unsigned char* GetSoundBuffer() const;

  // IRQ
  uint32_t GetIRQAddress() const;
  void SetIRQAddress(SPUAddr addr);

  // Accessor and Mutator
  SPUCore& core(int i) {
    const SPUBase* const this_const = this;
    return const_cast<SPUCore&>(this_const->core(i));
  }
  const SPUCore& core(int i) const { return cores_[i]; }
  unsigned int core_count() const { return cores_.size(); }
  uint32_t memory_size() const { return kMemorySize; }

  // Memory
  const uint8_t* mem8_ptr(SPUAddr addr) const {
    return mem8_.get() + addr;
  }
  const uint16_t* mem16_ptr(SPUAddr addr) const {
    return reinterpret_cast<const uint16_t*>(mem8_ptr(addr));
  }
  uint8_t* mem8_ptr(SPUAddr addr) {
    const SPUBase* const this_const = this;
    return const_cast<uint8_t*>(this_const->mem8_ptr(addr));
  }
  uint16_t* mem16_ptr(SPUAddr addr) {
    return reinterpret_cast<uint16_t*>(mem8_ptr(addr));
  }

  uint8_t mem8_val(SPUAddr addr) const {
    return *mem8_ptr(addr);
  }
  uint16_t mem16_val(SPUAddr addr) const {
    return *mem16_ptr(addr);
  }
  uint8_t& mem8_ref(SPUAddr addr) {
    return *mem8_ptr(addr);
  }
  uint16_t& mem16_ref(SPUAddr addr) {
    return *mem16_ptr(addr);
  }

  // Register
  unsigned short ReadRegister(uint32_t reg) const;
  void WriteRegister(uint32_t reg, uint16_t val);

  // Listener Registration
  // void AddListener(wxEvtHandler *listener, int ch);
  // void RemoveListener(wxEvtHandler *listener, int ch);

  SPUInstrument_New* GetSamplingTone(uint32_t addr) const;

  // Processing
  void PutRequest(const SPURequest* req);

  // Notify functions
  void NotifyOnUpdateStartAddress(int ch) const;
  void NotifyOnChangeLoopIndex(SPUVoice* pChannel) const;

  void NotifyOnAddTone(const SPUInstrument_New& tone) const;
  void NotifyOnChangeTone(const SPUInstrument_New& tone) const;
  void NotifyOnRemoveTone(const SPUInstrument_New& tone) const;

  void ReadDMA4Memory(PSX::PSXAddr addr, uint32_t size);
  void WriteDMA4Memory(PSX::PSXAddr addr, uint32_t size);
  void ReadDMA7Memory(PSX::PSXAddr addr, uint32_t size);
  void WriteDMA7Memory(PSX::PSXAddr addr, uint32_t size);

  static const int kMemorySize = 0x100000;  // for PS1

protected:
  void SetupStreams();
  void RemoveStreams();

  void ReadDMAMemoryEx(SPUCore* core, PSX::PSXAddr addr, uint32_t size);
  void WriteDMAMemoryEx(SPUCore* core, PSX::PSXAddr addr, uint32_t size);

public:
  unsigned char* m_pSpuIrq;
  // unsigned char* m_pSpuBuffer;
  unsigned char* m_pMixIrq;

  static const int NSSIZE = 45;
  int ns;

private:

  Soundbank soundbank_;
  NeilReverb reverb_;

  wxScopedArray<uint8_t> mem8_;
  uint16_t* p_mem16_;

  // wxCriticalSection csDMAReadable_;
  // wxCriticalSection csDMAWritable_;

  InterpolationType useInterpolation;

private:
  PSF *m_psf;

  uint32_t default_sampling_rate_;
  uint32_t output_sampling_rate_;

  bool isPlaying_;    // only used on multithread mode??

  unsigned long m_noiseVal;

  bool m_bSpuInit;
  bool m_bSpuIsOpen;

  int m_iFMod[NSSIZE];
  int m_iCycle;

protected:
  wxVector<SPUCore> cores_;
  SPUThread* thread_;
};



class SPU: public SPUBase
{
public:
  SPU(PSX::Composite* composite);

  void SetLength(int stop, int fade);
  void Flush();

public:
  void Async(uint32_t);

  // const ChannelInfo& GetChannelInfo(int ch) const;

  // ADSR
  // void InitADSR();
  // void StartADSR(ChannelInfo& ch);
  // int MixADSR(ChannelInfo& ch);

  // DMA
  uint16_t ReadDMA4(void);
  void WriteDMA4(uint16_t);

  //
  // Utilities
  //
public:
  int GetPCMNumber() const;
  int GetPCM(int index, void* pcm, int* loop) const;

  friend class SPUThread;

private:

  // PSX Buffer & Addresses
  // unsigned short m_regArea[10000];
  // uint8_t mem8_[0x100000];
  // uint16_t* const mem16_;


  friend class SPUVoice;
  friend class SPUVoiceManager;  // temp
  friend class REVERBInfo;    // temp
};



class SPU2 : public SPUBase {
public:
  SPU2(PSX::Composite*);

  uint32_t memory_size() const { return kMemorySize; }

  void InterruptDMA4();
  void InterruptDMA7();

  static const int kMemorySize = 0x200000;

private:
  uint16_t reg_area_[32*1024];
  uint8_t mem8_[2*1024*1024];
  // uint8_t* pSpuBuffer;

  SPUCore cores_[2];
};


/*
class SPUListener {
public:
    virtual ~SPUListener() {}

protected:
    void onUpdate(SPU*);
};
 */


inline SPUVoice& SPUBase::Voice(int ch) {
  // WARNING: cannot access reverb channels through this method.
  const int core_i = ch / 24;
  return cores_[core_i].Voice(ch % 24);
}

inline unsigned char* SPUBase::GetSoundBuffer() const {
  // return Memory;
  return (uint8_t*)p_mem16_;
}

inline uint32_t SPUBase::GetIRQAddress() const {
  return cores_[0].irq_;
}

inline void SPUBase::SetIRQAddress(SPUAddr addr) {
  cores_[0].irq_ = addr;
  m_pSpuIrq = GetSoundBuffer() + addr;
}


}   // namespace SPU


#include <wx/event.h>

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);


// extern SPU::SPU& Spu;
