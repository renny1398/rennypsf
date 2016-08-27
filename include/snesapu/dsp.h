#pragma once
#include <cstdint>

namespace snesapu {

//! Interpolation routines
const uint32_t INT_NONE	  = 0;  //!< None
const uint32_t INT_LINEAR	= 1;  //!< Linear
const uint32_t INT_CUBIC	= 2;  //!< Cubic Spline
const uint32_t INT_GAUSS	= 3;  //!< SNES Gaussian
const uint32_t INT_SINC	  = 4;  //!< 8-point Sinc
const uint32_t INT_GAUSS4	= 7;  //!< 4-point Gaussian

//! DSP options
const uint32_t DSP_ANALOG  = 0x01;        //!< Simulate analog anomalies (low-pass filter)
const uint32_t DSP_OLDSMP  = 0x02;        //!< Old ADPCM sample decompression routine
const uint32_t DSP_SURND   = 0x04;        //!< Surround sound
const uint32_t DSP_REVERSE = 0x08;        //!< Reverse stereo samples
const uint32_t DSP_NOECHO  = 0x10;        //!< Disable echo
const uint32_t DSP_NOPMOD  = 0x20;        //!< Disable pitch modulation
const uint32_t DSP_NOPREAD = 0x40;        //!< Disable pitch readƒ
const uint32_t DSP_NOFIR   = 0x80;        //!< Disable FIR filter
const uint32_t DSP_BASS    = 0x100;       //!< BASS BOOST (low-pass filter)
const uint32_t DSP_NOENV   = 0x200;       //!< Disable envelope
const uint32_t DSP_NONOISE = 0x400;       //!< Disable noise
const uint32_t DSP_ECHOMEM = 0x800;       //!< Write DSP echo memory map
const uint32_t DSP_NOSURND = 0x1000;      //!< Disable surround sound
const uint32_t DSP_FLOAT   = 0x40000000;  //!< 32bit floating-point volume output
const uint32_t DSP_NOSAFE  = 0x80000000;  //!< Disable volume safe

//! PackWave options
const uint32_t BRR_LINEAR = 0x01; //!< Use linear compression for all blocks
const uint32_t BRR_LOOP   = 0x02; //!< Set loop flag in block header
const uint32_t BRR_NOINIT = 0x04; //!< Don't create an initial block of silence
const uint32_t BRR_8BIT   = 0x10; //!< Input samples are 8-bit

//! Mixing flags
const uint32_t MFLG_MUTE  = 0x01; //!< Voice is muted (set by user)
const uint32_t MFLG_NOISE = 0x02; //!< Voice is noise (set by user)
const uint32_t MFLG_USER  = 0x03; //!< Flags by user
const uint32_t MFLG_KOFF  = 0x04; //!< Voice is in the process of keying off
const uint32_t MFLG_OFF   = 0x08; //!< Voice is currently inactive
const uint32_t MFLG_END   = 0x10; //!< End block was just played

//! Script700 DSP flags
const uint32_t S700_MUTE   = 0x01; //!< Mute voice
const uint32_t S700_CHANGE = 0x02; //!< Change sound source (note change)
const uint32_t S700_DETUNE = 0x04; //!< Detune sound pitch rate
const uint32_t S700_VOLUME = 0x08; //!< Change sound volume

//! Script700 DSP master parameters
const uint32_t S700_MVOL_L = 0x00; //!< Master volume (left)
const uint32_t S700_MVOL_R = 0x01; //!< Master volume (right)
const uint32_t S700_ECHO_L = 0x02; //!< Echo volume (left)
const uint32_t S700_ECHO_R = 0x03; //!< Echo volume (right)

//! DSP registers
struct DSPVoice {
  int8_t   volL;    //!< Volume Left
  int8_t   volR;    //!< Volume Right
  uint16_t pitch;   //!< Pitch (rate/32000) (3.11)
  uint8_t  srcn;    //!< Sound source being played back
  uint8_t  adsr[2]; //!< Envelope rates for attack, decay, and sustain
  uint8_t  gain;    //!< Envelope gain (if not using ADSR)
  int8_t   envx;    //!< Current envelope height (.7)
  int8_t   outx;    //!< Current sample being output (-.7)
  int8_t   __r[6];
};

struct DSPFIR {
  int8_t __r[15];
  int8_t c;       //!< Filter coefficient
};

union DSPReg
{
  DSPVoice  voice[8];   //! Voice registers

