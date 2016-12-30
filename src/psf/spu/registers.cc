#include "psf/spu/spu.h"
#include "common/debug.h"


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
uint16_t read1xx0(const SPUVoice& channelInfo)
{
  uint16_t ret = 0;
  int volume = channelInfo.iLeftVolume;
  if (channelInfo.isLeftSweep) {
    rennyLogWarning("SPURegisters", "Sweep mode is experimental.");
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
uint16_t read1xx2(const SPUVoice& channelInfo)
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
uint16_t read1xx4(const SPUVoice& channelInfo)
{
  return channelInfo.iRawPitch;
  // return channelInfo.iActFreq * 4096 / 44100;
}

// Get start address of Sound
uint16_t read1xx6(const SPUVoice& channelInfo)
{
  // const SPUBase& Spu = channelInfo.Spu();
  uint32_t soundBuffer = channelInfo.tone->addr();
  return soundBuffer >> 3;
}

// Get ADS level
uint16_t read1xx8(const SPUVoice& channelInfo)
{
  uint16_t ret = 0;
  const EnvelopePassive& adsr = channelInfo.ADSRX;
  if (adsr.attack_mode_exp()) ret |= 0x8000;
  ret |= (adsr.attack_rate()^0x7f) << 8;
  ret |= ( ((adsr.decay_rate()>>2)^0x1f) & 0xf ) << 4;
  ret |= (adsr.sustain_level() >> 27) & 0xf;
  return ret;
}

// Get sustain rate and release rate
uint16_t read1xxa(const SPUVoice& channelInfo)
{
  uint16_t ret = 0;
  const EnvelopePassive& adsr = channelInfo.ADSRX;
  if (adsr.sustain_mode_exp()) ret |= 0x8000;
  if (!adsr.sustain_increase()) ret |= 0x4000;
  ret |= (adsr.sustain_rate()^0x7f) << 6;
  if (adsr.release_mode_exp()) ret |= 0x0020;
  ret |= (adsr.release_rate()>>2)^0x1f;
  return ret;
}

// Get current ADSR volume
uint16_t read1xxc(const SPUVoice& channelInfo)
{
  const EnvelopeActive& adsr = channelInfo.ADSR;
  if (adsr.volume() > 0L && adsr.envelope_volume() == 0) return 1;
  return adsr.envelope_volume() >> 16;
}

// Get repeat address
uint16_t read1xxe(const SPUVoice& channelInfo)
{
  //    if (channelInfo.pLoop == 0) return 0;
  //    return (channelInfo.pLoop - Spu.Memory) >> 3;
  const uint32_t offsetLoop = channelInfo.addrExternalLoop;
  if (offsetLoop <= 0x80000000) return (offsetLoop >> 3);
  const int indexLoop = channelInfo.tone->loop();
  if (indexLoop < 0) return 0;
  return (channelInfo.tone->addr() + indexLoop / 28 * 16) >> 3;
}


uint16_t (*const readChannelRegisterLUT[8])(const SPUVoice&) = {
    read1xx0, read1xx2, read1xx4, read1xx6, read1xx8, read1xxa, read1xxc, read1xxe
};



uint16_t readGlobalNOP(const SPUBase& /*spu*/) { return 0; }

// Get main volume left
uint16_t read1d80(const SPUBase& /*spu*/) {
  rennyLogWarning("SPURegisters", "SPU::GetMainVolumeLeft() is not implemented.");
  return 0;
}

// Get main volume right
uint16_t read1d82(const SPUBase& /*spu*/) {
  rennyLogWarning("SPURegisters", "SPU::GetMainVolumeRight() is not implemeted.");
  return 0;
}

// Get reverberation depth left
uint16_t read1d84(const SPUBase& spu) {
  return spu.Reverb().VolLeft;
}

// Get reverberation depth right
uint16_t read1d86(const SPUBase& spu) {
  return spu.Reverb().VolRight;
}

uint16_t (*const read1d88)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d8a)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d8c)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d8e)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d90)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d92)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d94)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d96)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d98)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d9a)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d9c)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1d9e)(const SPUBase& /*spu*/) = readGlobalNOP;

