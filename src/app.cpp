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


bool RennypsfApp::OnInit() {

  SetExitOnFrameDelete(false);

  console_thread_ = new ConsoleThread();
  console_thread_->Create();
  console_thread_->Run();

  rennyCreateDebugWindow(NULL);
  rennyShowDebugWindow();

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
  sdd_.reset();
  console_thread_->Wait();
  delete console_thread_;

  wxMessageOutputDebug().Printf(wxT("Quit this application."));
  return 0;
}


bool RennypsfApp::Play(SoundData* sound)
{
  playing_sf_ = sound;
  //return sdd_->Play(sound);
  sound->ChangeOutputSamplingRate(96000); // for debug
  sdd_->SetSamplingRate(96000);
  return sound->Play(sdd_);
}

bool RennypsfApp::Stop()
{
  SoundData* sf = playing_sf_.get();
  if (sf == NULL) return true;
  bool ret = sf->Stop();
  if (sdd_ != 0) {
    sdd_->Stop();
  }
  playing_sf_.reset();
  return ret;
}


const wxSharedPtr<SoundData>& RennypsfApp::GetPlayingSound() const {
  return playing_sf_;
}

int main(int argc, char** argv) {
	wxMessageOutputDebug().Printf(wxT("Started this application."));

	wxApp::SetInstance(new RennypsfApp());
	wxEntryStart(argc, argv);
#ifdef __WXMAC__
	ProcessSerialNumber PSN = { 0, kCurrentProcess };
	TransformProcessType(&PSN, kProcessTransformToForegroundApplication);
#endif
  wxTheApp->OnInit();
  wxTheApp->OnRun();
  wxTheApp->OnExit();
	wxEntryCleanup();

	return 0;
}
