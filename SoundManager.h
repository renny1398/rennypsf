#pragma once
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <wx/vector.h>



class SoundFormat;

class SoundManager
{
public:
    SoundManager();

    bool Play(SoundFormat*);
    bool Stop();

    // void WriteBuffer(unsigned char * pSound, int lBytes);
    void WriteStereo(int ch, int left, int right);

    int GetEnvelopeVolume(int ch) const;
    void SetEnvelopeVolume(int ch, int vol);

private:
    class SoundSource {
    public:
        SoundSource();
        ~SoundSource();
        // void Write(int mono);
        void WriteStereo(int left, int right);
        void Stop();
    protected:
        void write();

    public:
        int GetEnvelopeVolume() const;
        void SetEnvelopeVolume(int);

    private:
        ALuint source, buffer;
        short lpcm[4096*2];
        int cursor;
        int envVolume;
    };

    SoundFormat *m_sound;

    ALCdevice *device;
    ALCcontext *context;
    //ALuint buffer, source;
    wxVector<SoundSource> sources;
};


inline int SoundManager::SoundSource::GetEnvelopeVolume() const {
    return envVolume;
}

inline void SoundManager::SoundSource::SetEnvelopeVolume(int vol) {
    envVolume = vol;
}

inline int SoundManager::GetEnvelopeVolume(int ch) const {
    return sources.at(ch).GetEnvelopeVolume();
}

inline void SoundManager::SetEnvelopeVolume(int ch, int vol) {
    sources.at(ch).SetEnvelopeVolume(vol);
}
