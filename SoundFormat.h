#pragma once

#include <wx/object.h>
#include "SoundFormatInfo.h"

class SoundFormat: public wxObject
{
public:
    virtual ~SoundFormat();

    virtual bool IsLoaded() const { return true; }

    void GetTag(const wxString &key, wxString *value) const;
    void SetTag(const wxString &key, const wxString &value);

    virtual bool Play() = 0;
    virtual bool Stop() = 0;

private:
    SoundFormatInfo m_info;
};


class PreloadedSoundFormat: public SoundFormat
{
public:
    virtual ~PreloadedSoundFormat() {}

    virtual bool IsLoaded() const { return false; }

    virtual bool Play() { return false; }
    virtual bool Stop() { return false; }
};
