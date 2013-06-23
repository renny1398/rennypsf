#include "spu.h"
#include <cstring>
#include <wx/debug.h>


namespace SPU {

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
        if (sReverbStart == NULL) {

        }
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
    if (ch.hasReverb && (Sp0 & 0x80)) {
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


void SPU::StoreReverb(const ChannelInfo &ch, int ns)
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
    ofs = (ofs*4) + Reverb.iCurrAddr;
    while (ofs > 0x3ffff) {
        ofs = Reverb.iStartAddr + (ofs - 0x40000);
    }
    while (ofs < Reverb.iStartAddr) {
        ofs = 0x3ffff - (Reverb.iStartAddr - ofs);
    }
    return (int)*(p+ofs);
}


void SPU::ReverbSetBuffer(int ofs, int val)
{
    short *p = (short*)m_spuMem;
    ofs = (ofs*4) + Reverb.iCurrAddr;
    while (ofs > 0x3ffff) {
        ofs = Reverb.iStartAddr + (ofs - 0x40000);
    }
    while (ofs < Reverb.iStartAddr) {
        ofs = 0x3ffff - (Reverb.iStartAddr - ofs);
    }
    if (val < -32768) val = -32768;
    if (val >  32767) val =  32767;
    *(p+ofs) = (short)val;
}


void SPU::ReverbSetBufferPlus1(int ofs, int val)
{
    short *p = (short*)m_spuMem;
    ofs = (ofs*4) + Reverb.iCurrAddr + 1;
    while (ofs > 0x3ffff) {
        ofs = Reverb.iStartAddr + (ofs - 0x40000);
    }
    while (ofs < Reverb.iStartAddr) {
        ofs = 0x3ffff - (Reverb.iStartAddr - ofs);
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
        if (Reverb.iStartAddr == 0) {
            Reverb.iLastRVBLeft = Reverb.iLastRVBRight = 0;
            Reverb.iRVBLeft = Reverb.iRVBRight = 0;
            return 0;
        }

        iCnt++;

        if (iCnt & 1) {
            if (Sp0 & 0x80) {
                int ACC0, ACC1, FB_A0, FB_A1, FB_B0, FB_B1;

                const int INPUT_SAMPLE_L = *(sReverbStart + (ns << 1));
                const int INPUT_SAMPLE_R = *(sReverbStart + (ns << 1) + 1);

                const int IIR_SRC_A0 = ReverbGetBuffer(Reverb.IIR_SRC_A0);
                const int IIR_SRC_A1 = ReverbGetBuffer(Reverb.IIR_SRC_A1);
                const int IIR_SRC_B0 = ReverbGetBuffer(Reverb.IIR_SRC_B0);
                const int IIR_SRC_B1 = ReverbGetBuffer(Reverb.IIR_SRC_B1);
                const int IIR_COEF = Reverb.IIR_COEF;
                const int IN_COEF_L = Reverb.IN_COEF_L;
                const int IN_COEF_R = Reverb.IN_COEF_R;

                const int IIR_INPUT_A0 = (IIR_SRC_A0*IIR_COEF)/32768 + (INPUT_SAMPLE_L*IN_COEF_L)/32768;
                const int IIR_INPUT_A1 = (IIR_SRC_A1*IIR_COEF)/32768 + (INPUT_SAMPLE_R*IN_COEF_R)/32768;
                const int IIR_INPUT_B0 = (IIR_SRC_B0*IIR_COEF)/32768 + (INPUT_SAMPLE_L*IN_COEF_L)/32768;
                const int IIR_INPUT_B1 = (IIR_SRC_B1*IIR_COEF)/32768 + (INPUT_SAMPLE_R*IN_COEF_R)/32768;

                const int IIR_DEST_A0 = ReverbGetBuffer(Reverb.IIR_DEST_A0);
                const int IIR_DEST_A1 = ReverbGetBuffer(Reverb.IIR_DEST_A1);
                const int IIR_DEST_B0 = ReverbGetBuffer(Reverb.IIR_DEST_B0);
                const int IIR_DEST_B1 = ReverbGetBuffer(Reverb.IIR_DEST_B1);
                const int IIR_ALPHA = Reverb.IIR_ALPHA;

                const int IIR_A0 = (IIR_INPUT_A0*IIR_ALPHA)/32768 + (IIR_DEST_A0*(32768-IIR_ALPHA))/32768;
                const int IIR_A1 = (IIR_INPUT_A1*IIR_ALPHA)/32768 + (IIR_DEST_A1*(32768-IIR_ALPHA))/32768;
                const int IIR_B0 = (IIR_INPUT_B0*IIR_ALPHA)/32768 + (IIR_DEST_B0*(32768-IIR_ALPHA))/32768;
                const int IIR_B1 = (IIR_INPUT_B1*IIR_ALPHA)/32768 + (IIR_DEST_B1*(32768-IIR_ALPHA))/32768;

                ReverbSetBufferPlus1(Reverb.IIR_DEST_A0, IIR_A0);
                ReverbSetBufferPlus1(Reverb.IIR_DEST_A1, IIR_A1);
                ReverbSetBufferPlus1(Reverb.IIR_DEST_B0, IIR_B0);
                ReverbSetBufferPlus1(Reverb.IIR_DEST_B1, IIR_B1);

                const int ACC_COEF_A = Reverb.ACC_COEF_A;
                const int ACC_COEF_B = Reverb.ACC_COEF_B;
                const int ACC_COEF_C = Reverb.ACC_COEF_C;
                const int ACC_COEF_D = Reverb.ACC_COEF_D;

                ACC0 = (ReverbGetBuffer(Reverb.ACC_SRC_A0)*ACC_COEF_A)/32768 +
                       (ReverbGetBuffer(Reverb.ACC_SRC_B0)*ACC_COEF_B)/32768 +
                       (ReverbGetBuffer(Reverb.ACC_SRC_C0)*ACC_COEF_C)/32768 +
                       (ReverbGetBuffer(Reverb.ACC_SRC_D0)*ACC_COEF_D)/32768;
                ACC1 = (ReverbGetBuffer(Reverb.ACC_SRC_A1)*ACC_COEF_A)/32768 +
                       (ReverbGetBuffer(Reverb.ACC_SRC_B1)*ACC_COEF_B)/32768 +
                       (ReverbGetBuffer(Reverb.ACC_SRC_C1)*ACC_COEF_C)/32768 +
                       (ReverbGetBuffer(Reverb.ACC_SRC_D1)*ACC_COEF_D)/32768;

                FB_A0 = ReverbGetBuffer(Reverb.MIX_DEST_A0 - Reverb.FB_SRC_A);
                FB_A1 = ReverbGetBuffer(Reverb.MIX_DEST_A1 - Reverb.FB_SRC_A);
                FB_B0 = ReverbGetBuffer(Reverb.MIX_DEST_B0 - Reverb.FB_SRC_B);
                FB_B1 = ReverbGetBuffer(Reverb.MIX_DEST_B1 - Reverb.FB_SRC_B);

                const int FB_ALPHA = Reverb.FB_ALPHA;
                const int FB_X = Reverb.FB_X;
                const int MIX_DEST_A0 = ACC0-(FB_A0*FB_ALPHA)/32768;
                const int MIX_DEST_A1 = ACC1-(FB_A1*FB_ALPHA)/32768;
                const int MIX_DEST_B0 = (FB_ALPHA*ACC0)/32768 - (FB_A0*(FB_ALPHA^0xffff8000))/32768 - (FB_B0*FB_X)/32768;
                const int MIX_DEST_B1 = (FB_ALPHA*ACC1)/32768 - (FB_A1*(FB_ALPHA^0xffff8000))/32768 - (FB_B1*FB_X)/32768;
                ReverbSetBuffer(Reverb.MIX_DEST_A0, MIX_DEST_A0);
                ReverbSetBuffer(Reverb.MIX_DEST_A1, MIX_DEST_A1);
                ReverbSetBuffer(Reverb.MIX_DEST_B0, MIX_DEST_B0);
                ReverbSetBuffer(Reverb.MIX_DEST_B1, MIX_DEST_B1);

                Reverb.iLastRVBLeft = Reverb.iRVBLeft;
                Reverb.iLastRVBRight = Reverb.iRVBRight;

                int rvbLeft = (MIX_DEST_A0 + MIX_DEST_B0) / 3;
                int rvbRight = (MIX_DEST_A1 + MIX_DEST_B1) / 3;
                Reverb.iRVBLeft = (rvbLeft * Reverb.VolLeft) / 0x4000;
                Reverb.iRVBRight = (rvbRight * Reverb.VolRight) / 0x4000;

                Reverb.iCurrAddr++;
                if (Reverb.iCurrAddr > 0x3ffff) {
                    Reverb.iCurrAddr = Reverb.iStartAddr;
                }
                return (Reverb.iLastRVBLeft + Reverb.iRVBLeft) / 2;
            }
            // reverb off
            Reverb.iLastRVBLeft = Reverb.iLastRVBRight = 0;
            Reverb.iRVBLeft = Reverb.iRVBRight = 0;
            Reverb.iCurrAddr++;
            if (Reverb.iCurrAddr > 0x3ffff) {
                Reverb.iCurrAddr = Reverb.iStartAddr;
            }
        }
        return Reverb.iLastRVBLeft;
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
        int i = (Reverb.iLastRVBRight + Reverb.iRVBRight) / 2;
        Reverb.iLastRVBRight = Reverb.iRVBRight;
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


}   // namespace SPU
