#pragma once
#include <stdint.h>
#include <pthread.h>

#include <wx/vector.h>
// #include <wx/scopedptr.h>
#include <wx/sharedptr.h>
#include <wx/thread.h>

#include "../psx/common.h"
#include "reverb.h"
#include "interpolation.h"
#include "soundbank.h"
#include "../SoundFormat.h"


class wxEvtHandler;
class SoundDeviceDriver;

namespace SPU
{
/*
class Volume
{
    int32_t volume;
public:
    uint16_t Int15() const {
        return volume & 0x3fff;
    }
    void Int15(uint16_t vol) {
        wxASSERT(vol < 0x8000);
        if (vol > 0x3fff) {
            volume = vol - 0x8000;
            wxASSERT(volume < 0);
        } else {
            volume = vol;
        }
    }
};

class Pitch
{
    uint32_t pitch;
public:
    uint16_t Uint14() const {
        return pitch;
    }
    void Uint14(uint16_t p) {
        pitch = p;
    }
};
 */

enum ADSR_STATE {
  ADSR_STATE_ATTACK,
  ADSR_STATE_DECAY,
  ADSR_STATE_SUSTAIN,
  ADSR_STATE_RELEASE
};


typedef uint32_t SPUAddr;


struct ADSRInfo
{
  int            AttackModeExp;
  long           AttackTime;
  long           DecayTime;
  long           SustainLevel;
  int            SustainModeExp;
  long           SustainModeDec;
  long           SustainTime;
  int            ReleaseModeExp;
  unsigned long  ReleaseVal;
  long           ReleaseTime;
  long           ReleaseStartTime;
  long           ReleaseVol;
  long           lTime;
  long           lVolume; // from -1024 to 1023
};


class ADSRInfoEx
{
public:
  void Start();

public:

  ADSR_STATE     State;
  bool           AttackModeExp;
  int            AttackRate;
  int            DecayRate;
  int            SustainLevel;
  bool           SustainModeExp;
  int            SustainIncrease;
  int            SustainRate;
  bool           ReleaseModeExp;
  int            ReleaseRate;
  int            EnvelopeVol;
  long           lVolume;
  long           lDummy1;
  long           lDummy2;
};



class ChannelInfo;

class ChannelInfoListener {

  friend class ChannelInfo;

public:
  virtual ~ChannelInfoListener() {}

protected:
  virtual void OnNoteOn(const ChannelInfo&) {}
  virtual void OnNoteOff(const ChannelInfo&) {}
};




class InterpolationBase;
typedef wxSharedPtr<InterpolationBase> InterpolationPtr;

class ChannelArray;

class ChannelInfo : public ::Sample16 {
public:
  ChannelInfo(SPU* pSPU = 0);
  void StartSound();
  static void InitADSR();
  int MixADSR();

  void Update();

  SPU& Spu() { return spu_; }
  const SPU& Spu() const { return spu_; }

protected:
  void VoiceChangeFrequency();
  // void ADPCM2LPCM();

private:
  SPU& spu_;

public:
  // for REVERB
  // wxSharedArray<short> lpcm_buffer_l;
  // wxSharedArray<short> lpcm_buffer_r;
  wxVector<short> lpcm_buffer_l;
  wxVector<short> lpcm_buffer_r;

  // bool            bNew;                               // start flag
  // bool            isUpdating;
  bool is_ready;

  int             iSBPos;                             // mixing stuff
  InterpolationPtr  pInterpolation;

  int             sval;
  SamplingTone* tone;
  SamplingToneIterator itrTone;

  bool            isStopped;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
  // bool            is_muted;
  bool            hasReverb;                            // can we do reverb on this channel? must have ctrl register bit, to get active
  int             iActFreq;                           // current psx pitch
  int             iUsedFreq;                          // current pc pitch
  double          Pitch;
  int             iLeftVolume;                        // left volume (s15)
  bool            isLeftSweep;
  bool            isLeftExpSlope;
  bool            isLeftDecreased;
  //int             iLeftVolRaw;                        // left psx volume value
  SPUAddr    addrExternalLoop;
  bool            useExternalLoop;                        // ignore loop bit, if an external loop address is used
  // int             iMute;                              // mute mode
  int             iRightVolume;                       // right volume (s15)
  //int             iRightVolRaw;                       // right psx volume value
  bool            isRightSweep;
  bool            isRightExpSlope;
  bool            isRightDecreased;
  int             iRawPitch;                          // raw pitch (0x0000 - 0x3fff)
  // int             iIrqDone;                           // debug irq done flag
  bool            bRVBActive;                         // reverb active flag
  int             iRVBOffset;                         // reverb offset
  int             iRVBRepeat;                         // reverb repeat
  bool            bNoise;                             // noise active flag
  int            bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
  int             iRVBNum;                            // another reverb helper
  int             iOldNoise;                          // old noise val for this channel
  ADSRInfo        ADSR;                               // active ADSR settings
  ADSRInfoEx      ADSRX;                              // next ADSR settings (will be moved to active on sample start)

  bool IsOn() const {
    // wxMutexLocker locker(on_mutex_);
    return is_on_;
  }

  void NotifyOnNoteOn() const;
  void NotifyOnNoteOff() const;
  // void NotifyOnChangeToneNumber() const;
  void NotifyOnChangePitch() const;
  void NotifyOnChangeVelocity() const;

private:
  static uint32_t rateTable[160];

  bool            is_on_;

  // mutable wxMutex on_mutex_;

  friend class ChannelArray;
};



class ChannelArray: public ::SoundBlock
{
public:
  ChannelArray(SPU* pSPU, int channelNumber);

  unsigned int block_size() const {
    return channelNumber_ * 2;
  }

  Sample& Ch(int ch) { return channels_.at(ch); }
  ChannelInfo& At(int ch) { return channels_.at(ch); }

  unsigned int channel_count() const {
    return channelNumber_;
  }

  // int GetChannelNumber() const;
  bool ExistsNew() const;
  void SoundNew(uint32_t flags, int start);
  void VoiceOff(uint32_t flags, int start);

  void Notify() const;

private:
  SPU* pSPU_;
  // ChannelInfo* channels_;
  wxVector<ChannelInfo> channels_;
  int channelNumber_;
  uint32_t flagNewChannels_;

  friend class ChannelInfo;
};




////////////////////////////////////////////////////////////////////////
// Enums
////////////////////////////////////////////////////////////////////////



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
  void ReadDMA4Memory(uint32_t psxAddr, uint32_t size);
  void WriteDMA4(uint16_t);
  void WriteDMA4Memory(uint32_t psxAddr, uint32_t size);

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
  unsigned short m_spuMem[256*1024];
public:
  uint8_t* const Memory;   // uchar* version of spuMem

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

public:
  unsigned short Sp0;
  unsigned short Status;
  SPUAddr IrqAddr;
  mutable SPUAddr dma_current_addr_;
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


  // ADSR
  // unsigned long m_RateTable[160];

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
  return (uint8_t*)m_spuMem;
}

inline uint32_t SPU::GetIRQAddress() const {
  return IrqAddr;
}

inline void SPU::SetIRQAddress(SPUAddr addr) {
  IrqAddr = addr;
  m_pSpuIrq = GetSoundBuffer() + addr;
}


}   // namespace SPU


#include <wx/event.h>

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);


// extern SPU::SPU& Spu;
