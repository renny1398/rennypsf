// #include "psx.h"
#include "psx/bios.h"
#include "psx/hardware.h"
#include "psx/rcnt.h"
#include "psx/interpreter.h"
#include <wx/msgout.h>

#include <cstring>
#include <cstdlib>


using namespace PSX::R3000A;


namespace PSX {


  BIOS::BIOS(Composite* composite)
    : Component(composite),
      GPR(R3000ARegs().GPR), CP0(R3000ARegs().CP0),
      PC(GPR.PC),
      A0(GPR.A0), A1(GPR.A1), A2(GPR.A2), A3(GPR.A3),
      V0(GPR.V0),
      GP(GPR.GP), SP(GPR.SP), FP(GPR.FP), RA(GPR.RA) {}


  void BIOS::SoftCall(u32 pc) {
    wxASSERT(pc != 0x80001000);
    PC = pc;
    RA = 0x80001000;
    do {
      Interp().ExecuteBlock();
    } while (PC != 0x80001000);
  }

  void BIOS::SoftCall2(u32 pc) {
    u32 saved_ra = RA;
    PC = pc;
    RA = 0x80001000;
    do {
      Interp().ExecuteBlock();
    } while (PC != 0x80001000);
    RA = saved_ra;
  }

  void BIOS::DeliverEventEx(u32 ev, u32 spec) {
    wxASSERT(Event != 0);

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
    PC = RA;
  }


  void BIOS::abs()   // A0:0e
  {
    int32_t a0 = static_cast<int32_t>(A0);
    if (a0 < 0) {
      V0 = -a0;
    } else {
      V0 = a0;
    }
    PC = RA;
  }

  // A0:0f
  void BIOS::labs() {
    BIOS::abs();
  }

  void BIOS::atoi()  // A0:10
  {
    V0 = ::atoi(S8M_ptr(A0));
    PC = RA;
  }

  // A0:11
  void BIOS::atol() {
    BIOS::atoi();
  }

  void BIOS::setjmp()    // A0:13
  {
    u32 *jmp_buf = U32M_ptr(A0);

    jmp_buf[0] = BFLIP32(RA);
    jmp_buf[1] = BFLIP32(SP);
    jmp_buf[2] = BFLIP32(FP);
    for (int i = 0; i < 8; i++) {
      jmp_buf[3+i] = BFLIP32(GPR.S[i]);
    }
    jmp_buf[11] = BFLIP32(GP);
    V0 = 0;
    PC = RA;
  }

  void BIOS::longjmp()   // A0:14
  {
    u32 *jmp_buf = U32M_ptr(A0);

    RA = BFLIP32(jmp_buf[0]);
    SP = BFLIP32(jmp_buf[1]);
    FP = BFLIP32(jmp_buf[2]);
    for (int i = 0; i < 8; i++) {
      GPR.S[i] = BFLIP32(jmp_buf[3+i]);
    }
    V0 = A1;
    PC = RA;
  }

  void BIOS::strcat()    // A0:15
  {
    u32 a0 = A0;
    char *dest = S8M_ptr(a0);
    const char *src = S8M_ptr(A1);

    ::strcat(dest, src);

    V0 = a0;
    PC = RA;
  }

  void BIOS::strncat()   // A0:16
  {
    u32 a0 = A0;
    char *dest = S8M_ptr(a0);
    const char *src = S8M_ptr(A1);
    const u32 count = U32M_ref(A2);

    ::strncat(dest, src, count);

    V0 = a0;
    PC = RA;
  }

  void BIOS::strcmp()    // A0:17
  {
    V0 = ::strcmp(S8M_ptr(A0), S8M_ptr(A1));
    PC = RA;
  }

  void BIOS::strncmp()    // A0:18
  {
    V0 = ::strncmp(S8M_ptr(A0), S8M_ptr(A1), U32M_ref(A2));
    PC = RA;
  }

  void BIOS::strcpy()    // A0:19
  {
    u32 a0 = A0;
    ::strcpy(S8M_ptr(a0), S8M_ptr(A1));
    V0 = a0;
    PC = RA;
  }

