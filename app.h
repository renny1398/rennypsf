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

    SoundDriver* GetSoundManager() const;
    void SetSoundDevice(SoundDriver*);

private:
    SoundDriver* soundDevice_;
};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)


inline SoundDriver* RennypsfApp::GetSoundManager() const {
    wxASSERT(soundDevice_ != 0);
    return soundDevice_;
}

inline void RennypsfApp::SetSoundDevice(SoundDriver* device) {
    soundDevice_ = device;
}
