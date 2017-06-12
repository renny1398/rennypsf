#pragma once
#include "common.h"
#include "memory.h"
#include <cstdint>
#include <wx/string.h>
#include <wx/vector.h>
#include <wx/hashmap.h>

class PSF2File;
class PSF2Directory;

namespace psx {

class IOP : Component, private UserMemoryAccessor {

public:
  IOP(PSX* psx);
  ~IOP();

  bool Call(PSXAddr pc, uint32_t call_num);

  bool stdio(uint32_t call_num);
  bool sifman(uint32_t call_num);
  bool thbase(uint32_t call_num);
  bool thevent(uint32_t call_num);
  bool thsemap(uint32_t call_num);
  bool timrman(uint32_t call_num);
  bool sysclib(uint32_t call_num);
  bool intrman(uint32_t call_num);
  bool loadcore(uint32_t call_num);
  bool sysmem(uint32_t call_num);
  bool modload(uint32_t call_num);
  bool ioman(uint32_t call_num);

  const wxString sprintf(const wxString& format, uint32_t param1, uint32_t param2, uint32_t param3) const;

  void SetRootDirectory(const PSF2Directory* root);
  unsigned int LoadELF(PSF2File* psf2irx);

  PSXAddr load_addr() const ;
  void set_load_addr(PSXAddr load_addr);

private:
  typedef bool (IOP::*InternalLibraryCallback)(uint32_t);
  void RegisterInternalLibrary(const wxString& name, InternalLibraryCallback callback);
  InternalLibraryCallback GetInternalLibraryCallback(const wxString& name);
  bool CallExternalLibrary(const wxString& name, uint32_t call_num);

  const PSF2Directory* root_;
#ifndef NDEBUG
  mips::Disassembler* p_disasm_;
#endif

  struct File {
    PSF2File* file;
    size_t pos;
    File() : file(0), pos(0) {}
  };
  wxVector<File> files_;
  WX_DECLARE_STRING_HASH_MAP(InternalLibraryCallback, InternalLibraryMap);
  InternalLibraryMap internal_lib_map_;
  struct ExternalLibEntry {
    wxString name_;
    uint32_t dispatch_;
    ExternalLibEntry(wxString name, uint32_t dispatch) :
      name_(name), dispatch_(dispatch) {}
  };
  wxVector<ExternalLibEntry> lib_entries_;

  PSXAddr load_addr_;
};



} // namespace PSX
