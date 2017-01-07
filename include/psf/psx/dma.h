#pragma once
#include "common.h"
#include "hardware.h"

namespace PSX {

  class DMA : public Component, private IRQAccessor, private HardwareRegisterAccessor {

    // 0x1f80_1080: DMA0 MDECin
    // 0x1f80_1090: DMA1 MDECout
    // 0x1f80_10a0: DMA2 GPU
    // 0x1f80_10b0: DMA3 CD-ROM
    // 0x1f80_10c0: DMA4 SPU
    // 0x1f80_10d0: DMA5 PIO
    // 0x1f80_10e0: DMA6 GPU OTC

   public:
    DMA(Composite* composite);

    bool DMAEnabled(u32 n);

    void Interrupt(u32 n);

    void Execute(u32 n);

    void Write(u32 n, u32 value);

    u32& DPCR;
    u32& DICR;

   protected:
    u32& D_MADR(u32 n);
    u32& D_BCR(u32 n);
    u32& D_CHCR(u32 n);

    bool isDMATransferBusy(u32 n);

    void startDMATransfer(u32 n);
    void endDMATransfer(u32 n);

    void execMDECin(u32, u32, u32);
    void execMDECout(u32, u32, u32);
    void execGPU(u32, u32, u32);
    void execCDROM(u32, u32, u32);
    void execSPU(u32 madr, u32 bcr, u32 chcr);
    void execPIO(u32, u32, u32);
    void execGPU_OTC(u32, u32, u32);

   private:
    // DMA Primary Control Register (DPCR)
    void (DMA::*executeTable[7])(u32, u32, u32);
  };
}   // namespace PSX
