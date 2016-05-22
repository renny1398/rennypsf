#pragma once

#include "SoundLoader.h"
#include "PSF.h"

/*
class PSFLoader: public SoundLoader
{
public:
    PSFLoader() {}
    ~PSFLoader();

    const wxString &GetPath() const;

    SoundInfo* LoadInfo();
};
*/



#include <wx/string.h>
#include <wx/file.h>
#include <wx/weakref.h>
#include <wx/vector.h>
#include <cstdint>

class SoundInfo;
class SoundData;
class PSF1;

class PSF1Loader: public SoundLoader
{
public:
  ~PSF1Loader() {}

  SoundInfo* LoadInfo();
  SoundData* LoadData();
  PSF1* LoadDataEx();

  static PSF1Loader* Instance(int fd, const wxString& filename);

  // Accessors
  // uint32_t pc0() const;
  // uint32_t gp0() const;
  // uint32_t sp0() const;

protected:
  PSF1Loader(int fd, const wxString& filename);

  bool LoadEXE();
  bool LoadText(PSF1* p_psf);

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
  wxFile file_;
  wxString path_;
  wxWeakRef<SoundInfo> ref_info_;
  wxWeakRef<PSF1> ref_data_;
  wxScopedPtr<PSXEXEHeader> header_;
  wxScopedArray<char> text_;

  //! PSF1 records
  uint32_t reserved_area_len_;
  uint32_t binary_len_;
  uint32_t binary_crc32_;

  //! Area offsets
  uint32_t reserved_area_ofs_;
  uint32_t binary_ofs_;

  //! psflibs
  wxVector<PSF1Loader*> psflibs_;
};


class PSF2;

/*
class PSF2Loader : public PSFLoader
{
public:
    static PSF2Loader *Instance(int fd, const wxString& filename);
    friend class PSF2;
};
*/
