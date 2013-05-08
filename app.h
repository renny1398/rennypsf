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

    SoundDevice* GetSoundManager() const;
    void SetSoundDevice(SoundDevice*);

private:
    SoundDevice* soundDevice_;
};

// allow wxAppGet() to be called
DECLARE_APP(RennypsfApp)


inline SoundDevice* RennypsfApp::GetSoundManager() const {
    wxASSERT(soundDevice_ != 0);
    return soundDevice_;
}

inline void RennypsfApp::SetSoundDevice(SoundDevice* device) {
    soundDevice_ = device;
}
