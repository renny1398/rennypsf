#include "spu/spu.h"
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

    dbpos_ = 0;

    ::memset(Config, 0, sizeof(Config));

    if (spu_.m_iUseReverb == 2) {
      memset(sReverbStart.get(), 0, NSSIZE*2*4);
    }

    wxMessageOutputDebug().Printf(wxT("Initialized SPU Reverb."));
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


  void NullReverb::StartReverb(ChannelInfo* ch) {
    ch->bRVBActive = false;
  }


  void PeteReverb::StartReverb(ChannelInfo *ch) {
    if (ch->hasReverb && (spu_.core(0).ctrl_ & 0x80)) {
      if (iReverbOff > 0) {
        ch->bRVBActive = true;
        ch->iRVBOffset = iReverbOff * 45;
        ch->iRVBRepeat = iReverbRepeat * 45;
        ch->iRVBNum = iReverbNum;
      }
    }
  }


  void NeilReverb::StartReverb(ChannelInfo *ch) {
    if (ch->hasReverb && (spu_.core(0).ctrl_ & 0x80)) {
      ch->bRVBActive = true;
    }
  }



  void NullReverb::StoreReverb(const ChannelInfo& /*ch*/) {
    return;
  }



  void PeteReverb::StoreReverb(const ChannelInfo &ch)
  {
    // Pete's easy fake reverb
    int *pN;
    int iRn, iRr = 0;

    int iRxl = (ch.sval * ch.iLeftVolume) / 0x8000;
    int iRxr = (ch.sval * ch.iRightVolume) / 0x8000;

    for (iRn = 1; iRn <= ch.iRVBNum; iRn++) {
      pN = sReverbPlay + ((ch.iRVBOffset + iRr + dbpos_) << 1);
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


  void NeilReverb::StoreReverb(const ChannelInfo &ch) {
    const int iRxl = (ch.sval * ch.iLeftVolume) / 0x4000;
    const int iRxr = (ch.sval * ch.iRightVolume) / 0x4000;
    sReverbStart[2*dbpos_+0] += iRxl;
    sReverbStart[2*dbpos_+1] += iRxr;
  }


  int REVERBInfo::GetBuffer(int ofs) const
  {
    short *p = (short*)spu_.mem16_;
    ofs = (ofs*4) + iCurrAddr;
    while (ofs > 0x3ffff) {
      ofs = iStartAddr + (ofs - 0x40000);
    }
    while (ofs < iStartAddr) {
      ofs = 0x3ffff - (iStartAddr - ofs);
    }
    return (int)*(p+ofs);
  }


  void REVERBInfo::SetBuffer(int ofs, int val)
  {
    short *p = (short*)spu_.mem16_;
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


  void REVERBInfo::SetBufferPlus1(int ofs, int val)
  {
    short *p = (short*)spu_.mem16_;
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



  void NullReverb::Mix() {
    return;
  }



  void PeteReverb::Mix() {
    const int iRV = *sReverbPlay;
    *sReverbPlay++ = 0;
    if (sReverbPlay >= sReverbEnd) {
      sReverbPlay = sReverbStart.get();
    }
    output_left_ = iRV;
    output_right_ = iRV;
  }



  void NeilReverb::Mix()
  {
    if (iStartAddr == 0) {
      iLastRVBLeft = iLastRVBRight = 0;
      iRVBLeft = iRVBRight = 0;
      output_left_ = output_right_ = 0;
      return;
    }

    const int INPUT_SAMPLE_L = sReverbStart[2*dbpos_+0];
    const int INPUT_SAMPLE_R = sReverbStart[2*dbpos_+1];
    dbpos_ = (dbpos_+1) % NSSIZE;

    if (dbpos_ & 1) {
      if (spu_.core(0).ctrl_ & 0x80) {
        int ACC0, ACC1, FB_A0, FB_A1, FB_B0, FB_B1;

        const int IIR_SRC_A0 = GetBuffer(this->IIR_SRC_A0_);
        const int IIR_SRC_A1 = GetBuffer(this->IIR_SRC_A1_);
        const int IIR_SRC_B0 = GetBuffer(this->IIR_SRC_B0_);
        const int IIR_SRC_B1 = GetBuffer(this->IIR_SRC_B1_);
        const int IIR_COEF = this->IIR_COEF_;
        const int IN_COEF_L = this->IN_COEF_L_;
        const int IN_COEF_R = this->IN_COEF_R_;

        const int IIR_INPUT_A0 = (IIR_SRC_A0*IIR_COEF)/32768L + (INPUT_SAMPLE_L*IN_COEF_L)/32768L;
        const int IIR_INPUT_A1 = (IIR_SRC_A1*IIR_COEF)/32768L + (INPUT_SAMPLE_R*IN_COEF_R)/32768L;
        const int IIR_INPUT_B0 = (IIR_SRC_B0*IIR_COEF)/32768L + (INPUT_SAMPLE_L*IN_COEF_L)/32768L;
        const int IIR_INPUT_B1 = (IIR_SRC_B1*IIR_COEF)/32768L + (INPUT_SAMPLE_R*IN_COEF_R)/32768L;

        const int IIR_DEST_A0 = GetBuffer(this->IIR_DEST_A0_);
        const int IIR_DEST_A1 = GetBuffer(this->IIR_DEST_A1_);
        const int IIR_DEST_B0 = GetBuffer(this->IIR_DEST_B0_);
        const int IIR_DEST_B1 = GetBuffer(this->IIR_DEST_B1_);
        const int IIR_ALPHA = this->IIR_ALPHA_;

        const int IIR_A0 = (IIR_INPUT_A0*IIR_ALPHA)/32768L + (IIR_DEST_A0*(32768L-IIR_ALPHA))/32768L;
        const int IIR_A1 = (IIR_INPUT_A1*IIR_ALPHA)/32768L + (IIR_DEST_A1*(32768L-IIR_ALPHA))/32768L;
        const int IIR_B0 = (IIR_INPUT_B0*IIR_ALPHA)/32768L + (IIR_DEST_B0*(32768L-IIR_ALPHA))/32768L;
        const int IIR_B1 = (IIR_INPUT_B1*IIR_ALPHA)/32768L + (IIR_DEST_B1*(32768L-IIR_ALPHA))/32768L;

        SetBufferPlus1(this->IIR_DEST_A0_, IIR_A0);
        SetBufferPlus1(this->IIR_DEST_A1_, IIR_A1);
        SetBufferPlus1(this->IIR_DEST_B0_, IIR_B0);
        SetBufferPlus1(this->IIR_DEST_B1_, IIR_B1);

        const int ACC_COEF_A = this->ACC_COEF_A_;
        const int ACC_COEF_B = this->ACC_COEF_B_;
        const int ACC_COEF_C = this->ACC_COEF_C_;
        const int ACC_COEF_D = this->ACC_COEF_D_;


        const int ACC_SRC_A0 = GetBuffer(this->ACC_SRC_A0_);
        const int ACC_SRC_B0 = GetBuffer(this->ACC_SRC_B0_);
        const int ACC_SRC_C0 = GetBuffer(this->ACC_SRC_C0_);
        const int ACC_SRC_D0 = GetBuffer(this->ACC_SRC_D0_);
        const int ACC_SRC_A1 = GetBuffer(this->ACC_SRC_A1_);
        const int ACC_SRC_B1 = GetBuffer(this->ACC_SRC_B1_);
        const int ACC_SRC_C1 = GetBuffer(this->ACC_SRC_C1_);
        const int ACC_SRC_D1 = GetBuffer(this->ACC_SRC_D1_);

        ACC0 = (ACC_SRC_A0*ACC_COEF_A)/32768L +
               (ACC_SRC_B0*ACC_COEF_B)/32768L +
               (ACC_SRC_C0*ACC_COEF_C)/32768L +
               (ACC_SRC_D0*ACC_COEF_D)/32768L;
        ACC1 = (ACC_SRC_A1*ACC_COEF_A)/32768L +
               (ACC_SRC_B1*ACC_COEF_B)/32768L +
               (ACC_SRC_C1*ACC_COEF_C)/32768L +
               (ACC_SRC_D1*ACC_COEF_D)/32768L;


        FB_A0 = GetBuffer(this->MIX_DEST_A0_ - this->FB_SRC_A_);
        FB_A1 = GetBuffer(this->MIX_DEST_A1_ - this->FB_SRC_A_);
        FB_B0 = GetBuffer(this->MIX_DEST_B0_ - this->FB_SRC_B_);
        FB_B1 = GetBuffer(this->MIX_DEST_B1_ - this->FB_SRC_B_);

        const int FB_ALPHA = this->FB_ALPHA_;
        const int FB_X = this->FB_X_;
        const int MIX_DEST_A0 = ACC0-(FB_A0*FB_ALPHA)/32768L;
        const int MIX_DEST_A1 = ACC1-(FB_A1*FB_ALPHA)/32768L;
        const int MIX_DEST_B0 = (FB_ALPHA*ACC0)/32768L - (FB_A0*(FB_ALPHA^0xffff8000))/32768L - (FB_B0*FB_X)/32768L;
        const int MIX_DEST_B1 = (FB_ALPHA*ACC1)/32768L - (FB_A1*(FB_ALPHA^0xffff8000))/32768L - (FB_B1*FB_X)/32768L;
        SetBuffer(this->MIX_DEST_A0_, MIX_DEST_A0);
        SetBuffer(this->MIX_DEST_A1_, MIX_DEST_A1);
        SetBuffer(this->MIX_DEST_B0_, MIX_DEST_B0);
        SetBuffer(this->MIX_DEST_B1_, MIX_DEST_B1);

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

        output_left_ = (iLastRVBLeft + iRVBLeft) / 2;
        output_right_ = (iLastRVBRight + iRVBRight) / 2;
        // wxMessageOutputDebug().Printf(wxT("left = %d"), output_left_);
      }
      // reverb off
      iLastRVBLeft = iLastRVBRight = 0;
      iRVBLeft = iRVBRight = 0;
      output_left_ = output_right_ = 0;
      iCurrAddr++;
      if (iCurrAddr > 0x3ffff) {
        iCurrAddr = iStartAddr;
      }
    }
  }




}   // namespace SPU
