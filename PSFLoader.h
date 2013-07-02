#pragma once

#include "SoundLoader.h"
#include "PSF.h"


class PSFLoader: public SoundLoader
{
public:
    PSFLoader() {}
    ~PSFLoader();

    const wxString &GetPath() const;

    SoundFormat* LoadInfo(const wxString& path);
    //SoundFormat* Load();

    friend class PSF;

protected:
    static void AddTags(PSF *psf, char *buff);

private:
    wxString m_path;
};




#include <wx/string.h>
#include <wx/vector.h>

class SoundFormat;
class PSF1;

class PSF1Loader: public PSFLoader
{
public:
    ~PSF1Loader();

    SoundFormat *LoadBinary();

    static SoundLoader *GetInstance();

//protected:
    PSF1Loader() {}

    friend class PSF1;
};


class PSF2;

class PSF2Loader : public PSFLoader
{
public:
    static SoundLoader *GetInstance();
    friend class PSF2;
};
