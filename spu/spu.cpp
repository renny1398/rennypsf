#include "spu.h"
#include <wx/msgout.h>
#include <cstring>


namespace SPU {


SPU::SPU()
{
    Init();
}


////////////////////////////////////////////////////////////////////////
// ADSR
////////////////////////////////////////////////////////////////////////


void SPU::InitADSR()
{
    // initialize RateTable
    // RateTable + 32 = { 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, ...}
    memset(m_RateTable, 0, sizeof(m_RateTable));
    unsigned long r = 3, rs = 1, rd = 0;
    for (int i = 32; i < 160; i++) {
        if (r < 0x3fffffff) {
            r += rs;
            if (++rd == 5) {
                rd = 1;
                rs *= 2;
            }
        }
        if (r > 0x3fffffff) {
            r = 0x3fffffff;
        }
        m_RateTable[i] = r;
    }
}


void SPU::StartADSR(ChannelInfo &channel)
{
    channel.ADSRX.lVolume = 1;
    channel.ADSRX.State = 0;
    channel.ADSRX.EnvelopeVol = 0;
}


static const unsigned long TableDisp[] = {
    -0x18+0+32, -0x18+4+32, -0x18+6+32, -0x18+8+32,
    -0x18+9+32, -0x18+10+32, -0x18+11+32, -0x18+12+32,
    -0x1b+0+32, -0x1b+4+32, -0x1b+6+32, -0x1b+8+32,
    -0x1b+9+32, -0x1b+10+32, -0x1b+11+32, -0x1b+12+32
};

int SPU::MixADSR(ChannelInfo &ch)
{
    unsigned long disp;
    long envVol = ch.ADSRX.EnvelopeVol;

    if (ch.isStopped) {
        // do Release
        disp = (ch.ADSRX.ReleaseModeExp) ? TableDisp[(envVol >> 28) & 0x7] : -0x0c + 32;
        envVol -= m_RateTable[ch.ADSRX.ReleaseRate + disp];
        if (envVol < 0) {
            envVol = 0;
            ch.isOn = false;
        }
        ch.ADSRX.EnvelopeVol = envVol;
        ch.ADSRX.lVolume = (envVol >>= 21);
        return envVol;
    }

    switch (ch.ADSRX.State) {
    case 0: // Attack
        disp = -0x10 + 32;
        if (ch.ADSRX.AttackModeExp && envVol >= 0x60000000) {
            disp = -0x18 + 32;
        }
        envVol += m_RateTable[ch.ADSRX.AttackRate + disp];
        if (static_cast<unsigned long>(envVol) > 0x7fffffff) {
            envVol = 0x7fffffff;
            ch.ADSRX.State = 1;    // move to Decay phase
        }
        ch.ADSRX.EnvelopeVol = envVol;
        ch.ADSRX.lVolume = (envVol >>= 21);
        return envVol;

    case 1: // Decay
        disp = TableDisp[(envVol >> 28) & 0x7];
        envVol -= m_RateTable[ch.ADSRX.DecayRate + disp];
        if (envVol < 0) envVol = 0;
        if (envVol <= ch.ADSRX.SustainLevel) {
            ch.ADSRX.State = 2; // mov to Sustain phase
        }
        ch.ADSRX.EnvelopeVol = envVol;
        ch.ADSRX.lVolume = (envVol >>= 21);
        return envVol;

    case 2: // Sustain
        if (ch.ADSRX.SustainIncrease) {
            disp = -0x10 + 32;
            if (ch.ADSRX.SustainModeExp) {
                if (envVol >= 0x60000000) {
                    disp = -0x18 + 32;
                }
            }
            envVol += m_RateTable[ch.ADSRX.SustainRate + disp];
            if (static_cast<unsigned long>(envVol) > 0x7fffffff) {
                envVol = 0x7fffffff;
            }
        } else {    // SustainFlat or SustainDecrease
            disp = (ch.ADSRX.SustainModeExp) ? TableDisp[((envVol >> 28) & 0x7) + 8] : -0x0f + 32;
            envVol -= m_RateTable[ch.ADSRX.SustainRate + disp];
            if (envVol < 0) {
                envVol = 0;
            }
        }
        ch.ADSRX.EnvelopeVol = envVol;
        ch.ADSRX.lVolume = (envVol >>= 21);
        return envVol;
    }
    wxMessageOutputDebug().Printf("WARNING: SPUChannel state is wrong.");
    return 0;
}



////////////////////////////////////////////////////////////////////////
// Reverb
////////////////////////////////////////////////////////////////////////


void SPU::InitReverb()
{
    sReverbPlay = 0;
    sReverbEnd = 0;
    sReverbStart = 0;
    iReverbOff = -1;
    iReverbRepeat = 0;
    iReverbNum = 1;

    if (m_iUseReverb == 2) {
        memset(sReverbStart, 0, NSSIZE*2*4);
    }
}


void SPU::SetReverb(unsigned short value)
{
    switch (value) {
    case 0x0000:    // Reverb off
        iReverbOff = -1;
        return;
    case 0x007d:    // seems to be Room
        iReverbOff = 32;  iReverbNum = 2; iReverbRepeat = 128;
        return;
    case 0x0033:    // Studio small
        iReverbOff = 32;  iReverbNum = 2; iReverbRepeat = 64;
        return;
    case 0x00b1:    // seems to be Studio medium
        iReverbOff = 48;  iReverbNum = 2; iReverbRepeat = 96;
        return;
    case 0x00e3:    // seems to be Studio large
        iReverbOff = 64;  iReverbNum = 2; iReverbRepeat = 128;
        return;
    case 0x01a5:    // seems to be Hall
        iReverbOff = 128; iReverbNum = 4; iReverbRepeat = 32;
        return;
    case 0x033d:    // Space echo
        iReverbOff = 256; iReverbNum = 4; iReverbRepeat = 64;
        return;
    case 0x0001:    // Echo or Delay
        iReverbOff = 184; iReverbNum = 3; iReverbRepeat = 128;
        return;
    case 0x0017:    // Half echo
        iReverbOff = 128; iReverbNum = 2; iReverbRepeat = 128;
        return;
    }
    iReverbOff = 32;  iReverbNum = 1; iReverbRepeat = 0;
}


void SPU::StartReverb(ChannelInfo &ch)
{
    if (ch.hasReverb && (m_spuCtrl & 0x80)) {
        if (m_iUseReverb == 2) {
            ch.bRVBActive = true;
            return;
        } else if (m_iUseReverb == 1 && iReverbOff > 0) {
            ch.bRVBActive = true;
            ch.iRVBOffset = iReverbOff * 45;
            ch.iRVBRepeat = iReverbRepeat * 45;
            ch.iRVBNum = iReverbNum;
            return;
        }
        return;
    }
    ch.bRVBActive = false;
}


void SPU::StoreReverb(ChannelInfo &ch, int ns)
{
    if (m_iUseReverb == 0) return;
    if (m_iUseReverb == 2) {    // Neil's reverb
        const int iRxl = (ch.sval * ch.iLeftVolume) / 0x4000;
        const int iRxr = (ch.sval * ch.iRightVolume) / 0x4000;
        ns <<= 1;
        *(sReverbStart+ns+0) += iRxl;
        *(sReverbStart+ns+1) += iRxr;
        return;
    }
    // Pete's easy fake reverb
    int *pN;
    int iRn, iRr = 0;

    int iRxl = (ch.sval * ch.iLeftVolume) / 0x8000;
    int iRxr = (ch.sval * ch.iRightVolume) / 0x8000;

    for (iRn = 1; iRn <= ch.iRVBNum; iRn++) {
        pN = sReverbPlay + ((ch.iRVBOffset + iRr + ns) << 1);
        if (pN >= sReverbEnd) {
            pN = sReverbStart + (pN - sReverbEnd);
        }
        (*pN) += iRxl;
        pN++;
        (*pN) += iRxr;
        iRr += ch.iRVBRepeat;
        iRxl >>= 1; iRxr >>= 1;
    }
}


int SPU::ReverbGetBuffer(int ofs) const
{
    short *p = (short*)m_spuMem;
    ofs = (ofs*4) + m_reverbInfo.iCurrAddr;
    while (ofs > 0x3ffff) {
        ofs = m_reverbInfo.iStartAddr + (ofs - 0x40000);
    }
    while (ofs < m_reverbInfo.iStartAddr) {
        ofs = 0x3ffff - (m_reverbInfo.iStartAddr - ofs);
    }
    return (int)*(p+ofs);
}


void SPU::ReverbSetBuffer(int ofs, int val)
{
    short *p = (short*)m_spuMem;
    ofs = (ofs*4) + m_reverbInfo.iCurrAddr;
    while (ofs > 0x3ffff) {
        ofs = m_reverbInfo.iStartAddr + (ofs - 0x40000);
    }
    while (ofs < m_reverbInfo.iStartAddr) {
        ofs = 0x3ffff - (m_reverbInfo.iStartAddr - ofs);
    }
    if (val < -32768) val = -32768;
    if (val >  32767) val =  32767;
    *(p+ofs) = (short)val;
}


void SPU::ReverbSetBufferPlus1(int ofs, int val)
{
    short *p = (short*)m_spuMem;
    ofs = (ofs*4) + m_reverbInfo.iCurrAddr + 1;
    while (ofs > 0x3ffff) {
        ofs = m_reverbInfo.iStartAddr + (ofs - 0x40000);
    }
    while (ofs < m_reverbInfo.iStartAddr) {
        ofs = 0x3ffff - (m_reverbInfo.iStartAddr - ofs);
    }
    if (val < -32768) val = -32768;
    if (val >  32767) val =  32767;
    *(p+ofs) = (short)val;
}


int SPU::MixReverbLeft(int ns)
{
    if (m_iUseReverb == 0) return 0;

    if (m_iUseReverb == 2) {
        static int iCnt = 0;
        if (m_reverbInfo.iStartAddr == 0) {
            m_reverbInfo.iLastRVBLeft = m_reverbInfo.iLastRVBRight = 0;
            m_reverbInfo.iRVBLeft = m_reverbInfo.iRVBRight = 0;
            return 0;
        }

        iCnt++;

        if (iCnt & 1) {
            if (m_spuCtrl & 0x80) {
                int ACC0, ACC1, FB_A0, FB_A1, FB_B0, FB_B1;

                const int INPUT_SAMPLE_L = *(sReverbStart + (ns << 1));
                const int INPUT_SAMPLE_R = *(sReverbStart + (ns << 1) + 1);

                const int IIR_SRC_A0 = ReverbGetBuffer(m_reverbInfo.IIR_SRC_A0);
                const int IIR_SRC_A1 = ReverbGetBuffer(m_reverbInfo.IIR_SRC_A1);
                const int IIR_SRC_B0 = ReverbGetBuffer(m_reverbInfo.IIR_SRC_B0);
                const int IIR_SRC_B1 = ReverbGetBuffer(m_reverbInfo.IIR_SRC_B1);
                const int IIR_COEF = m_reverbInfo.IIR_COEF;
                const int IN_COEF_L = m_reverbInfo.IN_COEF_L;
                const int IN_COEF_R = m_reverbInfo.IN_COEF_R;

                const int IIR_INPUT_A0 = (IIR_SRC_A0*IIR_COEF)/32768 + (INPUT_SAMPLE_L*IN_COEF_L)/32768;
                const int IIR_INPUT_A1 = (IIR_SRC_A1*IIR_COEF)/32768 + (INPUT_SAMPLE_R*IN_COEF_R)/32768;
                const int IIR_INPUT_B0 = (IIR_SRC_B0*IIR_COEF)/32768 + (INPUT_SAMPLE_L*IN_COEF_L)/32768;
                const int IIR_INPUT_B1 = (IIR_SRC_B1*IIR_COEF)/32768 + (INPUT_SAMPLE_R*IN_COEF_R)/32768;

                const int IIR_DEST_A0 = ReverbGetBuffer(m_reverbInfo.IIR_DEST_A0);
                const int IIR_DEST_A1 = ReverbGetBuffer(m_reverbInfo.IIR_DEST_A1);
                const int IIR_DEST_B0 = ReverbGetBuffer(m_reverbInfo.IIR_DEST_B0);
                const int IIR_DEST_B1 = ReverbGetBuffer(m_reverbInfo.IIR_DEST_B1);
                const int IIR_ALPHA = m_reverbInfo.IIR_ALPHA;

                const int IIR_A0 = (IIR_INPUT_A0*IIR_ALPHA)/32768 + (IIR_DEST_A0*(32768-IIR_ALPHA))/32768;
                const int IIR_A1 = (IIR_INPUT_A1*IIR_ALPHA)/32768 + (IIR_DEST_A1*(32768-IIR_ALPHA))/32768;
                const int IIR_B0 = (IIR_INPUT_B0*IIR_ALPHA)/32768 + (IIR_DEST_B0*(32768-IIR_ALPHA))/32768;
                const int IIR_B1 = (IIR_INPUT_B1*IIR_ALPHA)/32768 + (IIR_DEST_B1*(32768-IIR_ALPHA))/32768;

                ReverbSetBufferPlus1(m_reverbInfo.IIR_DEST_A0, IIR_A0);
                ReverbSetBufferPlus1(m_reverbInfo.IIR_DEST_A1, IIR_A1);
                ReverbSetBufferPlus1(m_reverbInfo.IIR_DEST_B0, IIR_B0);
                ReverbSetBufferPlus1(m_reverbInfo.IIR_DEST_B1, IIR_B1);

                const int ACC_COEF_A = m_reverbInfo.ACC_COEF_A;
                const int ACC_COEF_B = m_reverbInfo.ACC_COEF_B;
                const int ACC_COEF_C = m_reverbInfo.ACC_COEF_C;
                const int ACC_COEF_D = m_reverbInfo.ACC_COEF_D;

                ACC0 = (ReverbGetBuffer(m_reverbInfo.ACC_SRC_A0)*ACC_COEF_A)/32768 +
                       (ReverbGetBuffer(m_reverbInfo.ACC_SRC_B0)*ACC_COEF_B)/32768 +
                       (ReverbGetBuffer(m_reverbInfo.ACC_SRC_C0)*ACC_COEF_C)/32768 +
                       (ReverbGetBuffer(m_reverbInfo.ACC_SRC_D0)*ACC_COEF_D)/32768;
                ACC1 = (ReverbGetBuffer(m_reverbInfo.ACC_SRC_A1)*ACC_COEF_A)/32768 +
                       (ReverbGetBuffer(m_reverbInfo.ACC_SRC_B1)*ACC_COEF_B)/32768 +
                       (ReverbGetBuffer(m_reverbInfo.ACC_SRC_C1)*ACC_COEF_C)/32768 +
                       (ReverbGetBuffer(m_reverbInfo.ACC_SRC_D1)*ACC_COEF_D)/32768;

                FB_A0 = ReverbGetBuffer(m_reverbInfo.MIX_DEST_A0 - m_reverbInfo.FB_SRC_A);
                FB_A1 = ReverbGetBuffer(m_reverbInfo.MIX_DEST_A1 - m_reverbInfo.FB_SRC_A);
                FB_B0 = ReverbGetBuffer(m_reverbInfo.MIX_DEST_B0 - m_reverbInfo.FB_SRC_B);
                FB_B1 = ReverbGetBuffer(m_reverbInfo.MIX_DEST_B1 - m_reverbInfo.FB_SRC_B);

                const int FB_ALPHA = m_reverbInfo.FB_ALPHA;
                const int FB_X = m_reverbInfo.FB_X;
                const int MIX_DEST_A0 = ACC0-(FB_A0*FB_ALPHA)/32768;
                const int MIX_DEST_A1 = ACC1-(FB_A1*FB_ALPHA)/32768;
                const int MIX_DEST_B0 = (FB_ALPHA*ACC0)/32768 - (FB_A0*(FB_ALPHA^0xffff8000))/32768 - (FB_B0*FB_X)/32768;
                const int MIX_DEST_B1 = (FB_ALPHA*ACC1)/32768 - (FB_A1*(FB_ALPHA^0xffff8000))/32768 - (FB_B1*FB_X)/32768;
                ReverbSetBuffer(m_reverbInfo.MIX_DEST_A0, MIX_DEST_A0);
                ReverbSetBuffer(m_reverbInfo.MIX_DEST_A1, MIX_DEST_A1);
                ReverbSetBuffer(m_reverbInfo.MIX_DEST_B0, MIX_DEST_B0);
                ReverbSetBuffer(m_reverbInfo.MIX_DEST_B1, MIX_DEST_B1);

                m_reverbInfo.iLastRVBLeft = m_reverbInfo.iRVBLeft;
                m_reverbInfo.iLastRVBRight = m_reverbInfo.iRVBRight;

                int rvbLeft = (MIX_DEST_A0 + MIX_DEST_B0) / 3;
                int rvbRight = (MIX_DEST_A1 + MIX_DEST_B1) / 3;
                m_reverbInfo.iRVBLeft = (rvbLeft * m_reverbInfo.VolLeft) / 0x4000;
                m_reverbInfo.iRVBRight = (rvbRight * m_reverbInfo.VolRight) / 0x4000;

                m_reverbInfo.iCurrAddr++;
                if (m_reverbInfo.iCurrAddr > 0x3ffff) {
                    m_reverbInfo.iCurrAddr = m_reverbInfo.iStartAddr;
                }
                return (m_reverbInfo.iLastRVBLeft + m_reverbInfo.iRVBLeft) / 2;
            }
            // reverb off
            m_reverbInfo.iLastRVBLeft = m_reverbInfo.iLastRVBRight = 0;
            m_reverbInfo.iRVBLeft = m_reverbInfo.iRVBRight = 0;
            m_reverbInfo.iCurrAddr++;
            if (m_reverbInfo.iCurrAddr > 0x3ffff) {
                m_reverbInfo.iCurrAddr = m_reverbInfo.iStartAddr;
            }
        }
        return m_reverbInfo.iLastRVBLeft;
    }
    const int iRV = *sReverbPlay;
    *sReverbPlay++ = 0;
    if (sReverbPlay >= sReverbEnd) {
        sReverbPlay = sReverbStart;
    }
    return iRV;
}


int SPU::MixReverbRight()
{
    if (m_iUseReverb == 0) return 0;
    if (m_iUseReverb == 2) {
        int i = (m_reverbInfo.iLastRVBRight + m_reverbInfo.iRVBRight) / 2;
        m_reverbInfo.iLastRVBRight = m_reverbInfo.iRVBRight;
        return i;
    }
    const int iRV = *sReverbPlay;
    *sReverbPlay++ = 0;
    if (sReverbPlay >= sReverbEnd) {
        sReverbPlay = sReverbStart;
    }
    return iRV;
}

// TODO: should mix ReverbLeft and ReverbRight




void SPU::Init()
{
    m_spuMemC = (unsigned char*)m_spuMem;
    memset(channels, 0, sizeof(channels));
//    memset()
    InitADSR();
}


SPU SPU::Spu;

}   // namespace SPU

SPU::SPU& Spu = SPU::SPU::Spu;
