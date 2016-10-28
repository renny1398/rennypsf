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
// RennyPlayer class functions
////////////////////////////////////////////////////////////////////////

RennyPlayer::RennyPlayer()
  : wxThread(wxTHREAD_JOINABLE), p_device_(NULL), p_sound_(NULL), is_exiting_(false) {}

RennyPlayer::~RennyPlayer() {
  if (wxThread::IsRunning()) {
    Stop();
  }
}

bool RennyPlayer::PlayShared(wxSharedPtr<SoundData>& p_sound, SoundDevice *p_device) {
  if (p_sound == NULL || p_device == NULL) return false;
  switch (wxThread::Create()) {
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

  p_sound_ = p_sound;
  p_device_ = p_device;
  wxThread::Run();

  return true;
}

bool RennyPlayer::Play(SoundData *p_sound, SoundDevice *p_device) {
  wxSharedPtr<SoundData> sh_p_sound(p_sound);
  return PlayShared(sh_p_sound, p_device);
}

bool RennyPlayer::Stop() {
  is_exiting_ = true;
  ExitCode ret = wxThread::Wait();
  is_exiting_ = false;
  p_sound_->Close();
  return ret == 0;
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

wxThread::ExitCode RennyPlayer::Entry() {
  p_device_->SetSamplingRate(p_sound_->GetSamplingRate());
  p_device_->Listen();
  while (!TestDestroy() && !is_exiting_) {
    if (p_sound_->Advance(&block_) == false) break;
    p_device_->OnUpdate(&block_);
  }
  p_device_->Stop();
  return 0;
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

  while (!TestDestroy() && !is_exiting_) {
    std::cout << "> ";
    std::getline(std::cin, line);
    if (TestDestroy()) break;
    Command* cmd = CommandFactory::GetInstance()->CreateCommand(line);
    if (cmd == NULL) continue;
    cmd->Execute();
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
  if (console_thread_ && console_thread_->IsRunning()) {
    console_thread_->Exit();
    // if (wxThread::GetCurrentId() != console_thread_->GetId()) {
    //   console_thread_->Delete();
    //   console_thread_->Wait();
    // }
  }
  rennyDestroyDebugWindow();

  return wxAppConsole::ExitMainLoop();
}

#endif

int RennypsfApp::OnExit() {
  if (console_thread_ && console_thread_->IsRunning()) {
    console_thread_->Delete();
  }
  Stop();
  delete player_;
  sdd_ = NULL;
  console_thread_->Wait();
  delete console_thread_;

  wxMessageOutputDebug().Printf(wxT("Quit this application."));
  return 0;
}


bool RennypsfApp::Play(SoundData* sound)
{
  playing_sf_ = sound;
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
  if (player_->IsRunning() == false) return false;
  return player_->Mute(ch);
}

bool RennypsfApp::Unmute(int ch) {
  if (player_->IsRunning() == false) return false;
  return player_->Unmute(ch);
}

bool RennypsfApp::Switch(int ch) {
  if (player_->IsRunning() == false) return false;
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
    // sound->Play(sdd);
    wxGetApp().Play(sound);

    // delete loader;
  }

  wxTheApp->OnRun();
  wxTheApp->OnExit();
	wxEntryCleanup();

	return 0;
}
