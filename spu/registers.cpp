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
}


// Get Left Volume
uint16_t read1xx0(const ChannelInfo& channelInfo)
{
    uint16_t ret = 0;
    int volume = channelInfo.iLeftVolume;
    if (channelInfo.isLeftSweep) {
        wxMessageOutputDebug().Printf(wxT("WARNING: Sweep mode is experimental."));
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
uint16_t read1xx2(const ChannelInfo& channelInfo)
{
    uint16_t ret = 0;
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
uint16_t read1xx4(const ChannelInfo& channelInfo)
{
    return channelInfo.iActFreq * 4096 / 44100;
}

// Get start address of Sound
uint16_t read1xx6(const ChannelInfo& channelInfo)
{
    uint32_t soundBuffer = channelInfo.tone->GetADPCM() - Spu.GetSoundBuffer();
    return soundBuffer >> 3;
}

// Get ADS level
uint16_t read1xx8(const ChannelInfo& channelInfo)
{
    uint16_t ret = 0;
    if (channelInfo.ADSRX.AttackModeExp) ret |= 0x8000;
    ret |= (channelInfo.ADSRX.AttackRate^0x7f) << 8;
    ret |= ( ((channelInfo.ADSRX.DecayRate>>2)^0x1f) & 0xf ) << 4;
    ret |= (channelInfo.ADSRX.SustainLevel >> 27) & 0xf;
    return ret;
}

// Get sustain rate and release rate
uint16_t read1xxa(const ChannelInfo& channelInfo)
{
    uint16_t ret = 0;
    if (channelInfo.ADSRX.SustainModeExp) ret |= 0x8000;
    if (!channelInfo.ADSRX.SustainIncrease) ret |= 0x4000;
    ret |= (channelInfo.ADSRX.SustainRate^0x7f) << 6;
    if (channelInfo.ADSRX.ReleaseModeExp) ret |= 0x0020;
    ret |= (channelInfo.ADSRX.ReleaseRate>>2)^0x1f;
    return ret;
}

// Get current ADSR volume
uint16_t read1xxc(const ChannelInfo& channelInfo)
{
    if (channelInfo.bNew) return 1;
    if (channelInfo.ADSRX.lVolume && !channelInfo.ADSRX.EnvelopeVol) return 1;
    return channelInfo.ADSRX.EnvelopeVol >> 16;
}

// Get repeat address
uint16_t read1xxe(const ChannelInfo& channelInfo)
{
//    if (channelInfo.pLoop == 0) return 0;
//    return (channelInfo.pLoop - Spu.Memory) >> 3;
    const uint32_t offsetLoop = channelInfo.offsetExternalLoop;
    if (offsetLoop <= 0x80000000) return (offsetLoop >> 3);
    const uint32_t indexLoop = channelInfo.tone->GetLoopIndex();
    if (0x80000000 <= indexLoop) return 0;
    return (channelInfo.tone->GetSPUOffset() + indexLoop / 28 * 16) >> 3;
}


uint16_t (*const readChannelRegisterLUT[8])(const ChannelInfo&) = {
    read1xx0, read1xx2, read1xx4, read1xx6, read1xx8, read1xxa, read1xxc, read1xxe
};


uint16_t readGlobalNOP() { return 0; }

// Get main volume left
uint16_t read1d80() {
    wxMessageOutputDebug().Printf(wxT("WARNING: SPU::GetMainVolumeLeft is not implemented."));
    return 0;
}

// Get main volume right
uint16_t read1d82() {
    wxMessageOutputDebug().Printf(wxT("WARNING: SPU::GetMainVolumeRight is not implemeted."));
    return 0;
}

// Get reverberation depth left
uint16_t read1d84() {
    return Spu.Reverb.VolLeft;
}

// Get reverberation depth right
uint16_t read1d86() {
    return Spu.Reverb.VolRight;
}

uint16_t (*const read1d88)() = readGlobalNOP;
uint16_t (*const read1d8a)() = readGlobalNOP;
uint16_t (*const read1d8c)() = readGlobalNOP;
uint16_t (*const read1d8e)() = readGlobalNOP;
uint16_t (*const read1d90)() = readGlobalNOP;
uint16_t (*const read1d92)() = readGlobalNOP;
uint16_t (*const read1d94)() = readGlobalNOP;
uint16_t (*const read1d96)() = readGlobalNOP;
uint16_t (*const read1d98)() = readGlobalNOP;
uint16_t (*const read1d9a)() = readGlobalNOP;
uint16_t (*const read1d9c)() = readGlobalNOP;
uint16_t (*const read1d9e)() = readGlobalNOP;

uint16_t (*const read1da0)() = readGlobalNOP;
uint16_t (*const read1da2)() = readGlobalNOP;

// Get IRQ address
uint16_t read1da4()
{
    return Spu.GetIRQAddress() >> 3;
}

// Get address
uint16_t read1da6()
{
    return Spu.Addr >> 3;
}

// Get SPU data
uint16_t read1da8()
{
    uint16_t s = ((unsigned short*)Spu.Memory)[Spu.Addr>>1];
    Spu.Addr += 2;
    if (Spu.Addr > 0x7ffff) Spu.Addr = 0;
    return s;
}

// Get SPU control sp0
uint16_t read1daa()
{
    return Spu.Sp0;
}

static unsigned short var_1dac = 0x4;

uint16_t read1dac()
{
    return var_1dac;
}

uint16_t read1dae()
{
    return Spu.Status;
}

uint16_t (*const read1db0)() = readGlobalNOP;
uint16_t (*const read1db2)() = readGlobalNOP;
uint16_t (*const read1db4)() = readGlobalNOP;
uint16_t (*const read1db6)() = readGlobalNOP;


uint16_t (*const readGlobalRegisterLUT[])() = {
    read1d80, read1d82, read1d84, read1d86, read1d88, read1d8a, read1d8c, read1d8e,
    read1d90, read1d92, read1d94, read1d96, read1d98, read1d9a, read1d9c, read1d9e,
    read1da0, read1da2, read1da4, read1da6, read1da8, read1daa, read1dac, read1dae,
    read1db0, read1db2, read1db4, read1db6
};


////////////////////////////////////////////////////////////////
// SPUreadRegister
////////////////////////////////////////////////////////////////

uint16_t SPU::ReadRegister(uint32_t reg)
{
    wxASSERT((reg & 0xfffffe00) == 0x1f801c00);
    // TODO: mutex lock

    // wxMessageOutputDebug().Printf("SPUreadRegister at 0x%08x", reg);

    uint32_t ch = (reg & 0x3ff) >> 4;

    if (ch < 24) {
        const int ofs = (reg & 0xf) >> 1;
        return readChannelRegisterLUT[ofs](Spu.GetChannelInfo(ch));
    }

    // SPU Global Registers
    if (reg < 0x1f801db8) {
        const int ofs = (reg - 0x1f801d80) >> 1;
        return readGlobalRegisterLUT[ofs]();
    }

    // Reverb configuration
    const uint32_t ofs = (reg - 0x1f801dc0) >> 1;
    wxASSERT(ofs < 0x40);
    if (ofs < 0x20) {
        return Spu.Reverb.Config[ofs];
    }

    return 0;
}



////////////////////////////////////////////////////////////////////////
// Write register functions
////////////////////////////////////////////////////////////////////////


void writeChannelNOP(ChannelInfo&, uint16_t) {}

void write1xx0(ChannelInfo& channelInfo, uint16_t volume)
{
    if (volume & 0x8000) {
        channelInfo.isLeftSweep = true;
        channelInfo.isLeftExpSlope = volume & 0x4000;
        bool isDecreased = (volume & 0x2000);
        if (volume & 0x1000) volume ^= 0xffff;
        volume = ((volume & 0x7f)+1)/2;
        if (isDecreased) {
            channelInfo.isLeftDecreased = true;
            volume += volume / (-2);
        } else {
            channelInfo.isLeftDecreased = false;
            volume += volume / 2;
        }
        volume *= 128;
    } else {
        if (volume & 0x4000) {
            volume = 0x3fff - (volume&0x3fff);
        }
    }
    channelInfo.iLeftVolume = volume;
}


void write1xx2(ChannelInfo& channelInfo, uint16_t volume)
{
    if (volume & 0x8000) {
        channelInfo.isRightSweep = true;
        channelInfo.isRightExpSlope = volume & 0x4000;
        bool isDecreased = (volume & 0x2000);
        if (volume & 0x1000) volume ^= 0xffff;
        volume = ((volume & 0x7f)+1)/2;
        if (isDecreased) {
            channelInfo.isRightDecreased = true;
            volume += volume / (-2);
        } else {
            channelInfo.isRightDecreased = false;
            volume += volume / 2;
        }
        volume *= 128;
    } else {
        if (volume & 0x4000) {
            volume = 0x3fff - (volume&0x3fff);
        }
    }
    channelInfo.iRightVolume = volume;
}

// Set pitch
void write1xx4(ChannelInfo& channelInfo, uint16_t val)
{
    int NP = (val > 0x3fff) ? 0x3fff : val;
    channelInfo.iRawPitch = NP;
    NP = 44100*NP/4096;
    if (NP < 1) NP = 1;
    channelInfo.iActFreq = NP;
}

// Set start address of Sound
void write1xx6(ChannelInfo& channelInfo, uint16_t val)
{
//    channelInfo.pStart = Spu.GetSoundBuffer() + (static_cast<uint32_t>(val) << 3);
    SamplingTone* tone = Spu.GetSamplingTone(static_cast<uint32_t>(val) << 3);
    channelInfo.tone = tone;
    channelInfo.itrTone = tone->Iterator(&channelInfo);
}

// Set ADS level
void write1xx8(ChannelInfo& channelInfo, uint16_t val)
{
    ADSRInfoEx& adsrx = channelInfo.ADSRX;
    adsrx.AttackModeExp = ( (val & 0x8000) != 0 );
    adsrx.AttackRate = ((val>>8) & 0x007f)^0x7f;
    adsrx.DecayRate = 4*( ((val>>4) & 0x000f)^0x1f );
    adsrx.SustainLevel = (val & 0x000f) << 27;
}

// Set Sustain rate & Release rate
void write1xxa(ChannelInfo& channelInfo, uint16_t val)
{
    ADSRInfoEx& adsrx = channelInfo.ADSRX;
    adsrx.SustainModeExp = ( (val & 0x8000) != 0 );
    adsrx.SustainIncrease = ( (val & 0x4000) == 0 );
    adsrx.SustainRate = ((val>>6) & 0x007f)^0x7f;
    adsrx.ReleaseModeExp = ( (val & 0x0020) != 0 );
    adsrx.ReleaseRate = 4*((val & 0x001f)^0x1f);
}

// Set current ADSR volume = NOP
void (*const write1xxc)(ChannelInfo&, uint16_t) = writeChannelNOP;

// Set repeat address
void write1xxe(ChannelInfo& channelInfo, uint16_t val)
{
    // channelInfo.pLoop = Spu.GetSoundBuffer() + (static_cast<uint32_t>(val) << 3);
    // channelInfo.ignoresLoop = true;
    channelInfo.offsetExternalLoop = static_cast<uint32_t>(val) << 3;
}


void (*const writeChannelRegisterLUT[8])(ChannelInfo&, uint16_t) = {
    write1xx0, write1xx2, write1xx4, write1xx6, write1xx8, write1xxa, write1xxc, write1xxe
};


void writeGlobalNOP(uint16_t) {}

// Set main volume left
void write1d80(uint16_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetMainVolumeLeft is not implemented."));
}
// Set main volume right
void write1d82(uint16_t) {
    wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetMainVolumeRight is not implemented."));
}

// Set reverberation depth left
void write1d84(uint16_t depth)
{
    Spu.SetReverbDepthLeft(static_cast<int16_t>(depth));
}

// Set reverberation depth right
void write1d86(uint16_t depth)
{
    Spu.SetReverbDepthRight(static_cast<int16_t>(depth));
}

// Voice ON (inline func)
/*
void voiceON(int start, int end, uint16_t flag)
{
    for (int i = start; i < end; i++, flag>>=1) {
        if ((flag & 1) == 0) continue;
        ChannelInfo& channelInfo = Spu.GetChannelInfo(i);
        if (channelInfo.pStart == 0) continue;
        channelInfo.ignoresLoop = false;
        channelInfo.bNew = true;
        // dwNewChannel |= (1<<ch);
    }
}
*/

// Voice ON (0-15)
void write1d88(uint16_t flags)
{
    Spu.Channels.SoundNew(flags);
}

// Voice ON (16-23)
void write1d8a(uint16_t flags)
{
    Spu.Channels.SoundNew(static_cast<uint32_t>(flags) << 16);
}

// Voice OFF (inline func)
inline void voiceOFF(int start, int end, uint16_t flag)
{
    for (int i = start; i < end; i++, flag>>=1) {
        if ((flag & 1) == 0) continue;
        ChannelInfo& channelInfo = Spu.GetChannelInfo(i);
        channelInfo.isStopped = true;
    }
}

// Voice OFF (0-15)
void write1d8c(uint16_t flag)
{
    voiceOFF(0, 16, flag);
}

// Voice 0FF (16-23)
void write1d8e(uint16_t flag)
{
    voiceOFF(16, 24, flag);
}

// Channel FM mode (inline func)
inline void channelFMMode(int start, int end, uint16_t flag)
{
    for (int ch = start; ch < end; ch++, flag >>= 1) {
        if ((flag & 1) == 0) {
            Spu.GetChannelInfo(ch).bFMod = 0;
            continue;
        }
        if (ch == 0) continue;
        Spu.GetChannelInfo(ch).bFMod = 1;
        Spu.GetChannelInfo(ch-1).bFMod = 2;
    }
}

// Channel FM mode (0-15)
void write1d90(uint16_t flag)
{
    channelFMMode(0, 16, flag);
}

// Channel FM mode (16-23)
void write1d92(uint16_t flag)
{
    channelFMMode(16, 24, flag);
}

// Channel Noise mode (inline func)
inline void channelNoiseMode(int start, int end, uint16_t flag)
{
    for (int ch = start; ch < end; ch++, flag >>= 1) {
        if (flag & 1) {
            Spu.GetChannelInfo(ch).bNoise = true;
            continue;
        }
        Spu.GetChannelInfo(ch).bNoise = false;
    }
}

// Channel Noise mode (0-15)
void write1d94(uint16_t flag)
{
    channelNoiseMode(0, 16, flag);
}

// Channel Noise mode (16-23)
void write1d96(uint16_t flag)
{
    channelNoiseMode(16, 24, flag);
}

// Channel Reverb mode (inline func)
inline void channelReverbMode(int start, int end, uint16_t flag)
{
    for (int ch = start; ch < end; ch++, flag >>= 1) {
        if (flag & 1) {
            Spu.GetChannelInfo(ch).hasReverb = true;
            continue;
        }
        Spu.GetChannelInfo(ch).hasReverb = false;
    }
}

// Channel Reverb mode (0-15)
void write1d98(uint16_t flag)
{
    channelReverbMode(0, 16, flag);
}

// Channel Reverb mode (16-23)
void write1d9a(uint16_t flag)
{
    channelReverbMode(16, 24, flag);
}

// Channel mute
void (*const write1d9c)(uint16_t) = writeGlobalNOP;
void (*const write1d9e)(uint16_t) = writeGlobalNOP;

// nop
void (*const write1da0)(uint16_t) = writeGlobalNOP;

// Reverb work area start
void write1da2(uint16_t val)
{
    Spu.Reverb.WorkAreaStart(val);
}

// Sound buffer IRQ address
void write1da4(uint16_t val)
{
    Spu.SetIRQAddress(val << 3);
}

// Sound buffer address
void write1da6(uint16_t val)
{
    Spu.Addr = static_cast<uint32_t>(val) << 3;
}

// Set SPU data
void write1da8(uint16_t val)
{
    ((uint16_t*)Spu.GetSoundBuffer())[Spu.Addr>>1] = val;
    Spu.Addr += 2;
    if (Spu.Addr > 0x7ffff) Spu.Addr = 0;
}

// Set SPU control
void write1daa(uint16_t val)
{
    Spu.Sp0 = val;
}

// Set var_1dac
void write1dac(uint16_t val)
{
    var_1dac = val;
}

// Set SPU status
void write1dae(uint16_t val)
{
    // force Spu to be ready to transfer
    Spu.Status = val & 0xf800;
}

// Set CD volume left
void write1db0(uint16_t)
{
    wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetCDVolumeLeft is not implemented."));
}

// Set CD volume right
void write1db2(uint16_t)
{
    wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetCDVolumeRight is not implemented."));
}

// Set extern volumes
void write1db4(uint16_t) {}
void write1db6(uint16_t) {}


void (*const writeGlobalRegisterLUT[])(uint16_t) = {
    write1d80, write1d82, write1d84, write1d86, write1d88, write1d8a, write1d8c, write1d8e,
    write1d90, write1d92, write1d94, write1d96, write1d98, write1d9a, write1d9c, write1d9e,
    write1da0, write1da2, write1da4, write1da6, write1da8, write1daa, write1dac, write1dae,
    write1db0, write1db2, write1db4, write1db6
};


////////////////////////////////////////////////////////////////
// SPUwriteRegister
////////////////////////////////////////////////////////////////

void SPU::WriteRegister(uint32_t reg, uint16_t val)
{
    wxASSERT((reg & 0xfffffe00) == 0x1f801c00);

    wxCriticalSectionLocker csLocker(csDMAWritable_);

    //wxMessageOutputDebug().Printf("SPUwriteRegister at 0x%08x", reg);

    uint32_t ch = (reg & 0x3ff) >> 4;

    if (ch < 24) {
        ChannelInfo& channelInfo = GetChannelInfo(ch);
        const int ofs = (reg & 0xf) >> 1;
/*
        mutexUpdate_.Lock();
        while (channelInfo.isUpdating == true) {
            condUpdate_.Wait();
        }
        mutexUpdate_.Unlock();
*/
        writeChannelRegisterLUT[ofs](channelInfo, val);
        return;
    }

    // SPU Global Registers
    if (reg < 0x1f801db8) {
        const int ofs = (reg - 0x1f801d80) >> 1;
        writeGlobalRegisterLUT[ofs](val);
        return;
    }

    // Reverb configuration
    const uint32_t ofs = (reg - 0x1f801dc0) >> 1;
    wxASSERT(ofs < 0x40);
    if (ofs < 0x20) {
        Spu.Reverb.Config[ofs] = val;
    }
}


}  // namespace SPU

