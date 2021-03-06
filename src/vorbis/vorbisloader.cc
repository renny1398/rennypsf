#include "vorbis/vorbis.h"
#include "common/debug.h"
#include <vorbis/vorbisfile.h>
#include <stdint.h>

namespace {

  size_t vread(void *ptr, size_t size, size_t nmemb, void *datasource)
  {
    if (datasource == nullptr) return -1;
    wxFile* const fp = static_cast<wxFile*>(datasource);
    if (fp->Eof()) return 0;
    ssize_t read_size = fp->Read(ptr, size*nmemb);
    return read_size / size;
  }
  
  int vseek(void *datasource, ogg_int64_t offset, int whence)
  {
    if (datasource == nullptr) return -1;
    wxFile* const fp = static_cast<wxFile*>(datasource);
    switch (whence) {
      case SEEK_SET:
        return fp->Seek(offset, wxFromStart);
      case SEEK_CUR:
        return fp->Seek(offset, wxFromCurrent);
      case SEEK_END:
        return fp->Seek(offset, wxFromEnd);
      default:
        return -1;
    }
  }
  
  long vtell(void *datasource)
  {
    if (datasource == nullptr) return -1;
    wxFile* const fp = static_cast<wxFile*>(datasource);
    return fp->Tell();
  }
  
  int vclose(void *datasource)
  {
    if (datasource == nullptr) return -1;
    wxFile* const fp = static_cast<wxFile*>(datasource);
    fp->Close();
    return 0;
  }

}


ov_callbacks VorbisLoader::oc_ = {
  &vread, &vseek, &vclose, &vtell
};


VorbisLoader::VorbisLoader(int fd, const wxString& filename)
  : file_(fd), path_(filename), loop_start_(-1), loop_length_(0) {
    
  vf_tmp_ = new OggVorbis_File();
  file_.Seek(0, wxFromStart);

  switch (ov_open_callbacks(&file_, vf_tmp_, nullptr, 0, oc_)) {
  case OV_EREAD:
    rennyLogError("VorbisLoader", "A read from media returned an error");
    break;
  case OV_ENOTVORBIS:
    rennyLogError("VorbisLoader", "Bitstream is not Vorbis data.");
    break;
  case OV_EVERSION :
    rennyLogError("VorbisLoader", "Vorbis version mismatch.");
    break;
  case OV_EBADHEADER:
    rennyLogError("VorbisLoader", "Invalid Vorbis bitstream header.");
    break;
  case OV_EFAULT:
    rennyLogError("VorbisLoader", "Internal logic fault; indicates a bug or heap/stack corruption.");
    break;
  }
}

VorbisLoader::~VorbisLoader() {
  rennyLogDebug("VorbisLoader", "Destroy VorbisLoader.");
}


VorbisLoader* VorbisLoader::Instance(int fd, const wxString& filename) {
  return new VorbisLoader(fd, filename);
}


SoundInfo* VorbisLoader::LoadInfo() {

  if (ref_info_ != nullptr) {
    return ref_info_;
  }
  
  vorbis_comment* vc = ov_comment(vf_tmp_, -1);
  if (vc == nullptr) return nullptr;
  
  SoundInfo* info = new SoundInfo();
  char* comment;
  comment = vorbis_comment_query(vc, "TITLE", 0);
  if (comment) {
    info->set_title(comment);
  }
  comment = vorbis_comment_query(vc, "ARTIST", 0);
  if (comment) {
    info->set_artist(comment);
  }
  comment = vorbis_comment_query(vc, "LOOPSTART", 0);
  if (comment) {
    wxString(comment).ToLong(&loop_start_);
    // rennyLogDebug("VorbisLoader", "LOOPSTART = %d", loop_start_);
  } else {
    loop_start_ = -1L;
  }
  comment = vorbis_comment_query(vc, "LOOPLENGTH", 0);
  if (comment) {
    wxString(comment).ToLong(&loop_length_);
    // rennyLogDebug("VorbisLoader", "LOOPLENGTH = %d", loop_length_);
  } else {
    loop_length_ = 0L;
  }
  
  ref_info_ = info;
  return info;
}


Vorbis* VorbisLoader::LoadDataEx() {
  
  LoadInfo();
  if (ref_data_ != nullptr) {
    return ref_data_;
  }
  
  Vorbis* vorbis = new Vorbis(vf_tmp_, loop_start_, loop_length_);
  if (vorbis) vf_tmp_ = nullptr;

  ref_data_ = vorbis;
  return vorbis;
}


SoundData* VorbisLoader::LoadData() {
  return LoadDataEx();
}


