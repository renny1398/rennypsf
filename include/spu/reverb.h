#pragma once
#include <stdint.h>
#include <wx/ptr_scpd.h>


namespace SPU {


class SPUBase;
class SPUVoice;

class REVERBInfo
{
public:
    REVERBInfo(SPUBase* spu);
    virtual ~REVERBInfo();

    // Reverb
    void InitReverb();
    void Reset();
    virtual void DoReset() {}

    void SetReverb(unsigned short value);
    virtual void StartReverb(SPUVoice* ch) = 0;
    virtual void StoreReverb(const SPUVoice &ch) = 0;

    void SetReverbDepthLeft(int depth);
    void SetReverbDepthRight(int depth);

    int GetBuffer(int ofs) const;
    void SetBuffer(int ofs, int val);
    void SetBufferPlus1(int ofs, int val);
    // int MixReverbLeft(int ns);
    // int MixReverbRight();
    virtual void Mix() = 0;
    int GetLeft() const { return output_left_; }
    int GetRight() const { return output_right_; }

    void WorkAreaStart(uint16_t val);

    int iStartAddr;      // reverb area start addr in samples
    int iCurrAddr;       // reverb area curr addr in samples

    int VolLeft;
    int VolRight;

protected:
    SPUBase& spu_;

public:
    int *sReverbPlay;
    int *sReverbEnd;
    wxScopedArray<int> sReverbStart;
    int  iReverbOff;    // reverb offset
    int  iReverbRepeat;
    int  iReverbNum;

protected:
    int iLastRVBLeft;
    int iLastRVBRight;
    int iRVBLeft;
    int iRVBRight;

    int output_left_;
    int output_right_;

    int dbpos_;

public:
    union {
        int Config[32];
        struct {
            int FB_SRC_A_;       // (offset)
            int FB_SRC_B_;       // (offset)
            int IIR_ALPHA_;      // (coef.)
            int ACC_COEF_A_;     // (coef.)
            int ACC_COEF_B_;     // (coef.)
            int ACC_COEF_C_;     // (coef.)
            int ACC_COEF_D_;     // (coef.)
            int IIR_COEF_;       // (coef.)
            int FB_ALPHA_;       // (coef.)
            int FB_X_;           // (coef.)
            int IIR_DEST_A0_;    // (offset)
            int IIR_DEST_A1_;    // (offset)
            int ACC_SRC_A0_;     // (offset)
            int ACC_SRC_A1_;     // (offset)
            int ACC_SRC_B0_;     // (offset)
            int ACC_SRC_B1_;     // (offset)
            int IIR_SRC_A0_;     // (offset)
            int IIR_SRC_A1_;     // (offset)
            int IIR_DEST_B0_;    // (offset)
            int IIR_DEST_B1_;    // (offset)
            int ACC_SRC_C0_;     // (offset)
            int ACC_SRC_C1_;     // (offset)
            int ACC_SRC_D0_;     // (offset)
            int ACC_SRC_D1_;     // (offset)
            int IIR_SRC_B1_;     // (offset)
            int IIR_SRC_B0_;     // (offset)
            int MIX_DEST_A0_;    // (offset)
            int MIX_DEST_A1_;    // (offset)
            int MIX_DEST_B0_;    // (offset)
            int MIX_DEST_B1_;    // (offset)
            int IN_COEF_L_;      // (coef.)
            int IN_COEF_R_;      // (coef.)
        };
    };
};


inline void REVERBInfo::SetReverbDepthLeft(int depth) {
    VolLeft = depth;
}

inline void REVERBInfo::SetReverbDepthRight(int depth) {
    VolRight = depth;
}

inline void REVERBInfo::WorkAreaStart(uint16_t val)
{
    if (val >= 0xffff || val <= 0x200) {
        iStartAddr = iCurrAddr = 0;
        return;
    }
    const uint32_t lval = static_cast<uint32_t>(val) << 2;
    if ((uint32_t)iStartAddr != lval) {
        iStartAddr = lval;
        iCurrAddr = lval;
    }
}



class NullReverb : public REVERBInfo {
public:
  NullReverb(SPUBase* spu) : REVERBInfo(spu) {}
  virtual void StartReverb(SPUVoice* ch);
  virtual void StoreReverb(const SPUVoice &ch);
  virtual void Mix();
};



class PeteReverb : public REVERBInfo {
public:
  PeteReverb(SPUBase* spu) : REVERBInfo(spu) {}
  virtual void StartReverb(SPUVoice* ch);
  virtual void StoreReverb(const SPUVoice &ch);
  virtual void Mix();
};



class NeilReverb : public REVERBInfo {
public:
  NeilReverb(SPUBase* spu) : REVERBInfo(spu) {}
  void DoReset();
  virtual void StartReverb(SPUVoice* ch);
  virtual void StoreReverb(const SPUVoice &ch);
  virtual void Mix();
};

}   // namespace SPU
