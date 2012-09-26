#include "PSXBIOS.h"
#include "R3000A.h"
#include "PSXMemory.h"
#include <wx/msgout.h>

#include <cstring>
#include <cstdlib>


using namespace R3000A;
using namespace PSXMemory;

namespace PSXBIOS {

struct malloc_chunk {
    uint32_t stat;
    uint32_t size;
    uint32_t fd;
    uint32_t bk;
};

typedef struct EventCallback {
    uint32_t desc;
    int32_t status;
    int32_t mode;
    uint32_t fhandler;
} EvCB[32];

const uint32_t EVENT_STATUS_UNUSED = 0x0000;
const uint32_t EVENT_STATUS_WAIT	= 0x1000;
const uint32_t EVENT_STATUS_ACTIVE = 0x2000;
const uint32_t EVENT_STATUS_READY = 0x4000;

const uint32_t EVENT_MODE_INTERRUPT	= 0x1000;
const uint32_t EVENT_MODE_NO_INTERRUPT = 0x2000;

struct TCB {
    int32_t status;
    int32_t mode;
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
static int CurThread;


static inline void softCall(uint32_t pc) {
    PC = pc;
    GPR.RA = 0x80001000;
    do {
        Interpreter::ExecuteBlock();
    } while (PC != 0x80001000);
}

static inline void softCall2(uint32_t pc) {
    uint32_t saved_ra = GPR.RA;
    PC = pc;
    GPR.RA = 0x80001000;
    do {
        Interpreter::ExecuteBlock();
    } while (GPR.PC != 0x80001000);
    GPR.RA = saved_ra;
}

static inline void DeliverEvent(uint32_t ev, uint32_t spec) {
    wxASSERT(Event != 0);

    if (Event[ev][spec].status != BFLIP32(EVENT_STATUS_ACTIVE)) return;

    if (Event[ev][spec].mode == BFLIP32(EVENT_MODE_INTERRUPT)) {
        softCall2(BFLIP32(Event[ev][spec].fhandler));
        return;
    }
    Event[ev][spec].status = BFLIP32(EVENT_STATUS_READY);
}

////////////////////////////////////////////////////////////////
// BIOS function
// function: 0xbfc0:00n0 - t1
// args: a0, a1, a2, a3
// returns: v0
////////////////////////////////////////////////////////////////


static void bios_nop()
{
    PC = GPR.RA;
}


static void abs()   // A0:0e
{
    int32_t a0 = static_cast<int32_t>(GPR.A0);
    if (a0 < 0) {
        GPR.V0 = -a0;
    } else {
        GPR.V0 = a0;
    }
    PC = GPR.RA;
}

static void (*const labs)() = abs;  // A0:0f

static void atoi()  // A0:10
{
    GPR.V0 = ::atoi(CharPtr(GPR.A0));
    PC = GPR.RA;
}

static void (*const atol)() = atoi; // A0:11

static void setjmp()    // A0:13
{
    uint32_t *jmp_buf = Uint32Ptr(GPR.A0);

    jmp_buf[0] = BFLIP32(GPR.RA);
    jmp_buf[1] = BFLIP32(GPR.SP);
    jmp_buf[2] = BFLIP32(GPR.FP);
    for (int i = 0; i < 8; i++) {
        jmp_buf[3+i] = BFLIP32(GPR.S[i]);
    }
    GPR.V0 = 0;
    PC = GPR.RA;
}

static void longjmp()   // A0:14
{
    uint32_t *jmp_buf = Uint32Ptr(GPR.A0);

    GPR.RA = BFLIP32(jmp_buf[0]);
    GPR.SP = BFLIP32(jmp_buf[1]);
    GPR.FP = BFLIP32(jmp_buf[2]);
    for (int i = 0; i < 8; i++) {
        GPR.S[i] = BFLIP32(jmp_buf[3+i]);
    }
    GPR.V0 = GPR.A1;
    PC = GPR.RA;
}

static void strcat()    // A0:15
{
    uint32_t a0 = GPR.A0;
    char *dest = CharPtr(a0);
    const char *src = CharPtr(GPR.A1);

    ::strcat(dest, src);

    GPR.V0 = a0;
    PC = GPR.RA;
}

static void strncat()   // A0:16
{
    uint32_t a0 = GPR.A0;
    char *dest = CharPtr(a0);
    const char *src = CharPtr(GPR.A1);
    const uint32_t count = LoadUserMemory32(GPR.A2);

    ::strncat(dest, src, count);

    GPR.V0 = a0;
    PC = GPR.RA;
}

static void strcmp()    // A0:17
{
    GPR.V0 = ::strcmp(CharPtr(GPR.A0), CharPtr(GPR.A1));
    PC = GPR.RA;
}

static void strncmp()    // A0:18
{
    GPR.V0 = ::strncmp(CharPtr(GPR.A0), CharPtr(GPR.A1), LoadUserMemory32(GPR.A2));
    PC = GPR.RA;
}

static void strcpy()    // A0:19
{
    uint32_t a0 = GPR.A0;
    ::strcpy(CharPtr(a0), CharPtr(GPR.A1));
    GPR.V0 = a0;
    PC = GPR.RA;
}

static void strncpy()   // A0:1a
{
    uint32_t a0 = GPR.A0;
    ::strncpy(CharPtr(a0), CharPtr(GPR.A1), LoadUserMemory32(GPR.A2));
    GPR.V0 = a0;
    PC = GPR.RA;
}

static void strlen()    // A0:1b
{
    GPR.V0 = ::strlen(CharPtr(GPR.A0));
    PC = GPR.RA;
}

static void index()     // A0:1c
{
    uint32_t a0 = GPR.A0;
    const char *src = CharPtr(a0);
    char *ret = ::strchr(src, GPR.A1);
    if (ret) {
        GPR.V0 = a0 + (ret - src);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void rindex()    // A0:1d
{
    uint32_t a0 = GPR.A0;
    const char *src = CharPtr(a0);
    char *ret = ::strrchr(src, GPR.A1);
    if (ret) {
        GPR.V0 = a0 + (ret - src);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void (*const strchr)() = index;  // A0:1e
static void (*const strrchr)() = rindex;    // A0:1f

static void strpbrk()   // A0:20
{
    uint32_t a0 = GPR.A0;
    const char *src = CharPtr(a0);
    char *ret = ::strpbrk(src, CharPtr(GPR.A1));
    if (ret) {
        GPR.V0 = a0 + (ret - src);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void strspn()    // A0:21
{
    GPR.V0 = ::strspn(CharPtr(GPR.A0), CharPtr(GPR.A1));
    PC = GPR.RA;
}

static void strcspn()   // A0:22
{
    GPR.V0 = ::strcspn(CharPtr(GPR.A0), CharPtr(GPR.A1));
    PC = GPR.RA;
}

static void strtok()    // A0:23
{
    uint32_t a0 = GPR.A0;
    char *src = CharPtr(a0);
    char *ret = ::strtok(src, CharPtr(GPR.A1));
    if (ret) {
        GPR.V0 = a0 + (ret - src);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void strstr()    // A0:24
{
    uint32_t a0 = GPR.A0;
    const char *src = CharPtr(a0);
    char *ret = ::strstr(src, CharPtr(GPR.A1));
    if (ret) {
        GPR.V0 = a0 + (ret - src);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void toupper()   // A0:25
{
    GPR.V0 = ::toupper(GPR.A0);
    PC = GPR.RA;
}

static void tolower()   // A0:26
{
    GPR.V0 = ::tolower(GPR.A0);
    PC = GPR.RA;
}

static void bcopy() // A0:27
{
    uint32_t a1 = GPR.A1;
    ::memcpy(CharPtr(a1), CharPtr(GPR.A0), GPR.A2);
    // GPR.V0 = a1;
    PC = GPR.RA;
}

static void bzero() // A0:28
{
    uint32_t a0 = GPR.A0;
    ::memset(CharPtr(a0), 0, GPR.A1);
    // GPR.V0 = a0;
    PC = GPR.RA;
}

static void bcmp()  // A0:29
{
    GPR.V0 = ::memcmp(CharPtr(GPR.A0), CharPtr(GPR.A1), GPR.A2);
    PC = GPR.RA;
}

static void memcpy()    // A0:2A
{
    uint32_t a0 = GPR.A0;
    ::memcpy(CharPtr(a0), CharPtr(GPR.A1), GPR.A2);
    GPR.V0 = a0;
    PC = GPR.RA;
}

static void memset()    // A0:2b
{
    uint32_t a0 = GPR.A0;
    ::memset(CharPtr(a0), GPR.A1, GPR.A2);
    GPR.V0 = a0;
    PC = GPR.RA;
}

static void memmove()    // A0:2c
{
    uint32_t a0 = GPR.A0;
    ::memmove(CharPtr(a0), CharPtr(GPR.A1), GPR.A2);
    GPR.V0 = a0;
    PC = GPR.RA;
}

static void memcmp()    // A0:2d
{
    GPR.V0 = ::memcmp(CharPtr(GPR.A0), CharPtr(GPR.A1), GPR.A2);
    PC = GPR.RA;
}

static void memchr()    // A0:2e
{
    uint32_t a0 = GPR.A0;
    const char *src = CharPtr(a0);
    char *ret = static_cast<char*>(::memchr(src, GPR.A1, GPR.A2));
    if (ret) {
        GPR.V0 = a0 + (ret - src);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void rand()  // A0:2f
{
    GPR.V0 = 1 + static_cast<int>(32767.0 * ::rand() / (RAND_MAX + 1.0));
    PC = GPR.RA;
}

static void srand() // A0:30
{
    ::srand(GPR.A0);
    PC = GPR.RA;
}

static void malloc()    // A0:33
{
    uint32_t chunk = heap_addr;
    malloc_chunk *pChunk = reinterpret_cast<malloc_chunk*>(CharPtr(chunk));
    uint32_t a0 = GPR.A0;

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

    GPR.V0 = (chunk + sizeof(malloc_chunk)) | 0x80000000;
    PC = GPR.RA;
}

static void InitHeap()  // A0:39
{
    uint32_t a0 = GPR.A0;
    uint32_t a1 = GPR.A1;
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
    PC = GPR.RA;
}

static void (*const FlushCache)() = bios_nop;    // A0:44

static void _bu_init()      // A0:70
{
    DeliverEvent(0x11, 0x2);    // 0xf0000011, 0x0004
    DeliverEvent(0x81, 0x2);    // 0xf4000001, 0x0004
    PC = GPR.RA;
}

static void (*const _96_init)() = bios_nop;     // A0:71
static void (*const _96_remove)() = bios_nop;   // A0:72


static void SetRCnt()   // B0:02
{
    uint32_t a0 = GPR.A0;
    uint32_t a1 = GPR.A1;
    uint32_t a2 = GPR.A2;

    a0 &= 0x3;
    GPR.A0 = a0;
    if (a0 != 3) {
        uint32_t mode = 0;
        // PSXRootCounter::Wtarget(a0, a1);
        if (a2 & 0x1000) mode |= 0x050; // Interrupt Mode
        if (a2 & 0x0100) mode |= 0x008; // Count to 0xffff
        if (a2 & 0x0010) mode |= 0x001; // Timer stop mode
        if (a0 == 2) {
            if (a2 & 0x0001) mode |= 0x200; // System Clock mode
        } else {
            if (a2 & 0x0001) mode |= 0x100; // System Clock mode
        }
        // PSXRootCounter::Wmode(a0, mode);
    }
    PC = GPR.RA;
}

static void GetRCnt()   // B0:03
{
    uint32_t a0 = GPR.A0;
    a0 &= 0x3;
    GPR.A0 = a0;
    if (a0 != 3) {
        // GPR.V0 = PSXRootCounter::count(a0);
    } else {
        GPR.V0 = 0;
    }
    PC = GPR.RA;
}

static void StartRCnt() // B0:04
{

}

}   // namespace PSXBIOS

static R3000A::GeneralPurposeRegisters savedGPR;


void PSXBIOS::Exception()
{
    using namespace R3000A;

    uint32_t i;

    switch (CP0.CAUSE & 0x3c) {
    case 0x00:  // Interrupt
        savedGPR = GPR;
        break;
    case 0x20:  // SYSCALL
        switch (GPR.A0) {
        case 1: // EnterCritical (disable IRQs)
            CP0.SR &= ~0x404;
            break;
        case 2: // LeaveCritical (enable IRQs)
            CP0.SR |= 0x404;
        }
        GPR.PC = CP0.EPC + 4;
        return;
    default:
        wxMessageOutputDebug().Printf("Unknown BIOS Exception (0x%02x)", R3000A::CP0.CAUSE & 0x3c);
    }
}
