#ifndef SPU_CHANNEL_H_
#define SPU_CHANNEL_H_


#include <wx/scopedptr.h>
#include <wx/vector.h>

#include "SoundFormat.h"
#include "spu/soundbank.h"


namespace SPU {


class ChannelInfo;

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
  virtual int AdvanceEnvelope(ChannelInfo* ch) const = 0;
protected:
  //! A default constructor.
  EnvelopeState() {}
  //! A virtual destructor.
  virtual ~EnvelopeState() {}
};



class ChannelInfoListener {

  friend class ChannelInfo;

public:
  virtual ~ChannelInfoListener() {}

protected:
  virtual void OnNoteOn(const ChannelInfo&) {}
  virtual void OnNoteOff(const ChannelInfo&) {}
};



/*
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
*/


class ADSRInfoEx
{
public:
  void Start();

  const EnvelopeState* State;
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




class InterpolationBase;
typedef wxScopedPtr<InterpolationBase> InterpolationPtr;

class SPU;
class ChannelArray;


class ChannelInfo : public ::Sample16 {
public:
  ChannelInfo(SPU* pSPU = 0);
  ChannelInfo(const ChannelInfo&);  // for vector construction

  void StartSound();
  static void InitADSR();

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
  // ADSRInfo        ADSR;                               // active ADSR settings
  ADSRInfoEx      ADSRX;                              // next ADSR settings (will be moved to active on sample start)

  bool IsOn() const {
    // wxMutexLocker locker(on_mutex_);
    return is_on_;
  }

  void On();
  void Off();

  void NotifyOnNoteOn() const;
  void NotifyOnNoteOff() const;
  // void NotifyOnChangeToneNumber() const;
  void NotifyOnChangePitch() const;
  void NotifyOnChangeVelocity() const;

private:
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








} // namespace SPU


#endif  // SPU_CHANNEL_H_
