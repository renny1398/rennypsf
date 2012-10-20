#pragma once
#include <stdint.h>

namespace SPU {

struct REVERBInfo
{
public:
    void WorkAreaStart(uint16_t val);


    int iStartAddr;      // reverb area start addr in samples
    int iCurrAddr;       // reverb area curr addr in samples

    int VolLeft;
    int VolRight;
    int iLastRVBLeft;
    int iLastRVBRight;
    int iRVBLeft;
    int iRVBRight;

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
