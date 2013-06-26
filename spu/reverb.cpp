#include "spu.h"
#include <cstring>
#include <wx/debug.h>


namespace {
    const int NSSIZE = 45;
}


namespace SPU {

////////////////////////////////////////////////////////////////////////
// Reverb
////////////////////////////////////////////////////////////////////////


REVERBInfo::REVERBInfo(SPU *spu) :
        spu_(*spu) {
    InitReverb();
}


REVERBInfo::~REVERBInfo() {
}


void REVERBInfo::InitReverb() {
    sReverbStart.reset(new int[NSSIZE*2]);                       // alloc reverb buffer
    ::memset(sReverbStart.get(), 0, NSSIZE*8);
    Reset();
}


void REVERBInfo::Reset() {
    sReverbEnd  = sReverbStart.get() + NSSIZE*2;
    sReverbPlay = sReverbStart.get();

    iReverbOff = -1;
    iReverbRepeat = 0;
    iReverbNum = 1;

    iStartAddr = 0;
    iCurrAddr = 0;
    VolLeft = 0;
    VolRight = 0;
    iLastRVBLeft = 0;
    iLastRVBRight = 0;
    iRVBLeft = 0;
    iRVBRight = 0;

    ::memset(Config, 0, sizeof(Config));

    if (spu_.m_iUseReverb == 2) {
        memset(sReverbStart.get(), 0, NSSIZE*2*4);
    }
}



void REVERBInfo::SetReverb(unsigned short value)
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
    default:
        iReverbOff = 32;  iReverbNum = 1; iReverbRepeat = 0;
        return;
    }
}


