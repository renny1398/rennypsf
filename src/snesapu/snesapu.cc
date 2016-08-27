#include "snesapu/snesapu.h"
#include "snesapu/spc700.h"
#include "snesapu/apu.h"
#include "snesapu/dsp.h"


namespace snesapu {


SNESAPU::SNESAPU() : apu_(NULL), dsp_(NULL), spc700_(NULL) {}


uint8_t* SNESAPU::pAPURAM() {
  return apu_->pAPURAM;
}



void* SNESAPU::EmuAPU(void *pBuf, uint32_t len, bool type) {
  return apu_->EmuAPU(pBuf, len, type);
}

void*	SNESAPU::EmuDSP(void *pBuf, int32_t size) {
  return dsp_->EmuDSP(pBuf, size);
}

int32_t SNESAPU::EmuSPC(uint32_t cyc) {
  return spc700_->EmuSPC(cyc);
}

void      SNESAPU::FixAPU(u16 pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);
void      SNESAPU::FixDSP();
void      SNESAPU::FixSPC(u16 pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp);
void      SNESAPU::FixSeek(uint8_t reset);
void      SNESAPU::GetAPUData(uint8_t **ppAPURAM, uint8_t **ppExtraRAM, uint8_t **ppSPCOut, uint32_t **ppT64Cnt, DSPReg **ppDSP, Voice **ppMix, uint32_t **ppVMMaxL, uint32_t **ppVMMaxR);
void      SNESAPU::GetScript700Data(char *pVer, uint32_t **ppSPCReg, uint8_t **ppScript700);
uint32_t  SNESAPU::GetSNESAPUContext(void *pCtxOut);
uint32_t  SNESAPU::GetSNESAPUContextSize();
void      SNESAPU::GetSPCRegs(u16 *pPC, uint8_t *pA, uint8_t *pY, uint8_t *pX, uint8_t *pPSW, uint8_t *pSP);
void      SNESAPU::InPort(uint32_t port, uint32_t val);

void SNESAPU::InitAPU(uint32_t /*reason*/) {
  apu_ = new APU(this);
}

void SNESAPU::InitDSP() {

}

void      SNESAPU::InitSPC() {

}

void      SNESAPU::LoadSPCFile(void *pSPC);
void      SNESAPU::ResetAPU(uint32_t amp);
void      SNESAPU::ResetDSP();
void      SNESAPU::ResetSPC();
void      SNESAPU::SeekAPU(uint32_t time, bool fast);
uint32_t  SNESAPU::SetAPULength(uint32_t song, uint32_t fade);
void      SNESAPU::SetAPUOpt(uint32_t mix, uint32_t chn, uint32_t bits, uint32_t rate, uint32_t inter, uint32_t opts);
void      SNESAPU::SetAPURAM(uint32_t addr, uint8_t val);
void      SNESAPU::SetAPUSmpClk(uint32_t speed);
void      SNESAPU::SetDSPAmp(uint32_t level);
// DSPDebug*	SetDSPDbg(DSPDebug *pTrace);
void	    SNESAPU::SetDSPEFBCT(int32_t leak);
void      SNESAPU::SetDSPOpt(uint32_t mix, uint32_t chn, uint32_t bits, uint32_t rate, uint32_t inter, uint32_t opts);
void      SNESAPU::SetDSPPitch(uint32_t base);
bool      SNESAPU::SetDSPReg(uint8_t reg, uint8_t val);
void      SNESAPU::SetDSPStereo(uint32_t sep);
void      SNESAPU::SetDSPVol(uint32_t vol);
uint32_t	SNESAPU::SetScript700(void *pSource);
uint32_t	SNESAPU::SetScript700Data(uint32_t addr, void *pData, uint32_t size);
uint32_t	SNESAPU::SetSNESAPUContext(void *pCtxIn);
// SPCDebug*	SetSPCDbg(SPCDebug *pTrace, uint32_t opts);
void      SetTimerTrick(uint32_t port, uint32_t wait);
// CBFUNC SNESAPU::   SNESAPUCallback(CBFUNC pCbFunc, uint32_t cbMask);
void      SNESAPU::SNESAPUInfo(uint32_t *pVer, uint32_t *pMin, uint32_t *pOpt);


} // namespace snesapu
