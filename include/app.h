#pragma once

#include <wx/thread.h>

class ConsoleThread : public wxThread {
public:
  ConsoleThread() : wxThread(wxTHREAD_JOINABLE), is_exiting_(false) {}

  ExitCode Entry();
  void RequestExit() { is_exiting_ = true; }

private:
  bool is_exiting_;
};

#include "common/SoundManager.h"
class SoundData;

class RennyPlayer {
public:
  RennyPlayer();
  ~RennyPlayer();

  bool Play(SoundData* p_sound, SoundDevice* p_device);
  bool Stop();
  bool IsPlaying() const;

  bool Mute(int ch);
  bool Unmute(int ch);
  bool IsMuted(int ch) const;
  bool Switch(int ch);

private:
  class RennyPlayerThread : public wxThread {
  public:
    explicit RennyPlayerThread(RennyPlayer*);
  protected:
    ExitCode Entry() override;
  private:
    RennyPlayer* const player_;
  };
  friend class RennyPlayerThread;

  RennyPlayerThread* thread_;
  SoundDevice* p_device_;
  SoundData* p_sound_;
  SoundBlock block_;
};


#include <wx/app.h>
#include <wx/scopedptr.h>

class RennypsfApp: public wxApp
{
public:
  virtual bool OnInit() override;
  virtual int OnExit() override;

  void Exit() override;

#ifndef USE_GUI
  // virtual int OnRun();
  void ExitMainLoop() override;
  void ProcessExitEvent(wxThreadEvent&);
#ifdef __APPLE__
  bool OSXIsGUIApplication() override { return false; }
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

#ifndef USE_GUI
  ConsoleThread* console_thread_;
#endif
};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)
