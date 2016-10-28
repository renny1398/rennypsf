#include "psf/psf.h"
#include "psf/psx/psx.h"
#include "psf/spu/spu.h"
#include "common/debug.h"

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/hashmap.h>


PSF::PSF(uint32_t version)
  : psx_(new PSX::Composite(version)) {
}


PSF::~PSF()
{
  if (psx_ != NULL) {
    delete psx_;
  }
}


Soundbank& PSF::soundbank() {
  return psx_->Spu().soundbank();
}


void PSF::Init()
{
  psx_->Init();
}

void PSF::Reset()
{
  psx_->Reset();
}


#include "app.h"

// deprecated
bool PSF::DoPlay()
{
  // LoadBinary();
  // m_thread = psx_->Run();
  return true;
}

// deprecated
bool PSF::DoStop()
{
  // psx_->Terminate();
  // m_thread = 0;
  // rennyLogInfo("PSF", "PSF is stopped.");
  return true;
}

bool PSF::Open(SoundBlock* block) {
  psx_->Reset();
  psx_->Bios().Init();
  block->ChangeChannelCount(24); // TODO: 24 or 48
  block->EnableReverb();
  block->ReverbCh(0).set_volume_max(0x4000);
  block->ReverbCh(0).set_volume(0x4000, 0);
  block->ReverbCh(1).set_volume_max(0x4000);
  block->ReverbCh(1).set_volume(0, 0x4000);

  // Advance SPU
  const uint32_t cycles = 33868800 / psx_->Spu().GetCurrentSamplingRate() / 2;
  unprocessed_cycles_ = 0;
  do {
    uint32_t executed_cycles = psx_->Interp().Execute(cycles);
    unprocessed_cycles_ += executed_cycles;
  } while (unprocessed_cycles_ < cycles);
  psx_->Spu().Step(1);
  unprocessed_cycles_ -= cycles;
  return true;
}

bool PSF::Close() {
  psx_->Interp().Shutdown();
  psx_->Spu().Shutdown();
  psx_->Mem().Reset();
  return true;
}

bool PSF::DoAdvance(SoundBlock *dest) {
/*
  do {
    int ret = psx_->Interp().RCnt().SPURun(dest);
    if (ret < 0) return false;
    if (ret == 1) {
      return true;
    }
    psx_->Interp().ExecuteOnce();
  } while (true);
*/
  const uint32_t cycles = 33868800 / psx_->Spu().GetCurrentSamplingRate() / 2;
  while (unprocessed_cycles_ < cycles) {
    uint32_t ret = psx_->Interp().Execute(cycles - unprocessed_cycles_);
    unprocessed_cycles_ += ret;
  }
  unprocessed_cycles_ -= cycles;
  return psx_->Spu().GetAsync(dest);
}

bool PSF::ChangeOutputSamplingRate(uint32_t rate) {
  if (psx_ == NULL) {
    return false;
  }
  psx_->ChangeOutputSamplingRate(rate);
  return true;
}



PSF1::PSF1(uint32_t pc0, uint32_t gp0, uint32_t sp0) : PSF(1) {
  psx_->R3000ARegs().GPR.PC = pc0;
  psx_->R3000ARegs().GPR.GP = gp0;
  psx_->R3000ARegs().GPR.SP = sp0;
}


void PSF1::PSXMemCpy(PSX::PSXAddr dest, void *src, int length) {
  psx_->Memcpy(dest, src, length);
}



PSF2::PSF2(PSF2File* psf2irx)
  : PSF(2) {

  psx_->R3000ARegs().GPR.PC = psx_->LoadELF(psf2irx);
  psx_->R3000ARegs().GPR.SP = 0x801ffff0;
  psx_->R3000ARegs().GPR.FP = 0x801ffff0;

  psx_->R3000ARegs().GPR.RA = 0x80000000;
  psx_->R3000ARegs().GPR.A0 = 2;
  psx_->R3000ARegs().GPR.A1 = 0x80000004;
  psx_->Mu32ref(4) = BFLIP32(0x80000008);
  psx_->Mu32ref(0) = BFLIP32(PSX::R3000A::OPCODE_HLECALL);
  ::strcpy(psx_->Ms8ptr(8), "psf2:/");

  psx_->SetRootDirectory(psf2irx->GetRoot());
}


PSF2::~PSF2() {}


bool PSF2::IsOk() const {
  return psx_->R3000ARegs().GPR.PC != 0xffffffff;
}
