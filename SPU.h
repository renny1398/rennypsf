#pragma once
#include <stdint.h>

namespace SPU
{

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

struct Channel
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
    int             iLeftVolume;                        // left volume
    bool            isLeftSweep;
    bool            isLeftExpSlope;
    bool            isLeftDecreased;
    //int             iLeftVolRaw;                        // left psx volume value
    bool            ignoresLoop;                        // ignore loop bit, if an external loop address is used
    int             iMute;                              // mute mode
    int             iRightVolume;                       // right volume
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
    bool            bFMod;                              // freq mod (0=off, 1=sound channel, 2=freq channel)
    int             iRVBNum;                            // another reverb helper
    int             iOldNoise;                          // old noise val for this channel
    ADSRInfo        ADSR;                               // active ADSR settings
    ADSRInfoEx      ADSRX;                              // next ADSR settings (will be moved to active on sample start)
};

struct REVERBInfo
{
    int iStartAddr;      // reverb area start addr in samples
    int iCurrAddr;       // reverb area curr addr in samples

    int VolLeft;
    int VolRight;
    int iLastRVBLeft;
    int iLastRVBRight;
    int iRVBLeft;
    int iRVBRight;


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

}   // namespace SPU

class PSF;
class PSF1;

class SPU
{
public:
    SPU(PSF* psf);

    void Init();
    bool Open();
    void SetLength(int stop, int fade);
    bool Close();
    void Flush();


    // ADSR
    void InitADSR();
    void StartADSR(Channel& ch);
    int MixADSR(Channel& ch);

    // Reverb
    void InitReverb();
    void SetReverb(unsigned short value);
    void StartReverb(Channel& ch);
    void StoreReverb(Channel& ch, int ns);

    // Register
    unsigned short ReadRegister(unsigned long reg);
    void WriteRegister(unsigned long reg, unsigned short val);

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
    unsigned char* m_spuMemC;
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
    Channel m_spuChannels[25];
    REVERBInfo m_reverbInfo;

    unsigned long m_noiseVal;

    unsigned short m_spuCtrl;
    unsigned short m_spuState;
    unsigned short m_spuIrq;
    unsigned long m_spuAddr;
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
