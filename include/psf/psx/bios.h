#pragma once
#include "common.h"
#include "r3000a.h"
#include "memory.h"
#include "hardware.h"

namespace psx {
class BIOS : public Component, /*private mips::RegisterAccessor, */private UserMemoryAccessor, private IRQAccessor {

 public:
  BIOS(PSX* psx);

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
  void Return();
  void Return(u32 return_code);
  void SoftCall(u32 pc);
  void SoftCall2(u32 pc);
  void DeliverEventEx(u32 ev, u32 spec);

 private:
  // PSX* const psx_;
  mips::GeneralPurposeRegisters& GPR;
  mips::Cop0Registers& CP0;

  /*
  u32& PC;
  u32& A0, &A1, &A2, &A3;
  u32& V0;
  u32& GP;
  u32& SP;
  u32& FP;
  u32& RA;
  */

  struct JumpBuffer {
    JumpBuffer() = default;
    void Get(mips::GeneralPurposeRegisters* gpr) const {
      rennyAssert(gpr != nullptr);
      (*gpr)(GPR_RA) = BFLIP32(RA);
      (*gpr)(GPR_SP) = BFLIP32(SP);
      (*gpr)(GPR_FP) = BFLIP32(FP);
      (*gpr)(GPR_S0) = BFLIP32(S0);
      (*gpr)(GPR_S1) = BFLIP32(S1);
      (*gpr)(GPR_S2) = BFLIP32(S2);
      (*gpr)(GPR_S3) = BFLIP32(S3);
      (*gpr)(GPR_S4) = BFLIP32(S4);
      (*gpr)(GPR_S5) = BFLIP32(S5);
      (*gpr)(GPR_S6) = BFLIP32(S6);
      (*gpr)(GPR_S7) = BFLIP32(S7);
      (*gpr)(GPR_GP) = BFLIP32(GP);
    }
    void Set(const mips::GeneralPurposeRegisters& gpr) {
      RA = BFLIP32(gpr(GPR_RA));
      SP = BFLIP32(gpr(GPR_SP));
      FP = BFLIP32(gpr(GPR_FP));
      S0 = BFLIP32(gpr(GPR_S0));
      S1 = BFLIP32(gpr(GPR_S1));
      S2 = BFLIP32(gpr(GPR_S2));
      S3 = BFLIP32(gpr(GPR_S3));
      S4 = BFLIP32(gpr(GPR_S4));
      S5 = BFLIP32(gpr(GPR_S5));
      S6 = BFLIP32(gpr(GPR_S6));
      S7 = BFLIP32(gpr(GPR_S7));
      GP = BFLIP32(gpr(GPR_GP));
    }
    uint32_t RA;
    uint32_t SP;
    uint32_t FP;
    uint32_t S0, S1, S2, S3, S4, S5, S6, S7;
    uint32_t GP;
  };

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
    // u32 reg[32];
    mips::GeneralPurposeRegisters reg;
    u32 func;
  };

  JumpBuffer* jmp_int;

  mips::GeneralPurposeRegisters savedGPR;
  EvCB* events_base_;

  // EvCB *HwEV; // 0xf0
  // EvCB *EvEV; // 0xf1
  EvCB* rcnt_event_; // 0xf2
  // EvCB *UeEV; // 0xf3
  // EvCB *SwEV; // 0xf4
  // EvCB *ThEV; // 0xff

  u32 heap_addr;
  u32 SysIntRP[8];
  TCB Thread[8];
  u32 CurThread;
};

}   // namespace PSX
