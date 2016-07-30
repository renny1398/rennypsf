#pragma once
#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <wx/vector.h>

#include <wx/event.h>
#include <wx/thread.h>
#include <wx/msgqueue.h>


struct NoteInfo {
  int ch;
  bool is_on;
  int tone_number_;
  float pitch;
  int rate;
  float velocity;
};


struct ToneInfo {
  int number;
  int length;
  int loop;
  float pitch;
  const int* data;
  bool muted;
};



wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_NOTE_ON, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_NOTE_OFF, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_TONE_NUMBER, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_PITCH, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_VELOCITY, wxCommandEvent);

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_ADD_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_CHANGE_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_REMOVE_TONE, wxCommandEvent);

wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_MUTE_TONE, wxCommandEvent);
wxDECLARE_EXPORTED_EVENT(WXDLLIMPEXP_CORE, wxEVT_UNMUTE_TONE, wxCommandEvent);


#include "SoundFormat.h"
#include <wx/scopedarray.h>

#if 0
class SoundDeviceDriver;
#include <wx/sharedptr.h>
typedef wxSharedPtr<SoundDeviceDriver> SoundDeviceDriverPtr;
#endif

class SoundDeviceDriver
{
public:
  SoundDeviceDriver();
  virtual ~SoundDeviceDriver();

  virtual bool Play();
  virtual bool Stop();

  virtual float GetVolume() const = 0;
  virtual void SetVolume(float vol) = 0;

  bool IsPlaying() const;

  void OnUpdate(const SoundBlock*);

  // bool SwitchToneMuted(int id);

  void ZeroCounter();
  void IncrementCounter();
  int GetCounter() const;

  // static wxVector<ToneInfo>::iterator ToneExists(const wxVector<ToneInfo>& tones, int id);


protected:
  // const Sample* GetBuffer(int* size) const;
  const short* buffer(int* size) const;

  // void setChannelNumber(int number);
  void set_buffer_length(int size);
  virtual void WriteToDevice() = 0;

protected:
  static const int kDefaultBufferLength = 90;

private:
  wxVector<NoteInfo> notes_, next_notes_;
  // wxVector<ToneInfo> tones_;

  wxScopedArray<short> buffer_;
  int bufferSize_;      // sizeof(buffer_) / 4
  int bufferIndex_;

  bool is_playing_;
  // wxVector<bool> muted_;

  int counter_;
};



class WaveOutAL;
class WaveOutALCommand;

class WaveOutALThread : public wxThread {
public:
  WaveOutALThread(WaveOutAL* sound_driver)
    : wxThread(wxTHREAD_JOINABLE), sound_driver_(sound_driver)
  {}

  void PostMessageQueue(WaveOutALCommand* command) {
    command_queue_.Post(command);
  }

protected:
  virtual ExitCode Entry();

private:
  WaveOutAL* const sound_driver_;
  wxMessageQueue<WaveOutALCommand*> command_queue_;
};



// TODO: Replace a raw pointer with shared_ptr or weak_ptr
class WaveOutAL : public SoundDeviceDriver
{
  friend class WaveOutALThread;
  friend class WaveOutALCommand;

public:
  WaveOutAL();
  ~WaveOutAL();

  void Init();
  virtual bool Stop();
  void Shutdown();

  float GetVolume() const;
  void SetVolume(float vol);

  // Functions which only the Init()-ed thead can call
  void ThisThreadInit();
  bool ThisThreadStop();
  void ThisThreadShutdown();

  virtual void WriteToDevice();
  void ThisThreadWriteToDevice();

protected:
  void WaitForWritingToDevice();

private:
  // bool is_multithreading_; // force

  static ALCdevice *device_;
  static ALCcontext *context_;
  static int source_number_;

  static WaveOutALThread* thread_;

  ALuint buffer_, source_;
  wxMutex mutex_, mutex2_;
  wxCondition write_to_device_cond_;
  wxCondition write_to_device_cond2_;
  bool finished_writing_;
};


#include <wx/file.h>

class WaveOutDisk: public SoundDeviceDriver
{
public:
  WaveOutDisk();

  bool Play();
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
