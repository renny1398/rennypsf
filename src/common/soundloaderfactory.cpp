#if 0
#include "SoundLoaderFactory.h"
#include "stdwx.h"
#include <wx/string.h>

#include "PSFLoader.h"


SoundLoader *SoundLoaderFactory::LoaderTable::operator [](const wxString &ext)
{
    std::map<wxString, SoundLoader*>::iterator it = table.find(ext);
    if (it == table.end()) {
        return 0;
    }
    return it->second;
}


SoundLoaderFactory::SoundLoaderFactory()
{
    table.Append(wxT("psf"), PSF1Loader::GetInstance());
    table.Append(wxT("minipsf"), PSF1Loader::GetInstance());
    table.Append(wxT("psf2"), PSF2Loader::GetInstance());
    table.Append(wxT("minipsf2"), PSF2Loader::GetInstance());
}
#endif
