#pragma once
#include <array>
// #include "snesapu.h"

namespace snesapu {

//! SPC700 build options bits 7-0 ----------------
const uint32_t SA_HALFC = 0x02; //!< Half-carry enabled
const uint32_t SA_CNTBK = 0x04; //!< Counter Break
const uint32_t SA_SPEED = 0x08; //!< Speed hack
const uint32_t SA_IPLW  = 0x10; //!< IPL ROM region writeable
const uint32_t SA_DSPBK = 0x20; //!< Break SPC700/Update DSP if 0F3h is read from
    
//! DSP build options bits 15-8 ------------------
const uint32_t SA_VMETERM = 0x100; //!< Volume metering on main output (for APR)
const uint32_t SA_VMETERC = 0x200; //!< Volume metering on voices (for visualization)
const uint32_t SA_SNESINT = 0x400; //!< Use pregenerated gaussian curve from SNES
const uint32_t SA_STEREO  = 0x800; //!< Stereo controls (seperation and EFBCT)
    
//! APU build options bits 23-16 -----------------
const uint32_t SA_DEBUG    = 0x10000; //!< Debugging ability
const uint32_t SA_DSPINTEG = 0x20000; //!< DSP emulation is integrated with the SPC700
    
const uint32_t APURAMSIZE = 0x10000;        //!< APU RAM Memory Size
const uint32_t SCR700SIZE = 0x100000;       //!< Script700 Program Area Size
const uint32_t SCR700MASK = SCR700SIZE - 1; //!< Script700 Program Area Mask
    
//! SNESAPU callback effect
const uint32_t CBE_DSPREG  = 0x01;        //!< Write DSP value event
const uint32_t CBE_INCS700 = 0x40000000;  //!< Include Script700 text file
const uint32_t CBE_INCDATA = 0x20000000;  //!< Include Script700 binary file
    
//! SNESAPU callback function
typedef uint32_t (*CBFUNC)(uint32_t, uint32_t, uint32_t, void*);

// class SNESAPU;
    
class APU {
        
protected:
  APU(SNESAPU* snesapu);
  ~APU();

public:
  // SNESAPUInfo();
  /*!
   * \brief Set SNESAPU Callback Function
   * \param [in] pCbFunc Pointer of SNESAPU callback function <br/>
   *             Callback function definition: <br/>
   *             uint32_t Callback(uint32_t effect, uint32_t addr, uint32_t value, void *lpData) <br/>
   *             Usually, will return value of 'value' parameter.
   * \param [in] cbMask SNESAPU callback mask
   * \return CBFUNC
   */
  CBFUNC		SNESAPUCallback(CBFUNC pCbFunc, uint32_t cbMask);

  /*!
   * \brief Get SNESAPU Data Pointers
   * \param [out] ppAPURAM   64KB Sound RAM
   * \param [out] ppExtraRAM 128byte extra RAM
   * \param [out] ppSPCOut   APU 4 ports of output
   * \param [out] ppT64Cnt   64kHz timer counter
   * \param [out] ppDSP      128byte DSPRAM structure (see DSP)
   * \param [out] ppMix      VoiceMix structures of 8 voices (see DSP)
   * \param [out] ppVMMaxL   Max master volume (left)
   * \param [out] ppVMMaxR   Max master volume (right)
   */
  void      GetAPUData(uint8_t **ppAPURAM, uint8_t **ppExtraRAM, uint8_t **ppSPCOut, uint32_t **ppT64Cnt, DSPReg **ppDSP, Voice **ppMix, uint32_t **ppVMMaxL, uint32_t **ppVMMaxR);

  /*!
   * \brief Get Script700 Data Pointers
   * \param [out] pVer        SNESAPU version (32byte string)
   * \param [out] ppSPCReg    Pointer of SPC700 register
   * \param [out] ppScript700 Pointer of Script700 work memory
   */
  void      GetScript700Data(char *pVer, uint32_t **ppSPCReg, uint8_t **ppScript700);

  /*!
   * \brief Reset Audio Processor
   * \details Clears all memory, sets registers to default values, and sets the amplification level.
   * \param [in] amp Amplification (-1 = keep current amp level, see SetDSPAmp for more information)
   */
  void      ResetAPU(uint32_t amp);

