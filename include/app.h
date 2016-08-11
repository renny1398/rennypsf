#pragma once

#include <wx/thread.h>

class ConsoleThread : public wxThread {
public:
  ConsoleThread() : wxThread(wxTHREAD_JOINABLE), is_exiting_(false) {}

  ExitCode Entry();
  void Exit() { is_exiting_ = true; }

private:
  bool is_exiting_;
};


#include <wx/app.h>
#include "common/SoundManager.h"

class SoundData;

class RennypsfApp: public wxApp
{
public:
  virtual bool OnInit();
  virtual int OnExit();

#ifndef USE_GUI
  // virtual int OnRun();
  void ExitMainLoop();
#ifdef __APPLE__
  virtual bool OSXIsGUIApplication() { return false; }
#endif  // __APPLE__
#endif  // USE_GUI

  bool Play(SoundData*);
  bool Stop();

  const wxSharedPtr<SoundDeviceDriver>& GetSoundManager() const;
  void SetSoundDevice(const wxSharedPtr<SoundDeviceDriver>&);

  const wxSharedPtr<SoundData>& GetPlayingSound() const;

private:
  wxSharedPtr<SoundDeviceDriver> sdd_;
  wxSharedPtr<SoundData> playing_sf_;

#ifndef USE_GUI
  ConsoleThread* console_thread_;
#endif
};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)


#include "common/debug.h"
inline const wxSharedPtr<SoundDeviceDriver>& RennypsfApp::GetSoundManager() const {
  return sdd_;
}

inline void RennypsfApp::SetSoundDevice(const wxSharedPtr<SoundDeviceDriver>& device) {
  sdd_ = device;
}
