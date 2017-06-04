#include "psf/psx/psx.h"
#include "psf/psx/bios.h"
#include "psf/psx/hardware.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/interpreter.h"
#include "common/debug.h"

#include <cstring>
#include <cstdlib>

using namespace psx::mips;

namespace psx {

BIOS::BIOS(PSX* psx)
  : Component(psx),
    /*mips::RegisterAccessor(psx),*/
    UserMemoryAccessor(psx),
    IRQAccessor(psx),
    // psx_(psx),
    p_gpr_(&psx->R3000ARegs().GPR), p_cp0_(&psx->R3000ARegs().CP0)/*,
    GPR(GPR_PC)(GPR.GPR(GPR_PC)),
    GPR(GPR_A0)(GPR.GPR(GPR_A0)), GPR(GPR_A1)(GPR.GPR(GPR_A1)), GPR(GPR_A2)(GPR.GPR(GPR_A2)), GPR(GPR_A3)(GPR.GPR(GPR_A3)),
    Return(GPR.Return),
    GPR(GPR_GP)(GPR.GPR(GPR_GP)), GPR(GPR_SP)(GPR.GPR(GPR_SP)), GPR(GPR_FP)(GPR.GPR(GPR_FP)), GPR(GPR_RA)(GPR.GPR(GPR_RA))*/ {}

void BIOS::SetReferent(Registers *p_regs_) {
  p_gpr_ = &p_regs_->GPR;
  p_cp0_ = &p_regs_->CP0;
}

inline void BIOS::Return() {
  p_gpr_->PC = p_gpr_->RA;
}

inline void BIOS::Return(u32 return_code) {
  p_gpr_->V0 = return_code;
  p_gpr_->PC = p_gpr_->RA;
}

void BIOS::SoftCall(u32 pc) {
  rennyAssert(pc != 0x80001000);
  p_gpr_->PC = pc;
  p_gpr_->RA = 0x80001000;
  do {
    // Interp().ExecuteBlock();
    R3000a().Execute(&Interp(), true);
  } while (p_gpr_->PC != 0x80001000);
}

void BIOS::DeliverEventEx(u32 ev, u32 spec) {
  rennyAssert(events_base_ != 0);

  if (events_base_[ev][spec].status != BFLIP32(EVENT_STATUS_ACTIVE)) return;

  if (events_base_[ev][spec].mode == BFLIP32(EVENT_MODE_INTERRUPT)) {
    // SoftCall2(BFLIP32(Event[ev][spec].fhandler));
    u32 saved_ra = p_gpr_->RA;
    SoftCall(BFLIP32(events_base_[ev][spec].fhandler));
    p_gpr_->RA = saved_ra;
    return;
  }
  events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_ALREADY);
}

////////////////////////////////////////////////////////////////
// BIOS function
// function: 0xbfc0:00n0 - t1
// args: a0, a1, a2, a3
// returns: v0
////////////////////////////////////////////////////////////////

void BIOS::nop() {
  rennyLogDebug("PSXBIOS", "NOP (PC = 0x%08x)", (*p_gpr_)(GPR_PC));
  Return();
}

void BIOS::abs() {
  const int32_t a0 = static_cast<int32_t>(p_gpr_->A0);
  if (a0 < 0) {
    Return(-a0);
  } else {
    Return(a0);
  }
}

void BIOS::labs() {
  BIOS::abs();
}

void BIOS::atoi() {
  Return( ::atoi(psxMs8ptr(p_gpr_->A0) ) );
}

void BIOS::atol() {
  BIOS::atoi();
}

void BIOS::setjmp() {
  JumpBuffer* jmp_buf = reinterpret_cast<JumpBuffer*>(psxMptr(p_gpr_->A0));
  jmp_buf->Set(*p_gpr_);
  Return(0);
}

void BIOS::longjmp() {
  const JumpBuffer* jmp_buf = reinterpret_cast<JumpBuffer*>(psxMptr(p_gpr_->A0));
  const u32 gp = p_gpr_->GP;
  jmp_buf->Get(p_gpr_);
  p_gpr_->GP = gp;
  Return(p_gpr_->A1);
}

