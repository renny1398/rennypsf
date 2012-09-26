#ifndef SOUNDFORMATINFO_H
#define SOUNDFORMATINFO_H

#include <wx/string.h>
#include <wx/hashmap.h>


WX_DECLARE_STRING_HASH_MAP(wxString, StrStrHash);


class SoundFormatInfo
{
public:
    void GetTag(const wxString &key, wxString *value) const;
    void SetTag(const wxString &key, const wxString &value);

    /*
public:
    wxString title;
    wxString artist;
    wxString album;
    wxString year;
    wxString genre;
    wxString composer;
    wxString comment;
    wxString copyright;
    */

private:
    StrStrHash tags;
};

#endif // SOUNDFORMATINFO_H