   //! Global registers
  struct {
    int8_t  __r00[12];
    int8_t  mvolL;      //!< Main Volume Left (-.7)
    int8_t  efb;        //!< Echo Feedback (-.7)
    int8_t  __r0E;
    int8_t  c0;         //!< FIR filter coefficent (-.7)

    int8_t	__r10[12];
    int8_t	mvolR;      //!< Main Volume Right (-.7)
    int8_t	__r1D;
    int8_t	__r1E;
    int8_t	c1;

    int8_t	__r20[12];
    int8_t	evolL;      //!< Echo Volume Left (-.7)
    uint8_t	pmon;       //!< Pitch Modulation on/off for each voice
    int8_t	__r2E;
    int8_t	c2;

    int8_t	__r30[12];
    int8_t	evolR;      //!< Echo Volume Right (-.7)
    uint8_t	non;        //!< Noise output on/off for each voice
    int8_t	__r3E;
    int8_t	c3;

    int8_t	__r40[12];
    uint8_t	kon;        //!< Key On for each voice
    uint8_t	eon;        //!< Echo on/off for each voice
    int8_t	__r4E;
    int8_t	c4;

    int8_t	__r50[12];
    uint8_t	kof;        //!< Key Off for each voice (instantiates release mode)
    uint8_t	dir;        //!< Page containing source directory (wave table offsets)
    int8_t	__r5E;
    int8_t	c5;

    int8_t	__r60[12];
    uint8_t	flg;        //!< DSP flags and noise frequency
    uint8_t	esa;        //!< Starting page used to store echo waveform
    int8_t	__r6E;
    int8_t	c6;

    int8_t	__r70[12];
    uint8_t	endx;       //!< Waveform has ended
    uint8_t	edl;        //!< Echo Delay in ms >> 4
    int8_t	__r7E;
    int8_t	c7;
  };

  DSPFIR  fir[8];       //!< FIR filter

