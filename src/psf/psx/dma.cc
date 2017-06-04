#include "psf/psx/dma.h"
#include "psf/spu/spu.h"
#include "common/debug.h"
#include "psf/psx/psx.h"

namespace psx {


u32& DMA::D_MADR(u32 n)
{
  // rennyAssert(n < 7);
  return psxHu32ref(0x1080 + (n << 4));
}

u32& DMA::D_BCR(u32 n)
{
  // rennyAssert(n < 7);
  return psxHu32ref(0x1084 + (n << 4));
}

u32& DMA::D_CHCR(u32 n)
{
  // rennyAssert(n < 7);
  return psxHu32ref(0x1088 + (n << 4));
}


bool DMA::isDMATransferBusy(u32 n)
{
  // rennyAssert(n < 7);
  return ( BFLIP32(D_CHCR(n)) & 0x01000000 ) != 0;
}


void DMA::startDMATransfer(u32 n)
{
  // rennyAssert(n < 7);
  D_CHCR(n) &= BFLIP32(~0x01000000);
}

void DMA::endDMATransfer(u32 n)
{
  D_CHCR(n) &= BFLIP32(~0x01000000);
}


void DMA::execMDECin(u32, u32, u32) {}
void DMA::execMDECout(u32, u32, u32) {}
void DMA::execGPU(u32, u32, u32) {}
void DMA::execCDROM(u32, u32, u32) {}

void DMA::execSPU(u32 madr, u32 bcr, u32 chcr)
{
  // Transfer continuous stream of data
  switch (chcr) {
  case 0x01000201:    // CPU to DMA4 transfer
    bcr = (bcr >> 16) * (bcr & 0xffff) * sizeof(u32) * version_;
    Spu().WriteDMA4Memory(madr, bcr);
    return;
  case 0x01000200:    // DMA4 to CPU transfer
    bcr = (bcr >> 16) * (bcr & 0xffff) * sizeof(u32) * version_;
    Spu().ReadDMA4Memory(madr, bcr);
    return;
  }
}

void DMA::execPIO(u32, u32, u32) {}
void DMA::execGPU_OTC(u32, u32, u32) {}

void DMA::execSPU2(u32 madr, u32 bcr, u32 chcr)
{
  // Transfer continuous stream of data
  switch (chcr) {
  case 0x01000201:    // CPU to DMA4 transfer
  case 0x00100010:
  case 0x000f0010:
  case 0x00010010:
    bcr = (bcr >> 16) * (bcr & 0xffff) * sizeof(u32) * version_;
    Spu().WriteDMA7Memory(madr, bcr);
    return;
  case 0x01000200:    // DMA4 to CPU transfer
    bcr = (bcr >> 16) * (bcr & 0xffff) * sizeof(u32) * version_;
    Spu().ReadDMA7Memory(madr, bcr);
    return;
  }
}


DMA::DMA(PSX* composite)
  : Component(composite),
    IRQAccessor(composite),
    HardwareRegisterAccessor(composite),
    DPCR(psxHu32ref(0x10f0)),
    DICR(psxHu32ref(0x10f4)),
    version_(composite->version())
{
  executeTable[0] = &DMA::execMDECin;
  executeTable[1] = &DMA::execMDECout;
  executeTable[2] = &DMA::execGPU;
  executeTable[3] = &DMA::execCDROM;
  executeTable[4] = &DMA::execSPU;
  executeTable[5] = &DMA::execPIO;
  executeTable[6] = &DMA::execGPU_OTC;
  executeTable[7] = &DMA::execSPU2;
}


bool DMA::DMAEnabled(u32 n)
{
  rennyAssert(n < 7);
  return ( BFLIP32(DPCR) & (8 << (4*n)) ) != 0;
}


void DMA::Interrupt(u32 n)
{
  if ( BFLIP32(DICR) & (1 << (16 + n)) ) {
    DICR |= BFLIP32(1 << (24 + n));
    set_irq_data(8);
  }
}

void DMA::Execute(u32 n)
{
  if (isDMATransferBusy(n) && DMAEnabled(n)) {
    (this->*executeTable[n])( BFLIP32(D_MADR(n)), BFLIP32(D_BCR(n)), BFLIP32(D_CHCR(n)) );
    endDMATransfer(n);
    Interrupt(n);
  }
}


void DMA::Write(u32 n, u32 value)
{
  rennyAssert(n < 7);
  D_CHCR(n) = BFLIP32(value);
  Execute(n);
}

}   // namespace PSX
