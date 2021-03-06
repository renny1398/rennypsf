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
  : psx_(new psx::PSX(version)) {
}


PSF::~PSF()
{
  if (psx_ != nullptr) {
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
  psx_->Spu().set_output(block);
  do {
    psx_->R3000a().Execute(&psx_->Interp(), false);
  } while (psx_->RCnt().cycle32() < (psx::PSXCLK / psx_->Spu().GetCurrentSamplingRate()));
  return true;
}

bool PSF::Close() {
  psx_->Interp().Shutdown();
  psx_->Spu().Shutdown();
  psx_->Mem().Reset();
  return true;
}

bool PSF::DoAdvance(SoundBlock* dest) {
  psx_->Spu().set_output(dest);
  do {
    psx_->R3000a().Execute(&psx_->Interp(), false);
  } while (dest->sample_length() == 0);
  return true;
}

bool PSF::ChangeOutputSamplingRate(uint32_t rate) {
  if (psx_ == nullptr) {
    return false;
  }
  psx_->ChangeOutputSamplingRate(rate);
  return true;
}


using namespace psx;

PSF1::PSF1(uint32_t pc0, uint32_t gp0, uint32_t sp0) : PSF(1) {
  psx_->R3000a().SetGPR(GPR_PC, pc0);
  psx_->R3000a().SetGPR(GPR_GP, gp0);
  psx_->R3000a().SetGPR(GPR_SP, sp0);
}


void PSF1::PSXMemCpy(PSXAddr dest, void *src, int length) {
  psx_->Memcpy(dest, src, length);
}



PSF2::PSF2(PSF2File* psf2irx)
  : PSF(2) {

  psx_->R3000a().SetGPR(GPR_PC, psx_->LoadELF(psf2irx));
  psx_->R3000a().SetGPR(GPR_SP, 0x801ffff0);
  psx_->R3000a().SetGPR(GPR_FP, 0x801ffff0);

  psx_->R3000a().SetGPR(GPR_RA, 0x80000000);
  psx_->R3000a().SetGPR(GPR_A0, 2);
  psx_->R3000a().SetGPR(GPR_A1, 0x80000004);

  psx_->SetRootDirectory(psf2irx->GetRoot());
}


PSF2::~PSF2() {}


bool PSF2::IsOk() const {
  return psx_->R3000a().GPR(GPR_PC) != 0xffffffff;
}
