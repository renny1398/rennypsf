#include "PSF.h"
#include "R3000A.h"
#include "PSXMemory.h"
#include "PSXBIOS.h"
#include "PSXCounters.h"
#include <wx/msgout.h>


void PSF::Init()
{
}

void PSF::Reset()
{
    R3000A::Reset();
}


bool PSF::Play()
{
    wxMessageOutputDebug().Printf("initial PC = 0x%08x", m_header.pc0);
    PSXBIOS::Init();
    PSXRootCounter::Init();
    m_thread = R3000A::Interpreter::Execute();

    return true;
}


bool PSF::Stop()
{
    R3000A::Interpreter::Shutdown();
    m_thread = 0;
    PSXMemory::Reset();
    return true;
}
