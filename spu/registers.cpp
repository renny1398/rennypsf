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
    const SPU& Spu = channelInfo.Spu();
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
    const uint32_t offsetLoop = channelInfo.addrExternalLoop;
    if (offsetLoop <= 0x80000000) return (offsetLoop >> 3);
    const uint32_t indexLoop = channelInfo.tone->GetLoopOffset();
    if (0x80000000 <= indexLoop) return 0;
    return (channelInfo.tone->GetAddr() + indexLoop / 28 * 16) >> 3;
  }


  uint16_t (*const readChannelRegisterLUT[8])(const ChannelInfo&) = {
      read1xx0, read1xx2, read1xx4, read1xx6, read1xx8, read1xxa, read1xxc, read1xxe
};



uint16_t readGlobalNOP(const SPU& /*spu*/) { return 0; }

// Get main volume left
uint16_t read1d80(const SPU& /*spu*/) {
  wxMessageOutputDebug().Printf(wxT("WARNING: SPU::GetMainVolumeLeft is not implemented."));
  return 0;
}

// Get main volume right
uint16_t read1d82(const SPU& /*spu*/) {
  wxMessageOutputDebug().Printf(wxT("WARNING: SPU::GetMainVolumeRight is not implemeted."));
  return 0;
}

// Get reverberation depth left
uint16_t read1d84(const SPU& spu) {
  return spu.Reverb().VolLeft;
}

// Get reverberation depth right
uint16_t read1d86(const SPU& spu) {
  return spu.Reverb().VolRight;
}

uint16_t (*const read1d88)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d8a)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d8c)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d8e)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d90)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d92)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d94)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d96)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d98)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d9a)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d9c)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d9e)(const SPU& /*spu*/) = readGlobalNOP;

uint16_t (*const read1da0)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1da2)(const SPU& /*spu*/) = readGlobalNOP;

// Get IRQ address
uint16_t read1da4(const SPU& spu)
{
  return spu.GetIRQAddress() >> 3;
}

// Get address
uint16_t read1da6(const SPU& spu)
{
  return spu.dma_current_addr_ >> 3;
}

// Get SPU data
uint16_t read1da8(const SPU& spu)
{
  uint16_t s = ((unsigned short*)spu.Memory)[spu.dma_current_addr_>>1];
  spu.dma_current_addr_ += 2;
  if (spu.dma_current_addr_ > 0x7ffff) spu.dma_current_addr_ = 0;
  return s;
}

// Get SPU control sp0
uint16_t read1daa(const SPU& spu)
{
  return spu.Sp0;
}

static unsigned short var_1dac = 0x4;

uint16_t read1dac(const SPU& /*spu*/)
{
  return var_1dac;
}

uint16_t read1dae(const SPU& spu)
{
  return spu.Status;
}

uint16_t (*const read1db0)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1db2)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1db4)(const SPU& /*spu*/) = readGlobalNOP;
uint16_t (*const read1db6)(const SPU& /*spu*/) = readGlobalNOP;


uint16_t (*const readGlobalRegisterLUT[])(const SPU& /*spu*/) = {
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
      return readChannelRegisterLUT[ofs](Channel(ch));
    }

  // SPU Global Registers
  if (reg < 0x1f801db8) {
      const int ofs = (reg - 0x1f801d80) >> 1;
      return readGlobalRegisterLUT[ofs](*this);
    }

  // Reverb configuration
  const uint32_t ofs = (reg - 0x1f801dc0) >> 1;
  wxASSERT(ofs < 0x40);
  if (ofs < 0x20) {
      return Reverb().Config[ofs];
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
  NP = 44100*NP/0x1000;
  if (NP < 1) NP = 1;
  channelInfo.iActFreq = NP;
  if (channelInfo.tone != NULL) {
      const double freq = channelInfo.tone->GetFreq();
      if (1.0 <= freq) {
          channelInfo.Pitch = (freq * channelInfo.iRawPitch) / 0x1000;
        } else {
          channelInfo.Pitch = 0.0;
        }
    }
}

// Set start address of Sound
void write1xx6(ChannelInfo& channelInfo, uint16_t val)
{
  SPU& Spu = channelInfo.Spu();
  SamplingTone* tone = Spu.GetSamplingTone(static_cast<uint32_t>(val) << 3);
  channelInfo.tone = tone;

  Spu.NotifyOnUpdateStartAddress(channelInfo.ch);
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
  channelInfo.addrExternalLoop = static_cast<uint32_t>(val) << 3;
  channelInfo.useExternalLoop = true;
  if (channelInfo.tone) {
      channelInfo.tone->SetExternalLoopAddr(channelInfo.addrExternalLoop);
    }
  SPU& Spu = channelInfo.Spu();
  Spu.NotifyOnChangeLoopIndex(&channelInfo);
}


void (*const writeChannelRegisterLUT[8])(ChannelInfo&, uint16_t) = {
    write1xx0, write1xx2, write1xx4, write1xx6, write1xx8, write1xxa, write1xxc, write1xxe
};



void writeGlobalNOP(SPU* /*spu*/, uint16_t) {}

// Set main volume left
void write1d80(SPU* /*spu*/, uint16_t) {
  wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetMainVolumeLeft is not implemented."));
}
// Set main volume right
void write1d82(SPU* /*spu*/, uint16_t) {
  wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetMainVolumeRight is not implemented."));
}

// Set reverberation depth left
void write1d84(SPU* spu, uint16_t depth)
{
  spu->Reverb().SetReverbDepthLeft(static_cast<int16_t>(depth));
}

