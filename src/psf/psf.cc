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


PSF1::PSF1(uint32_t pc0, uint32_t gp0, uint32_t sp0) : PSF(1) {
  psx_->R3000ARegs().GPR.PC = pc0;
  psx_->R3000ARegs().GPR.GP = gp0;
  psx_->R3000ARegs().GPR.SP = sp0;
}


void PSF1::PSXMemCpy(PSX::PSXAddr dest, void *src, int length) {
  psx_->Memcpy(dest, src, length);
}



PSF2::PSF2(PSF2Entry* psf2irx)
  : PSF(2), load_addr_(0x23f00) {

  psx_->R3000ARegs().GPR.PC = LoadELF(psf2irx);
  psx_->R3000ARegs().GPR.SP = 0x801ffff0;
  psx_->R3000ARegs().GPR.FP = 0x801ffff0;

  psx_->R3000ARegs().GPR.RA = 0x80000000;
  psx_->R3000ARegs().GPR.A0 = 2;
  psx_->R3000ARegs().GPR.A1 = 0x80000004;
  psx_->U32M_ref(4) = BFLIP32(0x80000008);
  psx_->U32M_ref(0) = BFLIP32(PSX::R3000A::OPCODE_HLECALL);
  ::strcpy(psx_->S8M_ptr(8), "psf2:/");
}


bool PSF2::IsOk() const {
  return psx_->R3000ARegs().GPR.PC != 0xffffffff;
}



unsigned int PSF2::LoadELF(PSF2Entry* psf2irx) {

  PSF2File* irx = dynamic_cast<PSF2File*>(psf2irx);
  if (irx == NULL) {
    return 0xffffffff;
  }
  const unsigned char* data = irx->GetData();

  if (load_addr_ & 3) {
    load_addr_ &= ~3;
    load_addr_ += 4;
  }

  if ((data[0] != 0x7f) || (data[1] != 'E') ||
      (data[2] != 'L') || (data[3] != 'F') ) {
    rennyLogError("PSF2", "%s is not ELF file.", static_cast<const char*>(irx->GetName()));
    return 0xffffffff;
  }

  uint32_t entry = *(reinterpret_cast<const unsigned int*>(data + 24));
  uint32_t shoff = *(reinterpret_cast<const unsigned int*>(data + 32));

  uint32_t shentsize = *(reinterpret_cast<const unsigned short*>(data + 46));
  uint32_t shnum = *(reinterpret_cast<const unsigned short*>(data + 48));

  uint32_t shent = shoff;
  uint32_t totallen = 0;

  for (unsigned int i = 0; i < shnum; i++) {
    uint32_t type = *(reinterpret_cast<const unsigned int*>(data + shent + 4));
    uint32_t addr = *(reinterpret_cast<const unsigned int*>(data + shent + 12));
    uint32_t offset = *(reinterpret_cast<const unsigned int*>(data + shent + 16));
    uint32_t size = *(reinterpret_cast<const unsigned int*>(data + shent + 20));

    switch (type) {
    case 0:
      break;

    case 1:
      psx_->Memcpy(load_addr_ + addr, &data[offset], size);
      totallen += size;
      break;

    case 2:
      break;

    case 3:
      break;

    case 8:
      psx_->Memset(load_addr_ + addr, 0, size);
      totallen += size;
      break;

    case 9:
      for (unsigned int rec = 0; rec < (size/8); rec++) {
        uint32_t offs, info, target, temp, val, vallo;
        static uint32_t hi16offs = 0, hi16target = 0;

        offs = *(reinterpret_cast<const unsigned int*>(data + offset + (rec*8)));
        info = *(reinterpret_cast<const unsigned int*>(data + offset + (rec*8) + 4));
        target = BFLIP32(psx_->U32M_ref(load_addr_ + offs));

        switch (info & 0xff) {
        case 2:
          target += load_addr_;
          break;

        case 4:
          temp = (target & 0x03ffffff);
          target &= 0xfc000000;
          temp += (load_addr_ >> 2);
          target |= temp;
          break;

        case 5:
          hi16offs = offs;
          hi16target = target;
          break;

        case 6:
          vallo = ((target & 0xffff) ^ 0x8000) - 0x8000;

          val = ((hi16target & 0xffff) << 16) + vallo;
          val += load_addr_;

          val = ((val >> 16) + ((val & 0x8000) != 0)) & 0xffff;

          hi16target = (hi16target & ~0xffff) | val;

          val = load_addr_ + vallo;
          target = (target & ~0xffff) | (val & 0xffff);

          psx_->U32M_ref(load_addr_ + hi16offs) = BFLIP32(hi16target);
          break;

        default:
          break;
        }

        psx_->U32M_ref(load_addr_ + offs) = BFLIP32(target);
      }
      break;

    case 0x70000000:
      // do_iopmod
      break;

    default:
      break;
    }
    shent += shentsize;
  }

  entry += load_addr_;
  entry |= 0x80000000;
  load_addr_ += totallen;

  return entry;
}
