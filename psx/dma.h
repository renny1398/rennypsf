#pragma once
#include "memory.h"
#include <wx/debug.h>

namespace PSX {
namespace DMA {

// 0x1f80_1080: DMA0 MDECin
// 0x1f80_1090: DMA1 MDECout
// 0x1f80_10a0: DMA2 GPU
// 0x1f80_10b0: DMA3 CD-ROM
// 0x1f80_10c0: DMA4 SPU
// 0x1f80_10d0: DMA5 PIO
// 0x1f80_10e0: DMA6 GPU OTC

inline uint32_t& D_MADR(uint32_t n)
{
    // wxASSERT(n < 7);
    return u32Href(0x1080 + (n << 4));
}
inline uint32_t& D_BCR(uint32_t n)
{
    // wxASSERT(n < 7);
    return u32Href(0x1084 + (n << 4));
}
inline uint32_t& D_CHCR(uint32_t n)
{
    // wxASSERT(n < 7);
    return u32Href(0x1088 + (n << 4));
}

inline bool isDMATransferBusy(uint32_t n)
{
    // wxASSERT(n < 7);
    return ( BFLIP32(D_CHCR(n)) & 0x01000000 ) != 0;
}
/*
inline void startDMATransfer()
{
    // wxASSERT(n < 7);
    D_CHCR(n) &= BFLIP32(~0x01000000);
}
*/
inline void endDMATransfer(uint32_t n)
{
    D_CHCR(n) &= BFLIP32(~0x01000000);
}


// DMA4 Registers
/*
uint32_t& DMA4_MADR = D_MADR(4);
uint32_t& DMA4_BCR = D_BCR(4);
uint32_t& DMA4_CHCR = D_CHCR(4);
*/

// DMA Primary Control Register (DPCR)
extern uint32_t& DPCR;
extern uint32_t& DICR;

inline bool DMAEnabled(uint32_t n)
{
    wxASSERT(n < 7);
    return ( BFLIP32(DPCR) & (8 << (4*n)) ) != 0;
}


inline void Interrupt(uint32_t n)
{
    if ( BFLIP32(DICR) & (1 << (16 + n)) ) {
        DICR |= BFLIP32(1 << (24 + n));
        u32Href(0x1070) |= BFLIP32(8);
    }
}

extern void (*executeTable[7])(uint32_t, uint32_t, uint32_t);

inline void Execute(uint32_t n)
{
    if (isDMATransferBusy(n) && DMAEnabled(n)) {
        executeTable[n]( BFLIP32(D_MADR(n)), BFLIP32(D_BCR(n)), BFLIP32(D_CHCR(n)) );
        endDMATransfer(n);
        Interrupt(n);
    }
}

inline void Write(uint32_t n, uint32_t value)
{
    wxASSERT(n < 7);
    D_CHCR(n) = BFLIP32(value);
    Execute(n);
}

}   // namespace DMA
}   // namespace PSX
