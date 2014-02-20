#pragma once
#include <stdint.h>
#include <pthread.h>

#include <wx/vector.h>
#include <wx/thread.h>

#include "spu/channel.h"
#include "psx/common.h"
#include "reverb.h"
#include "interpolation.h"
#include "soundbank.h"
#include "SoundFormat.h"


class wxEvtHandler;
class SoundDeviceDriver;

namespace SPU
{

typedef uint32_t SPUAddr;


////////////////////////////////////////////////////////////////////////
// SPU Thread
////////////////////////////////////////////////////////////////////////

class SPU;

class SPUThread : public wxThread
{
public:
  SPUThread(SPU* pSPU);

protected:
  virtual void* Entry();

private:
  SPU* pSPU_;
  int numSamples_;

  friend class SPU;
};



////////////////////////////////////////////////////////////////////////
// SPU abstract class
////////////////////////////////////////////////////////////////////////


// WX_DECLARE_HASH_SET(wxEvtHandler*, wxPointerHash, wxPointerEqual, SPUListener);


/**
 * SPU Core
 */

struct SPUCore {

  static const int kStateFlagDMACompleted = 0x80;

  unsigned short ctrl_; // Sp0
  unsigned short stat_;
  unsigned int irq_;  // IRQ address
  mutable unsigned int addr_; // DMA current pointer
  // unsigned int rvb_addr_;
  // unsigned int rvb_addr_end_;
};



class SPUBase : public PSX::Component
{
public:
  SPUBase(PSX::Composite* composite)
: Component(composite) {}
  virtual ~SPUBase() {}

  virtual void Close() = 0;

  virtual uint32_t GetDefaultSamplingRate() const = 0;

  virtual ChannelInfo& Channel(int ch) = 0;

  virtual REVERBInfo& Reverb() = 0;
  virtual const REVERBInfo& Reverb() const = 0;

  virtual SoundBank& Soundbank() = 0;


  // Accessor and Mutator
  SPUCore& core(int i) {
    const SPUBase* const this_const = this;
    return const_cast<SPUCore&>(this_const->core(i));
  }
  virtual const SPUCore& core(int i) const = 0;
  virtual unsigned int core_count() const = 0;
  virtual uint32_t memory_size() const = 0;

  // Memory
  virtual const uint8_t* mem8_ptr(SPUAddr addr) const = 0;
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
  virtual unsigned short ReadRegister(uint32_t reg) = 0;
  virtual void WriteRegister(uint32_t reg, uint16_t val) = 0;

  // Listener Registration
  // void AddListener(wxEvtHandler *listener, int ch);
  // void RemoveListener(wxEvtHandler *listener, int ch);

  // Notify functions
  void NotifyOnUpdateStartAddress(int ch) const;
  void NotifyOnChangeLoopIndex(ChannelInfo* pChannel) const;

protected:
  mutable pthread_mutex_t process_mutex_;
  mutable pthread_mutex_t wait_start_mutex_;
  mutable pthread_cond_t process_cond_;
  mutable pthread_mutex_t dma_writable_mutex_;

  enum ProcessState {
    STATE_SHUTDOWN = -1,
    STATE_PSX_IS_READY = 0,
    STATE_START_PROCESS,
    STATE_NOTE_ON,
    STATE_NOTE_OFF,
    STATE_SET_OFFSET,
    STATE_NONE
  };

  mutable ProcessState process_state_;
  mutable int processing_channel_;

  void ChangeProcessState(ProcessState state, int ch = -1);

private:
  // SPUListener listeners_;
};








class PSF;
class PSF1;

class SPU: public SPUBase
{
public:
  SPU(PSX::Composite* composite);

  void Init();
  void Open();
  void SetLength(int stop, int fade);
  void Close();
  void Shutdown();
  void Flush();

  bool IsRunning() const;

  const SPUCore& core(int /*i*/) const { return core_; }
  SPUCore& core(int i) { return SPUBase::core(i); }
  unsigned int core_count() const { return 1; }

  uint32_t memory_size() const { return kMemorySize; }

protected:
  void SetupStreams();
  void RemoveStreams();

public:
  void Async(uint32_t);

  uint32_t GetDefaultSamplingRate() const { return 44100; }

  ChannelInfo& Channel(int ch);
  // const ChannelInfo& GetChannelInfo(int ch) const;
  unsigned char* GetSoundBuffer() const;

  // ADSR
  // void InitADSR();
  // void StartADSR(ChannelInfo& ch);
  // int MixADSR(ChannelInfo& ch);

  // Reverb
  virtual REVERBInfo& Reverb() { return reverb_; }
  virtual const REVERBInfo& Reverb() const { return reverb_; }

  // Register
  unsigned short ReadRegister(uint32_t reg);
  void WriteRegister(uint32_t reg, uint16_t val);

