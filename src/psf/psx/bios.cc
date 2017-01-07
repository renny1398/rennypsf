#include "psf/psx/psx.h"
#include "psf/psx/bios.h"
#include "psf/psx/hardware.h"
#include "psf/psx/rcnt.h"
#include "psf/psx/interpreter.h"
#include "common/debug.h"

#include <cstring>
#include <cstdlib>


using namespace PSX::R3000A;


namespace PSX {


BIOS::BIOS(Composite* psx)
  : Component(psx),
    R3000A::RegisterAccessor(psx),
    UserMemoryAccessor(psx),
    IRQAccessor(psx)/*,
    // psx_(psx),
    GPR(psx->R3000ARegs().GPR), CP0(psx->R3000ARegs().CP0),
    GPR(GPR_PC)(GPR.GPR(GPR_PC)),
    GPR(GPR_A0)(GPR.GPR(GPR_A0)), GPR(GPR_A1)(GPR.GPR(GPR_A1)), GPR(GPR_A2)(GPR.GPR(GPR_A2)), GPR(GPR_A3)(GPR.GPR(GPR_A3)),
    Return(GPR.Return),
    GPR(GPR_GP)(GPR.GPR(GPR_GP)), GPR(GPR_SP)(GPR.GPR(GPR_SP)), GPR(GPR_FP)(GPR.GPR(GPR_FP)), GPR(GPR_RA)(GPR.GPR(GPR_RA))*/ {}


inline void BIOS::Return() {
  MoveGPR(GPR_PC, GPR_RA);
}

inline void BIOS::Return(u32 return_code) {
  SetGPR(GPR_V0, return_code);
  MoveGPR(GPR_PC, GPR_RA);
}

void BIOS::SoftCall(u32 pc) {
  rennyAssert(pc != 0x80001000);
  SetGPR(GPR_PC, pc);
  SetGPR(GPR_RA, 0x80001000);
  do {
    Interp().ExecuteBlock();
  } while (GPR(GPR_PC) != 0x80001000);
}

void BIOS::SoftCall2(u32 pc) {
  u32 saved_ra = GPR(GPR_RA);
  SetGPR(GPR_PC, pc);
  SetGPR(GPR_RA, 0x80001000);
  do {
    Interp().ExecuteBlock();
  } while (GPR(GPR_PC) != 0x80001000);
  SetGPR(GPR_RA, saved_ra);
}

void BIOS::DeliverEventEx(u32 ev, u32 spec) {
  rennyAssert(Event != 0);

  if (Event[ev][spec].status != BFLIP32(EVENT_STATUS_ACTIVE)) return;

  if (Event[ev][spec].mode == BFLIP32(EVENT_MODE_INTERRUPT)) {
    SoftCall2(BFLIP32(Event[ev][spec].fhandler));
    return;
  }
  Event[ev][spec].status = BFLIP32(EVENT_STATUS_ALREADY);
}



////////////////////////////////////////////////////////////////
// BIOS function
// function: 0xbfc0:00n0 - t1
// args: a0, a1, a2, a3
// returns: v0
////////////////////////////////////////////////////////////////


void BIOS::nop()
{
  rennyLogDebug("PSXBIOS", "NOP (PC = 0x%08x)", GPR(GPR_PC));
  Return();
}


void BIOS::abs()   // BIOSA0:0e
{
  int32_t a0 = static_cast<int32_t>(GPR(GPR_A0));
  if (a0 < 0) {
    Return(-a0);
  } else {
    Return(a0);
  }
}

// BIOSA0:0f
void BIOS::labs() {
  BIOS::abs();
}

void BIOS::atoi()  // BIOSA0:10
{
  Return( ::atoi(psxMs8ptr( GPR(GPR_A0)) ) );
}

// BIOSA0:11
void BIOS::atol() {
  BIOS::atoi();
}

void BIOS::setjmp()    // BIOSA0:13
{
  u32 *jmp_buf = psxMu32ptr(GPR(GPR_A0));

  jmp_buf[0] = BFLIP32(GPR(GPR_RA));
  jmp_buf[1] = BFLIP32(GPR(GPR_SP));
  jmp_buf[2] = BFLIP32(GPR(GPR_FP));
  for (int i = 0; i < 8; i++) {
    jmp_buf[3+i] = BFLIP32(GPR(GPR_S0+i));
  }
  jmp_buf[11] = BFLIP32(GPR(GPR_GP));
  Return(0);
}

void BIOS::longjmp()   // BIOSA0:14
{
  u32 *jmp_buf = psxMu32ptr(GPR(GPR_A0));

  SetGPR(GPR_RA, BFLIP32(jmp_buf[0]));
  SetGPR(GPR_SP, BFLIP32(jmp_buf[1]));
  SetGPR(GPR_FP, BFLIP32(jmp_buf[2]));
  for (int i = 0; i < 8; i++) {
    SetGPR(GPR_S0+i, BFLIP32(jmp_buf[3+i]));
  }
  Return(GPR(GPR_A1));
}

void BIOS::strcat()    // BIOSA0:15
{
  u32 a0 = GPR(GPR_A0);
  char *dest = psxMs8ptr(a0);
  const char *src = psxMs8ptr(GPR(GPR_A1));

  ::strcat(dest, src);

  Return(a0);
}

void BIOS::strncat()   // BIOSA0:16
{
  u32 a0 = GPR(GPR_A0);
  char *dest = psxMs8ptr(a0);
  const char *src = psxMs8ptr(GPR(GPR_A1));
  const u32 count = psxMu32val(GPR(GPR_A2));

  ::strncat(dest, src, count);

  Return(a0);
}

void BIOS::strcmp()    // BIOSA0:17
{
  Return( ::strcmp( psxMs8ptr(GPR(GPR_A0)), psxMs8ptr(GPR(GPR_A1)) ) );
  Return();
}

void BIOS::strncmp()    // BIOSA0:18
{
  Return( ::strncmp(psxMs8ptr( GPR(GPR_A0)), psxMs8ptr(GPR(GPR_A1)), psxMu32val(GPR(GPR_A2)) ) );
}

void BIOS::strcpy()    // BIOSA0:19
{
  u32 a0 = GPR(GPR_A0);
  ::strcpy(psxMs8ptr(a0), psxMs8ptr(GPR(GPR_A1)));
  Return(a0);
}

void BIOS::strncpy()   // BIOSA0:1a
{
  u32 a0 = GPR(GPR_A0);
  ::strncpy(psxMs8ptr(a0), psxMs8ptr(GPR(GPR_A1)), psxMu32val(GPR(GPR_A2)));
  Return(a0);
}

void BIOS::strlen()    // BIOSA0:1b
{
  Return( ::strlen( psxMs8ptr(GPR(GPR_A0)) ) );
}

void BIOS::index()     // BIOSA0:1c
{
  u32 a0 = GPR(GPR_A0);
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strchr(src, GPR(GPR_A1));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::rindex()    // BIOSA0:1d
{
  u32 a0 = GPR(GPR_A0);
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strrchr(src, GPR(GPR_A1));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

// BIOSA0:1e
void BIOS::strchr() {
  BIOS::index();
}

// BIOSA0:1f
void BIOS::strrchr() {
  BIOS::rindex();
}

void BIOS::strpbrk()   // BIOSA0:20
{
  u32 a0 = GPR(GPR_A0);
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strpbrk(src, psxMs8ptr(GPR(GPR_A1)));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::strspn()    // BIOSA0:21
{
  Return( ::strspn( psxMs8ptr(GPR(GPR_A0)), psxMs8ptr(GPR(GPR_A1)) ) );
}

void BIOS::strcspn()   // BIOSA0:22
{
  Return( ::strcspn( psxMs8ptr(GPR(GPR_A0)), psxMs8ptr(GPR(GPR_A1)) ) );
}

void BIOS::strtok()    // BIOSA0:23
{
  u32 a0 = GPR(GPR_A0);
  char *src = psxMs8ptr(a0);
  char *ret = ::strtok(src, psxMs8ptr(GPR(GPR_A1)));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::strstr()    // BIOSA0:24
{
  u32 a0 = GPR(GPR_A0);
  const char *src = psxMs8ptr(a0);
  const char *ret = ::strstr(src, psxMs8ptr(GPR(GPR_A1)));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::toupper()   // BIOSA0:25
{
  Return( ::toupper(GPR(GPR_A0)) );
}

void BIOS::tolower()   // BIOSA0:26
{
  Return( ::tolower(GPR(GPR_A0)) );
}

void BIOS::bcopy() // BIOSA0:27
{
  u32 a1 = GPR(GPR_A1);
  ::memcpy(psxMu8ptr(a1), psxMu8ptr(GPR(GPR_A0)), GPR(GPR_A2));
  // Return(a1);
  Return();
}

void BIOS::bzero() // BIOSA0:28
{
  u32 a0 = GPR(GPR_A0);
  ::memset(psxMu8ptr(a0), 0, GPR(GPR_A1));
  // Return(a0);
  Return();
}

void BIOS::bcmp()  // BIOSA0:29
{
  Return( ::memcmp( psxMu8ptr(GPR(GPR_A0)), psxMu8ptr(GPR(GPR_A1)), GPR(GPR_A2) ) );
}

void BIOS::memcpy()    // BIOSA0:2A
{
  u32 a0 = GPR(GPR_A0);
  ::memcpy(psxMu8ptr(a0), psxMu8ptr(GPR(GPR_A1)), GPR(GPR_A2));
  Return(a0);
}

void BIOS::memset()    // BIOSA0:2b
{
  u32 a0 = GPR(GPR_A0);
  ::memset(psxMu8ptr(a0), GPR(GPR_A1), GPR(GPR_A2));
  Return(a0);
}

void BIOS::memmove()    // BIOSA0:2c
{
  u32 a0 = GPR(GPR_A0);
  ::memmove(psxMu8ptr(a0), psxMu8ptr(GPR(GPR_A1)), GPR(GPR_A2));
  Return(a0);
}

void BIOS::memcmp()    // BIOSA0:2d
{
  Return( ::memcmp( psxMu8ptr(GPR(GPR_A0)), psxMu8ptr(GPR(GPR_A1)), GPR(GPR_A2) ) );
}

void BIOS::memchr()    // BIOSA0:2e
{
  u32 a0 = GPR(GPR_A0);
  const char *src = psxMs8ptr(a0);
  const char *ret = static_cast<const char*>(::memchr(src, GPR(GPR_A1), GPR(GPR_A2)));
  if (ret) {
    Return(a0 + (ret - src));
  } else {
    Return(0);
  }
}

void BIOS::rand()  // BIOSA0:2f
{
  Return( 1 + static_cast<int>(32767.0 * ::rand() / (RAND_MAX + 1.0)) );
}

void BIOS::srand() // BIOSA0:30
{
  ::srand(GPR(GPR_A0));
  Return();
}

void BIOS::malloc()    // BIOSA0:33
{
  u32 chunk = heap_addr;
  malloc_chunk *pChunk = reinterpret_cast<malloc_chunk*>(psxMu8ptr(chunk));
  u32 a0 = GPR(GPR_A0);

  // search for first chunk that is large enough and not currently being used
  while ( a0 > BFLIP32(pChunk->size) || BFLIP32(pChunk->stat) == 1/*INUSE*/ ) {
    chunk = pChunk->fd;
    pChunk = reinterpret_cast<malloc_chunk*>(psxMu8ptr(chunk));
  }

  // split free chunk
  u32 fd = chunk + sizeof(malloc_chunk) + a0;
  malloc_chunk *pFd = reinterpret_cast<malloc_chunk*>(psxMu8ptr(fd));
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

void BIOS::InitHeap()  // BIOSA0:39
{
  u32 a0 = GPR(GPR_A0);
  u32 a1 = GPR(GPR_A1);
  heap_addr = a0;

  malloc_chunk *chunk = reinterpret_cast<malloc_chunk*>(psxMu8ptr(a0));
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

// BIOSA0:44
void BIOS::FlushCache() {
  BIOS::nop();
}

void BIOS::_bu_init()      // BIOSA0:70
{
  DeliverEventEx(0x11, 0x2);    // 0xf0000011, 0x0004
  DeliverEventEx(0x81, 0x2);    // 0xf4000001, 0x0004
  Return();
}

// BIOSA0:71
void BIOS::_96_init() {
  BIOS::nop();
}

// BIOSA0:72
void BIOS::_96_remove() {
  BIOS::nop();
}



void BIOS::SetRCnt()   // B0:02
{
  u32 a0 = GPR(GPR_A0);
  u32 a1 = GPR(GPR_A1);
  u32 a2 = GPR(GPR_A2);

  a0 &= 0x3;
  SetGPR(GPR_A0, a0);
  if (a0 != 3) {
    u32 mode = 0;
    RCnt().WriteTarget(a0, a1);
    if (a2 & 0x1000) mode |= 0x050; // Interrupt Mode
    if (a2 & 0x0100) mode |= 0x008; // Count to 0xffff
    if (a2 & 0x0010) mode |= 0x001; // Timer stop mode
    if (a0 == 2) {
      if (a2 & 0x0001) mode |= 0x200; // System Clock mode
    } else {
      if (a2 & 0x0001) mode |= 0x100; // System Clock mode
    }
    RCnt().WriteMode(a0, mode);
  }
  Return();
}

void BIOS::GetRCnt()   // B0:03
{
  u32 a0 = GPR(GPR_A0);
  a0 &= 0x3;
  SetGPR(GPR_A0,  a0);
  if (a0 != 3) {
    Return(RCnt().ReadCount(a0));
  } else {
    Return(0);
  }
}

void BIOS::StartRCnt() // B0:04
{
  u32 a0 = GPR(GPR_A0);
  a0 &= 0x3;
  if (a0 != 3) {
    set_irq_mask( irq_mask() | BFLIP32(1<<(a0+4)) );
  } else {
    set_irq_mask( irq_mask() | BFLIP32(1) );
  }
  // SetGPR(GPR_A0, a0);
  Return(1);
}

void BIOS::StopRCnt()  // B0:05
{
  u32 a0 = GPR(GPR_A0);
  a0 &= 0x3;
  if (a0 != 3) {
    set_irq_mask( irq_mask() & BFLIP32( ~(1<<(a0+4)) ) );
  } else {
    set_irq_mask( irq_mask() & BFLIP32(~1) );
  }
  // SetGPR(GPR_A0, a0);
  Return(1);
}

void BIOS::ResetRCnt() // B0:06
{
  u32 a0 = GPR(GPR_A0);
  a0 &= 0x3;
  if (a0 != 3) {
    RCnt().WriteMode(a0, 0);
    RCnt().WriteTarget(a0, 0);
    RCnt().WriteCount(a0, 0);
  }
  // SetGPR(GPR_A0, a0);
  Return();
}

int BIOS::GetEv(int a0)
{
  int ev;
  ev = (a0 >> 24) & 0xf;
  if (ev == 0xf) ev = 0x5;
  ev *= 32;
  ev += a0 & 0x1f;
  return ev;
}

int BIOS::GetSpec(int a1)
{
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

void BIOS::DeliverEvent()  // B0:07
{
  int ev = GetEv(GPR(GPR_A0));
  int spec = GetSpec(GPR(GPR_A1));
  DeliverEventEx(ev, spec);
  Return();
}

void BIOS::OpenEvent() // B0:08
{
  int ev = GetEv(GPR(GPR_A0));
  int spec = GetSpec(GPR(GPR_A1));
  Event[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
  Event[ev][spec].mode = BFLIP32(GPR(GPR_A2));
  Event[ev][spec].fhandler = BFLIP32(GPR(GPR_A3));
  Return(ev | (spec << 8));
}

void BIOS::CloseEvent()    // B0:09
{
  u32 a0 = GPR(GPR_A0);
  int ev = a0 & 0xff;
  int spec = (a0 >> 8) & 0xff;
  Event[ev][spec].status = BFLIP32(EVENT_STATUS_UNUSED);
  Return(1);
}

void BIOS::WaitEvent() // B0:0a
{
  // same as EnableEvent??
  u32 a0 = GPR(GPR_A0);
  int ev = a0 & 0xff;
  int spec = (a0 >> 8) & 0xff;
  Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
  Return(1);
}

void BIOS::TestEvent() // B0:0b
{
  u32 a0 = GPR(GPR_A0);
  int ev = a0 & 0xff;
  int spec = (a0 >> 8) & 0xff;
  if (Event[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY)) {
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    Return(1);
  } else {
    Return(0);
  }
}

void BIOS::EnableEvent()   // B0:0c
{
  u32 a0 = GPR(GPR_A0);
  int ev = a0 & 0xff;
  int spec = (a0 >> 8) & 0xff;
  Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
  Return(1);
}

void BIOS::DisableEvent()  // B0:0d
{
  u32 a0 = GPR(GPR_A0);
  int ev = a0 & 0xff;
  int spec = (a0 >> 8) & 0xff;
  Event[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
  Return(1);
}

void BIOS::OpenTh()    // B0:0e
{
  u32 th;
  for (th = 1; th < 8; th++) {
    if (Thread[th].status == 0) {
      // TS_READY
      break;
    }
  }
  Thread[th].status = BFLIP32(1);
  Thread[th].func = BFLIP32(GPR(GPR_A0));
  Thread[th].reg.SP = BFLIP32(GPR(GPR_A1));
  Thread[th].reg.GP = BFLIP32(GPR(GPR_A2));
  Return(th);
}

void BIOS::CloseTh()   // B0:0f
{
  u32 th = GPR(GPR_A0) & 0xff;
  if (Thread[th].status == 0) {
    Return(0);
  } else {
    Thread[th].status = 0;
    Return(1);
  }
}

void BIOS::ChangeTh()  // B0:10
{
  u32 th = GPR(GPR_A0) & 0xff;
  if (Thread[th].status == 0 || CurThread == th) {
    Return(0);
    return;
  }
  if (Thread[CurThread].status == BFLIP32(2)) {
    Thread[CurThread].status = BFLIP32(1);
    Thread[CurThread].func = BFLIP32(GPR(GPR_RA));
    // WARNING: thread.HI and thread.LO are destroyed.
    Thread[CurThread].reg.Set(GPR());
  }
  // WARNING: HI and LO are destroyed.
  SetGPR(Thread[CurThread].reg);
  SetGPR(GPR_PC, BFLIP32(Thread[th].func));
  Thread[th].status = BFLIP32(2);
  CurThread = th;
  Return(1);
}

void BIOS::ReturnFromException()   // B0:17
{
  //    ::memcpy(GPR.R, regs, 32*sizeof(u32));
  //    GPR.LO = regs[32];
  //    GPR.HI = regs[33];
  SetGPR(savedGPR);
  u32 pc = CP0(COP0_EPC);
  if (CP0(COP0_CAUSE) & 0x80000000) {
    pc += 4;
  }
  SetGPR(GPR_PC, pc);
  u32 status = CP0(COP0_SR);
  SetCP0(COP0_SR, (status & 0xfffffff0) | ((status & 0x3c) >> 2) );
}

void BIOS::ResetEntryInt() // B0:18
{
  jmp_int = 0;
  Return();
}

void BIOS::HookEntryInt()  // B0:19
{
  jmp_int = psxMu32ptr(GPR(GPR_A0));
  Return();
}

void BIOS::UnDeliverEvent()    // B0:20
{
  int ev = GetEv(GPR(GPR_A0));
  int spec = GetSpec(GPR(GPR_A1));
  if (Event[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY) && Event[ev][spec].mode == BFLIP32(EVENT_MODE_NO_INTERRUPT)) {
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
  }
  Return();
}

void BIOS::GetC0Table()    // B0:56
{
  Return(0x674);
}

void BIOS::GetB0Table()    // B0:57
{
  Return(0x874);
}


void BIOS::SysEnqIntRP()   // C0:02
{
  SysIntRP[GPR(GPR_A0)] = GPR(GPR_A1);
  Return(0);
}

void BIOS::SysDeqIntRP()   // C0:03
{
  SysIntRP[GPR(GPR_A0)] = 0;
  Return(0);
}

void BIOS::ChangeClearRCnt()   // C0:0a
{
  u32 *ptr = psxMu32ptr((GPR(GPR_A0) << 2) + 0x8600);
  Return(BFLIP32(*ptr));
  *ptr = BFLIP32(GPR(GPR_A1));
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
  jmp_int = 0;

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
  Event = static_cast<EvCB*>(psxRptr(base));
  base += size*6;
  ::memset(Event, 0, size*6);
  RcEV = Event + 32*2;

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

  // OGPR(GPR_PC)ODE HLE!!
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
    // psx_->Mu32ref(0) = BFLIP32(PSX::R3000A::OGPR(GPR_PC)ODE_HLECALL);
    ::strcpy(psxMs8ptr(8), "psf2:/");
  }
}


void BIOS::Shutdown() {}


void BIOS::Interrupt()
{
  // for Root Counter 3 (interrupt = 1)
  if ( BFLIP32(irq_data()) & 1 ) {
    if (RcEV[3][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
      SoftCall(BFLIP32(RcEV[3][1].fhandler));
    }
  }

  // for Root Counter 0, 1, 2 (interrupt = 0x10, 0x20, 0x40)
  if ( BFLIP32(irq_data()) & 0x70 ) {
    for (int i = 0; i < 3; i++) {
      if (BFLIP32(irq_data()) & (1 << (i+4))) {
        if (RcEV[i][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
          SoftCall(BFLIP32(RcEV[i][1].fhandler));
          HwRegs().Write32(0x1f801070, ~(1 << (i+4)));
        }
      }
    } 
  }
}


void BIOS::Exception()
{
  u32 status;

  switch (CP0(COP0_CAUSE) & 0x3c) {
  case 0x00:  // Interrupt
    savedGPR.Set(GPR());
    Interrupt();
    for (int i = 0; i < 8; i++) {
      if (SysIntRP[i]) {
        u32 *queue = psxMu32ptr(SysIntRP[i]);
        SetGPR(GPR_S0, BFLIP32(queue[2]));
        SoftCall(BFLIP32(queue[1]));
      }
    }
    if (jmp_int) {
      HwRegs().Write32(0x1f801070, 0xffffffff);
      SetGPR(GPR_RA, BFLIP32(jmp_int[0]));
      SetGPR(GPR_SP, BFLIP32(jmp_int[1]));
      SetGPR(GPR_FP, BFLIP32(jmp_int[2]));
      for (int i = 0; i < 8; i++) {
        SetGPR(GPR_S0+i, BFLIP32(jmp_int[3+i]));
      }
      SetGPR(GPR_GP, BFLIP32(jmp_int[11]));
      Return(1);
      return;
    }
    HwRegs().Write16(0x1f801070, 0);
    break;
  case 0x20:  // SYSCALL
    status = CP0(COP0_SR);
    switch (GPR(GPR_A0)) {
    case 1: // EnterCritical (disable IRQs)
      status &= ~0x404;
      break;
    case 2: // LeaveCritical (enable IRQs)
      status |= 0x404;
    }
    SetGPR(GPR_PC, CP0(COP0_EPC) + 4);
    SetCP0(COP0_SR, (status & 0xfffffff0) | ((status & 0x3c) >> 2) );
    return;
  default:
    rennyLogWarning("PSXBIOS", "Unknown BIOS Exception (0x%02x)", CP0(COP0_CAUSE) & 0x3c);
  }

  u32 pc = CP0(COP0_EPC);
  if (CP0(COP0_CAUSE) & 0x80000000) {
    pc += 4;
  }
  SetGPR(GPR_PC, pc);
  status = CP0(COP0_SR);
  SetCP0(COP0_SR, (status & 0xfffffff0) | ((status & 0x3c) >> 2) );
}


}   // namespace PSX