void BIOS::strcat() {
  const u32 a0 = p_gpr_->A0;
  char *dest = psxMs8ptr(a0);
  const char *src = psxMs8ptr(p_gpr_->A1);
  ::strcat(dest, src);
  Return(a0);
}

void BIOS::strncat() {
  const u32 a0 = p_gpr_->A0;
  char *dest = psxMs8ptr(a0);
  const char *src = psxMs8ptr(p_gpr_->A1);
  const u32 count = psxMu32val(p_gpr_->A2);
  ::strncat(dest, src, count);
  Return(a0);
}

void BIOS::strcmp() {
  Return( ::strcmp(psxMs8ptr(p_gpr_->A0), psxMs8ptr(p_gpr_->A1)) );
}

void BIOS::strncmp() {
  Return( ::strncmp(psxMs8ptr(p_gpr_->A0), psxMs8ptr(p_gpr_->A1), psxMu32val(p_gpr_->A2) ) );
}

void BIOS::strcpy() {
  const u32 a0 = p_gpr_->A0;
  ::strcpy(psxMs8ptr(a0), psxMs8ptr(p_gpr_->A1));
  Return(a0);
}

void BIOS::strncpy() {
  const u32 a0 = p_gpr_->A0;
  ::strncpy(psxMs8ptr(a0), psxMs8ptr(p_gpr_->A1), psxMu32val(p_gpr_->A2));
  Return(a0);
}

void BIOS::strlen() {
  Return( ::strlen(psxMs8ptr(p_gpr_->A0)) );
}