  /*!
   * \brief Fix Audio Processor After Load
   * \details Prepares the sound processor for emulation after an .SPC/.ZST is loaded.
   * \param [in] pc  SPC700 internal register
   * \param [in] a   SPC700 internal register
   * \param [in] y   SPC700 internal register
   * \param [in] x   SPC700 internal register
   * \param [in] psw SPC700 internal register
   * \param [in] sp  SPC700 internal register
   */
  void      FixAPU(uint16_t pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);

  /*!
   * \brief Load SPC File
   * \details Restores the APU state from an SPC file. This eliminates the need to call ResetAPU, copy memory, and call FixAPU.
   * \param [in] pSPC 66048 byte SPC file
   */
  void      LoadSPCFile(void *pSPC);

  /*!
   * \brief Set Audio Processor Options
   * \details Configures the sound processor emulator. Range checking is performed on all parameters.
   * \note -1 can be passed for any parameter you want to remain unchanged
//         see SetDSPOpt() in DSP.h for a more detailed explantion of the options
   * \param [in] mix   Mixing routine (default 1)
   * \param [in] chn   Number of channels (1 or 2, default 2)
   * \param [in] bits  Sample size (8, 16, 24, 32, or -32 [IEEE 754], default 16)
   * \param [in] rate  Sample rate (8000-192000, default 32000)
   * \param [in] inter Interpolation type (default INT_GAUSS)
   * \param [in] opts  See 'DSP options' in the Defines section of DSP.h
   */
  void      SetAPUOpt(uint32_t mix = 1, uint32_t chn = 2, uint32_t bits = 16, uint32_t rate = 32000, uint32_t inter = INT_GAUSS, uint32_t opts);

  /*!
   * \brief Set Audio Processor Sample Clock
   * \details Calculates the ratio of emulated clock cycles to sample output.
   *          Used to speed up or slow down a song without affecting the pitch.
   * \param [in] speed Multiplier [16.16] (1/2x to 16x)
   */
  void      SetAPUSmpClk(uint32_t speed);

  /*!
   * \brief Set Audio Processor Song Length
   * \details Sets the length of the song and fade.
   * \note If a song is not playing, you must call ResetAPU or set T64Cnt to 0 before calling this.
   *       To set a song with no length, pass -1 and 0 for the song and fade.
   * \param [in] song Length of song (in 1/64000ths second)
   * \param [in] fade Length of fade (in 1/64000ths second)
   * \return Total length
   */
  uint32_t  SetAPULength(uint32_t song, uint32_t fade);

  /*!
   * \brief Emulate Audio Processing Unit
   * \details Emulates the APU for a specified amount of time.  DSP output is placed in a buffer to be handled by the main program.
   * \param [out] pBuf Buffer to store output samples
   * \param [in] len Length of time to emulate (must be > 0)
   * \param [in] type Type of parameter passed in len <br/>
   *                  0 - len is the number of APU clock cycles to emulate (APU_CLK = 1 second) <br/>
   *                  1 - len is the number of samples to generate
   * \return End of buffer
   */
  void*	    EmuAPU(void *pBuf, uint32_t len, bool type);

  /*!
   * \brief Seek to Position
   * \details Seeks forward in the song from the current position.
   * \param [in] time 1/64000ths of a second to seek forward (must be >= 0)
   * \param [in] fast Use faster seeking method (may break some songs)
   */
  void      SeekAPU(uint32_t time, bool fast);

  /*!
   * \brief Set/Reset TimerTrick Compatible Function
   * \details The setting of TimerTrick is converted into Script700, and it functions as Script700.
   * \param [in] port SPC700 port number (0 - 3 / 0xF4 - 0xF7).
   * \param [in] wait Wait time (1 - 0xFFFFFFFF).  If this parameter is 0, TimerTrick and Script700 is disabled.
   */
  void SetTimerTrick(uint32_t port, uint32_t wait);

