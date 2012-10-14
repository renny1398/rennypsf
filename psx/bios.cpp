#include "psx.h"
#include "interpreter.h"
#include <wx/msgout.h>

#include <cstring>
#include <cstdlib>


using namespace PSX::R3000A;
using namespace PSX::Memory;


namespace {

using namespace PSX;
uint32_t& PC = R3000a.PC;
uint32_t& A0 = R3000a.GPR.A0;
uint32_t& A1 = R3000a.GPR.A1;
uint32_t& A2 = R3000a.GPR.A2;
uint32_t& A3 = R3000a.GPR.A3;
uint32_t& V0 = R3000a.GPR.V0;
uint32_t& GP = R3000a.GPR.GP;
uint32_t& SP = R3000a.GPR.SP;
uint32_t& FP = R3000a.GPR.FP;
uint32_t& RA = R3000a.GPR.RA;

GeneralPurposeRegisters& GPR = R3000a.GPR;
Cop0Registers& CP0 = R3000a.CP0;

}



namespace PSX {
namespace BIOS {

struct malloc_chunk {
    uint32_t stat;
    uint32_t size;
    uint32_t fd;
    uint32_t bk;
};

typedef struct EventCallback {
    uint32_t desc;
    uint32_t status;
    uint32_t mode;
    uint32_t fhandler;
} EvCB[32];

const uint32_t EVENT_STATUS_UNUSED = 0x0000;
const uint32_t EVENT_STATUS_WAIT	= 0x1000;
const uint32_t EVENT_STATUS_ACTIVE = 0x2000;
const uint32_t EVENT_STATUS_ALREADY = 0x4000;

const uint32_t EVENT_MODE_INTERRUPT	= 0x1000;
const uint32_t EVENT_MODE_NO_INTERRUPT = 0x2000;

struct TCB {
    uint32_t status;
    uint32_t mode;
    uint32_t reg[32];
    uint32_t func;
};

static uint32_t *jmp_int;

static uint32_t regs[35];
static EvCB *Event;

//static EvCB *HwEV; // 0xf0
//static EvCB *EvEV; // 0xf1
static EvCB *RcEV; // 0xf2
//static EvCB *UeEV; // 0xf3
//static EvCB *SwEV; // 0xf4
//static EvCB *ThEV; // 0xff

static uint32_t heap_addr;
static uint32_t SysIntRP[8];
static TCB Thread[8];
static uint32_t CurThread;


static inline void softCall(uint32_t pc) {
    PC = pc;
    RA = 0x80001000;
    do {
        Interpreter_.ExecuteBlock();
    } while (PC != 0x80001000);
}

static inline void softCall2(uint32_t pc) {
    uint32_t saved_ra = RA;
    PC = pc;
    RA = 0x80001000;
    do {
        Interpreter_.ExecuteBlock();
    } while (PC != 0x80001000);
    RA = saved_ra;
}

static inline void _DeliverEvent(uint32_t ev, uint32_t spec) {
    wxASSERT(Event != 0);

    if (Event[ev][spec].status != BFLIP32(EVENT_STATUS_ACTIVE)) return;

    if (Event[ev][spec].mode == BFLIP32(EVENT_MODE_INTERRUPT)) {
        softCall2(BFLIP32(Event[ev][spec].fhandler));
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


static void bios_nop()
{
    PC = RA;
}


static void abs()   // A0:0e
{
    int32_t a0 = static_cast<int32_t>(A0);
    if (a0 < 0) {
        V0 = -a0;
    } else {
        V0 = a0;
    }
    PC = RA;
}

static void (*const labs)() = abs;  // A0:0f

static void atoi()  // A0:10
{
    V0 = ::atoi(CharPtr(A0));
    PC = RA;
}

static void (*const atol)() = atoi; // A0:11

static void setjmp()    // A0:13
{
    uint32_t *jmp_buf = Uint32Ptr(A0);

    jmp_buf[0] = BFLIP32(RA);
    jmp_buf[1] = BFLIP32(SP);
    jmp_buf[2] = BFLIP32(FP);
    for (int i = 0; i < 8; i++) {
        jmp_buf[3+i] = BFLIP32(GPR.S[i]);
    }
    V0 = 0;
    PC = RA;
}

static void longjmp()   // A0:14
{
    uint32_t *jmp_buf = Uint32Ptr(A0);

    RA = BFLIP32(jmp_buf[0]);
    SP = BFLIP32(jmp_buf[1]);
    FP = BFLIP32(jmp_buf[2]);
    for (int i = 0; i < 8; i++) {
        GPR.S[i] = BFLIP32(jmp_buf[3+i]);
    }
    V0 = A1;
    PC = RA;
}

static void strcat()    // A0:15
{
    uint32_t a0 = A0;
    char *dest = CharPtr(a0);
    const char *src = CharPtr(A1);

    ::strcat(dest, src);

    V0 = a0;
    PC = RA;
}

static void strncat()   // A0:16
{
    uint32_t a0 = A0;
    char *dest = CharPtr(a0);
    const char *src = CharPtr(A1);
    const uint32_t count = u32M(A2);

    ::strncat(dest, src, count);

    V0 = a0;
    PC = RA;
}

static void strcmp()    // A0:17
{
    V0 = ::strcmp(CharPtr(A0), CharPtr(A1));
    PC = RA;
}

static void strncmp()    // A0:18
{
    V0 = ::strncmp(CharPtr(A0), CharPtr(A1), u32M(A2));
    PC = RA;
}

static void strcpy()    // A0:19
{
    uint32_t a0 = A0;
    ::strcpy(CharPtr(a0), CharPtr(A1));
    V0 = a0;
    PC = RA;
}

static void strncpy()   // A0:1a
{
    uint32_t a0 = A0;
    ::strncpy(CharPtr(a0), CharPtr(A1), u32M(A2));
    V0 = a0;
    PC = RA;
}

static void strlen()    // A0:1b
{
    V0 = ::strlen(CharPtr(A0));
    PC = RA;
}

static void index()     // A0:1c
{
    uint32_t a0 = A0;
    const char *src = CharPtr(a0);
    char *ret = ::strchr(src, A1);
    if (ret) {
        V0 = a0 + (ret - src);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void rindex()    // A0:1d
{
    uint32_t a0 = A0;
    const char *src = CharPtr(a0);
    char *ret = ::strrchr(src, A1);
    if (ret) {
        V0 = a0 + (ret - src);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void (*const strchr)() = index;  // A0:1e
static void (*const strrchr)() = rindex;    // A0:1f

static void strpbrk()   // A0:20
{
    uint32_t a0 = A0;
    const char *src = CharPtr(a0);
    char *ret = ::strpbrk(src, CharPtr(A1));
    if (ret) {
        V0 = a0 + (ret - src);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void strspn()    // A0:21
{
    V0 = ::strspn(CharPtr(A0), CharPtr(A1));
    PC = RA;
}

static void strcspn()   // A0:22
{
    V0 = ::strcspn(CharPtr(A0), CharPtr(A1));
    PC = RA;
}

static void strtok()    // A0:23
{
    uint32_t a0 = A0;
    char *src = CharPtr(a0);
    char *ret = ::strtok(src, CharPtr(A1));
    if (ret) {
        V0 = a0 + (ret - src);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void strstr()    // A0:24
{
    uint32_t a0 = A0;
    const char *src = CharPtr(a0);
    char *ret = ::strstr(src, CharPtr(A1));
    if (ret) {
        V0 = a0 + (ret - src);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void toupper()   // A0:25
{
    V0 = ::toupper(A0);
    PC = RA;
}

static void tolower()   // A0:26
{
    V0 = ::tolower(A0);
    PC = RA;
}

static void bcopy() // A0:27
{
    uint32_t a1 = A1;
    ::memcpy(CharPtr(a1), CharPtr(A0), A2);
    // V0 = a1;
    PC = RA;
}

static void bzero() // A0:28
{
    uint32_t a0 = A0;
    ::memset(CharPtr(a0), 0, A1);
    // V0 = a0;
    PC = RA;
}

static void bcmp()  // A0:29
{
    V0 = ::memcmp(CharPtr(A0), CharPtr(A1), A2);
    PC = RA;
}

static void memcpy()    // A0:2A
{
    uint32_t a0 = A0;
    ::memcpy(CharPtr(a0), CharPtr(A1), A2);
    V0 = a0;
    PC = RA;
}

static void memset()    // A0:2b
{
    uint32_t a0 = A0;
    ::memset(CharPtr(a0), A1, A2);
    V0 = a0;
    PC = RA;
}

static void memmove()    // A0:2c
{
    uint32_t a0 = A0;
    ::memmove(CharPtr(a0), CharPtr(A1), A2);
    V0 = a0;
    PC = RA;
}

static void memcmp()    // A0:2d
{
    V0 = ::memcmp(CharPtr(A0), CharPtr(A1), A2);
    PC = RA;
}

static void memchr()    // A0:2e
{
    uint32_t a0 = A0;
    const char *src = CharPtr(a0);
    char *ret = static_cast<char*>(::memchr(src, A1, A2));
    if (ret) {
        V0 = a0 + (ret - src);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void rand()  // A0:2f
{
    V0 = 1 + static_cast<int>(32767.0 * ::rand() / (RAND_MAX + 1.0));
    PC = RA;
}

static void srand() // A0:30
{
    ::srand(A0);
    PC = RA;
}

static void malloc()    // A0:33
{
    uint32_t chunk = heap_addr;
    malloc_chunk *pChunk = reinterpret_cast<malloc_chunk*>(CharPtr(chunk));
    uint32_t a0 = A0;

    // search for first chunk that is large enough and not currently being used
    while ( a0 > BFLIP32(pChunk->size) || BFLIP32(pChunk->stat) == 1/*INUSE*/ ) {
        chunk = pChunk->fd;
        pChunk = reinterpret_cast<malloc_chunk*>(CharPtr(chunk));
    }

    // split free chunk
    uint32_t fd = chunk + sizeof(malloc_chunk) + a0;
    malloc_chunk *pFd = reinterpret_cast<malloc_chunk*>(CharPtr(fd));
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

static void InitHeap()  // A0:39
{
    uint32_t a0 = A0;
    uint32_t a1 = A1;
    heap_addr = a0;

    malloc_chunk *chunk = reinterpret_cast<malloc_chunk*>(CharPtr(a0));
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

static void (*const FlushCache)() = bios_nop;    // A0:44

static void _bu_init()      // A0:70
{
    _DeliverEvent(0x11, 0x2);    // 0xf0000011, 0x0004
    _DeliverEvent(0x81, 0x2);    // 0xf4000001, 0x0004
    PC = RA;
}

static void (*const _96_init)() = bios_nop;     // A0:71
static void (*const _96_remove)() = bios_nop;   // A0:72



static void SetRCnt()   // B0:02
{
    uint32_t a0 = A0;
    uint32_t a1 = A1;
    uint32_t a2 = A2;

    a0 &= 0x3;
    A0 = a0;
    if (a0 != 3) {
        uint32_t mode = 0;
        RootCounter::WriteTarget(a0, a1);
        if (a2 & 0x1000) mode |= 0x050; // Interrupt Mode
        if (a2 & 0x0100) mode |= 0x008; // Count to 0xffff
        if (a2 & 0x0010) mode |= 0x001; // Timer stop mode
        if (a0 == 2) {
            if (a2 & 0x0001) mode |= 0x200; // System Clock mode
        } else {
            if (a2 & 0x0001) mode |= 0x100; // System Clock mode
        }
        RootCounter::WriteMode(a0, mode);
    }
    PC = RA;
}

static void GetRCnt()   // B0:03
{
    uint32_t a0 = A0;
    a0 &= 0x3;
    A0 = a0;
    if (a0 != 3) {
        V0 = RootCounter::ReadCount(a0);
    } else {
        V0 = 0;
    }
    PC = RA;
}

static void StartRCnt() // B0:04
{
    uint32_t a0 = A0;
    a0 &= 0x3;
    if (a0 != 3) {
        u32Href(0x1074) |= BFLIP32(1<<(a0+4));
    } else {
        u32Href(0x1074) |= BFLIP32(1);
    }
    // A0 = a0;
    V0 = 1;
    PC = RA;
}

static void StopRCnt()  // B0:05
{
    uint32_t a0 = A0;
    a0 &= 0x3;
    if (a0 != 3) {
        u32Href(0x1074) &= BFLIP32( ~(1<<(a0+4)) );
    } else {
        u32Href(0x1074) &= BFLIP32(~1);
    }
    // A0 = a0;
    V0 = 1;
    PC = RA;
}

static void ResetRCnt() // B0:06
{
    uint32_t a0 = A0;
    a0 &= 0x3;
    if (a0 != 3) {
        RootCounter::WriteMode(a0, 0);
        RootCounter::WriteTarget(a0, 0);
        RootCounter::WriteCount(a0, 0);
    }
    // A0 = a0;
    PC = RA;
}

static inline int GetEv(int a0)
{
    int ev;
    ev = (a0 >> 24) & 0xf;
    if (ev == 0xf) ev = 0x5;
    ev *= 32;
    ev += a0 & 0x1f;
    return ev;
}

static inline int GetSpec(int a1)
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

static void DeliverEvent()  // B0:07
{
    int ev = GetEv(A0);
    int spec = GetSpec(A1);
    _DeliverEvent(ev, spec);
    PC = RA;
}

static void OpenEvent() // B0:08
{
    int ev = GetEv(A0);
    int spec = GetSpec(A1);
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
    Event[ev][spec].mode = BFLIP32(A2);
    Event[ev][spec].fhandler = BFLIP32(A3);
    V0 = ev | (spec << 8);
    PC = RA;
}

static void CloseEvent()    // B0:09
{
    uint32_t a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_UNUSED);
    V0 = 1;
    PC = RA;
}

static void WaitEvent() // B0:0a
{
    // same as EnableEvent??
    uint32_t a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    V0 = 1;
    PC = RA;
}

static void TestEvent() // B0:0b
{
    uint32_t a0 = A0;
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

static void EnableEvent()   // B0:0c
{
    uint32_t a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    V0 = 1;
    PC = RA;
}

static void DisableEvent()  // B0:0d
{
    uint32_t a0 = A0;
    int ev = a0 & 0xff;
    int spec = (a0 >> 8) & 0xff;
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_WAIT);
    V0 = 1;
    PC = RA;
}

static void OpenTh()    // B0:0e
{
    uint32_t th;
    for (th = 1; th < 8; th++) {
        if (Thread[th].status == 0) break;
    }
    Thread[th].status = BFLIP32(1);
    Thread[th].func = BFLIP32(A0);
    Thread[th].reg[29] = BFLIP32(A1);
    Thread[th].reg[28] = BFLIP32(A2);
    V0 = th;
    PC = RA;
}

static void CloseTh()   // B0:0f
{
    uint32_t th = A0 & 0xff;
    if (Thread[th].status == 0) {
        V0 = 0;
    } else {
        Thread[th].status = 0;
        V0 = 1;
    }
    PC = RA;
}

static void ChangeTh()  // B0:10
{
    uint32_t th = A0 &0xff;
    if (Thread[th].status == 0 || CurThread == th) {
        V0 = 0;
        PC = RA;
        return;
    }
    V0 = 1;
    if (Thread[CurThread].status == BFLIP32(2)) {
        Thread[CurThread].status = BFLIP32(1);
        Thread[CurThread].func = BFLIP32(RA);
        ::memcpy(Thread[CurThread].reg, GPR.R, 32*sizeof(uint32_t));
    }
    ::memcpy(GPR.R, Thread[th].reg, 32*sizeof(uint32_t));
    PC = BFLIP32(Thread[th].func);
    Thread[th].status = BFLIP32(2);
    CurThread = th;
}

static void ReturnFromException()   // B0:17
{
    ::memcpy(GPR.R, regs, 32*sizeof(uint32_t));
    GPR.LO = regs[32];
    GPR.HI = regs[33];
    uint32_t pc = CP0.EPC;
    if (CP0.CAUSE & 0x80000000) {
        pc += 4;
    }
    PC = pc;
    uint32_t status = CP0.SR;
    CP0.SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
}

static void ResetEntryInt() // B0:18
{
    jmp_int = 0;
    PC = RA;
}

static void HookEntryInt()  // B0:19
{
    jmp_int = Uint32Ptr(A0);
    PC = RA;
}

static void UnDeliverEvent()    // B0:20
{
    int ev = GetEv(A0);
    int spec = GetSpec(A1);
    if (Event[ev][spec].status == BFLIP32(EVENT_STATUS_ALREADY) && Event[ev][spec].mode == BFLIP32(EVENT_MODE_NO_INTERRUPT)) {
        Event[ev][spec].status = BFLIP32(EVENT_STATUS_ACTIVE);
    }
    PC = RA;
}

static void GetC0Table()    // B0:56
{
    V0 = 0x674;
    PC = RA;
}

static void GetB0Table()    // B0:57
{
    V0 = 0x874;
    PC = RA;
}


static void SysEnqIntRP()   // C0:02
{
    SysIntRP[A0] = A1;
    V0 = 0;
    PC = RA;
}

static void SysDeqIntRP()   // C0:03
{
    SysIntRP[A0] = 0;
    V0 = 0;
    PC = RA;
}

static void ChangeClearRCnt()   // C0:0a
{
    uint32_t *ptr = Uint32Ptr((A0 << 2) + 0x8600);
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

void Init()
{
    heap_addr = 0;
    CurThread = 0;
    jmp_int = 0;

    for (int i = 0; i < 256; i++) {
        biosA0[i] = bios_nop;
        biosB0[i] = bios_nop;
        biosC0[i] = bios_nop;
    }

    biosA0[0x0e] = abs;
    biosA0[0x0f] = labs;
    biosA0[0x10] = atoi;
    biosA0[0x11] = atol;
    biosA0[0x13] = setjmp;
    biosA0[0x14] = longjmp;
    biosA0[0x15] = strcat;
    biosA0[0x16] = strncat;
    biosA0[0x17] = strcmp;
    biosA0[0x18] = strncmp;
    biosA0[0x19] = strcpy;
    biosA0[0x1a] = strncpy;
    biosA0[0x1b] = strlen;
    biosA0[0x1c] = index;
    biosA0[0x1d] = rindex;
    biosA0[0x1e] = strchr;
    biosA0[0x1f] = strrchr;
    biosA0[0x20] = strpbrk;
    biosA0[0x21] = strspn;
    biosA0[0x22] = strcspn;
    biosA0[0x23] = strtok;
    biosA0[0x24] = strstr;
    biosA0[0x25] = toupper;
    biosA0[0x26] = tolower;
    biosA0[0x27] = bcopy;
    biosA0[0x28] = bzero;
    biosA0[0x29] = bcmp;
    biosA0[0x2a] = memcpy;
    biosA0[0x2b] = memset;
    biosA0[0x2c] = memmove;
    biosA0[0x2d] = memcmp;
    biosA0[0x2e] = memchr;
    biosA0[0x2f] = rand;
    biosA0[0x30] = srand;
    biosA0[0x33] = malloc;
    biosA0[0x39] = InitHeap;
    biosA0[0x44] = FlushCache;
    biosA0[0x70] = _bu_init;
    biosA0[0x71] = _96_init;
    biosA0[0x72] = _96_remove;

    biosB0[0x02] = SetRCnt;
    biosB0[0x03] = GetRCnt;
    biosB0[0x04] = StartRCnt;
    biosB0[0x05] = StopRCnt;
    biosB0[0x06] = ResetRCnt;
    biosB0[0x07] = DeliverEvent;
    biosB0[0x08] = OpenEvent;
    biosB0[0x09] = CloseEvent;
    biosB0[0x0a] = WaitEvent;
    biosB0[0x0b] = TestEvent;
    biosB0[0x0c] = EnableEvent;
    biosB0[0x0d] = DisableEvent;
    biosB0[0x0e] = OpenTh;
    biosB0[0x0f] = CloseTh;
    biosB0[0x10] = ChangeTh;
    biosB0[0x17] = ReturnFromException;
    biosB0[0x18] = ResetEntryInt;
    biosB0[0x19] = HookEntryInt;
    biosB0[0x20] = UnDeliverEvent;
    biosB0[0x56] = GetC0Table;
    biosB0[0x57] = GetB0Table;

    biosC0[0x02] = SysEnqIntRP;
    biosC0[0x03] = SysDeqIntRP;
    biosC0[0x0a] = ChangeClearRCnt;

    uint32_t base = 0x1000;
    uint32_t size = sizeof(EvCB) * 32;
    Event = pointer_cast<EvCB*>(&memBIOS[base]);
    base += size*6;
    ::memset(Event, 0, size*6);
    RcEV = Event + 32*2;

    // set b0 table
    uint32_t *ptr = pointer_cast<uint32_t*>(&memUser[0x0874]);
    ptr[0] = BFLIP32(0x4c54 - 0x884);

    // set c0 table
    ptr = pointer_cast<uint32_t*>(&memUser[0x0674]);
    ptr[6] = BFLIP32(0x0c80);

    ::memset(SysIntRP, 0, sizeof(SysIntRP));
    ::memset(Thread, 0, sizeof(Thread));
    Thread[0].status = BFLIP32(2);

    u32Mref(0x0150) = BFLIP32(0x160);
    u32Mref(0x0154) = BFLIP32(0x320);
    u32Mref(0x0160) = BFLIP32(0x248);
    ::strcpy(pointer_cast<char*>(memUser) + 0x248, "bu");

    // OPCODE HLE!!
    u32Rref(0x0000) = BFLIP32((OPCODE_HLECALL << 26) | 4);
    u32Mref(0x0000) = BFLIP32((OPCODE_HLECALL << 26) | 0);
    u32Mref(0x00a0) = BFLIP32((OPCODE_HLECALL << 26) | 1);
    u32Mref(0x00b0) = BFLIP32((OPCODE_HLECALL << 26) | 2);
    u32Mref(0x00c0) = BFLIP32((OPCODE_HLECALL << 26) | 3);
    u32Mref(0x4c54) = BFLIP32((OPCODE_HLECALL << 26) | 0);
    u32Mref(0x8000) = BFLIP32((OPCODE_HLECALL << 26) | 5);
    u32Mref(0x07a0) = BFLIP32((OPCODE_HLECALL << 26) | 0);
    u32Mref(0x0884) = BFLIP32((OPCODE_HLECALL << 26) | 0);
    u32Mref(0x0894) = BFLIP32((OPCODE_HLECALL << 26) | 0);
}


void Shutdown() {}


void Interrupt()
{
    if (BFLIP32(u32H(0x1070)) & 1) {
        if (RcEV[3][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
            softCall(BFLIP32(RcEV[3][1].fhandler));
        }
    }

    if (BFLIP32(u32H(0x1070)) & 0x70) {
        for (int i = 0; i < 3; i++) {
            if (BFLIP32(u32H(0x1070)) & (1 << (i+4))) {
                if (RcEV[i][1].status == BFLIP32(EVENT_STATUS_ACTIVE)) {
                    softCall(BFLIP32(RcEV[i][1].fhandler));
                    Hardware::Write32(0x1f801070, ~(1 << (i+4)));
                }
            }
        }
    }
}


static GeneralPurposeRegisters savedGPR;

void Exception()
{
    uint32_t status;

   switch (CP0.CAUSE & 0x3c) {
    case 0x00:  // Interrupt
        savedGPR = GPR;
        Interrupt();
        for (int i = 0; i < 8; i++) {
            if (SysIntRP[i]) {
                uint32_t *queue = Uint32Ptr(SysIntRP[i]);
                GPR.S0 = BFLIP32(queue[2]);
                softCall(BFLIP32(queue[1]));
            }
        }
        if (jmp_int) {
            Hardware::Write32(0x1f801070, 0xffffffff);
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
        Hardware::Write16(0x1f801070, 0);
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
        wxMessageOutputDebug().Printf("Unknown BIOS Exception (0x%02x)", CP0.CAUSE & 0x3c);
    }

   uint32_t pc = CP0.EPC;
   if (CP0.CAUSE & 0x80000000) {
       pc += 4;
   }
   PC = pc;
   status = CP0.SR;
   CP0.SR = (status & 0xfffffff0) | ((status & 0x3c) >> 2);
}


}   // namespace BIOS
}   // namespace PSX
