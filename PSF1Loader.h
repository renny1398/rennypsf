#pragma once

#include "PSFLoader.h"
#include <wx/string.h>
#include <wx/vector.h>

class SoundFormat;
class PSF1;
class PreloadedPSF1;

class PSF1Loader: private PSFLoader
{
public:
    ~PSF1Loader();

//    SoundFormat* Preload(const wxString& path);
    SoundFormat *Load();

    static SoundLoader *GetInstance();

protected:
    PSF1Loader() {}
    PSF1 *LoadLib(const wxString &path);

private:
    wxVector<PSF1*> m_libs;
};
