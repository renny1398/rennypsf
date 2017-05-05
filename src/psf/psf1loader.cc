#include "psf/psfloader.h"
#include "psf/psf.h"
#include "common/debug.h"
#include <wx/string.h>
#include <wx/scopedarray.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/zstream.h>
#include <wx/filename.h>
#include <wx/regex.h>


PSFLoader::PSFLoader(int fd, const wxString &filename)
  : file_(fd), path_(filename),
    reserved_area_len_(0), binary_len_(0), binary_crc32_(0),
    reserved_area_ofs_(0), binary_ofs_(0) {
}


wxFile& PSFLoader::file() {
  return file_;
}

const wxString& PSFLoader::path() const {
  return path_;
}

uint32_t PSFLoader::reserved_area_len() const {
  return reserved_area_len_;
}

uint32_t PSFLoader::binary_len() const {
  return binary_len_;
}

uint32_t PSFLoader::binary_crc32() const {
  return binary_crc32_;
}

uint32_t PSFLoader::reserved_area_ofs() const {
  return reserved_area_ofs_;
}

uint32_t PSFLoader::binary_ofs() const {
  return binary_ofs_;
}


wxVector<PSFLoader*>::iterator PSFLoader::psflib_begin() {
  return psflibs_.begin();
}

wxVector<PSFLoader*>::const_iterator PSFLoader::psflib_begin() const {
  return psflibs_.begin();
}

wxVector<PSFLoader*>::const_iterator PSFLoader::psflib_end() const {
  return psflibs_.end();
}


SoundInfo* PSFLoader::LoadInfo() {
  if (ref_info_ != nullptr) {
    return ref_info_;
  }

  wxFileInputStream fs(file_);
  fs.SeekI(4, wxFromStart);

  fs.Read(&reserved_area_len_, 4);
  fs.Read(&binary_len_, 4);
  fs.Read(&binary_crc32_, 4);
  reserved_area_ofs_ = fs.TellI();
  fs.SeekI(reserved_area_len_, wxFromCurrent);
  binary_ofs_ = fs.TellI();
  fs.SeekI(binary_len_, wxFromCurrent);

  SoundInfo* info = new SoundInfo();
  if (fs.Eof() == false) {
    char strTAG[5];
    fs.Read(strTAG, 5);
    if (::memcmp(strTAG, "[TAG]", 5) == 0) {  // strTAG == "[TAG]"
      wxTextInputStream ts(fs, wxT("\n"));
      wxRegEx re("^([^=\\s]+)\\s*=\\s*(.*)$");
      rennyAssert(re.IsValid());
      SoundInfo::Tag tag;
      while (fs.Eof() == false) {
        wxString process_text;
        ts >> process_text;
        while (re.Matches(process_text)) {
          size_t start, len;
          re.GetMatch(&start, &len, 0);
          wxString k(re.GetMatch(process_text, 1));
          wxString v(re.GetMatch(process_text, 2));
          tag[k] = v;
          rennyLogDebug("PSFLoader", "[TAG] %s = %s", (const char*)k, (const char*)v);

          process_text = process_text.Mid(start + len);
        }
      }
      info->set_tags(tag);
    }
  }

  ref_info_ = info;
  return info;
}


bool PSFLoader::LoadLibraries() {

  const SoundInfo::Tag& tags = LoadInfo()->others();

  wxString directory, filename, ext;
  wxFileName::SplitPath(path_, &directory, &filename, &ext);

  for (int i = 1; i < 10; i++) {
    wxString _libN;
    if (i <= 1) {
      _libN.assign("_lib");
    } else {
      _libN.sprintf("_lib%d", i);
    }
    SoundInfo::Tag::const_iterator it = tags.find(_libN);
    if (it == tags.end()) break;

    wxString lib_filename(directory);
    lib_filename.Append(wxFileName::GetPathSeparator());
    lib_filename.Append(it->second);

    PSFLoader* loader = dynamic_cast<PSFLoader*>(SoundLoader::Instance(lib_filename));
    if (loader == nullptr) continue;

    psflibs_.push_back(loader);
  }

  return true;
}


PSF1Loader::PSF1Loader(int fd, const wxString &filename)
  : PSFLoader(fd, filename) {
}


PSF1Loader* PSF1Loader::Instance(int fd, const wxString &filename) {
  return new PSF1Loader(fd, filename);
}


bool PSF1Loader::GetInitRegs(uint32_t *pc0, uint32_t *gp0, uint32_t *sp0) const {
  const wxVector<PSFLoader*>::const_iterator it = psflib_begin();
  if (it != psflib_end()) {
    const PSF1Loader* const loader = dynamic_cast<PSF1Loader*>(*it);
    if (loader != nullptr) {
      return loader->GetInitRegs(pc0, gp0, sp0);
    }
  }
  if (header_ == 0) return false;
  if (pc0) *pc0 = header_->pc0;
  if (gp0) *gp0 = header_->gp0;
  if (sp0) *sp0 = header_->sp0;
  return true;
}


bool PSF1Loader::LoadEXE() {

  wxFileInputStream fs(file());
  fs.SeekI(binary_ofs());
  wxZlibInputStream zlib_stream(fs);

  wxScopedPtr<PSXEXEHeader> header(new PSXEXEHeader);
  zlib_stream.Read(header.get(), sizeof(PSXEXEHeader));
  zlib_stream.SeekI(0x800 - sizeof(PSXEXEHeader), wxFromCurrent);

  if (::memcmp(header->signature, "PS-X EXE", 8) != 0) /* header.signature <> "PS-X EXE" */ {
    const wxString str_sign(header->signature, 8);
    rennyLogError("PSF1Loader", "Uncompressed binary is not PS-X EXE! (%s)", static_cast<const char*>(str_sign));
    return false;
  }
  header.swap(header_);

  wxScopedArray<char> text(new char[0x200000]);
  zlib_stream.Read(text.get(), 0x200000);
  text.swap(text_);

  LoadLibraries();

  wxVector<PSFLoader*>::iterator lib_it = psflib_begin();
  wxVector<PSFLoader*>::const_iterator lib_it_end = psflib_end();
  for (; lib_it != lib_it_end; ++lib_it) {
    PSF1Loader* loader = dynamic_cast<PSF1Loader*>(*lib_it);
    if (loader != nullptr) {
      loader->LoadInfo();
      loader->LoadEXE();
    }
  }

  return true;
}


bool PSF1Loader::LoadText(PSF1* p_psf) {
  if (p_psf == nullptr || header_ == nullptr || text_ == nullptr) {
    return false;
  }

  for (wxVector<PSFLoader*>::iterator it = psflib_begin(); it != psflib_end(); ++it) {
    PSF1Loader* const loader = dynamic_cast<PSF1Loader*>(*it);
    rennyAssert(loader != nullptr);
    loader->LoadText(p_psf);
  }
  p_psf->PSXMemCpy(header_->text_addr, text_.get(), header_->text_size);
  return true;
}


PSF1* PSF1Loader::LoadDataEx() {

  LoadInfo();
  if (ref_data_ != nullptr) {
    // TODO: support reload
    return ref_data_;
  }

  LoadEXE();

  uint32_t pc0(0), gp0(0), sp0(0);
  GetInitRegs(&pc0, &gp0, &sp0);

  PSF1* p_psf = new PSF1(pc0, gp0, sp0);
  LoadText(p_psf);

  rennyLogDebug("PSF1Loader", "PSF File '%s' is loaded.", static_cast<const char*>(path()));

  ref_data_ = p_psf;
  return p_psf;
}


SoundData* PSF1Loader::LoadData() {
  return LoadDataEx();
}