  /*!
   * \brief Set/Reset Script700 Compatible Function
   * \details Script700 is a function to emulate the signal exchanged between 65C816 and SPC700 of SNES.
   * \param [in] pSource Pointer to a null-terminated string buffer in which the Script700 command data was stored.
   *                     If this parameter is NULL, Script700 is disabled.
   * \return a binary-converting result of the Script700 command. <br/>
   *         >=1 : Last index of array of the program memory used. Script700 is enabled. <br/>
   *         0   : NULL was set in the pSource parameter. Script700 is disabled. <br/>
   *         -1  : Error occurred by binary-converting Script700. Script700 is disabled. <br/>
   */
  uint32_t SetScript700(void *pSource);

  /*!
   * \brief Set Script700 Binary Data Function
   * \param [in] addr  Data area address of the destination to copy
   * \param [in] pData Pointer to binary data buffer. If this parameter is NULL, no operation.
   * \param [in] size  Size of buffer
   * \return a binary-converting result of the Script700 command. <br/>
   *         >=1 : Last index of array of the program memory used. Script700 is enabled. <br/>
   *         0   : NULL was set in the pData parameter. Script700 status will be not changed. <br/>
   *         -1  : Error occurred by binary-converting Script700. Script700 is disabled.
   */
  uint32_t SetScript700Data(uint32_t addr, void *pData, uint32_t size);

  /*!
   * \brief Get SNESAPU Context Buffer Size Function
   * \details SNESAPU Context is snapshot of global variables. This function returns buffer size required for snapshot.
   * \return Buffer size for copy of context data
   */
  uint32_t GetSNESAPUContextSize();

  /*!
   * \brief Get SNESAPU Context Data Function
   * \details Copy the contents of SNESAPU Context for buffer. This means that to take a snapshot of SNESAPU.
   * \param [in] pCtxOut Pointer of context buffer
   * \return Reserved
   */
  uint32_t GetSNESAPUContext(void *pCtxOut);

  /*!
   * \brief Set SNESAPU Context Data Function
   * \details Copy the contents of SNESAPU Context from buffer. This means that to revert a snapshot of SNESAPU.
   * \param [in] pCtxIn Pointer of context buffer
   * \return Reserved
   */
  uint32_t SetSNESAPUContext(void *pCtxIn);

private:
  SNESAPU* const snesapu_;

  std::array<uint8_t, APURAMSIZE*2+16> apuRAMBuf;
  std::array<uint8_t, SCR700SIZE+16> scrRAMBuf;

  std::array<uint32_t, 1024> scr700lbl;
  std::array<uint8_t,  256>  scr700dsp; //!< Script700 DSP Enable Flag (Channel)
  std::array<uint8_t,  32>   scr700mds; //!< Script700 DSP Enable Flag (Master)
  std::array<uint8_t,  256>  scr700chg; //!< Script700 DSP Note Change
  std::array<uint32_t, 256>  scr700det; //!< Script700 DSP Rate Detune
  std::array<uint32_t, 256>  scr700vol; //!< Script700 DSP Volume Change (Source)
  std::array<uint32_t, 256>  scr700mvl; //!< Script700 DSP Volume Change (Master)
  std::array<uint32_t, 8>    scr700wrk; //!< Script700 User Work Area
  std::array<uint32_t, 2>    scr700cmp; //!< Script700 Cmp Param

  uint32_t scr700cnt; //!< Script700 Wait Count
  uint32_t scr700ptr; //!< Script700 Program Pointer
  uint32_t scr700stf;
  uint32_t scr700dat; //!< Script700 Data Area Offset
  uint32_t scr700stp;

  uint32_t scr700jmp;
  uint32_t scr700tmp;

  std::array<uint32_t, 128> scr700stk;

  std::array<uint32_t, 3> scr700inc;
  std::array<uint32_t, 256> scr700pth;

  uint8_t* pAPURAM; //!< Pointer to SNESAPU 64KB RAM
  uint8_t* pSCRRAM; //!< Pointer to Script700 RAM

  uint32_t cycLeft;
  uint32_t smpRate;
  uint32_t smpRAdj;
  uint32_t smpREmu;

  uint32_t apuCbMask;
  CBFUNC   apuCbFunc;

  uint32_t apuVarEP;


};


} // namespace snesapu