uint16_t (*const read1da0)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1da2)(const SPUBase& /*spu*/) = readGlobalNOP;

// Get IRQ address
uint16_t read1da4(const SPUBase& spu)
{
  return spu.GetIRQAddress() >> 3;
}

// Get address
uint16_t read1da6(const SPUBase& spu)
{
  // TODO: is it corrected?
  return spu.core(0).addr_ >> 3;
}

// Get SPU data
uint16_t read1da8(const SPUBase& spu)
{
  // ReadDMA?
  uint16_t s = spu.mem16_val(spu.core(0).addr_);
  spu.core(0).addr_ += 2;
  if (spu.core(0).addr_ > 0x7ffff) spu.core(0).addr_ = 0;
  return s;
}

// Get SPU control sp0
uint16_t read1daa(const SPUBase& spu)
{
  return spu.core(0).ctrl_;
}

static unsigned short var_1dac = 0x4;

uint16_t read1dac(const SPUBase& /*spu*/)
{
  return var_1dac;
}

uint16_t read1dae(const SPUBase& spu)
{
  return spu.core(0).stat_;
}

uint16_t (*const read1db0)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1db2)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1db4)(const SPUBase& /*spu*/) = readGlobalNOP;
uint16_t (*const read1db6)(const SPUBase& /*spu*/) = readGlobalNOP;


uint16_t (*const readGlobalRegisterLUT[])(const SPUBase& /*spu*/) = {
    read1d80, read1d82, read1d84, read1d86, read1d88, read1d8a, read1d8c, read1d8e,
    read1d90, read1d92, read1d94, read1d96, read1d98, read1d9a, read1d9c, read1d9e,
    read1da0, read1da2, read1da4, read1da6, read1da8, read1daa, read1dac, read1dae,
    read1db0, read1db2, read1db4, read1db6
};


////////////////////////////////////////////////////////////////
// SPUreadRegister
////////////////////////////////////////////////////////////////

uint16_t SPUBase::ReadRegister(uint32_t reg) const
{
  rennyAssert((reg & 0xfffffe00) == 0x1f801c00);
  // TODO: mutex lock

  // wxMessageOutputDebug().Printf("SPUreadRegister at 0x%08x", reg);

  uint32_t ch = (reg & 0x3ff) >> 4;

  if (ch < 24) {
    const int ofs = (reg & 0xf) >> 1;
    return readChannelRegisterLUT[ofs](const_cast<SPUBase*>(this)->Voice(ch));
  }

  // SPU Global Registers
  if (reg < 0x1f801db8) {
    const int ofs = (reg - 0x1f801d80) >> 1;
    return readGlobalRegisterLUT[ofs](*this);
  }

  // Reverb configuration
  const uint32_t ofs = (reg - 0x1f801dc0) >> 1;
  if (ofs < 0x20) {
    return Reverb().Config[ofs];
  }

  return 0;
}



////////////////////////////////////////////////////////////////////////
// Write register functions
////////////////////////////////////////////////////////////////////////


void writeChannelNOP(SPUVoice&, uint16_t) {}

void write1xx0(SPUVoice& channelInfo, uint16_t volume)
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
  rennyAssert(volume < 0x4000);
  channelInfo.iLeftVolume = volume;
}


