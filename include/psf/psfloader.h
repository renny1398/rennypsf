#pragma once

#include "SoundLoader.h"
// #include "PSF.h"

#include <wx/string.h>
#include <wx/file.h>
#include <wx/wfstream.h>
#include <wx/scopedptr.h>
#include <wx/scopedarray.h>
#include <wx/weakref.h>
#include <wx/vector.h>
#include <cstdint>

class SoundInfo;
class SoundData;

////////////////////////////////////////////////////////////////////////
/// PSF Loader Base Class
////////////////////////////////////////////////////////////////////////

class PSFLoader : public SoundLoader {

public:
  SoundInfo* LoadInfo();

protected:
  PSFLoader(int fd, const wxString& filename);

  bool LoadLibraries();

  wxFile& file();
  const wxString& path() const;

  //! Parameter Accessors
  uint32_t reserved_area_len() const;
  uint32_t binary_len() const;
  uint32_t binary_crc32() const;
  uint32_t reserved_area_ofs() const;
  uint32_t binary_ofs() const;

  //! PSF Library Loader Accessors
  wxVector<PSFLoader*>::iterator psflib_begin();
  wxVector<PSFLoader*>::const_iterator psflib_begin() const;
  wxVector<PSFLoader*>::const_iterator psflib_end() const;

private:
  wxFile file_;
  wxString path_;
  wxWeakRef<SoundInfo> ref_info_;

  //! PSF records
  uint32_t reserved_area_len_;
  uint32_t binary_len_;
  uint32_t binary_crc32_;

  //! Area offsets
  uint32_t reserved_area_ofs_;
  uint32_t binary_ofs_;

  //! PSF Library Loaders
  wxVector<PSFLoader*> psflibs_;
};



////////////////////////////////////////////////////////////////////////
/// PSF1 Loader Classes
////////////////////////////////////////////////////////////////////////

class PSF1;

class PSF1Loader: public PSFLoader
{
public:
  SoundData* LoadData();
  PSF1* LoadDataEx();

  static PSF1Loader* Instance(int fd, const wxString& filename);

protected:
  PSF1Loader(int fd, const wxString& filename);

  bool LoadEXE();
  bool LoadText(PSF1* p_psf);

  //! Accessors
  // uint32_t pc0() const;
  // uint32_t gp0() const;
  // uint32_t sp0() const;
  bool GetInitRegs(uint32_t* pc0, uint32_t* gp0, uint32_t* sp0) const;

  struct PSXEXEHeader {
    char signature[8];
    uint32_t text;
    uint32_t data;
    uint32_t pc0;
    uint32_t gp0;
    uint32_t text_addr;
    uint32_t text_size;
    uint32_t d_addr;
    uint32_t d_size;
    uint32_t b_addr;
    uint32_t b_size;
    // uint32_t S_addr;
    uint32_t sp0;
    uint32_t s_size;
    uint32_t SP;
    uint32_t FP;
    uint32_t GP;
    uint32_t RA;
    uint32_t S0;
    char marker[0xB4];
  };

  // friend class PSF1;
private:
  wxWeakRef<PSF1> ref_data_;
  wxScopedPtr<PSXEXEHeader> header_;
  wxScopedArray<char> text_;
};


////////////////////////////////////////////////////////////////////////
/// PSF2 Loader Classes
////////////////////////////////////////////////////////////////////////

class PSF2;
class PSF2Directory;

class PSF2Loader : public PSFLoader
{
public:
  SoundData* LoadData();
  PSF2* LoadDataEx();

  static PSF2Loader* Instance(int fd, const wxString& filename);

protected:
  PSF2Loader(int fd, const wxString& filename);

  bool LoadPSF2Entries(PSF2Directory* root);

private:
  wxWeakRef<PSF2> ref_data_;
  // wxScopedArray<char> text_;
};