void REVERBInfo::StartReverb(ChannelInfo &ch)
{
    if (ch.hasReverb && (spu_.Sp0 & 0x80)) {
        if (spu_.m_iUseReverb == 2) {
            ch.bRVBActive = true;
            return;
        } else if (spu_.m_iUseReverb == 1 && iReverbOff > 0) {
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


void REVERBInfo::StoreReverb(const ChannelInfo &ch, int ns)
{
    if (spu_.m_iUseReverb == 0) return;
    if (spu_.m_iUseReverb == 2) {    // Neil's reverb
        const int iRxl = (ch.sval * ch.iLeftVolume) / 0x4000;
        const int iRxr = (ch.sval * ch.iRightVolume) / 0x4000;
        ns <<= 1;
        sReverbStart[ns+0] += iRxl;
        sReverbStart[ns+1] += iRxr;
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
            pN = sReverbStart.get() + (pN - sReverbEnd);
        }
        (*pN) += iRxl;
        pN++;
        (*pN) += iRxr;
        iRr += ch.iRVBRepeat;
        iRxl >>= 1; iRxr >>= 1;
    }
}


int REVERBInfo::ReverbGetBuffer(int ofs) const
{
    short *p = (short*)spu_.m_spuMem;
    ofs = (ofs*4) + iCurrAddr;
    while (ofs > 0x3ffff) {
        ofs = iStartAddr + (ofs - 0x40000);
    }
    while (ofs < iStartAddr) {
        ofs = 0x3ffff - (iStartAddr - ofs);
    }
    return (int)*(p+ofs);
}


void REVERBInfo::ReverbSetBuffer(int ofs, int val)
{
    short *p = (short*)spu_.m_spuMem;
    ofs = (ofs*4) + iCurrAddr;
    while (ofs > 0x3ffff) {
        ofs = iStartAddr + (ofs - 0x40000);
    }
    while (ofs < iStartAddr) {
        ofs = 0x3ffff - (iStartAddr - ofs);
    }
    if (val < -32768) val = -32768;
    if (val >  32767) val =  32767;
    *(p+ofs) = (short)val;
}


void REVERBInfo::ReverbSetBufferPlus1(int ofs, int val)
{
    short *p = (short*)spu_.m_spuMem;
    ofs = (ofs*4) + iCurrAddr + 1;
    while (ofs > 0x3ffff) {
        ofs = iStartAddr + (ofs - 0x40000);
    }
    while (ofs < iStartAddr) {
        ofs = 0x3ffff - (iStartAddr - ofs);
    }
    if (val < -32768) val = -32768;
    if (val >  32767) val =  32767;
    *(p+ofs) = (short)val;
}


int REVERBInfo::MixReverbLeft(int ns)
{
    if (spu_.m_iUseReverb == 0) return 0;

    if (spu_.m_iUseReverb == 2) {
        static int iCnt = 0;
        if (iStartAddr == 0) {
            iLastRVBLeft = iLastRVBRight = 0;
            iRVBLeft = iRVBRight = 0;
            return 0;
        }

        iCnt++;

        if (iCnt & 1) {
            if (spu_.Sp0 & 0x80) {
                int ACC0, ACC1, FB_A0, FB_A1, FB_B0, FB_B1;

                const int INPUT_SAMPLE_L = sReverbStart[ns<<1];
                const int INPUT_SAMPLE_R = sReverbStart[(ns<<1)+1];

                const int IIR_SRC_A0 = ReverbGetBuffer(this->IIR_SRC_A0);
                const int IIR_SRC_A1 = ReverbGetBuffer(this->IIR_SRC_A1);
                const int IIR_SRC_B0 = ReverbGetBuffer(this->IIR_SRC_B0);
                const int IIR_SRC_B1 = ReverbGetBuffer(this->IIR_SRC_B1);
                const int IIR_COEF = this->IIR_COEF;
                const int IN_COEF_L = this->IN_COEF_L;
                const int IN_COEF_R = this->IN_COEF_R;

                const int IIR_INPUT_A0 = (IIR_SRC_A0*IIR_COEF)/32768L + (INPUT_SAMPLE_L*IN_COEF_L)/32768L;
                const int IIR_INPUT_A1 = (IIR_SRC_A1*IIR_COEF)/32768L + (INPUT_SAMPLE_R*IN_COEF_R)/32768L;
                const int IIR_INPUT_B0 = (IIR_SRC_B0*IIR_COEF)/32768L + (INPUT_SAMPLE_L*IN_COEF_L)/32768L;
                const int IIR_INPUT_B1 = (IIR_SRC_B1*IIR_COEF)/32768L + (INPUT_SAMPLE_R*IN_COEF_R)/32768L;

                const int IIR_DEST_A0 = ReverbGetBuffer(this->IIR_DEST_A0);
                const int IIR_DEST_A1 = ReverbGetBuffer(this->IIR_DEST_A1);
                const int IIR_DEST_B0 = ReverbGetBuffer(this->IIR_DEST_B0);
                const int IIR_DEST_B1 = ReverbGetBuffer(this->IIR_DEST_B1);
                const int IIR_ALPHA = this->IIR_ALPHA;

                const int IIR_A0 = (IIR_INPUT_A0*IIR_ALPHA)/32768L + (IIR_DEST_A0*(32768L-IIR_ALPHA))/32768L;
                const int IIR_A1 = (IIR_INPUT_A1*IIR_ALPHA)/32768L + (IIR_DEST_A1*(32768L-IIR_ALPHA))/32768L;
                const int IIR_B0 = (IIR_INPUT_B0*IIR_ALPHA)/32768L + (IIR_DEST_B0*(32768L-IIR_ALPHA))/32768L;
                const int IIR_B1 = (IIR_INPUT_B1*IIR_ALPHA)/32768L + (IIR_DEST_B1*(32768L-IIR_ALPHA))/32768L;

                ReverbSetBufferPlus1(IIR_DEST_A0, IIR_A0);
                ReverbSetBufferPlus1(IIR_DEST_A1, IIR_A1);
                ReverbSetBufferPlus1(IIR_DEST_B0, IIR_B0);
                ReverbSetBufferPlus1(IIR_DEST_B1, IIR_B1);

                const int ACC_COEF_A = this->ACC_COEF_A;
                const int ACC_COEF_B = this->ACC_COEF_B;
                const int ACC_COEF_C = this->ACC_COEF_C;
                const int ACC_COEF_D = this->ACC_COEF_D;

                ACC0 = (ReverbGetBuffer(this->ACC_SRC_A0)*ACC_COEF_A)/32768L +
                       (ReverbGetBuffer(this->ACC_SRC_B0)*ACC_COEF_B)/32768L +
                       (ReverbGetBuffer(this->ACC_SRC_C0)*ACC_COEF_C)/32768L +
                       (ReverbGetBuffer(this->ACC_SRC_D0)*ACC_COEF_D)/32768L;
                ACC1 = (ReverbGetBuffer(this->ACC_SRC_A1)*ACC_COEF_A)/32768L +
                       (ReverbGetBuffer(this->ACC_SRC_B1)*ACC_COEF_B)/32768L +
                       (ReverbGetBuffer(this->ACC_SRC_C1)*ACC_COEF_C)/32768L +
                       (ReverbGetBuffer(this->ACC_SRC_D1)*ACC_COEF_D)/32768L;

                FB_A0 = ReverbGetBuffer(this->MIX_DEST_A0 - this->FB_SRC_A);
                FB_A1 = ReverbGetBuffer(this->MIX_DEST_A1 - this->FB_SRC_A);
                FB_B0 = ReverbGetBuffer(this->MIX_DEST_B0 - this->FB_SRC_B);
                FB_B1 = ReverbGetBuffer(this->MIX_DEST_B1 - this->FB_SRC_B);

                const int FB_ALPHA = this->FB_ALPHA;
                const int FB_X = this->FB_X;
                const int MIX_DEST_A0 = ACC0-(FB_A0*FB_ALPHA)/32768L;
                const int MIX_DEST_A1 = ACC1-(FB_A1*FB_ALPHA)/32768L;
                const int MIX_DEST_B0 = (FB_ALPHA*ACC0)/32768L - (FB_A0*(FB_ALPHA^0xffff8000))/32768L - (FB_B0*FB_X)/32768L;
                const int MIX_DEST_B1 = (FB_ALPHA*ACC1)/32768L - (FB_A1*(FB_ALPHA^0xffff8000))/32768L - (FB_B1*FB_X)/32768L;
                ReverbSetBuffer(this->MIX_DEST_A0, MIX_DEST_A0);
                ReverbSetBuffer(this->MIX_DEST_A1, MIX_DEST_A1);
                ReverbSetBuffer(this->MIX_DEST_B0, MIX_DEST_B0);
                ReverbSetBuffer(this->MIX_DEST_B1, MIX_DEST_B1);

                iLastRVBLeft = iRVBLeft;
                iLastRVBRight = iRVBRight;

                int rvbLeft = (MIX_DEST_A0 + MIX_DEST_B0) / 3;
                int rvbRight = (MIX_DEST_A1 + MIX_DEST_B1) / 3;
                iRVBLeft = (rvbLeft * VolLeft) / 0x4000;
                iRVBRight = (rvbRight * VolRight) / 0x4000;

                iCurrAddr++;
                if (iCurrAddr > 0x3ffff) {
                    iCurrAddr = iStartAddr;
                }
                return (iLastRVBLeft + iRVBLeft) / 2;
            }
            // reverb off
            iLastRVBLeft = iLastRVBRight = 0;
            iRVBLeft = iRVBRight = 0;
            iCurrAddr++;
            if (iCurrAddr > 0x3ffff) {
                iCurrAddr = iStartAddr;
            }
        }
        return iLastRVBLeft;
    }
    const int iRV = *sReverbPlay;
    *sReverbPlay++ = 0;
    if (sReverbPlay >= sReverbEnd) {
        sReverbPlay = sReverbStart.get();
    }
    return iRV;
}


int REVERBInfo::MixReverbRight()
{
    if (spu_.m_iUseReverb == 0) return 0;
    if (spu_.m_iUseReverb == 2) {
        int i = (iLastRVBRight + iRVBRight) / 2;
        iLastRVBRight = iRVBRight;
        return i;
    }
    const int iRV = *sReverbPlay;
    *sReverbPlay++ = 0;
    if (sReverbPlay >= sReverbEnd) {
        sReverbPlay = sReverbStart.get();
    }
    return iRV;
}

// TODO: should mix ReverbLeft and ReverbRight


}   // namespace SPU
