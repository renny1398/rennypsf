#include "stdwx.h"
#include "app.h"
#include "MainFrame.h"
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
	MainFrame *frame = new MainFrame("rennypsf", wxPoint(50,50), wxSize(640,480));
	frame->Show(true);
	return true;
}


bool RennypsfApp::Play(SoundFormat* sound)
{
    return m_soundManager.Play(sound);
}

bool RennypsfApp::Stop()
{
    return m_soundManager.Stop();
}
