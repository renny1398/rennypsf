#include "SoundManager.h"
#include "SoundFormat.h"

#include <iostream>

const int NUM_BUFFERS = 50;
const int NSSIZE = 45;


wxDEFINE_EVENT(wxEVT_NOTE_ON, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_NOTE_OFF, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHANGE_TONE_NUMBER, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHANGE_PITCH, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHANGE_VELOCITY, wxCommandEvent);

wxDEFINE_EVENT(wxEVT_ADD_TONE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_CHANGE_TONE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_REMOVE_TONE, wxCommandEvent);

wxDEFINE_EVENT(wxEVT_MUTE_TONE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_UNMUTE_TONE, wxCommandEvent);



//////////////////////////////////////////////////////////////////////
// SoundDeviceDriver definitions
//////////////////////////////////////////////////////////////////////



SoundDeviceDriver::SoundDeviceDriver()
  : buffer_(new short[2*NSSIZE*2]), bufferSize_(NSSIZE*2), bufferIndex_(0),
    is_playing_(false), counter_(0) {}



const short* SoundDeviceDriver::buffer(int* size) const {
  wxASSERT(size != NULL);
  *size = bufferSize_;
  return buffer_.get();
}

void SoundDeviceDriver::set_buffer_length(int size)
{
  buffer_.reset(new short[2*size]);
  bufferSize_ = size;
  bufferIndex_ = 0;
}




bool SoundDeviceDriver::Play()
{
  ZeroCounter();
  is_playing_ = true;
  return true;
}


bool SoundDeviceDriver::Stop()
{
  is_playing_ = false;
  return true;
}


bool SoundDeviceDriver::IsPlaying() const {
  return is_playing_;
}


void SoundDeviceDriver::OnUpdate(const SoundBlock* block)
{
  block->GetStereo16(&buffer_[bufferIndex_*2]);
  bufferIndex_++;
  IncrementCounter();
  if (bufferIndex_ >= bufferSize_) {
    WriteToDevice();
    bufferIndex_ = 0;
  }
}



void SoundDeviceDriver::ZeroCounter() {
  counter_ = 0;
}


void SoundDeviceDriver::IncrementCounter() {
  ++counter_;
}


int SoundDeviceDriver::GetCounter() const {
  return counter_;
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

  // sound_driver_->thread_ = 0;
  wxMessageOutputDebug().Printf(wxT("WaveOutALThread will be destroyed."));
  return 0;
}



////////////////////////////////////////////////////////////////////////
// WaveOutAL
////////////////////////////////////////////////////////////////////////


ALCdevice *WaveOutAL::device_ = NULL;
ALCcontext *WaveOutAL::context_ = NULL;
int WaveOutAL::source_number_ = 0;
WaveOutALThread* WaveOutAL::thread_ = NULL;


WaveOutAL::WaveOutAL()
  : write_to_device_cond_(mutex_), write_to_device_cond2_(mutex2_),
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

//
// This function can be called only from WaveOutALThread::Entry()
//
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


//
// This function can be called only from WaveOutALThread::Entry()
//
void WaveOutAL::ThisThreadShutdown()
{
  if (source_ != 0) {
    SoundDeviceDriver::Stop();
    ThisThreadStop();
  }
  if (--source_number_ <= 0) {
    alcMakeContextCurrent(0);
    alcDestroyContext(context_);
    alcCloseDevice(device_);
    device_ = 0;  // WaveOutALThread will be destroyed
    wxMessageOutputDebug().Output(wxT("WaveOutAL: Closed WaveOutAL."));
  }
}


void WaveOutAL::Shutdown() {
  thread_->PostMessageQueue(new ShutdownAL(this));
  thread_->Wait();
  delete thread_;
  thread_ = 0;
  wxMessageOutputDebug().Output(wxT("Destroyed WaveOutAL."));
}


//
// This function can be called only from WaveOutALThread::Entry()
//
void WaveOutAL::ThisThreadWriteToDevice()
{
  if (IsPlaying()) {
    ALint state, n;

    int size;
    const short* data = buffer(&size);

    if (source_ == 0) {
      alGenSources(1, &source_);
      // alSourcef(source_, AL_GAIN, 0.25);
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
  }

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



//
// This function can be called only from WaveOutALThread::Entry()
//
bool WaveOutAL::ThisThreadStop()
{
  ALint state, n;
  alGetSourcei(source_, AL_SOURCE_STATE, &state);
  if (state != AL_STOPPED) {
    alSourceStop(source_);
    alSourcei(source_, AL_BUFFER, 0);
    std::cout << "WaveOutAL: Stopped playing." << std::endl;
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

  wxMessageOutputDebug().Printf(wxT("WaveOutAL: deleted a source."));

  return true;
}


bool WaveOutAL::Stop() {
  thread_->PostMessageQueue(new StopAL(this));
  SoundDeviceDriver::Stop();
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


WaveOutDisk::WaveOutDisk() {}



bool WaveOutDisk::Play()
{
  if (SoundDeviceDriver::Play() == false) return false;
/*
  file_.Create(soundFormat->GetFileName(), true, wxFile::write);

  format_.channelNumber = 2;
  format_.samplingRate = 44100;
  format_.dataRate = 44100*2*2;
  format_.blockSize = 2*2;
  format_.bitNumber = 16;
  format_.dataSize = 0;

  file_.Write(&format_, sizeof(format_));
*/
  return true;
}


bool WaveOutDisk::Stop()
{
  if (SoundDeviceDriver::Stop() == false) return false;
  return true;
}
