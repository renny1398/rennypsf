#include "spu.h"
#include <wx/msgout.h>




namespace SPU {

namespace ChannelRegEnum {
enum {
    VolumeLeft, VolumeRight,
    Pitch,
    StartAddressOfSound,
    AttackDecaySustainLevel,
    SustainReleaseRate,
    CurrentADSRVolume,
    RepeatAddress

};
}

namespace GlobalRegEnum {
enum {
    MainVolumeLeft, MainVolumeRight,
    ReverbDepthLeft, ReverbDepthRight,
    VoiceON1, VoiceON2,
    VoiceOFF1, VoiceOFF2,
    FMMode1, FMMode2,
    NoiseMode1, NoiseMode2,
    ReverbMode1, ReverbMode2,
    ChannelOn1, ChannelOn2
};


// Get Left Volume
uint16_t read1xx0(int ch)
{
    uint16_t ret = 0;
    ChannelInfo& channelInfo = Spu.GetChannelInfo(ch);
    int volume = channelInfo.iLeftVolume;
    if (channelInfo.isLeftSweep) {
        ret |= 0x8000;
        if (channelInfo.isLeftExpSlope) ret |= 0x4000;
        if (channelInfo.isLeftDecreased) ret |= 0x2000;
        if (volume < 0) ret |= 0x1000;
        ret |= volume & 0x7f;
        return ret;
    }
    ret = volume & 0x7fff;
    return ret;
}

// Get Right Volume
uint16_t read1xx2(int ch)
{
    uint16_t ret = 0;
    ChannelInfo& channelInfo = Spu.GetChannelInfo(ch);
    int volume = channelInfo.iRightVolume;
    if (channelInfo.isRightSweep) {
        ret |= 0x8000;
        if (channelInfo.isRightExpSlope) ret |= 0x4000;
        if (channelInfo.isRightDecreased) ret |= 0x2000;
        if (volume < 0) ret |= 0x1000;
        ret |= volume & 0x7f;
        return ret;
    }
    ret = volume & 0x7fff;
    return ret;
}

// Get Pitch
uint16_t read1xx4(int ch)
{
    return Spu.GetChannelInfo(ch).iActFreq * 4096 / 44100;
}

// Get start address of Sound
uint16_t read1xx6(int ch)
{
    int soundBuffer = Spu.GetChannelInfo(ch).pStart - Spu.GetSoundBuffer();
    return soundBuffer >> 3;
}

// Get ADS level
uint16_t read1xx8(int ch)
{
    uint16_t ret = 0;
    ChannelInfo& channelInfo = Spu.GetChannelInfo(ch);
    if (channelInfo.ADSRX.AttackModeExp) ret |= 0x8000;
    ret |= channelInfo.ADSRX.AttackRate << 8;
    ret |= channelInfo.ADSRX.DecayRate << 4;
    ret |= channelInfo.ADSRX.SustainLevel;
    return ret;
}



}


uint16_t ReadRegister(unsigned long reg)
{
    wxASSERT((reg & 0xfffffe00) == 0x1f801c00);
    // TODO: mutex lock

    uint32_t ch = (reg & 0x3ff) >> 4;

    if (ch < 24) {
        const int ofs = (reg & 0xf) >> 1;

    }

    // SPU Global Registers
    int ofs = (reg - 0x1f801d80) >> 1;

    return 0;
}




}  // namespace SPU

