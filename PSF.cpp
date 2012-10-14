#include "PSF.h"
#include "psx/psx.h"
#include <wx/msgout.h>


void PSF::Init()
{
}

void PSF::Reset()
{
    PSX::R3000a.Init();
//    PSX::Hardware::Reset();
    PSX::RootCounter::Init();
}


bool PSF::Play()
{
    wxMessageOutputDebug().Printf("initial PC = 0x%08x", m_header.pc0);
    Reset();
    PSX::BIOS::Init();
    m_thread = PSX::Interpreter_.Execute();

    return true;
}


bool PSF::Stop()
{
    PSX::Interpreter_.Shutdown();
    m_thread = 0;
    PSX::Memory::Reset();
    return true;
}
