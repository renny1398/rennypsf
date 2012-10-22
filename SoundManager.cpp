#include "SoundManager.h"
#include "SoundFormat.h"

const int NUM_BUFFERS = 8;


//////////////////////////////////////////////////////////////////////
// SoundSource definitions
//////////////////////////////////////////////////////////////////////

SoundManager::SoundSource::SoundSource(): cursor(0), envVolume(0)
{
    alGenSources(1, &source);
}

SoundManager::SoundSource::~SoundSource()
{
    Stop();
    alDeleteSources(1, &source);
}

void SoundManager::SoundSource::write()
{
    ALint state, n;

    alGetSourcei(source, AL_BUFFERS_QUEUED, &n);
    if (n < NUM_BUFFERS) {
        alGenBuffers(1, &buffer);
    } else {
        while (alGetSourcei(source, AL_BUFFERS_PROCESSED, &n), n == 0) {
            usleep(1000);
        }
        alSourceUnqueueBuffers(source, 1, &buffer);
    }

    alBufferData(buffer, AL_FORMAT_STEREO16, lpcm, 4096*4, 44100);
    alSourceQueueBuffers(source, 1, &buffer);
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
    }
}

void SoundManager::SoundSource::WriteStereo(int left, int right)
{
    lpcm[2*cursor+0] = left;
    lpcm[2*cursor+1] = right;
    if (++cursor >= 4096) {
        write();
    }
}

void SoundManager::SoundSource::Stop()
{
    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_STOPPED) {
        alSourceStop(source);
    }
    do {
        alSourceUnqueueBuffers(source, 1, &buffer);
        if (alGetError() != AL_NO_ERROR) break;
        alDeleteBuffers(1, &buffer);
    } while (true);
    envVolume = 0;
}



//////////////////////////////////////////////////////////////////////
// SoundManager definitions
//////////////////////////////////////////////////////////////////////

SoundManager::SoundManager(): m_sound(0)
{
    device = alcOpenDevice(0);
    context = alcCreateContext(device, 0);
    alcMakeContextCurrent(context);

    sources.reserve(64);
    for (int i = 0; i < 24; i++) {
        sources.push_back(SoundSource());
    }
}


bool SoundManager::Play(SoundFormat *sound)
{
    if (m_sound) {
        m_sound->Stop();
    }
    if (sound->Play() == false) {
        return false;
    }
    m_sound = sound;

    //alGenSources(1, &source);

    return true;
}


bool SoundManager::Stop()
{
    SoundFormat *sound = m_sound;
    if (sound == 0) return true;
    if (sound->Stop() == false) {
        return false;
    }
    m_sound = 0;

    //alSourceStop(source);

    return true;
}


/*
void SoundManager::WriteBuffer(unsigned char *pSound, int lBytes)
{
    ALint state, n;

    alGetSourcei(source, AL_BUFFERS_QUEUED, &n);
    if (n < NUM_BUFFERS) {
        alGenBuffers(1, &buffer);
    } else {
        while (alGetSourcei(source, AL_BUFFERS_PROCESSED, &n), n == 0) {
            usleep(1000);
        }
        alSourceUnqueueBuffers(source, 1, &buffer);
    }
    if (pSound == 0) {
        // qDebug("WARNING: sound buffer is null.");
        return;
    }
    alBufferData(buffer, AL_FORMAT_STEREO16, pSound, lBytes, 44100);
    alSourceQueueBuffers(source, 1, &buffer);
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
    }
}
*/

void SoundManager::WriteStereo(int ch, int left, int right)
{
    sources.at(ch).WriteStereo(left, right);
}
