#pragma once
#include <stdint.h>


namespace PSXMemory
{
    void Init();
    bool IsInited();
    void Reset();

    void Load(uint32_t address, int32_t length, char *data);

    uint8_t LoadUserMemory8(uint32_t);
    uint32_t LoadUserMemory32(uint32_t);

    char* CharPtr(uint32_t);
    uint32_t* Uint32Ptr(uint32_t);

    template<typename T> T Read(uint32_t addr);
    template<typename T> void Write(uint32_t addr, T value);

    uint8_t Load8(uint32_t);
    uint16_t Load16(uint32_t);
    uint32_t Load32(uint32_t);


    extern uint8_t *SegmentLUT[0x10000];
    extern uint8_t memUser[0x200000];
    extern uint8_t memParallelPort[0x10000];
    extern uint8_t memHardware[0x3000];   // Scratch Pad [0x0000 - 0x03ff] + Hardware Registers [0x1000 - 0x2fff]
    extern uint8_t memBIOS[0x80000];
}


////////////////////////////////
// BFLIP Macros
////////////////////////////////

template<typename T> inline T BFLIP(T x)
{
    return x;
}

#ifdef MSB_FIRST    // for big-endian architecture
template<> inline uint16_t BFLIP(uint16_t x)
{
    return ((x >> 8) & 0xff) | ((x & 0xff) << 8);
}
template<> inline uint32_t BFLIP(uint32_t x)
{
    return (x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) | (x << 24);
}
//#else   // for little-endian architecture
#endif

inline uint32_t BFLIP32(uint32_t x)
{
    return BFLIP(x);
}


inline uint8_t PSXMemory::LoadUserMemory8(uint32_t addr)
{
    return memUser[addr & 0x1fffff];
}

inline uint32_t PSXMemory::LoadUserMemory32(uint32_t addr)
{
    return BFLIP(*(reinterpret_cast<uint32_t*>(memUser + (addr & 0x1fffff))));
}

inline char* PSXMemory::CharPtr(uint32_t addr)
{
    return reinterpret_cast<char*>(memUser + (addr & 0x1fffff));
}

inline uint32_t* PSXMemory::Uint32Ptr(uint32_t addr)
{
    return reinterpret_cast<uint32_t*>(memUser + (addr & 0x1fffff));
}

////////////////////////////////////////
// Read from & Write into PSX memory
////////////////////////////////////////

template<typename T> T PSXMemory::Read(uint32_t addr)
{
    uint32_t segment = addr >> 16;
    if (segment == 0x1f80) {
        if (addr < 0x1f801000)  // Scratch Pad
            return BFLIP(0);
        // Hardware Registers
        return BFLIP(0);
    }
    uint8_t *base_addr = SegmentLUT[segment];
    uint32_t offset = addr & 0xffff;
    return BFLIP(*(T*)(base_addr + offset));
}

template<typename T> void PSXMemory::Write(uint32_t addr, T value)
{
    uint32_t segment = addr >> 16;
    if (segment == 0x1f80) {
        if (addr < 0x1f801000)
            return;
        return;
    }
    uint8_t *base_addr = SegmentLUT[segment];
    uint32_t offset = addr & 0xffff;
    *(T*)(base_addr + offset) = BFLIP(value);
}



inline uint32_t psxMu32(uint32_t addr)
{
    return PSXMemory::LoadUserMemory32(addr);
}

inline uint32_t psxHu32(uint32_t addr)
{
    return *(reinterpret_cast<uint32_t*>(PSXMemory::memHardware + (addr & 0x3fff)));
}

inline uint32_t& psxHu32ref(uint32_t addr)
{
    return *(reinterpret_cast<uint32_t*>(PSXMemory::memHardware + (addr & 0x3fff)));
}