  uint8_t		reg[128];
};

//! Internal mixing data
struct MixF {
  uint8_t mute:1;     //!< Voice is muted (set by user)
  uint8_t noise:1;    //!< Voice is noise (set by user)
  uint8_t keyOff:1;   //!< Voice is in key off mode
  uint8_t inactive:1; //!< Voice is inactive, no samples are being played
  uint8_t keyEnd:1;   //!< End block was just played
  uint8_t __r2:3;
};

enum EnvM {
  ENV_DEC,        //!< Linear decrease
  ENV_EXP,        //!< Exponential decrease
  ENV_INC,        //!< Linear increase
  ENV_BENT = 6,   //!< Bent line increase
  ENV_DIR,        //!< Direct setting
  ENV_REL,        //!< Release mode (key off)
  ENV_SUST,       //!< Sustain mode
  ENV_ATTACK,     //!< Attack mode
  ENV_DECAY = 13  //!< Decay mode
};

const uint32_t ENVM_IDLE = 0x80;  //!< Envelope is marked as idle, or not changing
const uint32_t ENVM_MODE = 0x0F;  //!< Envelope mode is stored in lower four bits

struct Voice {
  // Voice
  uint16_t  vAdsr;    //!< ADSR parameters when KON was written
  uint8_t   vGain;    //!< Gain parameters when KON was written
  uint8_t   vRsv;     //!< Changed ADSR/Gain parameters flag
  int16_t   *sIdx;    //!< -> current sample in sBuf
  // Waveform
  void      *bCur;    //!< -> current block
  uint8_t		bHdr;     //!< Block Header for current block
  uint8_t		mFlg;     //!< Mixing flags (see MixF)
  // Envelope
  uint8_t   eMode;    //!< [3-0] Current mode (see EnvM)
                      //!< [6-4] ADSR mode to switch into from Gain
                      //!< [7]   Envelope is idle
  uint8_t   eRIdx;    //!< Index in RateTab (0-31)
  uint32_t  eRate;    //!< Rate of envelope adjustment (16.16)
  uint32_t  eCnt;     //!< Sample counter (16.16)
  uint32_t  eVal;     //!< Current envelope value
  int32_t   eAdj;     //!< Amount to adjust envelope height
  uint32_t  eDest;    //!< Envelope Destination
  // Visualization
  int32_t   vMaxL;    //!< Maximum absolute sample output
  int32_t   vMaxR;
  // Samples
  int16_t   sP1;      //!< Last sample decompressed (prev1)
  int16_t   sP2;      //!< Second to last sample (prev2)
  int16_t   sBufP[8]; //!< Last 8 samples from previous block (needed for inter.)
  int16_t   sBuf[16]; //!< 32 bytes for decompressed sample blocks
  // Mixing
  float     mTgtL;    //!< Target volume (floating-point routine only)
  float     mTgtR;    //!< "  "
  int32_t   mChnL;    //!< Channel Volume (-24.7)
  int32_t   mChnR;    //!< "  "
  uint32_t  mRate;    //!< Pitch Rate after modulation (16.16)
  uint16_t  mDec;     //!< Pitch Decimal (.16) (used as delta for interpolation)
  uint8_t   mSrc;     //!< Current source number
  uint8_t   mKOn;     //!< Delay time from writing KON to output
  uint32_t  mOrgP;    //!< Original pitch rate converted from the DSP (16.16)
  int32_t   mOut;     //!< Last sample output before chn vol (used for pitch mod)
};

/*!
 * \brief DSP Debugging Routine
 * \details A prototype for a function that gets called for debugging purposes. <br/>
 *          The paramaters passed in can be modified, and on return will be used to update the internal registers.
 *          Set bit-7 of 'reg' to prevent the DSP from handling the write. <br/>
 *          For calling a pointer to another debugging routine, use the following macro.
 * \param [in] reg Pointer to current register (LOBYTE = register index)
 * \param [in] val Value being written to register
 */
typedef void (*DSPDebug)(volatile uint8_t *reg, volatile uint8_t val);

// const uint32_t _CallDSPDebug(uint32_t (*proc)(uint32_t, uint32_t), uint32_t reg, uint32_t val);

class DSP {

public:
  /*!
   * \brief  Initialize DSP
   * \details Creates the lookup tables for interpolation, and sets the default mixing settings:
   * \par
   *        mixType = 1 <br/>
   *        numChn  = 2 <br/>
   *        bits    = 16 <br/>
   *        rate    = 32000 <br/>
   *        inter   = INT_GAUSS <br/>
   *        opts    = 0
   * \note Callers should use InitAPU instead
   * \par Destroys:
   *        ST(0-7)
   */
  DSP();

  /*!
   * \brief Reset DSP
   * \details Resets the DSP registers, erases internal variables, and resets the volume.
   * \note Callers should use ResetAPU instead
   * \par Destroys
   *        EAX
   */
  void ResetDSP();

  /*!
   * \brief Set DSP Options
   * \details Recalculates tables, changes the output sample rate, and sets up the mixing routine.
   * \note Range checking is performed on all parameters.
   *       If a parameter does not match the required range of values, the default value will be assumed. <br/>
   *       -1 can be used for any paramater that should remain unchanged. <br/>
   *       Callers should use SetAPUOpt instead
   * \param [in] mix   Mixing routine (default 1)
   * \param [in] chn   Number of channels (1 or 2, default 2)
   * \param [in] bits  Sample size (8, 16, 24, 32, or -32 [IEEE 754], default 16)
   * \param [in] rate  Sample rate (8000-192000, default 32000)
   * \param [in] inter Interpolation type (default INT_GAUSS)
   * \param [in] opts  See 'DSP options' in the Defines section
   * \par Destroys
   *        EAX
   */
  void SetDSPOpt(uint32_t mix, uint32_t chn, uint32_t bits, uint32_t rate, uint32_t inter, uint32_t opts);

