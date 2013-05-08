#include "spu.h"
#include <wx/msgout.h>
#include <cstring>

#include "../psx/hardware.h"
#include "../app.h"

namespace {

inline void CLIP(int32_t& x) {
    if (x > 32767) x = 32767;
    if (x < -32767) x = -32767;
}

static int32_t poo;

}   // namespace



namespace SPU {

wxDEFINE_SCOPED_PTR(AbstractInterpolation, InterpolationPtr)

ChannelInfo::ChannelInfo(): pInterpolation(new GaussianInterpolation(*this))
{
}


void ChannelInfo::StartSound()
{
    wxASSERT(bNew == true);

    ADSRX.Start();
    // StartREVERB();

    pCurr = pStart;

    s_1 = 0;
    s_2 = 0;
    iSBPos = 28;

    bNew = false;
    isStopped = false;
    isOn = true;

    pInterpolation->Start();

    // TODO: Change newChannelFlags
}


void ChannelInfo::VoiceChangeFrequency()
{
    wxASSERT(iActFreq != iUsedFreq);
    iUsedFreq = iActFreq;
    pInterpolation->SetSinc(iRawPitch);
}

// FModChangeFrequency



void ChannelInfo::ADPCM2LPCM()
{
    static const int xa_adpcm_table[5][2] = {
        {   0,   0 },
        {  60,   0 },
        { 115, -52 },
        {  98, -55 },
        { 122, -60 }
    };

    int prev1 = s_1;
    int prev2 = s_2;
    uint8_t* start = pCurr;
    int predict_nr = *start++;
    int shift_factor = predict_nr & 0xf;
    int flags = *start++;
    predict_nr >>= 4;

    int d, s, fa;

    for (int i = 0; i < 28; start++) {
        d = *start;

        s = (d & 0xf) << 12;
        if (s & 0x8000) s |= 0xffff0000;
        fa = (s >> shift_factor);
        fa += ((prev1 * xa_adpcm_table[predict_nr][0]) >> 6) + ((prev2 * xa_adpcm_table[predict_nr][1]) >> 6);
        prev2 = prev1; prev1 = fa;

        LPCM[i++] = fa;

        s = (d & 0xf0) << 8;
        if (s & 0x8000) s |= 0xffff0000;
        fa = (s >> shift_factor);
        fa += ((prev1 * xa_adpcm_table[predict_nr][0]) >> 6) + ((prev2 * xa_adpcm_table[predict_nr][1]) >> 6);
        prev2 = prev1; prev1 = fa;

        LPCM[i++] = fa;
    }
    wxASSERT(start == pCurr+16);

    if (Spu.Sp0 & 0x40) {
        if ( (start-16 < Spu.m_pSpuIrq && Spu.m_pSpuIrq <= start) || ((flags & 1) && (pLoop-16 < Spu.m_pSpuIrq && Spu.m_pSpuIrq <= pLoop)) ) {
            iIrqDone = 1;
            PSX::u32Href(0x1070) |= BFLIP32(0x200);
        }
    }

    if ( (flags & 4) && ignoresLoop == 0 ) {
        pLoop = start - 16;
    }

    if (flags & 1) {
        if (flags != 3 || pLoop == 0) { // DQ4
            // wxMessageOutputDebug().Printf(wxT("flags = %02x"), flags);
            start = (uint8_t*)0xffffffff;
        } else {
            wxASSERT( (flags & 4) == 0 );
            start = pLoop;
        }
    }

    pCurr = start;
    s_1 = prev1;
    s_2 = prev2;
}


void ChannelInfo::Update()
{
    if (bNew) StartSound();
    if (isOn == false) return;

    if (iActFreq != iUsedFreq) {
        VoiceChangeFrequency();
    }

    int fa;
    while (pInterpolation->spos >= 0x10000) {
        if (iSBPos == 28) {
            if (pCurr == (uint8_t*)0xffffffff) {
                isOn = false;
                ADSRX.lVolume = 0;
                ADSRX.EnvelopeVol = 0;
                return;
            }
            iSBPos = 0;
            ADPCM2LPCM();
        }
        fa = LPCM[iSBPos++];

        pInterpolation->StoreValue(fa);
        pInterpolation->spos -= 0x10000;
    }

    if (bNoise) {
        // TODO: Noise
        fa = 0;
    } else {
        fa = pInterpolation->GetValue();
    }

    sval = (MixADSR() * fa) / 1023;

    if (bFMod == 2) {
        // TODO: FM
    } else {
        if (iMute) sval = 0;
        else {
            // Spu.m_SSumL[]
            short left = (sval * iLeftVolume) / 0x4000;
            short right = (sval * iRightVolume) / 0x4000;
            //*(Spu.m_pS+0) += left;
            //*(Spu.m_pS+1) += right;
            int leftEnvelope = ADSRX.lVolume * iLeftVolume;
            int rightEnvelope = ADSRX.lVolume * iRightVolume;
            wxGetApp().GetSoundManager()->SetEnvelopeVolume(ch, leftEnvelope, rightEnvelope);
            wxGetApp().GetSoundManager()->WriteStereo(ch, left, right);
        }
    }
    pInterpolation->spos += pInterpolation->GetSinc();
    // wxMessageOutputDebug().Printf("spos = 0x%08x", pInterpolation->spos);
}


void SPU::SetupStreams()
{
    m_pSpuBuffer = new uint8_t[32768];

    for (int i = 0; i < 24; i++) {
        Channels[i].ADSRX.SustainLevel = 0xf << 27;
        Channels[i].iMute = 0;
        Channels[i].iIrqDone = 0;
        Channels[i].pLoop = Memory;
        Channels[i].pStart = Memory;
        Channels[i].pCurr = Memory;
    }
}

void SPU::RemoveStreams()
{
    delete [] m_pSpuBuffer;
    m_pSpuBuffer = 0;
}


SPU::SPU(): Memory((uint8_t*)m_spuMem)
{
    Init();
}


void SPU::Init()
{
    m_iXAPitch = 1;
    m_iUseTimer = 2;
    m_iDebugMode = 0;
    m_iRecordMode = 0;
    m_iUseReverb = 2;
    useInterpolation = GAUSS_INTERPOLATION;
    m_iDisStereo = 0;
    m_iUseDBufIrq = 0;


    //::memset(&Channels, 0, sizeof(Channels));
    ::memset(&Reverb, 0, sizeof(Reverb));
    ChannelInfo::InitADSR();

    nPCM_ = 0;

    wxMessageOutputDebug().Printf(wxT("Initialized SPU."));
}

void SPU::Open()
{
    m_iUseXA = 1;
    m_iVolume = 3;
    iReverbOff = -1;
    // spuIrq = 0;
    Addr = 0xffffffff;
    m_bEndThread = 0;
    m_bThreadEnded = 0;
    m_pMixIrq = 0;

    // pSpuIrq = 0;
    m_iSPUIRQWait = 1;

    SetupStreams();

    poo = 0;
    m_pS = (short*)m_pSpuBuffer;

    nPCM_ = 0;

    wxMessageOutputDebug().Printf(wxT("Reset SPU."));
}

void SPU::Close()
{
    RemoveStreams();
}

void SPU::Async(uint32_t cycles)
{
    int32_t do_samples;

    // 384 = PSXCLK / (44100*2)
    poo += cycles;
    do_samples = poo / 384;
    if (do_samples == 0) return;
    poo -= do_samples * 384;

    while (do_samples) {
        do_samples--;
        //*(m_pS+0) = 0;
        //*(m_pS+1) = 0;
        for (int ch = 0; ch < 24; ch++) {
            Channels[ch].Update();
        }
        wxGetApp().GetSoundManager()->Flush();
        //m_pS += 2;
        //if (((unsigned char*)m_pS - m_pSpuBuffer) >= NSSIZE*2) {
            // wxGetApp().GetSoundManager().WriteBuffer(m_pSpuBuffer, NSSIZE*4);
        //    m_pS = (short*)m_pSpuBuffer;
        //}
    }
}


SPU SPU::Spu;

}   // namespace SPU

SPU::SPU& Spu = SPU::SPU::Spu;
