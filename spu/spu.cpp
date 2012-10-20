#include "spu.h"
#include <wx/msgout.h>
#include <cstring>


namespace SPU {


SPU::SPU(): Memory((uint8_t*)m_spuMem)
{
    Init();
}


void SPU::Init()
{
    memset(channels, 0, sizeof(channels));
//    memset()
    InitADSR();
}


SPU SPU::Spu;

}   // namespace SPU

SPU::SPU& Spu = SPU::SPU::Spu;