  /*!
   * \brief Debug DSP
   * \details Installs a vector that gets called each time a value is written to the DSP data register.
   * \note The build option SA_DEBUG must be enabled
   * \param [in] pTrace Pointer to debug function (a null pointer turns off the debug call)
   * \return Previously installed vector
   */
  DSPDebug* SetDSPDbg(DSPDebug *pTrace);

  /*!
   * \brief  Fix DSP After Loading SPC File
   * \details Initializes the internal mixer variables.
   * \note Callers should use FixAPU instead
   * \par Destroys
   *        EAX
   */
  void FixDSP();

  /*!
   * \brief Fix DSP After Seeking
   * \details Puts all DSP voices in a key off state and erases echo region.
   * \param [in] reset True  = Reset all voices <br/> False = Only erase memory
   * \par Destroys
   *        EAX
   */
  void FixSeek(uint8_t reset);

  /*!
   * \brief Set DSP Base Pitch
   * \details Adjusts the pitch of the DSP.
   * \param [in] base Base sample rate (32000 = Normal pitch, 32458 = Old SB cards, 32768 = Old ZSNES)
   * \par Destroys
   *        EAX
   */
  void SetDSPPitch(uint32_t base);

  /*!
   * \brief Set DSP Amplification
   * \details This value is applied to the output with the main volumes.
   * \param [in] amp Amplification level [-15.16] (1.0 = SNES, negative values act as 0)
   * \par Destroys
   *        EAX
   */
  void SetDSPAmp(uint32_t amp);

  /*!
   * \brief Set DSP Volume
   * \details This value attenuates the output and was implemented to allow songs to be faded out.
   * \note ResetDSP sets this value to 65536 (no attenuation).
   *       This function is called internally by EmuAPU and SetAPULength, and should not be called by the user.
   * \param [in] vol Volume [-1.16] (0.0 to 1.0, negative values act as 0)
   * \par Destroys
   *        EAX
   */
  void SetDSPVol(uint32_t vol);

  /*!
   * \brief Set Voice Stereo Separation
   * \details Sets the amount to adjust the panning position of each voice.
   * \param [in] sep Separation [1.16] <br/>
   *                 1.0 - full separation (output is either left, center, or right) <br/>
   *                 0.5 - normal separation (output is unchanged) <br/>
   *                   0 - no separation (output is completely monaural)
   * \par Destroys
   *        EAX, ST(0-7)
   */
  void SetDSPStereo(uint32_t sep);

  /*!
   * \brief Set Echo Feedback Crosstalk
   * \details Sets the amount of crosstalk between the left and right channel during echo feedback.
   * \param [in] leak Crosstalk amount [-1.15] <br/>
   *                   1.0 - no crosstalk (SNES) <br/>
   *                     0 - full crosstalk (mono/center) <br/>
   *                  -1.0 - inverse crosstalk (L/R swapped)
   * \par Destroys
   *        EAX
   */
  void SetDSPEFBCT(int32_t leak);

  /*!
   * \brief DSP Data Port
   * \details Writes a value to a specified DSP register and alters the DSP accordingly.
   *          If the register write affects the output generated by the DSP, this function returns true.
   * \param [in] reg DSP Address
   * \param [in] val DSP Data
   * \return true, if the DSP state was affected
   */
  uint8_t SetDSPReg(uint8_t reg, uint8_t val);

  /*!
   * \brief Emulate DSP
   * \details Emulates the DSP of the SNES.
   * \note If 'pBuf' is NULL, the routine MIX_NONE will be used <br/>
   *       Range checking is performed on 'size' <br/>
   *       Callers should use EmuAPU instead
   * \param [in] pBuf Pointer to buffer to store output
   * \param [in] size Length of buffer (in samples, can be 0)
   * \return Pointer to end of buffer
   * \par Destroys
   *        ST(0-7)
   */
  void* EmuDSP(void *pBuf, int32_t size);

private:
  DSPReg    dsp;    //!< DSP registers
  Voice     mix[8]; //!< Mixing structures for each voice
  uint32_t  vMMaxL; //!< Maximum absolute sample output (left)
  uint32_t  vMMaxR; //!< Maximum absolute sample output (right)
};


} // namespace snesapu
