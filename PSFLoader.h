#pragma once

#include "SoundLoader.h"

class PreloadedPSF;

class PSFLoader: public SoundLoader
{
public:
    PSFLoader() {}
    ~PSFLoader();

    const wxString &GetPath() const;

    SoundFormat* Preload(const wxString& path);
//    SoundFormat* Load();

protected:
    PreloadedPSF *m_preloaded_psf;

    static void AddTags(PreloadedPSF *psf, char *buff);

private:
    wxString m_path;
};
