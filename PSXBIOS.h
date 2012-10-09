#pragma once

namespace PSXBIOS
{

void Init();
void Shutdown();
void Interrupt();
void Exception();

extern void (*biosA0[256])();
extern void (*biosB0[256])();
extern void (*biosC0[256])();

}   // namespace PSXBIOS
