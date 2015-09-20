#pragma once

#include <wx/app.h>
#include "SoundManager.h"

class SoundFormat;

class RennypsfApp: public wxApp
{
public:
  virtual bool OnInit();
  virtual int OnExit();

#ifndef USE_GUI
  virtual int OnRun();
#ifdef __APPLE__
  virtual bool OSXIsGUIApplication() { return false; }
#endif  // __APPLE__
#endif  // USE_GUI

  bool Play(SoundFormat*);
  bool Stop();

  const wxSharedPtr<SoundDeviceDriver>& GetSoundManager() const;
  void SetSoundDevice(const wxSharedPtr<SoundDeviceDriver>&);

  const wxSharedPtr<SoundFormat>& GetPlayingSound() const;

private:
  wxSharedPtr<SoundDeviceDriver> sdd_;
  wxSharedPtr<SoundFormat> playing_sf_;

};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)


inline const wxSharedPtr<SoundDeviceDriver>& RennypsfApp::GetSoundManager() const {
  wxASSERT(sdd_ != NULL);
  return sdd_;
}

inline void RennypsfApp::SetSoundDevice(const wxSharedPtr<SoundDeviceDriver>& device) {
  sdd_ = device;
}
