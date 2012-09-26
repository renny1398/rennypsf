#pragma once

#include <map>
#include <wx/string.h>

class SoundFormat;
class SoundLoader;


class SoundLoaderFactory
{
    class LoaderTable
    {
    public:
        LoaderTable() {}
        ~LoaderTable() {}

        void Append(const wxString& ext, SoundLoader *loader);
        SoundLoader *operator[](const wxString& ext);

    private:
        std::map<wxString, SoundLoader*> table;
    };

public:
    static SoundLoaderFactory *GetInstance();

    SoundLoader *GetLoader(const wxString& ext);

protected:
    SoundLoaderFactory();
    ~SoundLoaderFactory() {}

private:
    LoaderTable table;
};


inline void SoundLoaderFactory::LoaderTable::Append(const wxString &ext, SoundLoader *loader)
{
    table.insert(std::pair<wxString, SoundLoader*>(ext, loader));
}


inline SoundLoaderFactory *SoundLoaderFactory::GetInstance()
{
    static SoundLoaderFactory factory;
    return &factory;
}


inline SoundLoader *SoundLoaderFactory::GetLoader(const wxString &ext)
{
    return table[ext];
}
