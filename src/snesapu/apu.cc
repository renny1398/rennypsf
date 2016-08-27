#include "snesapu/apu.h"
#include <memory>

namespace snesapu {

APU::APU(SNESAPU* snesapu) : snesapu_(snesapu) {
  uint8_t* p_apuram = apuRAMBuf.data();
  unsigned long l_p_apuram = reinterpret_cast<unsigned long>(p_apuram);
  p_apuram = reinterpret_cast<uint8_t*>((l_p_apuram + 0xffff) & ~65535UL);
  pAPURAM = p_apuram;

  std::memset(p_apuram + 0x10000, 0, 4*12);

  scr700inc = 0;
  apuCbMask = 0;
  apuCbFunc = 0;

  pSCRRAM = scrRAMBuf;

  snesapu->InitSPC();
  snesapu->InitDSP();

  smpRate = 32000;
  smpRAdj = 0x10000;

  snesapu_.SetAPUSmpClk(smpRAdj);
  ResetAPU(0x10000);
  snesapu_.SetScript700(NULL);
}


CBFUNC APU::SNESAPUCallback(CBFUNC pCbFunc, uint32_t cbMask) {
  CBFUNC prev_pCbFunc = apuCbFunc;
  apuCbFunc = pCbFunc;
  apuCbMask = cbMask;
  return prev_pCbFunc;
}


void APU::GetAPUData(uint8_t **ppAPURAM, uint8_t **ppExtraRAM, uint8_t **ppSPCOut, uint32_t **ppT64Cnt, DSPReg **ppDSP, Voice **ppMix, uint32_t **ppVMMaxL, uint32_t **ppVMMaxR) {
  if (ppAPURAM) {
    *ppAPURAM = pAPURAM;
  }
  if (ppExtraRAM) {
    *ppExtraRam = snesapu_.spc700_.extraRAM;
  }
  if (ppSPCOut) {
    *ppSPCOut = snesapu_.outPort;
  }
  if (ppT64Cnt) {
    *ppT64Cnt = snesapu_.t64Cnt;
  }
  if (ppDSP) {
    *ppDSP = snesapu_.dsp;
  }
  if (ppVoice) {
    *ppVoice = snesapu_.spc700_.mix
  }
  if (ppVMMaxL) {
    *ppVMMaxL = snesapu_.vMMaxL;
  }
  if (ppVMMaxR) {
    *ppVMMaxR = snesapu_.vMMaxR;
  }
}


void APU::GetScript700Data(char /* *pVer */, uint32_t **ppSPCReg, uint8_t **ppScript700) {
  if (ppSPCReg) {
    *ppSPCReg = snesapu_.spc700_.pSPCReg;
  }
  if (ppScript700) {
    *ppScript700 = scr700wrk.data;
  }
}


void APU::ResetAPU(uint32_t amp) {
  snesapu_.ResetSPC();
  snesapu_.ResetDSP();
  if (amp != 0xffffffff) {
    snesapu_.SetDSPAmp(amp);
  }
  cycLeft = 0;
  smpDec = 0;
}


void APU::FixAPU(uint16_t pc, uint8_t a, uint8_t y, uint8_t x, uint8_t psw, uint8_t sp) {
  snesapu_.FixSPC(pc, a, y, x, psw, sp);
  snesapu_.FixDSP();
}


void APU::LoadSPCFile(void *pSPC) {
  uint8_t* p_spc = static_cast<uint8_t*>(pSPC);
  ResetAPU(0xffffffff);
  std::memcpy(pAPURAM, p_spc+0x100, 0x10000);
  std::memcpy(snesapu_.dsp, p_spc+0x10100, 128);
  std::memcpy(snespau_.spc700_.extraRAM, p_spc+0x101c0, 64);

  const uint16_t& pc = *reinterpret_cast<uint16_t*>(p_spc+0x25);
  const uint8_t& a = p_spc[0x27];
  const uint8_t& y = p_spc[0x29];
  const uint8_t& x = p_spc[0x28];
  const uint8_t& psw = p_spc[0x2a];
  const uint8_t& sp = p_spc[0x2b];
  FixAPU(pc, a, y, x, psw, sp);
}


void APU::SetAPUOpt(uint32_t mix, uint32_t chn, uint32_t bits, uint32_t rate, uint32_t inter, uint32_t opts) {
  if (rate != 0xffffffff) {
    if (rate < 8000) rate = 8000;
    if (192000 < rate) rate = 192000;
    smpRate = rate;
    SetAPUSmpClk(smpRAdj);
  }
  snesapu_.SetDSPOpt(mix, chn, bits, rate, inter, opts);
}


void APU::SetAPUSmpClk(uint32_t speed) {
  if (speed < 4096) speed = 4096;
  if (1048576 < speed) speed = 1048576;
  smpRAdj = speed;
  smpREmu = (smpRate << 16) / speed;
}


uint32_t APU::SetAPULength(uint32_t song, uint32_t fade) {
  return snesapu_.dsp_.SetLength(sond, fade);
}


void*	APU::EmuAPU(void *pBuf, uint32_t len, bool type) {
  uint32_t cycles;
  if (type) {
    snesapu_.SetEmuDSP(pBuf, len, smpREmu);
    cycles = (APU_CLK * len) / smpREmu;
    if ((int32_t)cycles + (int32_t)cycLeft <= 0) {
      cycLeft = 0;
      snesapu_.SetEmuDSP(0, 0, 0);
      return;
    }
    cycles += cycLeft;
  } else {
    if ((int32_t)len + (int32_t)cycLeft <= 0) {
      cycLeft = 0;
      snesapu_.SetEmuDSP(0, 0, 0);
      return;
    }
    cycles = len + cycLeft;
    snesapu_.SetEmuDSP(pBuf, (smpREmu * cycles) / APU_CLK, smpREmu);
  }
  snesapu_.EmuSPC(cycles);
  cycLeft = 0;
  snesapu_.SetEmuDSP(0, 0, 0);
}


void APU::SeekAPU(uint32_t time, bool fast) {
  if (time == 0) return;
  uint32_t num_seconds = time / 64000;
  uint32_t cyc = (time % 64000) * (APU_CLK / 64000);
  if (fast) {
    snesapu_.SetSPCDbg(0, SPC_NODSP);
    if (cyc) snesapu_.EmuSPC(cyc);
    while (0 < num_seconds) {
      snesapu_.EmuSPC(APU_CLK);
      num_seconds--;
    }
    snesapu_.SetSPCDbg(0, 0);
  } else {
    if (cyc) EmuAPU(0, cyc, false);
    while (0 < num_seconds) {
      EmuAPU(0, APU_CLK, false);
      num_seconds--;
    }
  }
  snesapu_.FixSeek(fast ? 1 : 0);
}


void APU::SetTimerTrick(uint32_t port, uint32_t wait) {
  if (reinterpret_cast<uint8_t*>(scr700inc.data())[2] != 0) {
    return;
  }
  SetScript700(NULL);
  if (wait == 0) {
    return;
  }
  pSCRRAM[0x00] = 0x01;
  pSCRRAM[0x01] = 0x00;
  *reinterpret_cast<uint32_t*>(pSCRRAM+2) = wait;
  pSCRRAM[0x06] = 0x04;
  pSCRRAM[0x07] = 0x00;
  pSCRRAM[0x08] = 0x00;
  pSCRRAM[0x09] = 0x01;
  pSCRRAM[0x0a] = 0x00;
  pSCRRAM[0x0b] = 0x00;
  pSCRRAM[0x0c] = 0x00;
  pSCRRAM[0x0d] = 0x02;
  pSCRRAM[0x0e] = port & 0xff;
  pSCRRAM[0x0f] = 0x05;
  pSCRRAM[0x10] = 0x00;
  pSCRRAM[0x11] = 0x00;
  //pSCRRAM[0x12] = 0x00;
  scr700lbl = 0;
}



} // namespace snesapu
