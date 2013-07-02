#include "dma.h"
#include "../spu/spu.h"


namespace PSX {
namespace DMA {


void execMDECin(uint32_t, uint32_t, uint32_t) {}
void execMDECout(uint32_t, uint32_t, uint32_t) {}
void execGPU(uint32_t, uint32_t, uint32_t) {}
void execCDROM(uint32_t, uint32_t, uint32_t) {}


void execSPU(uint32_t madr, uint32_t bcr, uint32_t chcr)
{
    // Transfer continuous stream of data
    switch (chcr) {
    case 0x01000201:    // CPU to DMA4 transfer
        bcr = (bcr >> 16) * (bcr & 0xffff) * sizeof(uint32_t);
        Spu.WriteDMAMemory(madr, bcr);
        return;
    case 0x01000200:    // DMA4 to CPU transfer
        bcr = (bcr >> 16) * (bcr & 0xffff) * sizeof(uint32_t);
        Spu.ReadDMAMemory(madr, bcr);
        return;
    }
}


void execPIO(uint32_t, uint32_t, uint32_t) {}
void execGPU_OTC(uint32_t, uint32_t, uint32_t) {}


void (*executeTable[7])(uint32_t, uint32_t, uint32_t) = {
    execMDECin, execMDECout, execGPU, execCDROM,
    execSPU, execPIO, execGPU_OTC
};


uint32_t& DPCR = u32Href(0x10f0);
uint32_t& DICR = u32Href(0x10f4);


}   // namespace DMA
}   // namespace PSX
