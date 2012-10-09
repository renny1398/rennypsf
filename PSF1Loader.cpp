#include "PSF1Loader.h"
#include "PSF1.h"
#include "R3000A.h"
#include "PSXMemory.h"

#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <cstring>
#include <wx/hashmap.h>
#include <wx/filename.h>


PSF1Loader::~PSF1Loader()
{
    while (!m_libs.empty()) {
        m_libs.pop_back();
    }
}



SoundLoader *PSF1Loader::GetInstance()
{
    return new PSF1Loader();
}


WX_DECLARE_HASH_MAP(int, wxString, wxIntegerHash, wxIntegerEqual, IntStrHash);


SoundFormat *PSF1Loader::Load()
{
    wxASSERT_MSG(m_preloaded_psf, "PSF file is not preloaded.");
    wxFile file(GetPath());
    if (file.IsOpened() == false) {
        wxMessageOutputStderr().Printf("File '%s' can't be opened.", GetPath());
        return 0;
    }

    const PreloadedPSF *preloaded_psf = m_preloaded_psf;
    wxFileInputStream stream(file);

    stream.SeekI(preloaded_psf->m_ofsBinary);
    // char *compressed = new char[preloaded_psf->m_lenBinary];
    char *uncompressed = new char[0x800 + 0x200000];    // header + UserMemory
    wxZlibInputStream zlib_stream(stream);
    zlib_stream.Read(uncompressed, 0x800);

    const PSF::PSXEXEHeader &header = *(reinterpret_cast<PSF::PSXEXEHeader*>(uncompressed));
    if (memcmp(header.signature, "PS-X EXE", 8) != 0) {
        wxMessageOutputStderr().Printf("Uncompressed binary is not PS-X EXE!");
        return 0;
    }
    wxMessageOutputDebug().Printf(header.marker);

    wxString directory, filename, ext;
    wxFileName::SplitPath(GetPath(), &directory, &filename, &ext);
    IntStrHash libname_hash;

    for (int i = 1; i < 10; i++) {
        wxString _libN, libname;
        if (i <= 1) {
            _libN.assign("_lib");
        } else {
            _libN.sprintf("_lib%d", i);
        }
        preloaded_psf->GetTag(_libN, &libname);
        if (libname.IsEmpty() == false) {
            libname_hash[i] = directory + '/' + libname;
        }
    }

    R3000A::GPR.PC = header.pc0;
    R3000A::GPR.GP = header.gp0;
    R3000A::GPR.SP = header.sp0;

    for (IntStrHash::iterator it = libname_hash.begin(), it_end = libname_hash.end(); it != it_end; ++it) {
        PSF1 *psflib = LoadLib(it->second);
        if (psflib) {
            m_libs.push_back(psflib);
        }
    }

    /*
    binary = uncompressed;
    for (int i = 0; i < 0x800; i++) {
        wxMessageOutputDebug().Printf("0x%08X", binary);
        binary++;
    }*/
    zlib_stream.Read(uncompressed + 0x800, 0x200000);
    PSXMemory::Init();
    PSXMemory::Load(header.text_addr, header.text_size, uncompressed + 0x800);

    PSF1 *psf1 = new PSF1();
    psf1->m_header = header;

    wxMessageOutputDebug().Printf("PSF File '%s' is loaded.", GetPath());
    delete [] uncompressed;
    return psf1;
}


PSF1 *PSF1Loader::LoadLib(const wxString &path)
{
    PSF1Loader loader;
    loader.Preload(path);
    PSF1 *psflib = static_cast<PSF1*>(loader.Load());
    if (psflib == 0) {
        return 0;
    }

    for (wxVector<PSF1*>::const_iterator it = loader.m_libs.begin(), it_end = loader.m_libs.end(); it != it_end; ++it) {
        m_libs.push_back(*it);
    }

    return psflib;
}
