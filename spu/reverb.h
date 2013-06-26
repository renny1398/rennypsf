#pragma once
#include <stdint.h>
#include <wx/ptr_scpd.h>


namespace SPU {


class SPU;
class ChannelInfo;

class REVERBInfo
{
public:
    REVERBInfo(SPU* spu);
    ~REVERBInfo();

    // Reverb
    void InitReverb();
    void Reset();

    void SetReverb(unsigned short value);
    void StartReverb(ChannelInfo& ch);
    void StoreReverb(const ChannelInfo &ch, int ns);

    void SetReverbDepthLeft(int depth);
    void SetReverbDepthRight(int depth);

    int ReverbGetBuffer(int ofs) const;
    void ReverbSetBuffer(int ofs, int val);
    void ReverbSetBufferPlus1(int ofs, int val);
    int MixReverbLeft(int ns);
    int MixReverbRight();

    void WorkAreaStart(uint16_t val);

    int iStartAddr;      // reverb area start addr in samples
    int iCurrAddr;       // reverb area curr addr in samples

    int VolLeft;
    int VolRight;

private:
    SPU& spu_;

public:
    int *sReverbPlay;
    int *sReverbEnd;
    wxScopedArray<int> sReverbStart;
    int  iReverbOff;    // reverb offset
    int  iReverbRepeat;
    int  iReverbNum;

private:
    int iLastRVBLeft;
    int iLastRVBRight;
    int iRVBLeft;
    int iRVBRight;

public:
    union {
        int Config[32];
        struct {
            int FB_SRC_A;       // (offset)
            int FB_SRC_B;       // (offset)
            int IIR_ALPHA;      // (coef.)
            int ACC_COEF_A;     // (coef.)
            int ACC_COEF_B;     // (coef.)
            int ACC_COEF_C;     // (coef.)
            int ACC_COEF_D;     // (coef.)
            int IIR_COEF;       // (coef.)
            int FB_ALPHA;       // (coef.)
            int FB_X;           // (coef.)
            int IIR_DEST_A0;    // (offset)
            int IIR_DEST_A1;    // (offset)
            int ACC_SRC_A0;     // (offset)
            int ACC_SRC_A1;     // (offset)
            int ACC_SRC_B0;     // (offset)
            int ACC_SRC_B1;     // (offset)
            int IIR_SRC_A0;     // (offset)
            int IIR_SRC_A1;     // (offset)
            int IIR_DEST_B0;    // (offset)
            int IIR_DEST_B1;    // (offset)
            int ACC_SRC_C0;     // (offset)
            int ACC_SRC_C1;     // (offset)
            int ACC_SRC_D0;     // (offset)
            int ACC_SRC_D1;     // (offset)
            int IIR_SRC_B1;     // (offset)
            int IIR_SRC_B0;     // (offset)
            int MIX_DEST_A0;    // (offset)
            int MIX_DEST_A1;    // (offset)
            int MIX_DEST_B0;    // (offset)
            int MIX_DEST_B1;    // (offset)
            int IN_COEF_L;      // (coef.)
            int IN_COEF_R;      // (coef.)
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

}   // namespace SPU
