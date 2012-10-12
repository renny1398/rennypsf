#pragma once
#include <stdint.h>


namespace PSX {
namespace Hardware {

void Reset();
uint8_t Read8(uint32_t addr);
uint16_t Read16(uint32_t addr);
uint32_t Read32(uint32_t addr);
void Write8(uint32_t addr, uint8_t value);
void Write16(uint32_t addr, uint16_t value);
void Write32(uint32_t addr, uint32_t value);

template<typename T> T Read(uint32_t addr);
template<> uint8_t Read(uint32_t addr);
template<> uint16_t Read(uint32_t addr);
template<> uint32_t Read(uint32_t addr);

template<typename T> void Write(uint32_t addr, T value);
template<> void Write(uint32_t addr, uint8_t value);
template<> void Write(uint32_t addr, uint16_t value);
template<> void Write(uint32_t addr, uint32_t value);


}   // namespace PSXHardware
}   // namespace PSX



#include "memory.h"

template<typename T>
inline T PSX::Hardware::Read(uint32_t addr) {
    return psxH<T>(addr);   // dummy
}
template<typename T>
inline void PSX::Hardware::Write(uint32_t addr, T value) {
    psxHref<T>(addr) = value;   // dummy
}

template<>
inline uint8_t PSX::Hardware::Read(uint32_t addr) {
    return Read8(addr);
}
template<>
inline uint16_t PSX::Hardware::Read(uint32_t addr) {
    return Read16(addr);
}
template<>
inline uint32_t PSX::Hardware::Read(uint32_t addr) {
    return Read32(addr);
}

template<>
inline void PSX::Hardware::Write(uint32_t addr, uint8_t value) {
    Write8(addr, value);
}
template<>
inline void PSX::Hardware::Write(uint32_t addr, uint16_t value) {
    Write16(addr, value);
}
template<>
inline void PSX::Hardware::Write(uint32_t addr, uint32_t value) {
    Write32(addr, value);
}
