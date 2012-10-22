#pragma once

#include <wx/app.h>
#include "SoundManager.h"

class SoundFormat;

class RennypsfApp: public wxApp
{
public:
	bool OnInit();

    bool Play(SoundFormat*);
    bool Stop();

    SoundManager& GetSoundManager();

private:
    SoundManager m_soundManager;
};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)

inline SoundManager& RennypsfApp::GetSoundManager() {
    return m_soundManager;
}
