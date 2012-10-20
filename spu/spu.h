#pragma once
#include <stdint.h>
#include <wx/debug.h>

#include "reverb.h"

namespace SPU
{

class Volume
{
    int32_t volume;
public:
    uint16_t Int15() const {
        return volume & 0x3fff;
    }
    void Int15(uint16_t vol) {
        wxASSERT(vol < 0x8000);
        if (vol > 0x3fff) {
            volume = vol - 0x8000;
            wxASSERT(volume < 0);
        } else {
            volume = vol;
        }
    }
};

class Pitch
{
    uint32_t pitch;
public:
    uint16_t Uint14() const {
        return pitch;
    }
    void Uint14(uint16_t p) {
        pitch = p;
    }
};


struct ADSRInfo
{
    int            AttackModeExp;
    long           AttackTime;
    long           DecayTime;
    long           SustainLevel;
    int            SustainModeExp;
    long           SustainModeDec;
    long           SustainTime;
    int            ReleaseModeExp;
    unsigned long  ReleaseVal;
    long           ReleaseTime;
    long           ReleaseStartTime;
    long           ReleaseVol;
    long           lTime;
    long           lVolume; // from -1024 to 1023
};

struct ADSRInfoEx
{
    int            State;
    bool           AttackModeExp;
    int            AttackRate;
    int            DecayRate;
    int            SustainLevel;
    bool           SustainModeExp;
    int            SustainIncrease;
    int            SustainRate;
    bool           ReleaseModeExp;
    int            ReleaseRate;
    int            EnvelopeVol;
    long           lVolume;
    long           lDummy1;
    long           lDummy2;
};

struct ChannelInfo
{
    bool            bNew;                               // start flag

    int             iSBPos;                             // mixing stuff
    int             spos;
    int             sinc;
    int             SB[32+32];                          // sound bank
    int             sval;

    unsigned char*  pStart;                             // start ptr into sound mem
    unsigned char*  pCurr;                              // current pos in sound mem
    unsigned char*  pLoop;                              // loop ptr in sound mem

    bool            isOn;                                // is channel active (sample playing?)
    bool            isStopped;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
    bool            hasReverb;                            // can we do reverb on this channel? must have ctrl register bit, to get active
    int             iActFreq;                           // current psx pitch
    int             iUsedFreq;                          // current pc pitch
    int             iLeftVolume;                        // left volume (s15)
    bool            isLeftSweep;
    bool            isLeftExpSlope;
    bool            isLeftDecreased;
    //int             iLeftVolRaw;                        // left psx volume value
    bool            ignoresLoop;                        // ignore loop bit, if an external loop address is used
    int             iMute;                              // mute mode
    int             iRightVolume;                       // right volume (s15)
    //int             iRightVolRaw;                       // right psx volume value
    bool            isRightSweep;
    bool            isRightExpSlope;
    bool            isRightDecreased;
    int             iRawPitch;                          // raw pitch (0x0000 - 0x3fff)
    int             iIrqDone;                           // debug irq done flag
    int             s_1;                                // last decoding infos
    int             s_2;
    bool            bRVBActive;                         // reverb active flag
    int             iRVBOffset;                         // reverb offset
    int             iRVBRepeat;                         // reverb repeat
    bool            bNoise;                             // noise active flag
    int            bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
    int             iRVBNum;                            // another reverb helper
    int             iOldNoise;                          // old noise val for this channel
    ADSRInfo        ADSR;                               // active ADSR settings
    ADSRInfoEx      ADSRX;                              // next ADSR settings (will be moved to active on sample start)
};


class PSF;
class PSF1;

class SPU
{
public:
    SPU();

    void Init();
    bool Open();
    void SetLength(int stop, int fade);
    bool Close();
    void Flush();

    ChannelInfo& GetChannelInfo(int ch);
    unsigned char* GetSoundBuffer() const;

    // ADSR
    void InitADSR();
    void StartADSR(ChannelInfo& ch);
    int MixADSR(ChannelInfo& ch);

    // Reverb
    void InitReverb();
    void SetReverb(unsigned short value);
    void StartReverb(ChannelInfo& ch);
    void StoreReverb(ChannelInfo& ch, int ns);

    void SetReverbDepthLeft(int depth);
    void SetReverbDepthRight(int depth);

    // Register
    unsigned short ReadRegister(uint32_t reg);
    void WriteRegister(uint32_t reg, uint16_t val);

    // DMA
    uint16_t ReadDMA(void);
    void ReadDMAMemory(uint32_t psxAddr, uint32_t size);
    void WriteDMA(uint16_t);
    void WriteDMAMemory(uint32_t psxAddr, uint32_t size);

    // IRQ
    uint32_t GetIRQAddress() const;
    void SetIRQAddress(uint32_t addr);

    // substance
    static SPU Spu;

protected:
    // Reverb
    int ReverbGetBuffer(int ofs) const;
    void ReverbSetBuffer(int ofs, int val);
    void ReverbSetBufferPlus1(int ofs, int val);
    int MixReverbLeft(int ns);
    int MixReverbRight();

private:

    static const int NSSIZE = 45;

    PSF *m_psf;


    // PSX Buffer & Addresses
    unsigned short m_regArea[10000];
    unsigned short m_spuMem[256*1024];
public:
    uint8_t* const Memory;   // uchar* version of spuMem
private:
    unsigned char* m_pSpuIrq;
    unsigned char* m_pSpuBuffer;
    unsigned char* m_pMixIrq;

    // User settings
    int m_iUseXA;
    int m_iVolume;
    int m_iXAPitch;
    int m_iUseTimer;    // should be false (use thread)
    int m_iSPUIRQWait;
    int m_iDebugMode;
    int m_iRecordMode;
    int m_iUseReverb;
    int m_iUseInterpolation;
    int m_iDisStereo;
    int m_iUseDBufIrq;

    // MAIN infos struct for each channel
    ChannelInfo channels[25];
public:
    REVERBInfo Reverb;

private:
    unsigned long m_noiseVal;

public:
    unsigned short Sp0;
    unsigned short Status;
    uint32_t IrqAddr;
    unsigned int Addr;
private:
    bool m_bEndThread;
    bool m_bThreadEnded;
    bool m_bSpuInit;
    bool m_bSpuIsOpen;

    // unsigned long m_newChannel;

    void (*irqCallback)();
    void (*cddavCallback)(unsigned short, unsigned short);
    void (*irqQSound)(unsigned char*, long*, long);

    int f[5][2];

    int m_SSumR[NSSIZE];
    int m_SSumL[NSSIZE];
    int m_iFMod[NSSIZE];
    int m_iCycle;
    short *m_pS;


    // ADSR
    unsigned long m_RateTable[160];

    // Reverb
    int *sReverbPlay;
    int *sReverbEnd;
    int *sReverbStart;
    int  iReverbOff;    // reverb offset
    int  iReverbRepeat;
    int  iReverbNum;

};


inline ChannelInfo& SPU::GetChannelInfo(int ch) {
    return channels[ch];
}

inline unsigned char* SPU::GetSoundBuffer() const {
    return Memory;
}

inline void SPU::SetReverbDepthLeft(int depth) {
    Reverb.VolLeft = depth;
}

inline void SPU::SetReverbDepthRight(int depth) {
    Reverb.VolRight = depth;
}

inline uint32_t SPU::GetIRQAddress() const {
    return IrqAddr;
}

inline void SPU::SetIRQAddress(uint32_t addr) {
    IrqAddr = addr;
    m_pSpuIrq = GetSoundBuffer() + addr;
}


}   // namespace SPU

extern SPU::SPU& Spu;
