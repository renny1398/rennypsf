#include "PSF.h"
#include "psx/psx.h"
#include "spu/spu.h"
#include <wx/msgout.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/hashmap.h>


PSF::~PSF()
{
    while (!m_libs.empty()) {
		PSF* psf = m_libs.back();
		delete psf;
		m_libs.pop_back();
	}
}

void PSF::Init()
{
}

void PSF::Reset()
{
    PSX::R3000a.Init();
//    PSX::Hardware::Reset();
    PSX::RootCounter::Init();
    Spu.Open();
}


bool PSF::Play()
{
	LoadBinary();
    Reset();
    PSX::BIOS::Init();
    m_thread = PSX::Interpreter_.Execute();

    return true;
}


bool PSF::Stop()
{
    PSX::Interpreter_.Shutdown();
    m_thread = 0;
    PSX::Memory::Reset();
    return true;
}





PSF1::PSF1()
{
}


WX_DECLARE_HASH_MAP(int, wxString, wxIntegerHash, wxIntegerEqual, IntStrHash);


bool PSF1::LoadBinary()
{
    if (file_.IsOpened() == false) {
        wxMessageOutputStderr().Printf(wxT("file '%s' can't be opened."), path_);
        return 0;
    }

    wxFileInputStream stream(file_);

    stream.SeekI(m_ofsBinary);
    // char *compressed = new char[preloaded_psf->m_lenBinary];
    char *uncompressed = new char[0x800 + 0x200000];    // header + UserMemory
    wxZlibInputStream zlib_stream(stream);
    zlib_stream.Read(uncompressed, 0x800);

    const PSF::PSXEXEHeader &header = *(reinterpret_cast<PSF::PSXEXEHeader*>(uncompressed));
    if (memcmp(header.signature, "PS-X EXE", 8) != 0) {
        wxMessageOutputStderr().Printf(wxT("Uncompressed binary is not PS-X EXE!"));
        return 0;
    }

    wxString directory, filename, ext;
    wxFileName::SplitPath(path_, &directory, &filename, &ext);
    IntStrHash libname_hash;

    for (int i = 1; i < 10; i++) {
        wxString _libN, libname;
        if (i <= 1) {
            _libN.assign("_lib");
        } else {
            _libN.sprintf("_lib%d", i);
        }
        GetTag(_libN, &libname);
        if (libname.IsEmpty() == false) {
            libname_hash[i] = directory + '/' + libname;
        }
    }

    PSX::R3000a.GPR.PC = header.pc0;
    PSX::R3000a.GPR.GP = header.gp0;
    PSX::R3000a.GPR.SP = header.sp0;
    // wxMessageOutputDebug().Printf("PC0 = 0x%08x, GP0 = 0x%08x, SP0 = 0x%08x", header.pc0, header.gp0, header.sp0);

    for (IntStrHash::iterator it = libname_hash.begin(), it_end = libname_hash.end(); it != it_end; ++it) {
        PSF *psflib = LoadLib(it->second);
        if (psflib) {
            m_libs.push_back(psflib);
        }
    }

    zlib_stream.Read(uncompressed + 0x800, 0x200000);
    PSX::Memory::Init();
    PSX::Memory::Load(header.text_addr, header.text_size, uncompressed + 0x800);

    m_header = header;

    wxMessageOutputDebug().Printf("PSF File '%s' is loaded.", path_);
    delete [] uncompressed;
    return true;
}


#include "PSFLoader.h"

PSF *PSF1::LoadLib(const wxString &path)
{
    PSF1Loader loader;
    PSF1 *psflib = dynamic_cast<PSF1*>( loader.LoadInfo(path) );
    if (psflib->LoadBinary() == false) {
        return 0;
    }

    for (wxVector<PSF*>::const_iterator it = psflib->m_libs.begin(), it_end = psflib->m_libs.end(); it != it_end; ++it) {
        m_libs.push_back(*it);
    }

    return psflib;
}

