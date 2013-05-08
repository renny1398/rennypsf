#pragma once

#include <wx/object.h>
#include <wx/file.h>
#include "SoundFormatInfo.h"


class SoundFormat: public wxObject
{
public:
    virtual ~SoundFormat();

    bool IsLoaded() const;

    void GetTag(const wxString &key, wxString *value) const;
    void SetTag(const wxString &key, const wxString &value);

    virtual bool Play() = 0;
    virtual bool Stop() = 0;

    const wxString& GetFileName() const;

protected:
    wxFile file_;
    wxString path_;
    bool infoLoaded_;

private:
    SoundFormatInfo m_info;
};


inline bool SoundFormat::IsLoaded() const {
	return infoLoaded_;
}

inline const wxString& SoundFormat::GetFileName() const {
    return path_;
}