void write1xx2(SPUVoice& channelInfo, uint16_t volume)
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
void write1xx4(SPUVoice& channelInfo, uint16_t val)
{
  int NP = (val > 0x3fff) ? 0x3fff : val;
  if (channelInfo.iRawPitch != NP) {
    channelInfo.iRawPitch = NP;
    channelInfo.NotifyOnChangePitch();
  }
  const uint32_t sampling_rate = channelInfo.Spu().GetCurrentSamplingRate();
  NP = sampling_rate * NP / 0x1000;
  if (NP < 1) NP = 1;
  channelInfo.iActFreq = NP;
  if (channelInfo.tone != NULL) {
    const double freq = channelInfo.tone->freq();
    if (1.0 <= freq) {
      channelInfo.Pitch = (freq * channelInfo.iRawPitch) / 0x1000;
    } else {
      channelInfo.Pitch = 0.0;
    }
  }
}

// Set start address of Sound
void write1xx6(SPUVoice& channelInfo, uint16_t val)
{
/*
  SPUBase& Spu = channelInfo.Spu();
  int id = static_cast<int>(val);
  // SPUInstrument_New* tone = Spu.GetSamplingTone(addr);
  Instrument* inst = &(Spu.soundbank().instrument(id));
  SPUInstrument_New* spu_inst = dynamic_cast<SPUInstrument_New*>(inst);
  if (spu_inst == 0) {
    spu_inst = new SPUInstrument_New(Spu, id << 3, channelInfo.addrExternalLoop);
    Spu.soundbank().set_instrument(spu_inst);
    wxMessageOutputDebug().Printf(wxT("SPU: created a new instrument. (id = %d)"), inst->id());
  }
  channelInfo.tone = spu_inst;
*/
  SPUAddr addr = static_cast<SPUAddr>(val) << 3;
  if (channelInfo.IsOn() == true) {
    // rennyLogWarning("SPURegisters", "Set start address: channel %d is on.", channelInfo.ch);
  }
  channelInfo.addr = addr;

  // Spu.NotifyOnUpdateStartAddress(channelInfo.ch);
}

// Set ADS level
void write1xx8(SPUVoice& channelInfo, uint16_t val)
{
  EnvelopePassive& adsr = channelInfo.ADSRX;
  adsr.set_attack_mode_exp( (val & 0x8000) != 0 );
  adsr.set_attack_rate( ((val>>8) & 0x007f)^0x7f );
  adsr.set_decay_rate( 4 * ( ((val>>4) & 0x000f)^0x1f ) );
  adsr.set_sustain_level((val & 0x000f) << 27);
}

// Set Sustain rate & Release rate
void write1xxa(SPUVoice& channelInfo, uint16_t val)
{
  EnvelopePassive& adsr = channelInfo.ADSRX;
  adsr.set_sustain_mode_exp( (val & 0x8000) != 0 );
  adsr.set_sustain_increase( (val & 0x4000) == 0 );
  adsr.set_sustain_rate( ((val>>6) & 0x007f)^0x7f );
  adsr.set_release_mode_exp( (val & 0x0020) != 0 );
  adsr.set_release_rate( 4 * ((val & 0x001f)^0x1f) );
}

// Set current ADSR volume = NOP
void (*const write1xxc)(SPUVoice&, uint16_t) = writeChannelNOP;

// Set repeat address
void write1xxe(SPUVoice& channelInfo, uint16_t val)
{
  // channelInfo.pLoop = Spu.GetSoundBuffer() + (static_cast<uint32_t>(val) << 3);
  SPUAddr addr = static_cast<SPUAddr>(val) << 3;
  if (channelInfo.IsOn() == true) {
    // rennyLogWarning("SPURegisters", "Set repeat address: channel %d is on.", channelInfo.ch);
  }
  channelInfo.addrExternalLoop = addr;
  channelInfo.useExternalLoop = true;
  if (addr >= 0x80000000) return;
/*
  if (channelInfo.tone) {
    // SPUAddr prev_addr = channelInfo.tone->GetExternalLoopAddr();
    // hannelInfo.tone->SetExternalLoopAddr(channelInfo.addrExternalLoop);
    if (prev_addr != channelInfo.addrExternalLoop) {
      SPUBase& Spu = channelInfo.Spu();
      Spu.NotifyOnChangeLoopIndex(&channelInfo);
    }
  }
*/
}


