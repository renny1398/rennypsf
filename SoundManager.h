#pragma once
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <wx/vector.h>


class Sample {
public:
    short left;
    short right;

    Sample() : left(0), right(0) {}
    Sample(short l, short r) : left(l), right(r) {}

    operator short*() { return reinterpret_cast<short*>(this); }
};



class SoundFormat;

class SoundDriver
{
public:
    SoundDriver(int channelNumber);
    virtual ~SoundDriver();

    virtual bool Play(SoundFormat*);
    virtual bool Stop();

    // void WriteBuffer(unsigned char * pSound, int lBytes);
    // virtual void Write(int left, int right) = 0;
    void WriteStereo(int ch, short left, short right);
    void WriteStereo(int ch, short samples[2]);

    void Flush();

    // int GetChannelNumber() const;

    virtual int GetEnvelopeVolume(int ch) const;
    virtual void SetEnvelopeVolume(int ch, int vol);

protected:
    void setChannelNumber(int number);
    void setBufferSize(int size);
    virtual void writeToDevice(short* buffer, int size) = 0;

protected:
    SoundFormat *m_sound;

private:
    int leftSample_, rightSample_;
    Sample* chSample_;
    int *chEnvelope_;
    // int *chEnvelopeLeft_, *chEnvelopeRight_;
    int channelNumber_;
    Sample* buffer_;
    int bufferSize_;
    int bufferIndex_;
};


class WaveOutAL: public SoundDriver
{
public:
    WaveOutAL(int channelNumber);
    ~WaveOutAL();

    void Init();
    bool Stop();
    void Shutdown();

    // int GetEnvelopeVolume(int ch) const;
    // void SetEnvelopeVolume(int ch, int vol);

protected:
    void writeToDevice(short *data, int size);

private:
    static ALCdevice *device_;
    static ALCcontext *context_;
    static int source_number_;

    ALuint buffer_, source_;
};


#include <wx/file.h>

class WaveOutDisk: public SoundDriver
{
public:
    WaveOutDisk(int channelNumber);

    bool Play(SoundFormat *);
    bool Stop();

private:
    struct WaveOutFormat
    {
        WaveOutFormat();
        char RIFF[4];
        int size;
        char WAVE[4];
        char fmt[4];
        int fmtSize;
        short formatId;
        short channelNumber;
        int samplingRate;
        int dataRate;
        short blockSize;
        short bitNumber;
        char DATA[4];
        int dataSize;
    };

    wxString fileName_;
    wxFile file_;
    WaveOutFormat format_;
};