  // DMA
  uint16_t ReadDMA4(void);
  void ReadDMA4Memory(PSX::PSXAddr psxAddr, uint32_t size);
  void WriteDMA4(uint16_t);
  void WriteDMA4Memory(PSX::PSXAddr psxAddr, uint32_t size);

  // IRQ
  uint32_t GetIRQAddress() const;
  void SetIRQAddress(SPUAddr addr);

  //
  // Utilities
  //
public:
  SamplingTone* GetSamplingTone(uint32_t addr) const;

  void NotifyOnAddTone(const SamplingTone& tone) const;
  void NotifyOnChangeTone(const SamplingTone& tone) const;
  void NotifyOnRemoveTone(const SamplingTone& tone) const;

  int GetPCMNumber() const;
  int GetPCM(int index, void* pcm, int* loop) const;

  friend class SPUThread;

private:

  static const int NSSIZE = 45;

  PSF *m_psf;

  bool isPlaying_;    // only used on multithread mode??

  // PSX Buffer & Addresses
  // unsigned short m_regArea[10000];
  uint8_t mem8_[0x100000];
  uint16_t* const mem16_;

public:
  const uint8_t* mem8_ptr(SPUAddr addr) const {
    return mem8_ + addr;
  }

  static const int kMemorySize = 0x100000;

public:
  unsigned char* m_pSpuIrq;
  unsigned char* m_pSpuBuffer;
  unsigned char* m_pMixIrq;

private:
  // User settings
  // int m_iUseXA;
  //int m_iVolume;
  //int m_iXAPitch;
  //int m_iUseTimer;    // should be false (use thread)
  //int m_iSPUIRQWait;
  //int m_iDebugMode;
  //int m_iRecordMode;
  int m_iUseReverb;
  InterpolationType useInterpolation;
  //int m_iDisStereo;
  //int m_iUseDBufIrq;

  // MAIN infos struct for each channel
public:
  ChannelArray Channels;

private:
  NeilReverb reverb_;

  unsigned long m_noiseVal;

//  unsigned short Sp0;
//  unsigned short Status;
//  SPUAddr IrqAddr;
//  mutable SPUAddr dma_current_addr_;
  SPUCore core_;

private:
  bool m_bEndThread;
  bool m_bThreadEnded;
  bool m_bSpuInit;
  bool m_bSpuIsOpen;

  // unsigned long m_newChannel;

  // void (*irqCallback)();
  // void (*cddavCallback)(unsigned short, unsigned short);
  // void (*irqQSound)(unsigned char*, long*, long);

  // int f[5][2];

  // int m_SSumR[NSSIZE];
  // int m_SSumL[NSSIZE];
  int m_iFMod[NSSIZE];
  int m_iCycle;
  // short *m_pS;

  int ns;


  // Sound bank
public:
  SoundBank SoundBank_;
  virtual SoundBank& Soundbank() { return SoundBank_; }

  // DMA
private:
  wxCriticalSection csDMAReadable_;
  wxCriticalSection csDMAWritable_;

  // Multithread
  SPUThread* thread_;
  // bool isReadyToProcess_;
  // wxMutex mutexReadyToProcess_;
  // wxCondition condReadyToProcess_;

  // wxMutex mutexUpdate_;
  // wxCondition condUpdate_;


  friend class ChannelInfo;
  friend class ChannelArray;  // temp
  friend class REVERBInfo;    // temp
};



class SPU2 : public SPUBase {
public:
  SPU2(PSX::Composite*);

  uint32_t memory_size() const { return kMemorySize; }

  void ReadDMA4Memory(PSX::PSXAddr psx_addr, uint32_t size);
  void ReadDMA7Memory(PSX::PSXAddr psx_addr, uint32_t size);
  void WriteDMA4Memory(PSX::PSXAddr psx_addr, uint32_t size);
  void WriteDMA7Memory(PSX::PSXAddr psx_addr, uint32_t size);
  void InterruptDMA4();
  void InterruptDMA7();

  const uint8_t* mem8_ptr(SPUAddr addr) const {
    return mem8_ + addr;
  }

  static const int kMemorySize = 0x200000;

private:
  void ReadDMAMemoryEx(SPUCore*, PSX::PSXAddr, uint32_t);
  void WriteDMAMemoryEx(SPUCore*, PSX::PSXAddr, uint32_t);

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



inline ChannelInfo& SPU::Channel(int ch) {
  return Channels.At(ch);
}

inline unsigned char* SPU::GetSoundBuffer() const {
  // return Memory;
  return (uint8_t*)mem16_;
}

inline uint32_t SPU::GetIRQAddress() const {
  return core_.irq_;
}

inline void SPU::SetIRQAddress(SPUAddr addr) {
  core_.irq_ = addr;
  m_pSpuIrq = GetSoundBuffer() + addr;
}


}   // namespace SPU


#include <wx/event.h>

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);


// extern SPU::SPU& Spu;