void (*const writeChannelRegisterLUT[8])(SPUVoice&, uint16_t) = {
    write1xx0, write1xx2, write1xx4, write1xx6, write1xx8, write1xxa, write1xxc, write1xxe
};



void writeGlobalNOP(SPUBase* /*spu*/, uint16_t) {}

// Set main volume left
void write1d80(SPUBase* /*spu*/, uint16_t) {
  rennyLogWarning("SPURegisters", "SPU::SetMainVolumeLeft() is not implemented.");
}
// Set main volume right
void write1d82(SPUBase* /*spu*/, uint16_t) {
  rennyLogWarning("SPURegisters", "SPU::SetMainVolumeRight() is not implemented.");
}

// Set reverberation depth left
void write1d84(SPUBase* spu, uint16_t depth)
{
  spu->Reverb().SetReverbDepthLeft(static_cast<int16_t>(depth));
}

// Set reverberation depth right
void write1d86(SPUBase* spu, uint16_t depth)
{
  spu->Reverb().SetReverbDepthRight(static_cast<int16_t>(depth));
}

// Voice ON (0-15)
void write1d88(SPUBase* spu, uint16_t flags)
{
  spu->core(0).Voices().SoundNew(flags, 0);
}

// Voice ON (16-23)
void write1d8a(SPUBase* spu, uint16_t flags)
{
  spu->core(0).Voices().SoundNew(flags & 0xff, 16);
}

// Voice OFF (0-15)
void write1d8c(SPUBase* spu, uint16_t flags)
{
  spu->core(0).Voices().VoiceOff(flags, 0);
}

// Voice 0FF (16-23)
void write1d8e(SPUBase* spu, uint16_t flags)
{
  // 0xff can't be omitted.
  spu->core(0).Voices().VoiceOff(flags & 0xff, 16);
}

// Channel FM mode (inline func)
inline void channelFMMode(SPUBase* spu, int start, int end, uint16_t flag)
{
  for (int ch = start; ch < end; ch++, flag >>= 1) {
    if ((flag & 1) == 0) {
      spu->Voice(ch).bFMod = 0;
      continue;
    }
    if (ch == 0) continue;
    spu->Voice(ch).bFMod = 1;
    spu->Voice(ch-1).bFMod = 2;
  }
}

// Channel FM mode (0-15)
void write1d90(SPUBase* spu, uint16_t flag)
{
  channelFMMode(spu, 0, 16, flag);
}

// Channel FM mode (16-23)
void write1d92(SPUBase* spu, uint16_t flag)
{
  channelFMMode(spu, 16, 24, flag);
}

// Channel Noise mode (inline func)
inline void channelNoiseMode(SPUBase* spu, int start, int end, uint16_t flag)
{
  for (int ch = start; ch < end; ch++, flag >>= 1) {
    if (flag & 1) {
      spu->Voice(ch).bNoise = true;
      continue;
    }
    spu->Voice(ch).bNoise = false;
  }
}

// Channel Noise mode (0-15)
void write1d94(SPUBase* spu, uint16_t flag)
{
  channelNoiseMode(spu, 0, 16, flag);
}

// Channel Noise mode (16-23)
void write1d96(SPUBase* spu, uint16_t flag)
{
  channelNoiseMode(spu, 16, 24, flag);
}

// Channel Reverb mode (inline func)
inline void channelReverbMode(SPUBase* spu, int start, int end, uint16_t flag)
{
  for (int ch = start; ch < end; ch++, flag >>= 1) {
    SPUVoice& chInfo = spu->Voice(ch);
    if (flag & 1) {
      // wxMessageOutputDebug().Printf(wxT("SPU Reverb enabled: Ch.%d"), ch);
      chInfo.hasReverb = true;
      continue;
    }
    // wxMessageOutputDebug().Printf(wxT("SPU Reverb disabled: Ch.%d"), ch);
    chInfo.hasReverb = false;
  }
}

