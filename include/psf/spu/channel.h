#ifndef SPU_CHANNEL_H_
#define SPU_CHANNEL_H_

#include <wx/scopedptr.h>
#include <wx/vector.h>

// #include "common/SoundFormat.h"
#include "psf/spu/soundbank.h"


class Sample;

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
typedef wxScopedPtr<InterpolationBase> InterpolationPtr;

class SPUBase;
class SPUVoiceManager;


class SPUVoice/* : public ::Sample16 */ {
public:
  SPUVoice();   // for vector constructor
  SPUVoice(const SPUVoice&);
  SPUVoice(SPUBase* pSPU, int ch);

  void StartSound();
  static void InitADSR();
  int AdvanceEnvelope();

  void Step();
  bool Get(Sample* dest) const;

  SPUBase& Spu() { return *p_spu_; }
  const SPUBase& Spu() const { return *p_spu_; }

protected:
  void VoiceChangeFrequency();
  // void ADPCM2LPCM();

private:
  SPUBase* const p_spu_;

public:
  const int ch;

  // for REVERB
  // wxSharedArray<short> lpcm_buffer_l;
  // wxSharedArray<short> lpcm_buffer_r;
  wxVector<short> lpcm_buffer_l;
  wxVector<short> lpcm_buffer_r;

  // bool            bNew;                               // start flag
  // bool            isUpdating;

  int             iSBPos;                             // mixing stuff
  InterpolationPtr  pInterpolation;

  int             sval;
  SPUInstrument_New* tone;
  InstrumentDataIterator itrTone;
  SPUAddr addr;

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
  EnvelopeActive        ADSR;                               // active ADSR settings
  EnvelopePassive      ADSRX;                              // next ADSR settings (will be moved to active on sample start)

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



class SPUCoreVoiceManager/* : public ::SoundBlock */ {

public:
  SPUCoreVoiceManager();
  SPUCoreVoiceManager(SPUBase* pSPU, int channel_count);

  unsigned int block_size() const {
    return channel_count_ * sizeof(float);
  }

  Sample& Ch(int ch);

private:
  SPUBase* const pSPU_;
  int channel_count_;
};





class SPUVoiceManager/* : public ::SoundBlock */ {
public:
  SPUVoiceManager();  // for vector constructor
  SPUVoiceManager(SPUBase* pSPU, int channelNumber);

  /* Sample& Ch(int ch) { return channels_.at(ch); } */
  SPUVoice& At(int ch) { return channels_.at(ch); }

  unsigned int channel_count() const {
    return channels_.size();
  }

  bool ExistsNew() const;
  void SoundNew(uint32_t flags, int start);
  void VoiceOff(uint32_t flags, int start);

  void StepForAll();
  void ResetStepStatus();

private:
  SPUBase* const pSPU_;
  // ChannelInfo* channels_;
  wxVector<SPUVoice> channels_;
  uint32_t flagNewChannels_;

  friend class SPUVoice;
};








} // namespace SPU


#endif  // SPU_CHANNEL_H_
