#include "SoundManager.h"
#include "SoundFormat.h"

#include <iostream>

const int NUM_BUFFERS = 8;
const int NSSIZE = 45;


//////////////////////////////////////////////////////////////////////
// SoundManager definitions
//////////////////////////////////////////////////////////////////////


void SoundDriver::setChannelNumber(int number)
{
    if (chSample_) {
        delete [] chEnvelope_;
        delete [] chSample_;
    }
    leftSample_ = 0;
    rightSample_ = 0;
    chSample_ = new Sample[number];
    chEnvelope_ = new int[number];
    channelNumber_ = number;
}

void SoundDriver::setBufferSize(int size)
{
    if (buffer_) {
        delete [] buffer_;
    }
    buffer_ = new Sample[size];
    bufferSize_ = size;
    bufferIndex_ = 0;
}


int SoundDriver::GetEnvelopeVolume(int ch) const
{
    return chEnvelope_[ch];
}

void SoundDriver::SetEnvelopeVolume(int ch, int vol)
{
    chEnvelope_[ch] = vol;
}


SoundDriver::SoundDriver(int channelNumber)
    : m_sound(0), chSample_(0), buffer_(0)
{
    setChannelNumber(channelNumber);
    setBufferSize(512);
}

SoundDriver::~SoundDriver()
{
    if (buffer_) delete [] buffer_;
    if (chSample_) {
        delete [] chEnvelope_;
        delete [] chSample_;
    }
}


bool SoundDriver::Play(SoundFormat *sound)
{
    if (m_sound) {
        m_sound->Stop();
    }
    if (sound->Play() == false) {
        return false;
    }
    m_sound = sound;
    return true;
}


bool SoundDriver::Stop()
{
    SoundFormat *sound = m_sound;
    if (sound == 0) return true;
    if (sound->Stop() == false) {
        return false;
    }
    m_sound = 0;
    return true;
}


void SoundDriver::WriteStereo(int ch, short left, short right)
{
    wxASSERT(chSample_ != 0 && ch < channelNumber_);
    chSample_[ch].left = left;
    chSample_[ch].right = right;
    leftSample_ += left;
    rightSample_ += right;
}

void SoundDriver::WriteStereo(int ch, short samples[2])
{
    wxASSERT(chSample_ != 0 && ch < channelNumber_);
    chSample_[ch].left = samples[0];
    chSample_[ch].right = samples[1];
    leftSample_ += samples[0];
    rightSample_ += samples[1];
}


void SoundDriver::Flush()
{
    if (leftSample_ > 32767) {
        leftSample_ = 32767;
    } else if (leftSample_ < -32768) {
        leftSample_ = -32768;
    }
    if (rightSample_ > 32767) {
        rightSample_ = 32767;
    } else if (rightSample_ < -32768) {
        rightSample_ = -32768;
    }
    buffer_[bufferIndex_].left = leftSample_;
    buffer_[bufferIndex_].right = rightSample_;
    bufferIndex_++;
    leftSample_ = 0;
    rightSample_ = 0;
    if (bufferIndex_ >= bufferSize_) {
        writeToDevice((short*)buffer_, bufferIndex_);
        bufferIndex_ = 0;
    }
}



////////////////////////////////////////////////////////////////////////
// WaveOutAL
////////////////////////////////////////////////////////////////////////


ALCdevice *WaveOutAL::device_ = NULL;
ALCcontext *WaveOutAL::context_ = NULL;
int WaveOutAL::source_number_ = 0;


WaveOutAL::WaveOutAL(int channelNumber)
    :SoundDriver(channelNumber)
{
    Init();
}


WaveOutAL::~WaveOutAL()
{
    Shutdown();
}


void WaveOutAL::Init()
{
    if (device_ == NULL) {
        device_ = alcOpenDevice(0);
        context_ = alcCreateContext(device_, 0);
        alcMakeContextCurrent(context_);
    }
    alGenSources(1, &source_);
    source_number_++;
}


void WaveOutAL::Shutdown()
{
    if (source_ == 0) return;
    Stop();
    alDeleteSources(1, &source_);
    // source = 0;
    if (--source_number_ <= 0) {
        alcMakeContextCurrent(0);
        alcDestroyContext(context_);
        alcCloseDevice(device_);
        std::cout << "WaveOutAL: Closed a sound device." << std::endl;
    }
}

void WaveOutAL::writeToDevice(short *data, int size)
{
    ALint state, n;

    alGetSourcei(source_, AL_BUFFERS_QUEUED, &n);
    if (n < NUM_BUFFERS) {
        alGenBuffers(1, &buffer_);
    } else {
        alGetSourcei(source_, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            alSourcePlay(source_);
            std::cout << "WaveOutAL: Started playing. source =" << source_ << std::endl;
        }
        while (alGetSourcei(source_, AL_BUFFERS_PROCESSED, &n), n == 0) {
            usleep(100);
        }
        alSourceUnqueueBuffers(source_, 1, &buffer_);
    }
    alBufferData(buffer_, AL_FORMAT_STEREO16, data, size*4, 44100);
    alSourceQueueBuffers(source_, 1, &buffer_);
}


bool WaveOutAL::Stop()
{
    SoundDriver::Stop();
    ALint state, n;
    alGetSourcei(source_, AL_SOURCE_STATE, &state);
    if (state != AL_STOPPED) {
        alSourceStop(source_);
        std::cout << "WaveOutAL: Stopped playing.";
    }
    while (alGetSourcei(source_, AL_SOURCE_STATE, &state), state == AL_PLAYING) {
        usleep(100);
    }
    while (alGetSourcei(source_, AL_BUFFERS_PROCESSED, &n), n > 0) {
        alSourceUnqueueBuffers(source_, 1, &buffer_);
        alDeleteBuffers(1, &buffer_);
    }
    // envVolume = 0;
    return true;
}



////////////////////////////////////////////////////////////////////////
// WaveOutDisk
////////////////////////////////////////////////////////////////////////


WaveOutDisk::WaveOutFormat::WaveOutFormat()
{
    RIFF[0] = 'R';
    RIFF[1] = 'I';
    RIFF[2] = 'F';
    RIFF[3] = 'F';

    WAVE[0] = 'W';
    WAVE[1] = 'A';
    WAVE[2] = 'V';
    WAVE[3] = 'E';

    fmt[0] = 'f';
    fmt[1] = 'm';
    fmt[2] = 't';
    fmt[3] = ' ';

    fmtSize = 16;
    formatId = 1;

    DATA[0] = 'd';
    DATA[1] = 'a';
    DATA[2] = 't';
    DATA[3] = 'a';
}


WaveOutDisk::WaveOutDisk(int channelNumber)
    : SoundDriver(channelNumber)
{
}



bool WaveOutDisk::Play(SoundFormat *soundFormat)
{
    if (SoundDriver::Play(soundFormat) == false) return false;
    file_.Create(soundFormat->GetFileName(), true, wxFile::write);

    format_.channelNumber = 2;
    format_.samplingRate = 44100;
    format_.dataRate = 44100*2*2;
    format_.blockSize = 2*2;
    format_.bitNumber = 16;
    format_.dataSize = 0;

    file_.Write(&format_, sizeof(format_));

    return true;
}


bool WaveOutDisk::Stop()
{
    if (SoundDriver::Stop() == false) return false;
    return true;
}
