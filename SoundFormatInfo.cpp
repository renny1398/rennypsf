#include "SoundFormatInfo.h"
#include "wx/msgout.h"

void SoundFormatInfo::GetTag(const wxString &key, wxString *value) const
{
    StrStrHash::const_iterator it = tags.find(key);
    if (it == tags.end()) {
        value->Empty();
    } else {
        *value = it->second;
    }
}

void SoundFormatInfo::SetTag(const wxString &key, const wxString &value)
{
    tags.insert(StrStrHash_wxImplementation_Pair(key, value));
    wxMessageOutputDebug().Printf("Add tag: %s = %s", key, value);
}
