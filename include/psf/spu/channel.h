#pragma once

#include <wx/vector.h>

// #include "common/SoundFormat.h"
#include "psf/spu/soundbank.h"

// class Sample;
class SampleSequence;

namespace SPU {


class SPUVoice;

/*!
 * @class Rate
 * @brief Rate class for envelope.
 */
class Rate {
public:
  Rate() : rate_(0) {}
  ~Rate();

  int32_t Get() const;
  void Set(int32_t r);

private:
  int32_t rate_;
};


/*!
 * @class EnvelopeState
 * @brief A base class of envelope state.
 */
class EnvelopeState {
public:
  /*!
   * \brief Advance the envelope state of a ChannelInfo instance.
   * \param ch A pointer to ChannelInfo.
   * \return An envelope state.
   */
  virtual int Advance(SPUVoice* ch) const = 0;

  static void SetState(SPUVoice* ch, const EnvelopeState* state);

protected:
  //! A default constructor.
  EnvelopeState() {}
  //! A virtual destructor.
  virtual ~EnvelopeState() {}
};


class SPUVoiceListener {

  friend class SPUVoice;

public:
  virtual ~SPUVoiceListener() {}

protected:
  virtual void OnNoteOn(const SPUVoice&) {}
  virtual void OnNoteOff(const SPUVoice&) {}
};


class EnvelopeInfo {

public:
  EnvelopeInfo();
  ~EnvelopeInfo();

  bool attack_mode_exp() const;
  char attack_rate() const;

  char decay_rate() const;

  int sustain_level() const;
  bool sustain_mode_exp() const;
  int sustain_increase() const;
  char sustain_rate() const;

  bool release_mode_exp() const;
  char release_rate() const;

protected:
  bool attack_mode_exp_;
  char attack_rate_;            // 0 - 127

  char decay_rate_;             // 64 - 127

  int  sustain_level_;
  bool sustain_mode_exp_;
  int sustain_increase_;
  char sustain_rate_;           // 0 - 127

  bool release_mode_exp_;
  char release_rate_;           // 0 - 127
};


class EnvelopePassive : public EnvelopeInfo {

public:
  void set_attack_mode_exp(bool exp);
  void set_attack_rate(char rate);

  void set_decay_rate(char rate);

  void set_sustain_level(int level);
  void set_sustain_mode_exp(bool exp);
  void set_sustain_increase(int inc);
  void set_sustain_rate(char rate);

  void set_release_mode_exp(bool exp);
  void set_release_rate(char rate);
};


class EnvelopeActive : public EnvelopeInfo {

public:
  EnvelopeActive();

  void Set(const EnvelopePassive& passive);
  void Release();
  void Stop();
  int AdvanceEnvelope(SPUVoice* ch);

  bool IsAttack() const;
  bool IsDecay() const;
  bool IsSustain() const;
  bool IsRelease() const;
  bool IsOff() const;

  int envelope_volume() const;
  void set_envelope_volume(int vol);
  long volume() const;
  void set_volume(long vol);

private:
  int            EnvelopeVol;
  long           lVolume;

  const EnvelopeState* State;

  friend class EnvelopeState;
  void set_envelope_state(const EnvelopeState* state);
};


class InterpolationBase;
typedef InterpolationBase* InterpolationPtr;

class SPUBase;
class SPUCore;
class SPUVoiceManager;

class SPUVoice {
public:
  // SPUVoice();   // for vector constructor
  SPUVoice(const SPUVoice&);
  SPUVoice(SPUCore* p_core);
  ~SPUVoice();

  void StartSound();
  static void InitADSR();
  int AdvanceEnvelope();

  void Advance();
  bool Get(SampleSequence* dest) const;

  SPUBase* p_spu();
  const SPUBase* p_spu() const;

protected:
  void VoiceChangeFrequency();
  // void ADPCM2LPCM();

private:
  // SPUBase* const p_spu_;
  SPUCore* const p_core_;

public:
  // for REVERB
  wxVector<short> lpcm_buffer_l;
  wxVector<short> lpcm_buffer_r;

