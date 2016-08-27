#ifndef	SNESAPU_H_
#define	SNESAPU_H_

namespace snesapu {

const int CPU_CYC = 24;  // 1.024MHz
const int DEBUG = 1;
const int DSPINTEG = 1;
const int HALFC = 1;
const int SPEED = 1;
const int IPLW = 1;
const int CNTBK = 1;
const int DSPBK = 1;
const int VMETERM = 1;
const int VMETERV = 1;
const int STEREO = 1;
const int APURAMSIZE = 0x10000;
const int SCR700SIZE = 0x100000;
const int SCR700MASK = SCR700SIZE - 1;
const int CBE_DSPREG = 0x01;
const int CBE_INCS700 = 0x40000000;
const int CBE_INCDATA = 0x20000000;

} // namespace snesapu

#include <typeinfo>
// #include "spc700.h"
// #include "apu.h"
// #include "dsp.h"

namespace snesapu {

class APU;
class DSP;
class SPC700;

class SNESAPU
{
private:
  SPC700* spc700_;
  APU* apu_;
  DSP* dsp_;

private:
  uint8_t   *ram;            // Base pointer to APU RAM
  uint8_t   *xram;           // Pointer to writeable IPL region
  uint8_t   *outPort;        // SPC700 output ports
  uint32_t  *t64Cnt;         // 64kHz timer counter
  DSPReg    *dsp;            // DSP registers
  Voice     *voice;          // Internal DSP mixing data
  uint32_t  *vMMaxL,*vMMaxR; // Max main sample output

public:
  SNESAPU();
  void*     EmuAPU(void *pBuf, uint32_t len, bool type);
  void*	    EmuDSP(void *pBuf, int32_t size);
  int32_t   EmuSPC(uint32_t cyc);
  void      FixAPU(u16 pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);
  void      FixDSP();
  void      FixSPC(u16 pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);
  void      FixSeek(uint8_t reset);
  void      GetAPUData(uint8_t **ppAPURAM, uint8_t **ppExtraRAM, uint8_t **ppSPCOut, uint32_t **ppT64Cnt, DSPReg **ppDSP, Voice **ppMix, uint32_t **ppVMMaxL, uint32_t **ppVMMaxR);
  void      GetScript700Data(char *pVer, uint32_t **ppSPCReg, uint8_t **ppScript700);
  uint32_t  GetSNESAPUContext(void *pCtxOut);
  uint32_t  GetSNESAPUContextSize();
  void      GetSPCRegs(u16 *pPC, uint8_t *pA, uint8_t *pY, uint8_t *pX, uint8_t *pPSW, uint8_t *pSP);
  void      InPort(uint32_t port, uint32_t val);
  void InitAPU(uint32_t reason);
  void      InitDSP();
  void      InitSPC();
  void      LoadSPCFile(void *pSPC);
  void      ResetAPU(uint32_t amp);
  void      ResetDSP();
  void      ResetSPC();
  void      SeekAPU(uint32_t time, bool fast);
  uint32_t  SetAPULength(uint32_t song, uint32_t fade);
  void      SetAPUOpt(uint32_t mix, uint32_t chn, uint32_t bits, uint32_t rate, uint32_t inter, uint32_t opts);
  void      SetAPURAM(uint32_t addr, uint8_t val);
  void      SetAPUSmpClk(uint32_t speed);
  void      SetDSPAmp(uint32_t level);
  // DSPDebug*	SetDSPDbg(DSPDebug *pTrace);
  void	    SetDSPEFBCT(int32_t leak);
  void      SetDSPOpt(uint32_t mix, uint32_t chn, uint32_t bits, uint32_t rate, uint32_t inter, uint32_t opts);
  void      SetDSPPitch(uint32_t base);
  bool      SetDSPReg(uint8_t reg, uint8_t val);
  void      SetDSPStereo(uint32_t sep);
  void      SetDSPVol(uint32_t vol);
  uint32_t	SetScript700(void *pSource);
  uint32_t	SetScript700Data(uint32_t addr, void *pData, uint32_t size);
  uint32_t	SetSNESAPUContext(void *pCtxIn);
  // SPCDebug*	SetSPCDbg(SPCDebug *pTrace, uint32_t opts);
  void      SetTimerTrick(uint32_t port, uint32_t wait);
  // CBFUNC    SNESAPUCallback(CBFUNC pCbFunc, uint32_t cbMask);
  void      SNESAPUInfo(uint32_t *pVer, uint32_t *pMin, uint32_t *pOpt);

private:
  // APU Variables
  // scr700lbl
  // scr700stk
  // scr700dsp
  // scr700mds
  // scr700chg
  // scr700det
  // scr700vol
  // scr700mvl
  // scr700wrk
  // scr700cmp
  // scr700cnt
  // scr700ptr
  // scr700stf
  // scr700dat
  // scr700stp
  uint8_t* pAPURAM();
  // pSCRRAM
  // apuCbMask
  // apuCbFunc
};

} // namespace snesapu

#endif	// SNESAPU_H_
