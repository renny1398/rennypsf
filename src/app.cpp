#include "stdwx.h"
#include "app.h"
#include "common/command.h"
#include "MainFrame.h"
#include "channelframe.h"
#include "common/SoundManager.h"
#include "common/debug.h"

#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif

////////////////////////////////////////////////////////////////////////
// RennyPlayerThread class functions
////////////////////////////////////////////////////////////////////////

RennyPlayer::RennyPlayerThread::RennyPlayerThread(RennyPlayer* player)
  : wxThread(wxTHREAD_JOINABLE), player_(player) {}

wxThread::ExitCode RennyPlayer::RennyPlayerThread::Entry() {
  SoundDevice* p_device = player_->p_device_;
  SoundData* p_sound = player_->p_sound_;
  SoundBlock* p_block = &player_->block_;
  p_device->SetSamplingRate(p_sound->GetSamplingRate());
  p_device->Listen();
  while (!TestDestroy()) {
    if (p_sound->Advance(p_block) == false) break;
    p_device->OnUpdate(p_block);
  }
  p_device->Stop();
  return 0;
}

////////////////////////////////////////////////////////////////////////
// RennyPlayer class functions
////////////////////////////////////////////////////////////////////////

RennyPlayer::RennyPlayer()
  : thread_(nullptr), p_device_(nullptr), p_sound_(nullptr) {}

RennyPlayer::~RennyPlayer() {
  if (IsPlaying()) {
    Stop();
  }
  if (p_sound_) {
    delete p_sound_;
  }
}

bool RennyPlayer::Play(SoundData *p_sound, SoundDevice *p_device) {
  if (p_sound == NULL || p_device == NULL) return false;
  if (thread_ == nullptr) {
    thread_ = new RennyPlayerThread(this);
  }
  switch (thread_->Create()) {
  case wxTHREAD_NO_ERROR:
    break;
  case wxTHREAD_NO_RESOURCE:
    return false;
  case wxTHREAD_RUNNING:
    Stop();
    break;
  default:
    break;
  }

  if (p_sound->Open(&block_) == false) return false;

  if (p_sound_) {
    delete p_sound_;
  }
  p_sound_ = p_sound;
  p_device_ = p_device;
  thread_->Run();

  return true;
}

bool RennyPlayer::Stop() {
  if (IsPlaying()) {
    thread_->Delete();
    thread_->Wait();
    delete thread_;
    thread_ = nullptr;
  }
  return p_sound_->Close();
}

bool RennyPlayer::IsPlaying() const {
  return (thread_ && thread_->IsRunning());
}

bool RennyPlayer::Mute(int ch) {
  if (static_cast<int>(block_.channel_count()) <= ch) return false;
  block_.Ch(ch).Mute();
  return true;
}

bool RennyPlayer::Unmute(int ch) {
  if (static_cast<int>(block_.channel_count()) <= ch) return false;
  block_.Ch(ch).Unmute();
  return true;
}

bool RennyPlayer::Switch(int ch) {
  if (IsMuted(ch)) {
    return Unmute(ch);
  } else {
    return Mute(ch);
  }
}

bool RennyPlayer::IsMuted(int ch) const {
  if (static_cast<int>(block_.channel_count()) <= ch) return true;
  return block_.Ch(ch).IsMuted();
}


wxIMPLEMENT_APP_NO_MAIN(RennypsfApp);

#if USE_GUI

bool RennypsfApp::OnInit()
{
  wxMessageOutputDebug().Printf(wxT("RennypsfApp::OnInit()"));

#ifdef __WXDEBUG__
  MainFrame *frame = new MainFrame("Rennypsf (Debug mode)", wxPoint(50,50), wxSize(640,480));
#else
  rennyAssert(0);
  MainFrame *frame = new MainFrame("Rennypsf", wxPoint(50,50), wxSize(640,480));
#endif
  sdd_.reset(new WaveOutAL);

  ChannelFrame* ch_frame = new ChannelFrame(frame);
  frame->Show(true);
  ch_frame->Show(true);

  wxMessageOutputDebug().Printf(wxT("Started this application."));

  return true;
}


#else

#include <wx/cmdline.h>