// Set reverberation depth right
void write1d86(SPU* spu, uint16_t depth)
{
  spu->Reverb().SetReverbDepthRight(static_cast<int16_t>(depth));
}

// Voice ON (0-15)
void write1d88(SPU* spu, uint16_t flags)
{
  spu->Channels.SoundNew(flags, 0);
}

// Voice ON (16-23)
void write1d8a(SPU* spu, uint16_t flags)
{
  spu->Channels.SoundNew(flags & 0xff, 16);
}

// Voice OFF (0-15)
void write1d8c(SPU* spu, uint16_t flags)
{
  spu->Channels.VoiceOff(flags, 0);
}

// Voice 0FF (16-23)
void write1d8e(SPU* spu, uint16_t flags)
{
  // 0xff can't be omitted.
  spu->Channels.VoiceOff(flags & 0xff, 16);
}

// Channel FM mode (inline func)
inline void channelFMMode(SPU* spu, int start, int end, uint16_t flag)
{
  for (int ch = start; ch < end; ch++, flag >>= 1) {
      if ((flag & 1) == 0) {
          spu->Channel(ch).bFMod = 0;
          continue;
        }
      if (ch == 0) continue;
      spu->Channel(ch).bFMod = 1;
      spu->Channel(ch-1).bFMod = 2;
    }
}

// Channel FM mode (0-15)
void write1d90(SPU* spu, uint16_t flag)
{
  channelFMMode(spu, 0, 16, flag);
}

// Channel FM mode (16-23)
void write1d92(SPU* spu, uint16_t flag)
{
  channelFMMode(spu, 16, 24, flag);
}

// Channel Noise mode (inline func)
inline void channelNoiseMode(SPU* spu, int start, int end, uint16_t flag)
{
  for (int ch = start; ch < end; ch++, flag >>= 1) {
      if (flag & 1) {
          spu->Channel(ch).bNoise = true;
          continue;
        }
      spu->Channel(ch).bNoise = false;
    }
}

// Channel Noise mode (0-15)
void write1d94(SPU* spu, uint16_t flag)
{
  channelNoiseMode(spu, 0, 16, flag);
}

// Channel Noise mode (16-23)
void write1d96(SPU* spu, uint16_t flag)
{
  channelNoiseMode(spu, 16, 24, flag);
}

// Channel Reverb mode (inline func)
inline void channelReverbMode(SPU* spu, int start, int end, uint16_t flag)
{
  for (int ch = start; ch < end; ch++, flag >>= 1) {
      if (flag & 1) {
          spu->Channel(ch).hasReverb = true;
          continue;
        }
      spu->Channel(ch).hasReverb = false;
    }
}

// Channel Reverb mode (0-15)
void write1d98(SPU* spu, uint16_t flag)
{
  channelReverbMode(spu, 0, 16, flag);
}

// Channel Reverb mode (16-23)
void write1d9a(SPU* spu, uint16_t flag)
{
  channelReverbMode(spu, 16, 24, flag);
}

// Channel mute
void (*const write1d9c)(SPU* /*spu*/, uint16_t) = writeGlobalNOP;
void (*const write1d9e)(SPU* /*spu*/, uint16_t) = writeGlobalNOP;

// nop
void (*const write1da0)(SPU* /*spu*/, uint16_t) = writeGlobalNOP;

// Reverb work area start
void write1da2(SPU* spu, uint16_t val)
{
  spu->Reverb().WorkAreaStart(val);
}

// Sound buffer IRQ address
void write1da4(SPU* spu, uint16_t val)
{
  spu->SetIRQAddress(val << 3);
}

// Sound buffer address
void write1da6(SPU* spu, uint16_t val)
{
  spu->dma_current_addr_ = static_cast<uint32_t>(val) << 3;
}

// Set SPU data
void write1da8(SPU* spu, uint16_t val)
{
  ((uint16_t*)spu->GetSoundBuffer())[spu->dma_current_addr_>>1] = val;
  spu->dma_current_addr_ += 2;
  if (spu->dma_current_addr_ > 0x7ffff) spu->dma_current_addr_ = 0;
}

// Set SPU control
void write1daa(SPU* spu, uint16_t val)
{
  spu->Sp0 = val;
}

// Set var_1dac
void write1dac(SPU* /*spu*/, uint16_t val)
{
  var_1dac = val;
}

// Set SPU status
void write1dae(SPU* spu, uint16_t val)
{
  // force Spu to be ready to transfer
  spu->Status = val & 0xf800;
}

// Set CD volume left
void write1db0(SPU* /*spu*/, uint16_t)
{
  wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetCDVolumeLeft is not implemented."));
}

// Set CD volume right
void write1db2(SPU* /*spu*/, uint16_t)
{
  wxMessageOutputDebug().Printf(wxT("WARNING: SPU::SetCDVolumeRight is not implemented."));
}

// Set extern volumes
void write1db4(SPU* /*spu*/, uint16_t) {}
void write1db6(SPU* /*spu*/, uint16_t) {}


void (*const writeGlobalRegisterLUT[])(SPU* spu, uint16_t) = {
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
      ChannelInfo& channelInfo = Channel(ch);
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
      writeGlobalRegisterLUT[ofs](this, val);
      return;
    }

  // Reverb configuration
  const uint32_t ofs = (reg - 0x1f801dc0) >> 1;
  wxASSERT(ofs < 0x40);
  if (ofs < 0x20) {
      Reverb().Config[ofs] = val;
    }
}


}  // namespace SPU

