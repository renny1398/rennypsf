#include "SoundFormat.h"
#include <wx/msgout.h>

SoundFormat::~SoundFormat()
{
}


void SoundFormat::GetTag(const wxString &key, wxString *value) const
{
    m_info.GetTag(key, value);
}

void SoundFormat::SetTag(const wxString &key, const wxString &value)
{
    // wxMessageOutputDebug().Printf(wxT("set tag '%s = %s'"), key, value);
    m_info.SetTag(key, value);
}
