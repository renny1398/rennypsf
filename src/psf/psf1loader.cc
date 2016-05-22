#include "psf/psfloader.h"
#include <wx/string.h>
#include <wx/scopedarray.h>
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/zstream.h>
#include <wx/filename.h>
#include <wx/regex.h>
#include <wx/msgout.h>


PSF1Loader::PSF1Loader(int fd, const wxString &filename)
  : file_(fd), path_(filename) {
}


PSF1Loader* PSF1Loader::Instance(int fd, const wxString &filename) {
  return new PSF1Loader(fd, filename);
}


bool PSF1Loader::GetInitRegs(uint32_t *pc0, uint32_t *gp0, uint32_t *sp0) const {
  if (psflibs_.empty() == false) {
    return psflibs_[0]->GetInitRegs(pc0, gp0, sp0);
  }
  if (header_ == 0) return false;
  if (pc0) *pc0 = header_->pc0;
  if (gp0) *gp0 = header_->gp0;
  if (sp0) *sp0 = header_->sp0;
  return true;
}


SoundInfo* PSF1Loader::LoadInfo() {
  if (ref_info_ != NULL) {
    return ref_info_;
  }

  wxFileInputStream fs(file_);
  fs.SeekI(4, wxFromStart);

  // fs >> reserved_area_len_ >> binary_len_ >> binary_crc32_;
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
    if (::memcmp(strTAG, "[TAG]", 5) == 0) {
      wxTextInputStream ts(fs, wxT("\n"));
      wxRegEx re("^([^=\\s]+)\\s*=\\s*(.*)$");
      wxASSERT(re.IsValid());
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
          wxMessageOutputDebug().Printf(wxT("[TAG] %s = %s"), k, v);
          process_text = process_text.Mid(start + len);
        }
      }
      info->set_tags(tag);
    }
  }

  ref_info_ = info;
  return info;
}


bool PSF1Loader::LoadEXE() {

  wxFileInputStream fs(file_);
  fs.SeekI(binary_ofs_);
  wxZlibInputStream zlib_stream(fs);

  wxScopedPtr<PSXEXEHeader> header(new PSXEXEHeader);
  zlib_stream.Read(header.get(), sizeof(PSXEXEHeader));
  zlib_stream.SeekI(0x800 - sizeof(PSXEXEHeader), wxFromCurrent);

  if (::memcmp(header->signature, "PS-X EXE", 8) != 0) /* header.signature <> "PS-X EXE" */ {
    const wxString str_sign(header->signature, 8);
    wxMessageOutputStderr().Printf(wxT("Uncompressed binary is not PS-X EXE! (%s)"), str_sign);
    return false;
  }
  header.swap(header_);

  wxScopedArray<char> text(new char[0x200000]);
  zlib_stream.Read(text.get(), 0x200000);
  text.swap(text_);

  const SoundInfo::Tag& tags = ref_info_->others();

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

    PSF1Loader* loader = dynamic_cast<PSF1Loader*>(SoundLoader::Instance(lib_filename));
    if (loader == NULL) continue;

    loader->LoadInfo();
    loader->LoadEXE();
    psflibs_.push_back(loader);
  }

  return true;
}


bool PSF1Loader::LoadText(PSF1* p_psf) {
  if (p_psf == NULL || header_ == NULL || text_ == NULL) {
    return false;
  }
  for (wxVector<PSF1Loader*>::iterator it = psflibs_.begin(); it != psflibs_.end(); ++it) {
    (*it)->LoadText(p_psf);
  }
  p_psf->PSXMemCpy(header_->text_addr, text_.get(), header_->text_size);
  return true;
}


PSF1* PSF1Loader::LoadDataEx() {

  if (ref_info_ == NULL) {
    LoadInfo();
  } else if (ref_data_ != NULL) {
    // TODO: support reload
    return ref_data_;
  }

  LoadEXE();

  uint32_t pc0(0), gp0(0), sp0(0);
  GetInitRegs(&pc0, &gp0, &sp0);

  PSF1* p_psf = new PSF1(pc0, gp0, sp0);
  LoadText(p_psf);

  // wxMessageOutputDebug().Printf("PC0 = 0x%08x, GP0 = 0x%08x, SP0 = 0x%08x", psx_->R3000ARegs().GPR.PC, psx_->R3000ARegs().GPR.GP, psx_->R3000ARegs().GPR.SP);
  wxMessageOutputDebug().Printf("PSF File '%s' is loaded.", path_);

  ref_data_ = p_psf;
  return p_psf;
}


SoundData* PSF1Loader::LoadData() {
  return LoadDataEx();
}
