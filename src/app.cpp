#include "stdwx.h"
#include "app.h"
#include "MainFrame.h"
#include "channelframe.h"
#include "SoundManager.h"


#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif


wxIMPLEMENT_APP_NO_MAIN(RennypsfApp);


bool RennypsfApp::OnInit()
{
  wxMessageOutputDebug().Printf(wxT("RennypsfApp::OnInit()"));

#ifdef __WXDEBUG__
  MainFrame *frame = new MainFrame("Rennypsf (Debug mode)", wxPoint(50,50), wxSize(640,480));
#else
  wxASSERT(0);
  MainFrame *frame = new MainFrame("Rennypsf", wxPoint(50,50), wxSize(640,480));
#endif
  sdd_.reset(new WaveOutAL);

  ChannelFrame* ch_frame = new ChannelFrame(frame);
  frame->Show(true);
  ch_frame->Show(true);

  wxMessageOutputDebug().Printf(wxT("Started this application."));

  return true;
}


int RennypsfApp::OnExit() {
  sdd_.reset();

  wxMessageOutputDebug().Printf(wxT("Quit this application."));
  return 0;
}


bool RennypsfApp::Play(SoundFormat* sound)
{
  //return sdd_->Play(sound);
  return sound->Play(sdd_);
}

bool RennypsfApp::Stop()
{
  return sdd_->Stop();
}


const wxSharedPtr<SoundFormat>& RennypsfApp::GetPlayingSound() const {
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
