#pragma once
#include "common.h"
#include "r3000a.h"


namespace PSX {
  class BIOS : public Component {

   public:
    BIOS(Composite* composite);

    void Init();
    void Shutdown();
    void Interrupt();
    void Exception();

    static void (BIOS::*biosA0[256])();
    static void (BIOS::*biosB0[256])();
    static void (BIOS::*biosC0[256])();


    // BIOS functions
    void nop();

    void abs();
    void labs();
    void atoi();
    void atol();
    void setjmp();
    void longjmp();
    void strcat();
    void strncat();
    void strcmp();
    void strncmp();
    void strcpy();
    void strncpy();
    void strlen();
    void index();
    void rindex();
    void strchr();
    void strrchr();
    void strpbrk();
    void strspn();
    void strcspn();
    void strtok();
    void strstr();
    void toupper();
    void tolower();
    void bcopy();
    void bzero();
    void bcmp();
    void memcpy();
    void memset();
    void memmove();
    void memcmp();
    void memchr();
    void rand();
    void srand();
    void malloc();
    void InitHeap();
    void FlushCache();
    void _bu_init();
    void _96_init();
    void _96_remove();

    void SetRCnt();
    void GetRCnt();
    void StartRCnt();
    void StopRCnt();
    void ResetRCnt();

    static int GetEv(int a0);
    static int GetSpec(int a1);

    void DeliverEvent();
    void OpenEvent();
    void CloseEvent();
    void WaitEvent();
    void TestEvent();
    void EnableEvent();
    void DisableEvent();
    void OpenTh();
    void CloseTh();
    void ChangeTh();
    void ReturnFromException();
    void ResetEntryInt();
    void HookEntryInt();
    void UnDeliverEvent();
    void GetC0Table();
    void GetB0Table();

    void SysEnqIntRP();
    void SysDeqIntRP();
    void ChangeClearRCnt();


   protected:
    void SoftCall(u32 pc);
    void SoftCall2(u32 pc);
    void DeliverEventEx(u32 ev, u32 spec);

   private:
    R3000A::GeneralPurposeRegisters& GPR;
    R3000A::Cop0Registers CP0;

    u32& PC;
    u32& A0, &A1, &A2, &A3;
    u32& V0;
    u32& GP;
    u32& SP;
    u32& FP;
    u32& RA;


    struct malloc_chunk {
      u32 stat;
      u32 size;
      u32 fd;
      u32 bk;
    };

    typedef struct EventCallback {
      u32 desc;
      u32 status;
      u32 mode;
      u32 fhandler;
    } EvCB[32];

    static const u32 EVENT_STATUS_UNUSED = 0x0000;
    static const u32 EVENT_STATUS_WAIT	= 0x1000;
    static const u32 EVENT_STATUS_ACTIVE = 0x2000;
    static const u32 EVENT_STATUS_ALREADY = 0x4000;

    static const u32 EVENT_MODE_INTERRUPT = 0x1000;
    static const u32 EVENT_MODE_NO_INTERRUPT = 0x2000;

    struct TCB {
      u32 status;
      u32 mode;
      u32 reg[32];
      u32 func;
    };

    u32 *jmp_int;

    R3000A::GeneralPurposeRegisters savedGPR;
    EvCB *Event;

    //static EvCB *HwEV; // 0xf0
    //static EvCB *EvEV; // 0xf1
    EvCB *RcEV; // 0xf2
    //static EvCB *UeEV; // 0xf3
    //static EvCB *SwEV; // 0xf4
    //static EvCB *ThEV; // 0xff

    u32 heap_addr;
    u32 SysIntRP[8];
    TCB Thread[8];
    u32 CurThread;
  };

}   // namespace PSX
