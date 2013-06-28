#pragma once
#include <stdint.h>
#include <pthread.h>
#include <set>

#include <wx/ptr_scpd.h>
#include <wx/thread.h>
#include <wx/hashset.h>

#include "reverb.h"
#include "interpolation.h"
#include "soundbank.h"


class wxEvtHandler;


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


typedef uint32_t SPUAddr;


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



class ChannelInfo;

class ChannelInfoListener {

    friend class ChannelInfo;

public:
    virtual ~ChannelInfoListener() {}

protected:
    virtual void OnNoteOn(const ChannelInfo&) {}
    virtual void OnNoteOff(const ChannelInfo&) {}
};




class InterpolationBase;

wxDECLARE_SCOPED_PTR(InterpolationBase, InterpolationPtr)

class ChannelArray;

class ChannelInfo
{
public:
    ChannelInfo(SPU* pSPU = 0);
    void StartSound();
    static void InitADSR();
    int MixADSR();

    void Update();
protected:
    void VoiceChangeFrequency();
    // void ADPCM2LPCM();

private:
    SPU* pSPU_;

public:
    int ch;
    wxScopedArray<short> lpcm_buffer_l;
    wxScopedArray<short> lpcm_buffer_r;

    bool            bNew;                               // start flag
    // bool            isUpdating;
    bool is_ready;

    int             iSBPos;                             // mixing stuff
    //int             spos;
    //int             sinc;
    //int             SB[32+32];                          // sound bank
    InterpolationPtr  pInterpolation;
    // int32_t         LPCM[28];

    int             sval;
    SamplingTone* tone;
    SamplingToneIterator itrTone;

    bool            isOn;                                // is channel active (sample playing?)
    bool            isStopped;                              // is channel stopped (sample _can_ still be playing, ADSR Release phase)
    bool            is_muted;
    bool            hasReverb;                            // can we do reverb on this channel? must have ctrl register bit, to get active
    int             iActFreq;                           // current psx pitch
    int             iUsedFreq;                          // current pc pitch
    double          Pitch;
    int             iLeftVolume;                        // left volume (s15)
    bool            isLeftSweep;
    bool            isLeftExpSlope;
    bool            isLeftDecreased;
    //int             iLeftVolRaw;                        // left psx volume value
    SPUAddr    addrExternalLoop;
    bool            useExternalLoop;                        // ignore loop bit, if an external loop address is used
    // int             iMute;                              // mute mode
    int             iRightVolume;                       // right volume (s15)
    //int             iRightVolRaw;                       // right psx volume value
    bool            isRightSweep;
    bool            isRightExpSlope;
    bool            isRightDecreased;
    int             iRawPitch;                          // raw pitch (0x0000 - 0x3fff)
    // int             iIrqDone;                           // debug irq done flag
    bool            bRVBActive;                         // reverb active flag
    int             iRVBOffset;                         // reverb offset
    int             iRVBRepeat;                         // reverb repeat
    bool            bNoise;                             // noise active flag
    int            bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
    int             iRVBNum;                            // another reverb helper
    int             iOldNoise;                          // old noise val for this channel
    ADSRInfo        ADSR;                               // active ADSR settings
    ADSRInfoEx      ADSRX;                              // next ADSR settings (will be moved to active on sample start)


    void AddListener(ChannelInfoListener* listener);
    void RemoveListener(ChannelInfoListener* listener);

    void NotifyOnNoteOn() const;
    void NotifyOnNoteOff() const;

private:
    static uint32_t rateTable[160];

    std::set<ChannelInfoListener*> listeners_;

    friend class ChannelArray;
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
    ChannelArray(SPU* pSPU, int channelNumber);

    int GetChannelNumber() const;
    bool ExistsNew() const;
    void SoundNew(uint32_t flags, int start);
    void VoiceOff(uint32_t flags, int start);

    ChannelInfo& operator[](int i);

private:
    SPU* pSPU_;
    ChannelInfo* channels_;
    int channelNumber_;
    uint32_t flagNewChannels_;

    friend class ChannelInfo;
};




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


WX_DECLARE_HASH_SET(wxEvtHandler*, wxPointerHash, wxPointerEqual, SPUListener);


class SPUBase
{
public:
    virtual ~SPUBase() {}
    virtual uint32_t GetDefaultSamplingRate() const = 0;