void BIOS::index() {
  const u32 a0 = p_gpr_->A0;
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strchr(src, p_gpr_->A1);
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::rindex() {
  const u32 a0 = p_gpr_->A0;
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strchr(src, p_gpr_->A1);
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::strchr() {
  BIOS::index();
}

void BIOS::strrchr() {
  BIOS::rindex();
}

void BIOS::strpbrk() {
  const u32 a0 = p_gpr_->A0;
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strpbrk(src, psxMs8ptr(p_gpr_->A1));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::strspn() {
  Return( ::strspn(psxMs8ptr(p_gpr_->A0), psxMs8ptr(p_gpr_->A1)) );
}

void BIOS::strcspn() {
  Return( ::strcspn(psxMs8ptr(p_gpr_->A0), psxMs8ptr(p_gpr_->A1)) );
}

void BIOS::strtok() {
  const u32 a0 = p_gpr_->A0;
  char *src = psxMs8ptr(a0);
  char *ret = ::strtok(src, psxMs8ptr(p_gpr_->A1));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::strstr() {
  const u32 a0 = p_gpr_->A0;
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strstr(src, psxMs8ptr(p_gpr_->A1));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::toupper() {
  Return(::toupper(p_gpr_->A0));
}

void BIOS::tolower() {
  Return(::tolower(p_gpr_->A0));
}

void BIOS::bcopy() {
  const u32 a1 = p_gpr_->A1;
  ::memcpy(psxMu8ptr(a1), psxMu8ptr(p_gpr_->A0), p_gpr_->A2);
  // Return(a1);
  Return();
}

void BIOS::bzero() {
  const u32 a0 = p_gpr_->A0;
  ::memset(psxMu8ptr(a0), 0, p_gpr_->A1);
  // Return(a0);
  Return();
}

void BIOS::bcmp() {
  Return( ::memcmp(psxMu8ptr(p_gpr_->A0), psxMu8ptr(p_gpr_->A1), p_gpr_->A2) );
}

void BIOS::memcpy() {
  const u32 a0 = p_gpr_->A0;
  ::memcpy(psxMu8ptr(a0), psxMu8ptr(p_gpr_->A1), p_gpr_->A2);
  Return(a0);
}

void BIOS::memset() {
  const u32 a0 = p_gpr_->A0;
  ::memset(psxMu8ptr(a0), p_gpr_->A1, p_gpr_->A2);
  Return(a0);
}

void BIOS::memmove() {
  const u32 a0 = p_gpr_->A0;
  ::memmove(psxMu8ptr(a0), psxMu8ptr(p_gpr_->A1), p_gpr_->A2);
  Return(a0);
}

void BIOS::memcmp() {
  Return( ::memcmp( psxMu8ptr(p_gpr_->A0), psxMu8ptr(p_gpr_->A1), p_gpr_->A2) );
}

void BIOS::memchr() {
  const u32 a0 = p_gpr_->A0;
  const char* src = psxMs8ptr(a0);
  const char* ret = static_cast<const char*>(::memchr(src, p_gpr_->A1, p_gpr_->A2));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::rand() {
  Return( 1 + static_cast<int>(32767.0 * ::rand() / (RAND_MAX + 1.0)) );
}

void BIOS::srand() {
  ::srand(p_gpr_->A0);
  Return();
}

void BIOS::malloc() {
  u32 chunk = heap_addr;
  malloc_chunk *pChunk = reinterpret_cast<malloc_chunk*>(psxMptr(chunk));
  const u32 a0 = p_gpr_->A0;

  // search for first chunk that is large enough and not currently being used
  while ( a0 > BFLIP32(pChunk->size) || BFLIP32(pChunk->stat) == 1/*INUSE*/ ) {
    chunk = pChunk->fd;
    pChunk = reinterpret_cast<malloc_chunk*>(psxMptr(chunk));
  }

  // split free chunk
  u32 fd = chunk + sizeof(malloc_chunk) + a0;
  malloc_chunk *pFd = reinterpret_cast<malloc_chunk*>(psxMptr(fd));
  pFd->stat = pChunk->stat;
  pFd->size = BFLIP32( BFLIP32(pChunk->size) - a0 );
  pFd->fd = pChunk->fd;
  pFd->bk = chunk;

  // set new chunk
  pChunk->stat = BFLIP32(1);
  pChunk->size = BFLIP32(a0);
  pChunk->fd = fd;

  Return( (chunk + sizeof(malloc_chunk)) | 0x80000000 );
}

void BIOS::InitHeap() {
  const u32 a0 = p_gpr_->A0;
  const u32 a1 = p_gpr_->A1;
  heap_addr = a0;

  malloc_chunk *chunk = reinterpret_cast<malloc_chunk*>(psxMptr(a0));
  chunk->stat = 0;
  if ( (a0 & 0x1fffff) + a1 >= 0x200000 ) {
    chunk->size = BFLIP32( 0x1ffffc - (a0 & 0x1fffff) );
  } else {
    chunk->size = BFLIP32(a1);
  }
  chunk->fd = 0;
  chunk->bk = 0;
  Return();
}

void BIOS::FlushCache() {
  BIOS::nop();
}

void BIOS::_bu_init() {
  DeliverEventEx(0x11, 0x2);    // 0xf0000011, 0x0004
  DeliverEventEx(0x81, 0x2);    // 0xf4000001, 0x0004
  Return();
}

void BIOS::_96_init() {
  BIOS::nop();
}

void BIOS::_96_remove() {
  BIOS::nop();
}


void BIOS::SetRCnt() {
  const u32 a0 = p_gpr_->A0 & 0x3;
  const u32 a1 = p_gpr_->A1;
  const u32 a2 = p_gpr_->A2;
  p_gpr_->A0 = a0;
  if (a0 != 3) {
    u32 mode = 0;
    RCnt().WriteTargetEx(a0, a1);
    if (a2 & 0x1000) mode |= 0x050; // Interrupt Mode
    if (a2 & 0x0100) mode |= 0x008; // Count to 0xffff
    if (a2 & 0x0010) mode |= 0x001; // Timer stop mode
    if (a0 == 2) {
      if (a2 & 0x0001) mode |= 0x200; // System Clock mode
    } else {
      if (a2 & 0x0001) mode |= 0x100; // System Clock mode
    }
    RCnt().WriteModeEx(a0, mode);
  }
  Return();
}

void BIOS::GetRCnt() {
  const u32 a0 = p_gpr_->A0 & 0x3;
  p_gpr_->A0 = a0;
  if (a0 != 3) {
    Return(RCnt().ReadCountEx(a0));
  } else {
    Return(0);
  }
}

void BIOS::StartRCnt() {
  const u32 a0 = p_gpr_->A0 & 0x3;
  if (a0 != 3) {
    set_irq_mask( irq_mask() | BFLIP32(1<<(a0+4)) );
  } else {
    set_irq_mask( irq_mask() | BFLIP32(1) );
  }
  // SetGPR(GPR_A0, a0);
  Return(1);
}

void BIOS::StopRCnt() {
  const u32 a0 = p_gpr_->A0 & 0x3;
  if (a0 != 3) {
    set_irq_mask( irq_mask() & BFLIP32( ~(1<<(a0+4)) ) );
  } else {
    set_irq_mask( irq_mask() & BFLIP32(~1) );
  }
  // SetGPR(GPR_A0, a0);
  Return(1);
}

void BIOS::ResetRCnt() {
  const u32 a0 = p_gpr_->A0 & 0x3;
  if (a0 != 3) {
    RCnt().WriteModeEx(a0, 0);
    RCnt().WriteTargetEx(a0, 0);
    RCnt().WriteCountEx(a0, 0);
  }
  // SetGPR(GPR_A0, a0);
  Return();
}

int BIOS::GetEv(int a0) {
  int ev;
  ev = (a0 >> 24) & 0xf;
  if (ev == 0xf) ev = 0x5;
  ev *= 32;
  ev += a0 & 0x1f;
  return ev;
}

int BIOS::GetSpec(int a1) {
  switch (a1) {
  case 0x0301:
    return 16;
  case 0x0302:
    return 17;
  }
  for (int i = 0; i < 16; i++) {
    if (a1 & (1 << i)) {
      return i;
    }
  }
  return 0;
}

void BIOS::DeliverEvent() {
  const int ev = GetEv(p_gpr_->A0);
  const int spec = GetSpec(p_gpr_->A1);
  DeliverEventEx(ev, spec);
  Return();
}

void BIOS::OpenEvent() {
  const int ev = GetEv(p_gpr_->A0);
  const int spec = GetSpec(p_gpr_->A1);
  events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
  events_base_[ev][spec].mode = BFLIP32(p_gpr_->A2);
  events_base_[ev][spec].fhandler = BFLIP32(p_gpr_->A3);
  Return(ev | (spec << 8));
}

void BIOS::CloseEvent() {
  const u32 a0 = p_gpr_->A0;
  const int ev = a0 & 0xff;
  const int spec = (a0 >> 8) & 0xff;
  events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_UNUSED);
  Return(1);
}

void BIOS::WaitEvent() {
  // same as EnableEvent??
  const u32 a0 = p_gpr_->A0;
  const int ev = a0 & 0xff;
  const int spec = (a0 >> 8) & 0xff;
  events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_UNUSED);
  Return(1);
}

void BIOS::TestEvent() {
  const u32 a0 = p_gpr_->A0;
  const int ev = a0 & 0xff;
  const int spec = (a0 >> 8) & 0xff;
  if (events_base_[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY)) {
    events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    Return(1);
  } else {
    Return(0);
  }
}

void BIOS::EnableEvent() {
  const u32 a0 = p_gpr_->A0;
  const int ev = a0 & 0xff;
  const int spec = (a0 >> 8) & 0xff;
  events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
  Return(1);
}

void BIOS::DisableEvent() {
  const u32 a0 = p_gpr_->A0;
  const int ev = a0 & 0xff;
  const int spec = (a0 >> 8) & 0xff;
  events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
  Return(1);
}

void BIOS::OpenTh() {
  u32 th;
  for (th = 1; th < 8; th++) {
    if (Thread[th].status == 0) {
      // TS_READY
      break;
    }
  }
  Thread[th].status = BFLIP32(1);
  Thread[th].func = BFLIP32(p_gpr_->A0);
  Thread[th].reg.SP = BFLIP32(p_gpr_->A1);
  Thread[th].reg.GP = BFLIP32(p_gpr_->A2);
  Return(th);
}

void BIOS::CloseTh() {
  const u32 th = p_gpr_->A0 & 0xff;
  if (Thread[th].status == 0) {
    Return(0);
  } else {
    Thread[th].status = 0;
    Return(1);
  }
}

void BIOS::ChangeTh() {
  const u32 th = p_gpr_->A0 & 0xff;
  if (Thread[th].status == 0 || CurThread == th) {
    Return(0);
    return;
  }
  if (Thread[CurThread].status == BFLIP32(2)) {
    Thread[CurThread].status = BFLIP32(1);
    Thread[CurThread].func = BFLIP32(p_gpr_->RA);
    // WARNING: thread.HI and thread.LO are destroyed.
    Thread[CurThread].reg.Set(*p_gpr_);
  }
  // WARNING: HI and LO are destroyed.
  p_gpr_->Set(Thread[CurThread].reg);
  p_gpr_->PC = BFLIP32(Thread[th].func);
  Thread[th].status = BFLIP32(2);
  CurThread = th;
  Return(1);
}

void BIOS::ReturnFromException() {
  //    ::memcpy(GPR.R, regs, 32*sizeof(u32));
  //    GPR.LO = regs[32];
  //    GPR.HI = regs[33];
  p_gpr_->Set(savedGPR);
  u32 pc = p_cp0_->EPC;
  if (p_cp0_->CAUSE & 0x80000000) {
    pc += 4;
  }
  p_gpr_->PC = pc;
  u32 status = p_cp0_->SR;
  p_cp0_->SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
}

void BIOS::ResetEntryInt() {
  jmp_int = nullptr;
  Return();
}

void BIOS::HookEntryInt() {
  jmp_int = reinterpret_cast<JumpBuffer*>(psxMu32ptr(p_gpr_->A0));
  Return();
}

void BIOS::UnDeliverEvent() {
  const int ev = GetEv(p_gpr_->A0);
  const int spec = GetSpec(p_gpr_->A1);
  if (events_base_[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY) && events_base_[ev][spec].mode == BFLIP32(EVENT_MODE_NO_INTERRUPT)) {
    events_base_[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
  }
  Return();
}

void BIOS::GetC0Table() {
  Return(0x674);
}

void BIOS::GetB0Table() {
  Return(0x874);
}


void BIOS::SysEnqIntRP() {
  SysIntRP[p_gpr_->A0] = p_gpr_->A1;
  Return(0);
}

void BIOS::SysDeqIntRP() {
  SysIntRP[p_gpr_->A0] = 0;
  Return(0);
}

void BIOS::ChangeClearRCnt() {
  u32 *ptr = psxMu32ptr((p_gpr_->A0 << 2) + 0x8600);
  Return(BFLIP32(*ptr));
  *ptr = BFLIP32(p_gpr_->A1);
}


void (*biosA0[256])();
void (*biosB0[256])();
void (*biosC0[256])();


////////////////////////////////////////////////////////////////
// PSXBIOS public functions
////////////////////////////////////////////////////////////////

void (BIOS::*BIOS::biosA0[256])() = {};
void (BIOS::*BIOS::biosB0[256])() = {};
void (BIOS::*BIOS::biosC0[256])() = {};

void BIOS::Init()
{
  heap_addr = 0;
  CurThread = 0;
  jmp_int = nullptr;

  for (int i = 0; i < 256; i++) {
    biosA0[i] = &BIOS::nop;
    biosB0[i] = &BIOS::nop;
    biosC0[i] = &BIOS::nop;
  }

  biosA0[0x0e] = &BIOS::abs;
  biosA0[0x0f] = &BIOS::labs;
  biosA0[0x10] = &BIOS::atoi;
  biosA0[0x11] = &BIOS::atol;
  biosA0[0x13] = &BIOS::setjmp;
  biosA0[0x14] = &BIOS::longjmp;
  biosA0[0x15] = &BIOS::strcat;
  biosA0[0x16] = &BIOS::strncat;
  biosA0[0x17] = &BIOS::strcmp;
  biosA0[0x18] = &BIOS::strncmp;
  biosA0[0x19] = &BIOS::strcpy;
  biosA0[0x1a] = &BIOS::strncpy;
  biosA0[0x1b] = &BIOS::strlen;
  biosA0[0x1c] = &BIOS::index;
  biosA0[0x1d] = &BIOS::rindex;
  biosA0[0x1e] = &BIOS::strchr;
  biosA0[0x1f] = &BIOS::strrchr;
  biosA0[0x20] = &BIOS::strpbrk;
  biosA0[0x21] = &BIOS::strspn;
  biosA0[0x22] = &BIOS::strcspn;
  biosA0[0x23] = &BIOS::strtok;
  biosA0[0x24] = &BIOS::strstr;
  biosA0[0x25] = &BIOS::toupper;
  biosA0[0x26] = &BIOS::tolower;
  biosA0[0x27] = &BIOS::bcopy;
  biosA0[0x28] = &BIOS::bzero;
  biosA0[0x29] = &BIOS::bcmp;
  biosA0[0x2a] = &BIOS::memcpy;
  biosA0[0x2b] = &BIOS::memset;
  biosA0[0x2c] = &BIOS::memmove;
  biosA0[0x2d] = &BIOS::memcmp;
  biosA0[0x2e] = &BIOS::memchr;
  biosA0[0x2f] = &BIOS::rand;
  biosA0[0x30] = &BIOS::srand;
  biosA0[0x33] = &BIOS::malloc;
  biosA0[0x39] = &BIOS::InitHeap;
  biosA0[0x44] = &BIOS::FlushCache;
  biosA0[0x70] = &BIOS::_bu_init;
  biosA0[0x71] = &BIOS::_96_init;
  biosA0[0x72] = &BIOS::_96_remove;

  biosB0[0x02] = &BIOS::SetRCnt;
  biosB0[0x03] = &BIOS::GetRCnt;
  biosB0[0x04] = &BIOS::StartRCnt;
  biosB0[0x05] = &BIOS::StopRCnt;
  biosB0[0x06] = &BIOS::ResetRCnt;
  biosB0[0x07] = &BIOS::DeliverEvent;
  biosB0[0x08] = &BIOS::OpenEvent;
  biosB0[0x09] = &BIOS::CloseEvent;
  biosB0[0x0a] = &BIOS::WaitEvent;
  biosB0[0x0b] = &BIOS::TestEvent;
  biosB0[0x0c] = &BIOS::EnableEvent;
  biosB0[0x0d] = &BIOS::DisableEvent;
  biosB0[0x0e] = &BIOS::OpenTh;
  biosB0[0x0f] = &BIOS::CloseTh;
  biosB0[0x10] = &BIOS::ChangeTh;
  biosB0[0x17] = &BIOS::ReturnFromException;
  biosB0[0x18] = &BIOS::ResetEntryInt;
  biosB0[0x19] = &BIOS::HookEntryInt;
  biosB0[0x20] = &BIOS::UnDeliverEvent;
  biosB0[0x56] = &BIOS::GetC0Table;
  biosB0[0x57] = &BIOS::GetB0Table;

  biosC0[0x02] = &BIOS::SysEnqIntRP;
  biosC0[0x03] = &BIOS::SysDeqIntRP;
  biosC0[0x0a] = &BIOS::ChangeClearRCnt;

  u32 base = 0x1000;
  u32 size = sizeof(EvCB) * 32;
  events_base_ = static_cast<EvCB*>(psxRptr(base));
  base += size*6;
  ::memset(events_base_, 0, size*6);
  // f0+2: root counter event
  rcnt_event_ = events_base_ + 32*2;

  // set b0 table
  u32 *ptr = psxMu32ptr(0x0874);
  ptr[0] = BFLIP32(0x4c54 - 0x884);

  // set c0 table
  ptr = psxMu32ptr(0x0674);
  ptr[6] = BFLIP32(0x0c80);

  ::memset(SysIntRP, 0, sizeof(SysIntRP));
  ::memset(Thread, 0, sizeof(Thread));
  Thread[0].status = BFLIP32(2);

  psxMu32ref(0x0150) = BFLIP32(0x160);
  psxMu32ref(0x0154) = BFLIP32(0x320);
  psxMu32ref(0x0160) = BFLIP32(0x248);
  ::strcpy(psxMs8ptr(0x248), "bu");

  // OPCODE HLE!!
  psxRu32ref(0x0000) = BFLIP32((OPCODE_HLECALL << 26) | 4);
  psxMu32ref(0x0000) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  psxMu32ref(0x00a0) = BFLIP32((OPCODE_HLECALL << 26) | 1);
  psxMu32ref(0x00b0) = BFLIP32((OPCODE_HLECALL << 26) | 2);
  psxMu32ref(0x00c0) = BFLIP32((OPCODE_HLECALL << 26) | 3);
  psxMu32ref(0x4c54) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  psxMu32ref(0x8000) = BFLIP32((OPCODE_HLECALL << 26) | 5);
  psxMu32ref(0x07a0) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  psxMu32ref(0x0884) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  psxMu32ref(0x0894) = BFLIP32((OPCODE_HLECALL << 26) | 0);

  if (Psx().version() == 2) {
    psxMu32ref(4) = BFLIP32(0x80000008);
    // psx_->Mu32ref(0) = BFLIP32(PSX::R3000A::OPCODE_HLECALL);
    ::strcpy(psxMs8ptr(8), "aofile:/");
    // ::strcpy(psxMs8ptr(8), "psf2:/");
  }
}

void BIOS::Shutdown() {}

void BIOS::Interrupt()
{
  /*
  // for Root Counter 3 (interrupt = 1)
  if ( BFLIP32(irq_data()) & 1 ) {
    if (rcnt_event_[3][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
      SoftCall(BFLIP32(rcnt_event_[3][1].fhandler));
    }
  }

  // for Root Counter 0, 1, 2 (interrupt = 0x10, 0x20, 0x40)
  if ( BFLIP32(irq_data()) & 0x70 ) {
    for (int i = 0; i < 3; i++) {
      if (BFLIP32(irq_data()) & (1 << (i+4))) {
        if (rcnt_event_[i][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
          SoftCall(BFLIP32(rcnt_event_[i][1].fhandler));
          HwRegs().Write32(0x1f801070, ~(1 << (i+4)));
        }
      }
    } 
  }
  */
  // for RootCounter
  for (int i = 0; i < 4; i++) {
    const auto interrupt = RCnt().interrupt(i);
    const auto& ev = rcnt_event_[i][1];
    if ((BFLIP32(irq_data()) & interrupt) &&
        ev.status == BFLIP32(EVENT_STATUS_ACTIVE)) {
      SoftCall(BFLIP32(ev.fhandler));
      HwRegs().set_irq32(~interrupt);
    }
  }
}

void BIOS::Exception()
{
  u32 status;

  switch (p_cp0_->CAUSE & 0x3c) {
  case 0x00:  // Interrupt
    savedGPR.Set(*p_gpr_);
    Interrupt();
    for (int i = 0; i < 8; i++) {
      if (SysIntRP[i]) {
        u32 *queue = psxMu32ptr(SysIntRP[i]);
        p_gpr_->S0 = BFLIP32(queue[2]);
        SoftCall(BFLIP32(queue[1]));
      }
    }
    if (jmp_int) {
      HwRegs().set_irq32(0xffffffff);
      jmp_int->Get(p_gpr_);
      Return(1);
      return;
    }
    HwRegs().Write16(0x1f801070, 0);
    break;
  case 0x20:  // SYSCALL
    status = p_cp0_->SR;
    switch (p_gpr_->A0) {
    case 1: // EnterCritical (disable IRQs)
      status &= ~0x404;
      break;
    case 2: // LeaveCritical (enable IRQs)
      status |= 0x404;
    }
    p_gpr_->PC = p_cp0_->EPC + 4;
    p_cp0_->SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
    return;
  default:
    rennyLogWarning("PSXBIOS", "Unknown BIOS Exception (0x%02x)", (*p_cp0_)(COP0_CAUSE) & 0x3c);
  }

  u32 pc = p_cp0_->EPC;
  if (p_cp0_->CAUSE & 0x80000000) {
    pc += 4;
  }
  p_gpr_->PC = pc;
  status = p_cp0_->SR;
  p_cp0_->SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
}

}   // namespace PSX
