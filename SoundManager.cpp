#include "SoundManager.h"
#include "SoundFormat.h"

#include <iostream>

const int NUM_BUFFERS = 50;
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
    setBufferSize(NSSIZE);
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


const Sample* SoundDriver::GetBuffer(int *size) const {
    wxASSERT(size);
    *size = bufferIndex_;
    return buffer_;
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
        WriteToDevice();
        bufferIndex_ = 0;
    }
}



////////////////////////////////////////////////////////////////////////
// Command for WaveOutAL Thread
////////////////////////////////////////////////////////////////////////



class WaveOutALCommand {
public:
    WaveOutALCommand(WaveOutAL* sound_driver)
        : sound_driver_(sound_driver) {}
    virtual ~WaveOutALCommand() {}
    virtual void Execute() = 0;

protected:
    WaveOutAL* const sound_driver_;
};




class InitAL : public WaveOutALCommand {
public:
    InitAL(WaveOutAL* sound_driver)
        : WaveOutALCommand(sound_driver) {}
    virtual void Execute() {
        sound_driver_->ThisThreadInit();
    }
};


class ShutdownAL : public WaveOutALCommand {
public:
    ShutdownAL(WaveOutAL* sound_driver)
        : WaveOutALCommand(sound_driver) {}
    virtual void Execute() {
        sound_driver_->ThisThreadShutdown();
    }
};


class StopAL : public WaveOutALCommand {
public:
    StopAL(WaveOutAL* sound_driver)
        : WaveOutALCommand(sound_driver) {}
    virtual void Execute() {
        sound_driver_->ThisThreadStop();
    }
};


class WriteToDeviceAL : public WaveOutALCommand {
public:
    WriteToDeviceAL(WaveOutAL* sound_driver)
        : WaveOutALCommand(sound_driver) {}
    virtual void Execute() {
        sound_driver_->ThisThreadWriteToDevice();
    }
};


wxThread::ExitCode WaveOutALThread::Entry() {

    WaveOutALCommand* msg;

    do {
        command_queue_.Receive(msg);
        msg->Execute();
        delete msg;
        if (WaveOutAL::device_ == 0) break;
    } while (true);

    sound_driver_->thread_ = 0;
    return 0;
}



////////////////////////////////////////////////////////////////////////
// WaveOutAL
////////////////////////////////////////////////////////////////////////


ALCdevice *WaveOutAL::device_ = NULL;
ALCcontext *WaveOutAL::context_ = NULL;
int WaveOutAL::source_number_ = 0;
WaveOutALThread* WaveOutAL::thread_ = NULL;


WaveOutAL::WaveOutAL(int channelNumber)
    : SoundDriver(channelNumber),
      write_to_device_cond_(mutex_), write_to_device_cond2_(mutex2_),
      finished_writing_(false)
{
    if (thread_ == 0) {
        thread_ = new WaveOutALThread(this);
        thread_->Create();
        thread_->Run();
    }
    Init();
}


WaveOutAL::~WaveOutAL()
{
    Shutdown();
}


void WaveOutAL::ThisThreadInit()
{
    if (device_ == NULL) {
        device_ = alcOpenDevice(0);
        context_ = alcCreateContext(device_, 0);
        alcMakeContextCurrent(context_);
    }
    // alGenSources(1, &source_);
    source_ = 0;
    source_number_++;
    std::cout << "WaveOutAL: Initialized a sound device." << std::endl;
}


void WaveOutAL::Init() {
    thread_->PostMessageQueue(new InitAL(this));
}


void WaveOutAL::ThisThreadShutdown()
{
    if (source_ == 0) return;
    Stop();
    // alDeleteSources(1, &source_);
    //source_ = 0;
    if (--source_number_ <= 0) {
        alcMakeContextCurrent(0);
        alcDestroyContext(context_);
        alcCloseDevice(device_);
        device_ = 0;
        std::cout << "WaveOutAL: Closed a sound device." << std::endl;
    }
}


void WaveOutAL::Shutdown() {
    thread_->PostMessageQueue(new ShutdownAL(this));
}


void WaveOutAL::ThisThreadWriteToDevice()
{
    ALint state, n;

    int size;
    const short* data = reinterpret_cast<const short*>(GetBuffer(&size));

    if (source_ == 0) {
        alGenSources(1, &source_);
    }

    alGetSourcei(source_, AL_BUFFERS_QUEUED, &n);
    if (n < NUM_BUFFERS) {
        alGenBuffers(1, &buffer_);
    } else {
        alGetSourcei(source_, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            alSourcePlay(source_);
            std::cout << "WaveOutAL: Started playing. source = " << source_ << std::endl;
        }
        while (alGetSourcei(source_, AL_BUFFERS_PROCESSED, &n), n == 0) {
            usleep(1000);
        }
        alSourceUnqueueBuffers(source_, 1, &buffer_);
    }
    alBufferData(buffer_, AL_FORMAT_STEREO16, data, size*4, 44100);
    alSourceQueueBuffers(source_, 1, &buffer_);

    mutex_.Lock();
    finished_writing_ = true;
    write_to_device_cond_.Broadcast();
    mutex_.Unlock();
    mutex2_.Lock();
    while (finished_writing_ == true) {
        write_to_device_cond2_.Wait();
    }
    mutex2_.Unlock();
}


void WaveOutAL::WaitForWritingToDevice() {
    mutex_.Lock();
    while (finished_writing_ == false) {
        write_to_device_cond_.Wait();
    }
    mutex_.Unlock();
    mutex2_.Lock();
    finished_writing_ = false;
    write_to_device_cond2_.Broadcast();
    mutex2_.Unlock();
}



void WaveOutAL::WriteToDevice() {
    thread_->PostMessageQueue(new WriteToDeviceAL(this));
    WaitForWritingToDevice();
}



bool WaveOutAL::ThisThreadStop()
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
    alDeleteSources(1, &source_);
    source_ = 0;

    return true;
}


bool WaveOutAL::Stop() {
    thread_->PostMessageQueue(new StopAL(this));
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