  void BIOS::strncpy()   // A0:1a
  {
    u32 a0 = A0;
    ::strncpy(S8M_ptr(a0), S8M_ptr(A1), U32M_ref(A2));
    V0 = a0;
    PC = RA;
  }

  void BIOS::strlen()    // A0:1b
  {
    V0 = ::strlen(S8M_ptr(A0));
    PC = RA;
  }

  void BIOS::index()     // A0:1c
  {
    u32 a0 = A0;
    const char *src = S8M_ptr(a0);
    const char *ret = ::strchr(src, A1);
    if (ret) {
      V0 = a0 + (ret - src);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::rindex()    // A0:1d
  {
    u32 a0 = A0;
    const char *src = S8M_ptr(a0);
    const char *ret = ::strrchr(src, A1);
    if (ret) {
      V0 = a0 + (ret - src);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  // A0:1e
  void BIOS::strchr() {
    BIOS::index();
  }

  // A0:1f
  void BIOS::strrchr() {
    BIOS::rindex();
  }

  void BIOS::strpbrk()   // A0:20
  {
    u32 a0 = A0;
    const char *src = S8M_ptr(a0);
    const char *ret = ::strpbrk(src, S8M_ptr(A1));
    if (ret) {
      V0 = a0 + (ret - src);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::strspn()    // A0:21
  {
    V0 = ::strspn(S8M_ptr(A0), S8M_ptr(A1));
    PC = RA;
  }

  void BIOS::strcspn()   // A0:22
  {
    V0 = ::strcspn(S8M_ptr(A0), S8M_ptr(A1));
    PC = RA;
  }

  void BIOS::strtok()    // A0:23
  {
    u32 a0 = A0;
    char *src = S8M_ptr(a0);
    char *ret = ::strtok(src, S8M_ptr(A1));
    if (ret) {
      V0 = a0 + (ret - src);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::strstr()    // A0:24
  {
    u32 a0 = A0;
    const char *src = S8M_ptr(a0);
    const char *ret = ::strstr(src, S8M_ptr(A1));
    if (ret) {
      V0 = a0 + (ret - src);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::toupper()   // A0:25
  {
    V0 = ::toupper(A0);
    PC = RA;
  }

  void BIOS::tolower()   // A0:26
  {
    V0 = ::tolower(A0);
    PC = RA;
  }

  void BIOS::bcopy() // A0:27
  {
    u32 a1 = A1;
    ::memcpy(U8M_ptr(a1), U8M_ptr(A0), A2);
    // V0 = a1;
    PC = RA;
  }

  void BIOS::bzero() // A0:28
  {
    u32 a0 = A0;
    ::memset(U8M_ptr(a0), 0, A1);
    // V0 = a0;
    PC = RA;
  }

  void BIOS::bcmp()  // A0:29
  {
    V0 = ::memcmp(U8M_ptr(A0), U8M_ptr(A1), A2);
    PC = RA;
  }

  void BIOS::memcpy()    // A0:2A
  {
    u32 a0 = A0;
    ::memcpy(U8M_ptr(a0), U8M_ptr(A1), A2);
    V0 = a0;
    PC = RA;
  }

  void BIOS::memset()    // A0:2b
  {
    u32 a0 = A0;
    ::memset(U8M_ptr(a0), A1, A2);
    V0 = a0;
    PC = RA;
  }

  void BIOS::memmove()    // A0:2c
  {
    u32 a0 = A0;
    ::memmove(U8M_ptr(a0), U8M_ptr(A1), A2);
    V0 = a0;
    PC = RA;
  }

  void BIOS::memcmp()    // A0:2d
  {
    V0 = ::memcmp(U8M_ptr(A0), U8M_ptr(A1), A2);
    PC = RA;
  }

  void BIOS::memchr()    // A0:2e
  {
    u32 a0 = A0;
    const char *src = S8M_ptr(a0);
    const char *ret = static_cast<const char*>(::memchr(src, A1, A2));
    if (ret) {
      V0 = a0 + (ret - src);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::rand()  // A0:2f
  {
    V0 = 1 + static_cast<int>(32767.0 * ::rand() / (RAND_MAX + 1.0));
    PC = RA;
  }

  void BIOS::srand() // A0:30
  {
    ::srand(A0);
    PC = RA;
  }

  void BIOS::malloc()    // A0:33
  {
    u32 chunk = heap_addr;
    malloc_chunk *pChunk = reinterpret_cast<malloc_chunk*>(U8M_ptr(chunk));
    u32 a0 = A0;

    // search for first chunk that is large enough and not currently being used
    while ( a0 > BFLIP32(pChunk->size) || BFLIP32(pChunk->stat) == 1/*INUSE*/ ) {
      chunk = pChunk->fd;
      pChunk = reinterpret_cast<malloc_chunk*>(U8M_ptr(chunk));
    }

    // split free chunk
    u32 fd = chunk + sizeof(malloc_chunk) + a0;
    malloc_chunk *pFd = reinterpret_cast<malloc_chunk*>(U8M_ptr(fd));
    pFd->stat = pChunk->stat;
    pFd->size = BFLIP32( BFLIP32(pChunk->size) - a0 );
    pFd->fd = pChunk->fd;
    pFd->bk = chunk;

    // set new chunk
    pChunk->stat = BFLIP32(1);
    pChunk->size = BFLIP32(a0);
    pChunk->fd = fd;

    V0 = (chunk + sizeof(malloc_chunk)) | 0x80000000;
    PC = RA;
  }

  void BIOS::InitHeap()  // A0:39
  {
    u32 a0 = A0;
    u32 a1 = A1;
    heap_addr = a0;

    malloc_chunk *chunk = reinterpret_cast<malloc_chunk*>(U8M_ptr(a0));
    chunk->stat = 0;
    if ( (a0 & 0x1fffff) + a1 >= 0x200000 ) {
      chunk->size = BFLIP32( 0x1ffffc - (a0 & 0x1fffff) );
    } else {
      chunk->size = BFLIP32(a1);
    }
    chunk->fd = 0;
    chunk->bk = 0;
    PC = RA;
  }

  // A0:44
  void BIOS::FlushCache() {
    BIOS::nop();
  }

  void BIOS::_bu_init()      // A0:70
  {
    DeliverEventEx(0x11, 0x2);    // 0xf0000011, 0x0004
    DeliverEventEx(0x81, 0x2);    // 0xf4000001, 0x0004
    PC = RA;
  }

  // A0:71
  void BIOS::_96_init() {
    BIOS::nop();
  }

  // A0:72
  void BIOS::_96_remove() {
    BIOS::nop();
  }



  void BIOS::SetRCnt()   // B0:02
  {
    u32 a0 = A0;
    u32 a1 = A1;
    u32 a2 = A2;

    a0 &= 0x3;
    A0 = a0;
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
    PC = RA;
  }

  void BIOS::GetRCnt()   // B0:03
  {
    u32 a0 = A0;
    a0 &= 0x3;
    A0 = a0;
    if (a0 != 3) {
      V0 = RCnt().ReadCount(a0);
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::StartRCnt() // B0:04
  {
    u32 a0 = A0;
    a0 &= 0x3;
    if (a0 != 3) {
      U32H_ref(0x1074) |= BFLIP32(1<<(a0+4));
    } else {
      U32H_ref(0x1074) |= BFLIP32(1);
    }
    // A0 = a0;
    V0 = 1;
    PC = RA;
  }

  void BIOS::StopRCnt()  // B0:05
  {
    u32 a0 = A0;
    a0 &= 0x3;
    if (a0 != 3) {
      U32H_ref(0x1074) &= BFLIP32( ~(1<<(a0+4)) );
    } else {
      U32H_ref(0x1074) &= BFLIP32(~1);
    }
    // A0 = a0;
    V0 = 1;
    PC = RA;
  }

  void BIOS::ResetRCnt() // B0:06
  {
    u32 a0 = A0;
    a0 &= 0x3;
    if (a0 != 3) {
      RCnt().WriteMode(a0, 0);
      RCnt().WriteTarget(a0, 0);
      RCnt().WriteCount(a0, 0);
    }
    // A0 = a0;
    PC = RA;
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
    int ev = GetEv(A0);
    int spec = GetSpec(A1);
    DeliverEventEx(ev, spec);
    PC = RA;
  }

  void BIOS::OpenEvent() // B0:08
  {
    int ev = GetEv(A0);
    int spec = GetSpec(A1);
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
    Event[ev][spec].mode = BFLIP32(A2);
    Event[ev][spec].fhandler = BFLIP32(A3);
    V0 = ev | (spec << 8);
    PC = RA;
  }

  void BIOS::CloseEvent()    // B0:09
  {
    u32 a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_UNUSED);
    V0 = 1;
    PC = RA;
  }

  void BIOS::WaitEvent() // B0:0a
  {
    // same as EnableEvent??
    u32 a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    V0 = 1;
    PC = RA;
  }

  void BIOS::TestEvent() // B0:0b
  {
    u32 a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    if (Event[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY)) {
      Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
      V0 = 1;
    } else {
      V0 = 0;
    }
    PC = RA;
  }

  void BIOS::EnableEvent()   // B0:0c
  {
    u32 a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    V0 = 1;
    PC = RA;
  }

  void BIOS::DisableEvent()  // B0:0d
  {
    u32 a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
    V0 = 1;
    PC = RA;
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
    Thread[th].func = BFLIP32(A0);
    Thread[th].reg[29] = BFLIP32(A1);
    Thread[th].reg[28] = BFLIP32(A2);
    V0 = th;
    PC = RA;
  }

  void BIOS::CloseTh()   // B0:0f
  {
    u32 th = A0 & 0xff;
    if (Thread[th].status == 0) {
      V0 = 0;
    } else {
      Thread[th].status = 0;
      V0 = 1;
    }
    PC = RA;
  }

  void BIOS::ChangeTh()  // B0:10
  {
    u32 th = A0 &0xff;
    if (Thread[th].status == 0 || CurThread == th) {
      V0 = 0;
      PC = RA;
      return;
    }
    V0 = 1;
    if (Thread[CurThread].status == BFLIP32(2)) {
      Thread[CurThread].status = BFLIP32(1);
      Thread[CurThread].func = BFLIP32(RA);
      ::memcpy(Thread[CurThread].reg, GPR.R, 32*sizeof(u32));
    }
    ::memcpy(GPR.R, Thread[th].reg, 32*sizeof(u32));
    PC = BFLIP32(Thread[th].func);
    Thread[th].status = BFLIP32(2);
    CurThread = th;
  }

  void BIOS::ReturnFromException()   // B0:17
  {
    //    ::memcpy(GPR.R, regs, 32*sizeof(u32));
    //    GPR.LO = regs[32];
    //    GPR.HI = regs[33];
    GPR = savedGPR;
    u32 pc = CP0.EPC;
    if (CP0.CAUSE & 0x80000000) {
      pc += 4;
    }
    PC = pc;
    u32 status = CP0.SR;
    CP0.SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
  }

  void BIOS::ResetEntryInt() // B0:18
  {
    jmp_int = 0;
    PC = RA;
  }

  void BIOS::HookEntryInt()  // B0:19
  {
    jmp_int = U32M_ptr(A0);
    PC = RA;
  }

  void BIOS::UnDeliverEvent()    // B0:20
  {
    int ev = GetEv(A0);
    int spec = GetSpec(A1);
    if (Event[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY) && Event[ev][spec].mode == BFLIP32(EVENT_MODE_NO_INTERRUPT)) {
      Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    }
    PC = RA;
  }

  void BIOS::GetC0Table()    // B0:56
  {
    V0 = 0x674;
    PC = RA;
  }

  void BIOS::GetB0Table()    // B0:57
  {
    V0 = 0x874;
    PC = RA;
  }


  void BIOS::SysEnqIntRP()   // C0:02
  {
    SysIntRP[A0] = A1;
    V0 = 0;
    PC = RA;
  }

  void BIOS::SysDeqIntRP()   // C0:03
  {
    SysIntRP[A0] = 0;
    V0 = 0;
    PC = RA;
  }

  void BIOS::ChangeClearRCnt()   // C0:0a
  {
    u32 *ptr = U32M_ptr((A0 << 2) + 0x8600);
    V0 = BFLIP32(*ptr);
    *ptr = BFLIP32(A1);
    PC = RA;
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
  Event = static_cast<EvCB*>(R_ptr(base));
  base += size*6;
  ::memset(Event, 0, size*6);
  RcEV = Event + 32*2;

  // set b0 table
  u32 *ptr = U32M_ptr(0x0874);
  ptr[0] = BFLIP32(0x4c54 - 0x884);

  // set c0 table
  ptr = U32M_ptr(0x0674);
  ptr[6] = BFLIP32(0x0c80);

  ::memset(SysIntRP, 0, sizeof(SysIntRP));
  ::memset(Thread, 0, sizeof(Thread));
  Thread[0].status = BFLIP32(2);

  U32M_ref(0x0150) = BFLIP32(0x160);
  U32M_ref(0x0154) = BFLIP32(0x320);
  U32M_ref(0x0160) = BFLIP32(0x248);
  ::strcpy(S8M_ptr(0x248), "bu");

  // OPCODE HLE!!
  U32R_ref(0x0000) = BFLIP32((OPCODE_HLECALL << 26) | 4);
  U32M_ref(0x0000) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  U32M_ref(0x00a0) = BFLIP32((OPCODE_HLECALL << 26) | 1);
  U32M_ref(0x00b0) = BFLIP32((OPCODE_HLECALL << 26) | 2);
  U32M_ref(0x00c0) = BFLIP32((OPCODE_HLECALL << 26) | 3);
  U32M_ref(0x4c54) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  U32M_ref(0x8000) = BFLIP32((OPCODE_HLECALL << 26) | 5);
  U32M_ref(0x07a0) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  U32M_ref(0x0884) = BFLIP32((OPCODE_HLECALL << 26) | 0);
  U32M_ref(0x0894) = BFLIP32((OPCODE_HLECALL << 26) | 0);
}


void BIOS::Shutdown() {}


void BIOS::Interrupt()
{
  // wxMessageOutputDebug().Printf(wxT("Interrupt (PC = 0x%08x)"), R3000a.PC-4);
  if ( BFLIP32(U32H_ref(0x1070)) & 1 ) {
    if (RcEV[3][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
      SoftCall(BFLIP32(RcEV[3][1].fhandler));
    }
  }

  if ( BFLIP32(U32H_ref(0x1070)) & 0x70 ) {
    for (int i = 0; i < 3; i++) {
      if (BFLIP32(U32H_ref(0x1070)) & (1 << (i+4))) {
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

  switch (CP0.CAUSE & 0x3c) {
  case 0x00:  // Interrupt
    savedGPR = GPR;
    Interrupt();
    for (int i = 0; i < 8; i++) {
      if (SysIntRP[i]) {
        u32 *queue = U32M_ptr(SysIntRP[i]);
        GPR.S0 = BFLIP32(queue[2]);
        SoftCall(BFLIP32(queue[1]));
      }
    }
    if (jmp_int) {
      HwRegs().Write32(0x1f801070, 0xffffffff);
      RA = BFLIP32(jmp_int[0]);
      SP = BFLIP32(jmp_int[1]);
      FP = BFLIP32(jmp_int[2]);
      for (int i = 0; i < 8; i++) {
        GPR.R[16+i] = BFLIP32(jmp_int[3+i]);
      }
      GP = BFLIP32(jmp_int[11]);
      V0 = 1;
      PC = RA;
      return;
    }
    HwRegs().Write16(0x1f801070, 0);
    break;
  case 0x20:  // SYSCALL
    status = CP0.SR;
    switch (A0) {
    case 1: // EnterCritical (disable IRQs)
      status &= ~0x404;
      break;
    case 2: // LeaveCritical (enable IRQs)
      status |= 0x404;
    }
    GPR.PC = CP0.EPC + 4;
    CP0.SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
    return;
  default:
    wxMessageOutputDebug().Printf(wxT("Unknown BIOS Exception (0x%02x)"), CP0.CAUSE & 0x3c);
  }

  u32 pc = CP0.EPC;
  if (CP0.CAUSE & 0x80000000) {
    pc += 4;
  }
  PC = pc;
  status = CP0.SR;
  CP0.SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
}


}   // namespace PSX