    void AddListener(wxEvtHandler* listener);
    void RemoveListener(wxEvtHandler* listener);

    // Notify functions
    void NotifyOnUpdateStartAddress(int ch) const;
    void NotifyOnChangeLoopIndex(ChannelInfo* pChannel) const;

protected:
    mutable pthread_mutex_t process_mutex_;
    mutable pthread_mutex_t wait_start_mutex_;
    mutable pthread_cond_t process_cond_;
    mutable pthread_mutex_t dma_writable_mutex_;

    enum ProcessState {
        STATE_SHUTDOWN = -1,
        STATE_PSX_IS_READY = 0,
        STATE_START_PROCESS,
        STATE_NOTE_ON,
        STATE_NOTE_OFF,
        STATE_SET_OFFSET,
        STATE_NONE
    };

    mutable ProcessState process_state_;
    mutable int processing_channel_;

    void ChangeProcessState(ProcessState state, int ch = -1);

private:
    SPUListener listeners_;
};



class PSF;
class PSF1;

class SPU: public SPUBase
{
public:
    SPU();

    void Init();
    void Open();
    void SetLength(int stop, int fade);
    void Close();
    void Shutdown();
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
    void SetIRQAddress(SPUAddr addr);

    // substance
    static SPU Spu;

    //
    // Utilities
    //
public:
    SamplingTone* GetSamplingTone(uint32_t addr) const;
    int GetPCMNumber() const;
    int GetPCM(int index, void* pcm, int* loop) const;

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

public:
    unsigned char* m_pSpuIrq;
    unsigned char* m_pSpuBuffer;
    unsigned char* m_pMixIrq;

private:
    // User settings
    // int m_iUseXA;
    //int m_iVolume;
    //int m_iXAPitch;
    //int m_iUseTimer;    // should be false (use thread)
    //int m_iSPUIRQWait;
    //int m_iDebugMode;
    //int m_iRecordMode;
    int m_iUseReverb;
    InterpolationType useInterpolation;
    //int m_iDisStereo;
    //int m_iUseDBufIrq;

    // MAIN infos struct for each channel
public:
    ChannelArray Channels;
    REVERBInfo Reverb;

private:
    unsigned long m_noiseVal;

public:
    unsigned short Sp0;
    unsigned short Status;
    SPUAddr IrqAddr;
    SPUAddr Addr;
private:
    bool m_bEndThread;
    bool m_bThreadEnded;
    bool m_bSpuInit;
    bool m_bSpuIsOpen;

    // unsigned long m_newChannel;

    // void (*irqCallback)();
    // void (*cddavCallback)(unsigned short, unsigned short);
    // void (*irqQSound)(unsigned char*, long*, long);

    // int f[5][2];

    // int m_SSumR[NSSIZE];
    // int m_SSumL[NSSIZE];
    int m_iFMod[NSSIZE];
    int m_iCycle;
    // short *m_pS;

    int ns;


    // ADSR
    // unsigned long m_RateTable[160];

    // Sound bank
public:
    SoundBank SoundBank_;

    // DMA
private:
    wxCriticalSection csDMAReadable_;
    wxCriticalSection csDMAWritable_;

    // Multithread
    SPUThread* thread_;
    // bool isReadyToProcess_;
    // wxMutex mutexReadyToProcess_;
    // wxCondition condReadyToProcess_;

    // wxMutex mutexUpdate_;
    // wxCondition condUpdate_;


    friend class ChannelInfo;
    friend class ChannelArray;  // temp
    friend class REVERBInfo;    // temp
};


/*
class SPUListener {
public:
    virtual ~SPUListener() {}

protected:

    void onUpdate(SPU*);
};
*/



inline ChannelInfo& SPU::GetChannelInfo(int ch) {
    return Channels[ch];
}

inline unsigned char* SPU::GetSoundBuffer() const {
    // return Memory;
    return (uint8_t*)m_spuMem;
}

inline uint32_t SPU::GetIRQAddress() const {
    return IrqAddr;
}

inline void SPU::SetIRQAddress(SPUAddr addr) {
    IrqAddr = addr;
    m_pSpuIrq = GetSoundBuffer() + addr;
}


}   // namespace SPU


#include <wx/event.h>

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVENT_SPU_CHANNEL_CHANGE_LOOP, wxCommandEvent);


extern SPU::SPU& Spu;
