#include "stdwx.h"
#include "app.h"
#include "MainFrame.h"
#include "channelframe.h"
#include "SoundManager.h"


#ifdef __WXMAC__
#include <ApplicationServices/ApplicationServices.h>
#endif


// implicit definition main function
wxIMPLEMENT_APP(RennypsfApp);


bool RennypsfApp::OnInit()
{
#ifdef __WXMAC__
	ProcessSerialNumber PSN;
	GetCurrentProcess(&PSN);
	TransformProcessType(&PSN, kProcessTransformToForegroundApplication);
#endif
#ifdef __WXDEBUG__
    MainFrame *frame = new MainFrame("Rennypsf (Debug mode)", wxPoint(50,50), wxSize(640,480));
#else
    wxASSERT(0);
    MainFrame *frame = new MainFrame("Rennypsf", wxPoint(50,50), wxSize(640,480));
#endif
    soundDevice_ = new WaveOutAL(24);

    ChannelFrame* ch_frame = new ChannelFrame(frame);
    frame->Show(true);
    ch_frame->Show(true);

	return true;
}


int RennypsfApp::OnExit() {
    delete soundDevice_;
    return 0;
}


bool RennypsfApp::Play(SoundFormat* sound)
{
    return soundDevice_->Play(sound);
}

bool RennypsfApp::Stop()
{
    return soundDevice_->Stop();
}
