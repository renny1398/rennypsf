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

#include "common/SoundManager.h"
class SoundData;

class RennyPlayer : public wxThread {
public:
  RennyPlayer();
  ~RennyPlayer();

  bool Play(SoundData* p_sound, SoundDevice* p_device);
  bool PlayShared(wxSharedPtr<SoundData>& p_sound, SoundDevice* p_device);
  bool Stop();

  bool Mute(int ch);
  bool Unmute(int ch);
  bool IsMuted(int ch) const;
  bool Switch(int ch);

protected:
  ExitCode Entry();

private:
  SoundDevice* p_device_;
  wxSharedPtr<SoundData> p_sound_;
  SoundBlock block_;
  bool is_exiting_;
};


#include <wx/app.h>
#include <wx/scopedptr.h>

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

  bool Mute(int ch);
  bool Unmute(int ch);
  bool Switch(int ch);

  SoundDevice* GetSoundManager();
  void SetSoundDevice(SoundDevice*);

  const wxSharedPtr<SoundData>& GetPlayingSound() const;

private:
  RennyPlayer* player_;
  SoundDevice* sdd_;
  wxSharedPtr<SoundData> playing_sf_;

#ifndef USE_GUI
  ConsoleThread* console_thread_;
#endif
};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)
