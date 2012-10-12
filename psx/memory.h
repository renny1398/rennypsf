#pragma once
#include <stdint.h>


template <class T>
inline T pointer_cast(void* p)
{
    return static_cast<T>(p);
}


namespace PSX {
namespace Memory {

    void Init();
    bool IsInited();
    void Reset();

    void Load(uint32_t address, int32_t length, char *data);

//    uint8_t LoadUserMemory8(uint32_t);
//    uint32_t LoadUserMemory32(uint32_t);

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

}   // namespace Memory
}   // namespace PSX


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

inline uint16_t BFLIP16(uint16_t x) {
    return BFLIP(x);
}
inline uint32_t BFLIP32(uint32_t x) {
    return BFLIP(x);
}

/*
inline uint8_t PSXMemory::LoadUserMemory8(uint32_t addr)
{
    return memUser[addr & 0x1fffff];
}

inline uint32_t PSXMemory::LoadUserMemory32(uint32_t addr)
{
    return BFLIP(*(reinterpret_cast<uint32_t*>(memUser + (addr & 0x1fffff))));
}
*/

inline char* PSX::Memory::CharPtr(uint32_t addr)
{
    return pointer_cast<char*>(memUser + (addr & 0x1fffff));
}

inline uint32_t* PSX::Memory::Uint32Ptr(uint32_t addr)
{
    return pointer_cast<uint32_t*>(memUser + (addr & 0x1fffff));
}


template<typename T>
inline T psxM(uint32_t addr) {
    return BFLIP( *pointer_cast<T*>(PSX::Memory::memUser + (addr & 0x1fffff)) );
}

template<typename T>
inline T& psxMref(uint32_t addr) {
    return *pointer_cast<T*>(PSX::Memory::memUser + (addr & 0x1fffff));
}

template<typename T>
inline T psxH(uint32_t addr) {
    return BFLIP( *pointer_cast<T*>(PSX::Memory::memHardware + (addr & 0x3fff)) );
}

template<typename T>
inline T& psxHref(uint32_t addr) {
    return *pointer_cast<T*>(PSX::Memory::memHardware + (addr & 0x3fff));
}

template<typename T>
inline T& psxRref(uint32_t addr) {
    return *pointer_cast<T*>(PSX::Memory::memBIOS + (addr & 0xffff));
}


inline uint8_t psxMu8(uint32_t addr) {
    return psxM<uint8_t>(addr);
}

inline uint16_t psxMu16(uint32_t addr) {
    return psxM<uint16_t>(addr);
}

inline uint32_t psxMu32(uint32_t addr) {
    return psxM<uint32_t>(addr);
}


inline uint8_t& psxMu8ref(uint32_t addr) {
    return psxMref<uint8_t>(addr);
}

inline uint16_t& psxMu16ref(uint32_t addr) {
    return psxMref<uint16_t>(addr);
}

inline uint32_t& psxMu32ref(uint32_t addr) {
    return psxMref<uint32_t>(addr);
}


inline uint8_t psxHu8(uint32_t addr) {
    return psxH<uint8_t>(addr);
}

inline uint16_t psxHu16(uint32_t addr) {
    return psxH<uint16_t>(addr);
}

inline uint32_t psxHu32(uint32_t addr) {
    return psxH<uint32_t>(addr);
}


inline uint8_t& psxHu8ref(uint32_t addr) {
    return psxHref<uint8_t>(addr);
}

inline uint16_t& psxHu16ref(uint32_t addr) {
    return psxHref<uint16_t>(addr);
}

inline uint32_t& psxHu32ref(uint32_t addr) {
    return psxHref<uint32_t>(addr);
}


inline uint32_t& psxRu32ref(uint32_t addr) {
    return psxRref<uint32_t>(addr);
}


////////////////////////////////////////
// Read from & Write into PSX memory
////////////////////////////////////////

#include "hardware.h"
#include <wx/msgout.h>

template<typename T>
T PSX::Memory::Read(uint32_t addr)
{
    uint32_t segment = addr >> 16;
    if (segment == 0x1f80) {
        if (addr < 0x1f801000)  // Scratch Pad
            return psxH<T>(addr);
        // Hardware Registers
        return Hardware::Read<T>(addr);
    }
    uint8_t *base_addr = SegmentLUT[segment];
    uint32_t offset = addr & 0xffff;
    if (base_addr == 0) {
        wxMessageOutputDebug().Printf("PSX::Memory::Read : bad segment: %04x", segment);
        return 0;
    }
    return BFLIP(*pointer_cast<T*>(base_addr + offset));
}

template<typename T>
void PSX::Memory::Write(uint32_t addr, T value)
{
    uint32_t segment = addr >> 16;
    if (segment == 0x1f80) {
        if (addr < 0x1f801000) {
            psxHref<T>(addr) = BFLIP(value);
            return;
        }
        Hardware::Write(addr, value);
        return;
    }
    uint8_t *base_addr = SegmentLUT[segment];
    uint32_t offset = addr & 0xffff;
    if (base_addr == 0) {
        wxMessageOutputDebug().Printf("PSX::Memory::Write : bad segment: %04x", segment);
        return;
    }
    *pointer_cast<T*>(base_addr + offset) = BFLIP(value);
}
