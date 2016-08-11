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

bool PSF::DoPlay()
{
  // LoadBinary();
  m_thread = psx_->Run();
  return true;
}


bool PSF::DoStop()
{
  psx_->Terminate();
  m_thread = 0;
  rennyLogInfo("PSF", "PSF is stopped.");
  return true;
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
  psx_->U32M_ref(4) = BFLIP32(0x80000008);
  psx_->U32M_ref(0) = BFLIP32(PSX::R3000A::OPCODE_HLECALL);
  ::strcpy(psx_->S8M_ptr(8), "psf2:/");

  psx_->SetRootDirectory(psf2irx->GetRoot());
}


PSF2::~PSF2() {}


bool PSF2::IsOk() const {
  return psx_->R3000ARegs().GPR.PC != 0xffffffff;
}
