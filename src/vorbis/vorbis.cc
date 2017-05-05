#include "vorbis/vorbis.h"
#include "common/soundbank.h"
#include "common/SoundManager.h"
#include "common/debug.h"
#include <wx/thread.h>


class VorbisSoundBlock : public SoundBlock {
public:
  VorbisSoundBlock(vorbis_info* vi);
  ~VorbisSoundBlock();
  
  unsigned int channel_count() const;
  unsigned int block_size() const;
  
  Sample& Ch(int);
  
private:
  vorbis_info* vi_;
  wxVector<Sample16> samples_;
};


VorbisSoundBlock::VorbisSoundBlock(vorbis_info* vi)
: SoundBlock(vi->channels), vi_(vi), samples_(vi->channels) {

  if (vi->channels == 1) {
    Sample& sample = samples_.at(0);
    sample.set_volume_max(32767);
    sample.set_volume(32767, 32767);
    return;
  }

  for (int i = 0; i < vi->channels; i++) {
    Sample& sample = samples_.at(i);
    sample.set_volume_max(32767);
    if (i & 1) {
      sample.set_volume(0, 32767);
    } else {
      sample.set_volume(32767, 0);
    }
  }
}

VorbisSoundBlock::~VorbisSoundBlock() {}

unsigned int VorbisSoundBlock::channel_count() const {
  return vi_->channels;
}

unsigned int VorbisSoundBlock::block_size() const {
  return vi_->channels * 2; // TODO: support float32.
}

Sample& VorbisSoundBlock::Ch(int ch) {
  return samples_.at(ch);
}


Vorbis::Vorbis(OggVorbis_File* vf, long loop_start, long loop_length)
: soundbank_(new Soundbank()), vf_(vf), length_(ov_pcm_total(vf, -1)),
  loop_start_(loop_start), loop_length_(loop_length), pos_(0L), buffer_pos_(0), buffer_size_(0) {
}

Vorbis::~Vorbis() {
  DoStop();
  ov_clear(vf_);
  delete vf_;
  delete soundbank_;
}


Soundbank& Vorbis::soundbank() { return *soundbank_; }

unsigned int Vorbis::GetSamplingRate() const {
  vorbis_info* vi = ov_info(vf_, -1);
  return vi->rate;
}

bool Vorbis::ChangeOutputSamplingRate(uint32_t WXUNUSED(rate)) {
  return false;
}


bool Vorbis::DoPlay() {
  return true;
}


bool Vorbis::DoStop() {
  return true;
}


bool Vorbis::Open(SoundBlock* block) {

  vorbis_info* vi = ov_info(vf_, -1);

  block->ChangeChannelCount(vi->channels);

  if (vi->channels == 1) {
    SampleSequence& sample_seq = block->Ch(0);
    sample_seq.set_volume(1.0f, 1.0f);
    return true;
  }

  for (int i = 0; i < vi->channels; i++) {
    SampleSequence& sample_seq = block->Ch(i);
    if (i & 1) {
      sample_seq.set_volume(0, 1.0f);
    } else {
      sample_seq.set_volume(1.0f, 0);
    }
  }
  return true;
}

bool Vorbis::Close() {
  return true;
}

bool Vorbis::DoAdvance(SoundBlock* dest) {
  if (buffer_pos_ == 0 || buffer_size_ <= buffer_pos_) {
    buffer_pos_ = 0;
    buffer_size_ = 0;
    int bitstream;
    do {
      long read_size = ov_read(vf_, buffer_ + buffer_size_, 4096, 0, 2, 1, &bitstream);
      if (read_size <= 0) break;
      buffer_size_ += read_size;
    } while (buffer_size_ < 4096);
  }
  // TODO: support monoral sound.
  short l = reinterpret_cast<short*>(&buffer_[buffer_pos_+0])[0];
  short r = reinterpret_cast<short*>(&buffer_[buffer_pos_+2])[0];
  buffer_pos_ += 4;
  pos_++;
  
  dest->Ch(0).Push16i(l);
  dest->Ch(1).Push16i(r);

  // NotifyDevice();
  
  if (0 <= loop_start_ && 0 < loop_length_) {
    if (loop_start_ + loop_length_ <= pos_) {
      rennyLogDebug("Vorbis", "Playing position reached the loop-end point.");
      pos_ = loop_start_;
      buffer_pos_ = 0;
      buffer_size_ = 0;
      ov_pcm_seek(vf_, pos_);
    }
  } else if (IsRepeated() && static_cast<decltype(pos_)>(length_) <= pos_) {
    rennyLogDebug("Vorbis", "Playing position reached the end.");
    pos_ = 0;
    buffer_pos_ = 0;
    buffer_size_ = 0;
    ov_pcm_seek(vf_, 0);
  }
  
  return true;
}