wxThread::ExitCode ConsoleThread::Entry() {

  std::string line;
  is_exiting_ = false;

  while (!TestDestroy()) {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (TestDestroy()) break;
    Command* cmd = CommandFactory::GetInstance()->CreateCommand(line);
    if (cmd == NULL) continue;
    cmd->Execute();
    if (is_exiting_) break;
  }

  // wxGetApp().ExitMainLoop();
  return 0;
}


SoundDevice* RennypsfApp::GetSoundManager() {
  return sdd_;
}


void RennypsfApp::SetSoundDevice(SoundDevice* device) {
  sdd_ = device;
}


bool RennypsfApp::OnInit() {

  SetExitOnFrameDelete(false);

  console_thread_ = new ConsoleThread();
  console_thread_->Create();
  console_thread_->Run();

  rennyCreateDebugWindow(NULL);
  rennyShowDebugWindow();

  player_ = new RennyPlayer();

  return true;
}


void RennypsfApp::ExitMainLoop() {
  if (console_thread_) {
    console_thread_->Delete();
    console_thread_->Wait();
    delete console_thread_;
    console_thread_ = nullptr;
  }
  rennyDestroyDebugWindow();

  return wxAppConsole::ExitMainLoop();
}


void RennypsfApp::ProcessExitEvent(wxThreadEvent&) {
  return ExitMainLoop();
}

#endif

void RennypsfApp::Exit(int code) {
#ifdef USE_GUI
  // main_window_->Destroy();
  ExitMainLoop();
#else
  if (console_thread_ && console_thread_->IsRunning()) {
    console_thread_->RequestExit();
  }
  const int wxID_EXIT = 1000;
  Bind(wxEVT_THREAD, &RennypsfApp::ProcessExitEvent, this, wxID_EXIT);
  wxThreadEvent event(wxEVT_THREAD, wxID_EXIT);
  event.SetInt(code);
  QueueEvent(event.Clone());
#endif
}

int RennypsfApp::OnExit() {
  Stop();
  delete player_;
  delete sdd_;

  wxMessageOutputDebug().Printf(wxT("Quit this application."));
  return 0;
}


bool RennypsfApp::Play(SoundData* sound)
{
  // sdd_->SetSamplingRate(sound->GetSamplingRate());
  // return sound->Play(sdd_);
  return player_->Play(sound, sdd_);
}

bool RennypsfApp::Stop()
{
  /*
  SoundData* sf = playing_sf_.get();
  if (sf == NULL) return true;
  bool ret = sf->Stop();
  if (sdd_ != NULL) {
    sdd_->Stop();
  }
  playing_sf_.reset();
  return ret;
  */
  return player_->Stop();
}

bool RennypsfApp::Mute(int ch) {
  if (player_->IsPlaying() == false) return false;
  return player_->Mute(ch);
}

bool RennypsfApp::Unmute(int ch) {
  if (player_->IsPlaying() == false) return false;
  return player_->Unmute(ch);
}

bool RennypsfApp::Switch(int ch) {
  if (player_->IsPlaying() == false) return false;
  return player_->Switch(ch);
}

const wxSharedPtr<SoundData>& RennypsfApp::GetPlayingSound() const {
  static wxSharedPtr<SoundData> dummy;
  return dummy;
}

#include "common/SoundLoader.h"

int main(int argc, char** argv) {
  wxMessageOutputDebug().Printf(wxT("Started this application."));

  wxApp::SetInstance(new RennypsfApp());
  wxEntryStart(argc, argv);
#ifdef __WXMAC__
  ProcessSerialNumber PSN = { 0, kCurrentProcess };
  TransformProcessType(&PSN, kProcessTransformToForegroundApplication);
#endif
  wxTheApp->OnInit();

  if (argc > 1) {
    SoundLoader* loader = SoundLoader::Instance(argv[1]);
    if (loader == NULL) {
      return false;
    }

    // SoundInfo* info = loader->LoadInfo();
    SoundData *sound = loader->LoadData();
    if (sound == 0) {
      std::cout << "Failed to load a sound file." << std::endl;
      return false;
    }

    SoundDevice* sdd = wxGetApp().GetSoundManager();
    if (sdd == NULL) {
      wxGetApp().SetSoundDevice(new WaveOutAL);
    }
    wxGetApp().Play(sound);

    // delete loader;
  }

  wxTheApp->OnRun();
  wxTheApp->OnExit();
  wxEntryCleanup();

  return 0;
}
