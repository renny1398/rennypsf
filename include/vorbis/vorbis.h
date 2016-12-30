#pragma once
#include "common/SoundLoader.h"
#include <wx/string.h>
#include <wx/file.h>
#include <wx/weakref.h>
#include <vorbis/vorbisfile.h>

#include "common/SoundFormat.h"

class VorbisSoundBlock;
class VorbisThread;

class Vorbis : public SoundData {
public:
  Vorbis(OggVorbis_File* vf, long loop_start = -1, long loop_length = 0);
  ~Vorbis();
  
  Soundbank& soundbank();
  
  unsigned int GetSamplingRate() const;
  bool ChangeOutputSamplingRate(uint32_t rate);
  
protected:
  bool DoPlay();
  bool DoStop();

  bool Open(SoundBlock* block);
  bool Close();
  bool DoAdvance(SoundBlock* dest);
  friend class VorbisThread;
  
private:
  Soundbank* soundbank_;
  
  OggVorbis_File* vf_;
  size_t length_;
  long loop_start_;
  long loop_length_;
  long pos_;
  char buffer_[4096*2];
  int buffer_pos_;
  int buffer_size_;
};

class VorbisLoader : public SoundLoader {
public:
  VorbisLoader(int fd, const wxString& filename);
  ~VorbisLoader();
  
  static VorbisLoader* Instance(int fd, const wxString& filename);
  
  SoundInfo* LoadInfo();
  SoundData* LoadData();
  Vorbis* LoadDataEx();
  
private:
  wxFile file_;
  wxString path_;
  
  OggVorbis_File* vf_tmp_;
  static ov_callbacks oc_;
  
  wxWeakRef<SoundInfo> ref_info_;
  wxWeakRef<Vorbis> ref_data_;
  
  long loop_start_;
  long loop_length_;
};