// Channel Reverb mode (0-15)
void write1d98(SPUBase* spu, uint16_t flag)
{
  channelReverbMode(spu, 0, 16, flag);
}

// Channel Reverb mode (16-23)
void write1d9a(SPUBase* spu, uint16_t flag)
{
  channelReverbMode(spu, 16, 24, flag);
}

// Channel mute
void (*const write1d9c)(SPUBase* /*spu*/, uint16_t) = writeGlobalNOP;
void (*const write1d9e)(SPUBase* /*spu*/, uint16_t) = writeGlobalNOP;

// nop
void (*const write1da0)(SPUBase* /*spu*/, uint16_t) = writeGlobalNOP;

// Reverb work area start
void write1da2(SPUBase* spu, uint16_t val)
{
  spu->Reverb().WorkAreaStart(val);
}

// Sound buffer IRQ address
void write1da4(SPUBase* spu, uint16_t val)
{
  spu->SetIRQAddress(val << 3);
}

// Sound buffer address
void write1da6(SPUBase* spu, uint16_t val)
{
  spu->core(0).addr_ = static_cast<uint32_t>(val) << 3;
}

// Set SPU data
void write1da8(SPUBase* spu, uint16_t val)
{
  ((uint16_t*)spu->GetSoundBuffer())[spu->core(0).addr_>>1] = val;
  spu->core(0).addr_ += 2;
  if (spu->core(0).addr_ > 0x7ffff) spu->core(0).addr_ = 0;
}

// Set SPU control
void write1daa(SPUBase* spu, uint16_t val)
{
  SPUCore& core = spu->core(0);
  core.ctrl_ = val;
}

// Set var_1dac
void write1dac(SPUBase* /*spu*/, uint16_t val)
{
  var_1dac = val;
}

// Set SPU status
void write1dae(SPUBase* spu, uint16_t val)
{
  // force Spu to be ready to transfer
  SPUCore& core = spu->core(0);
  core.stat_ = val & 0xf800;
}

// Set CD volume left
void write1db0(SPUBase* /*spu*/, uint16_t)
{
  rennyLogWarning("SPURegisters", "SPU::SetCDVolumeLeft() is not implemented.");
}

// Set CD volume right
void write1db2(SPUBase* /*spu*/, uint16_t)
{
  rennyLogWarning("SPURegisters", "SPU::SetCDVolumeRight() is not implemented.");
}

// Set extern volumes
void write1db4(SPUBase* /*spu*/, uint16_t) {}
void write1db6(SPUBase* /*spu*/, uint16_t) {}


void (*const writeGlobalRegisterLUT[])(SPUBase* spu, uint16_t) = {
    write1d80, write1d82, write1d84, write1d86, write1d88, write1d8a, write1d8c, write1d8e,
    write1d90, write1d92, write1d94, write1d96, write1d98, write1d9a, write1d9c, write1d9e,
    write1da0, write1da2, write1da4, write1da6, write1da8, write1daa, write1dac, write1dae,
    write1db0, write1db2, write1db4, write1db6
};


////////////////////////////////////////////////////////////////
// SPUwriteRegister
////////////////////////////////////////////////////////////////

void SPUBase::WriteRegister(uint32_t reg, uint16_t val)
{
  rennyAssert((reg & 0xfffffe00) == 0x1f801c00);

  // wxCriticalSectionLocker csLocker(csDMAWritable_);

  // wxMessageOutputDebug().Printf("SPUwriteRegister at 0x%08x", reg);

  uint32_t ch = (reg & 0x3ff) >> 4;

  if (ch < 24) {
    SPUVoice& channelInfo = Voice(ch);
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
  if (ofs < 0x20) {
    Reverb().Config[ofs] = (int16_t)val;
    rennyLogDebug("SPURegisters", "SPU Reverb: set Config[0x%02x] = %d", ofs, val);
  }
}

}  // namespace SPU