  int             iSBPos;                             // mixing stuff
  InterpolationPtr  pInterpolation;

  int             sval;
  SPUInstrument_New* tone;
  InstrumentDataIterator itrTone;
  SPUAddr addr;

  // bool            is_muted;
  bool    hasReverb;                            // can we do reverb on this channel? must have ctrl register bit, to get active
  int     iActFreq;                           // current psx pitch
  int     iUsedFreq;                          // current pc pitch
  double  Pitch;
  int     iLeftVolume;                        // left volume (s15)
  bool    isLeftSweep;
  bool    isLeftExpSlope;
  bool    isLeftDecreased;
  SPUAddr addrExternalLoop;
  bool    useExternalLoop;                        // ignore loop bit, if an external loop address is used
  int     iRightVolume;                       // right volume (s15)
  bool    isRightSweep;
  bool    isRightExpSlope;
  bool    isRightDecreased;
  int     iRawPitch;                          // raw pitch (0x0000 - 0x3fff)
  // int     iIrqDone;                           // debug irq done flag
  bool    bRVBActive;                         // reverb active flag
  int     iRVBOffset;                         // reverb offset
  int     iRVBRepeat;                         // reverb repeat
  bool    bNoise;                             // noise active flag
  int     bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
  int     iRVBNum;                            // another reverb helper
  int     iOldNoise;                          // old noise val for this channel
  EnvelopeActive  ADSR;                               // active ADSR settings
  EnvelopePassive ADSRX;                              // next ADSR settings (will be moved to active on sample start)

  bool IsOn() const {
    // wxMutexLocker locker(on_mutex_);
    return is_on_;
  }

  void VoiceOn();
  void VoiceOff();
  void VoiceOffAndStop();

  void NotifyOnNoteOn() const;
  void NotifyOnNoteOff() const;
  // void NotifyOnChangeToneNumber() const;
  void NotifyOnChangePitch() const;
  void NotifyOnChangeVelocity() const;

  // temporary
  int envelope() const { return env_; }
  // int envelope_max() const;
  void set_envelope(int env) { env_ = env; }

protected:
  // bool IsReady() const;
  void SetReady() const;
  void SetUnready() const;

private:
  mutable bool is_ready_;
  mutable wxMutex ready_mutex_;
  mutable wxCondition ready_cond_;

  bool is_on_;

  // temporary
  int env_;

  // mutable wxMutex on_mutex_;

  friend class SPUVoiceManager;
};

class SPUCoreVoiceManager {
public:
  // SPUCoreVoiceManager() // for vector constructor
  //   : p_core_(nullptr) {}
  SPUCoreVoiceManager(SPUCore* p_core, int GetVoiceCount);

  SPUVoice& VoiceRef(int ch);
  unsigned int GetVoiceCount() const;
  int GetVoiceIndex(SPUVoice*) const;

  bool ExistsNew() const;
  void SoundNew(uint32_t flags, int start);
  void VoiceOff(uint32_t flags, int start);

  void Advance();

private:
  // SPUBase* const p_spu_;
  SPUCore* const p_core_;
  // int core_seq_;
  wxVector<SPUVoice> voices_;
  uint32_t new_flags_;
};



class SPUVoiceManager {
public:
  SPUVoiceManager();  // for vector constructor
  SPUVoiceManager(SPUBase* p_spu);

  SPUVoice& VoiceRef(int ch);
  unsigned int voice_count() const;

  bool ExistsNew() const;
  // void SoundNew(uint32_t flags, int start); // deprecated
  // void VoiceOff(uint32_t flags, int start); // deprecated

  void Advance();
  // void ResetStepStatus();

private:
  SPUBase* const p_spu_;
};

} // namespace SPU
