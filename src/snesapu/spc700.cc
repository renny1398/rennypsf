#include "snesapu/snesapu.h"
#include "snesapu/dsp.h"
#include "snesapu/apu.h"
#include "snesapu/spc700.h"

namespace {

const uint32_t CPU = 386;
const uint32_t BITS = 32;

// Clock cycles per operation
const uint32_t T64_CYC = 384;
const uint32_t T8_CYC  = 8*T64_CYC;

const uint32_t na   = 0x0;
const uint32_t WD   = 0x1;
const uint32_t RD   = 0x2;
const uint32_t WD16 = 0x5;
const uint32_t RD16 = 0x6;
const uint32_t WA   = 0x9;
const uint32_t RA   = 0xa;


inline void ExpPSW(const uint32_t PS, uint32_t* PSW) {
  for (int i = 0; i < 8; i++) {
    PSW[i] = ((PS >> i) & 1) << 8;
  }
}

inline void CmpPSW(const uint32_t* PSW, uint32_t* PS) {
  uint32_t ps = 0;
  for (int i = 0; i < 8; i++) {
    ps |= (PSW[i] >> 8) << i;
  }
  *PS = ps;
}

} // namespace


namespace snesapu {


SPC700::SPC700(SNESAPU* snesapu) : snesapu_(snesapu) {
  uint8_t* const pAPURAM = snesapu->pAPURAM();
  regPC = pAPURAM;
  regSP = pAPURAM + 0x1ff;

  pAPURAM[0xf1] = 0x80;
  *reinterpret_cast<uint32_t*>(&pAPURAM[0xfc]) = 0x0f0f0f00;
  pSPCReg = PSW;

  // TODO:
  // apu->scr700stk

  SetSPCDbg(NULL, 0);
}


void SPC700::ResetSPC() {
  // Erase 64K SPC RAM
  uint8_t* const pAPURAM = snesapu_->pAPURAM();
  for (int i = 0; i < 0x4000; i++) {
    reinterpret_cast<uint32_t*>(pAPURAM)[i] = 0xffffffff;
  }
  // Reset Function Registers
  pAPURAM[0xf0] = 0x0a;
  pAPURAM[0xf1] &= 0x07;
  pAPURAM[0xf1] |= 0x80;
  *reinterpret_cast<uint32_t*>(&pAPURAM[0xf4]) = 0;
  *reinterpret_cast<uint16_t*>(&pAPURAM[0xf8]) = 0xffff;
  *reinterpret_cast<uint32_t*>(&pAPURAM[0xfa]) = 0;
  *reinterpret_cast<uint16_t*>(&pAPURAM[0xfe]) = 0;
  pAPURAM[0xe1] = 0x80;

}


} // namespace snesapu
