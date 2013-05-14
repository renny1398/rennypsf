#pragma once
#include <stdint.h>
#include <wx/ptr_scpd.h>
#include <wx/thread.h>

#include "reverb.h"
#include "interpolation.h"

namespace SPU
{
/*
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
*/

enum ADSR_STATE {
    ADSR_STATE_ATTACK,
    ADSR_STATE_DECAY,
    ADSR_STATE_SUSTAIN,
    ADSR_STATE_RELEASE
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


class ADSRInfoEx
{
public:
    void Start();

public:

    ADSR_STATE     State;
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

class AbstractInterpolation;

wxDECLARE_SCOPED_PTR(AbstractInterpolation, InterpolationPtr)

class ChannelInfo
{
public:
    ChannelInfo();
    void StartSound();
    static void InitADSR();
    int MixADSR();

    void Update();
protected:
    void VoiceChangeFrequency();
    void ADPCM2LPCM();

public:
    int ch;

    bool            bNew;                               // start flag

    int             iSBPos;                             // mixing stuff
    //int             spos;
    //int             sinc;
    //int             SB[32+32];                          // sound bank
    InterpolationPtr pInterpolation;
    int32_t         LPCM[28];

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

private:
    static uint32_t rateTable[160];
};



class IChannelArray
{
public:
    virtual ~IChannelArray() {}

    virtual int GetChannelNumber() const = 0;
};

class ChannelArray: public IChannelArray
{
public:
    ChannelArray();

    int GetChannelNumber() const {
        return 24;
    }

    bool ExistsNew() const;
    void SoundNew(uint32_t flags);

    ChannelInfo& operator[](int i);

private:
    ChannelInfo channels[25];
    uint32_t newChannelFlags;

    friend class ChannelInfo;
};


inline ChannelArray::ChannelArray()
{
    for (int i = 0; i < 25; i++) {
        channels[i].ch = i;
    }
}


inline bool ChannelArray::ExistsNew() const {
    return (newChannelFlags != 0);
}


inline void ChannelArray::SoundNew(uint32_t flags)
{
    wxASSERT(flags < 0x01000000);
    newChannelFlags |= flags;
    for (int i = 0; flags != 0; i++, flags >>= 1) {
        if (!(flags & 1) || (channels[i].pStart == 0)) continue;
        channels[i].ignoresLoop = false;
        channels[i].bNew = true;
    }
}

inline ChannelInfo& ChannelArray::operator [](int i) {
    wxASSERT(0 <= i && i < 24);
    return channels[i];
}


////////////////////////////////////////////////////////////////////////
// Enums
////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////
// SPU Thread
////////////////////////////////////////////////////////////////////////

class SPU;

class SPUThread : public wxThread
{
public:
    SPUThread(SPU* pSPU);

protected:
    virtual void* Entry();

private:
    SPU* pSPU_;
    int numSamples_;

    friend class SPU;
};



////////////////////////////////////////////////////////////////////////
// SPU interface (experimental)
////////////////////////////////////////////////////////////////////////

class ISPU
{
public:
    virtual ~ISPU() {}
    virtual uint32_t GetDefaultSamplingRate() const = 0;
};


class PSF;
class PSF1;

class SPU: public ISPU
{
public:
    SPU();

    void Init();
    void Open();
    void SetLength(int stop, int fade);
    void Close();
    void Flush();

    bool IsRunning() const;

protected:
    void SetupStreams();
    void RemoveStreams();

public:
    void Async(uint32_t);

    uint32_t GetDefaultSamplingRate() const { return 44100; }

    ChannelInfo& GetChannelInfo(int ch);
    unsigned char* GetSoundBuffer() const;

    // ADSR
    // void InitADSR();
    // void StartADSR(ChannelInfo& ch);
    // int MixADSR(ChannelInfo& ch);

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

    //
    // Utilities
    //
public:
    int GetPCMNumber() const;
    int GetPCM(int index, void* pcm, int* loop) const;


    // Multiprocess
    void GetReadyToSync();
    void ProcessSamples(int numSamples);
    friend class SPUThread;

private:

    static const int NSSIZE = 45;

    PSF *m_psf;

    bool isPlaying_;    // only used on multithread mode??

    // PSX Buffer & Addresses
    // unsigned short m_regArea[10000];
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
    InterpolationType useInterpolation;
    int m_iDisStereo;
    int m_iUseDBufIrq;

    // MAIN infos struct for each channel
public:
    ChannelArray Channels;
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
    // unsigned long m_RateTable[160];

    // Reverb
    int *sReverbPlay;
    int *sReverbEnd;
    int *sReverbStart;
    int  iReverbOff;    // reverb offset
    int  iReverbRepeat;
    int  iReverbNum;

    // DMA
    wxCriticalSection csDMAReadable_;
    wxCriticalSection csDMAWritable_;

    // Multithread
    SPUThread* thread_;
    bool isReadyToProcess_;
    wxMutex mutexReadyToProcess_;
    wxCondition condReadyToProcess_;
    wxMutex mutexProcessSamples_;
    wxCondition condProcessSamples_;

    friend class ChannelInfo;

    // Utilities
    struct PCMInfo {
        int addr;
        int loop;
        int end;
        bool muted;
    };
    PCMInfo pcmInfo_[256];
    int nPCM_;
};


inline ChannelInfo& SPU::GetChannelInfo(int ch) {
    return Channels[ch];
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
