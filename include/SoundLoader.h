#ifndef SOUNDLOADER_H
#define SOUNDLOADER_H

#include <wx/object.h>

class SoundFormat;
class wxString;

class SoundLoader: public wxObject
{
public:
    virtual ~SoundLoader();

    virtual const wxString &GetPath() const = 0;

    virtual SoundFormat *LoadInfo(const wxString &path) = 0;
    //virtual SoundFormat *Load() = 0;


// private: inherited class must have PreloadedSoundFormat class
};

#endif // SOUNDLOADER_H
