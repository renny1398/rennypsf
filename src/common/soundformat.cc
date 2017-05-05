#include "common/SoundFormat.h"
#include "common/SoundManager.h"
#include "common/debug.h"
#include <wx/thread.h>

/*
 * Type Conversion Functions
 */
namespace {

inline int CLIP16(int x) {
  if (x < -32768) x = -32768;
  else if (32767 < x) x = 32767;
  return x;
}

unsigned char WordToByte(int word) {
  return static_cast<unsigned char>( ((word >> 8) + 128) & 0xff );
}

unsigned char Word24ToByte(int word24) {
  return static_cast<unsigned char>( ((word24 >> 16) + 128) & 0xff );
}

int ByteToWord(unsigned char byte) {
  return (static_cast<int>(byte) - 128) << 8;
}

int Word24ToWord(int word24) {
  return word24 >> 8;
}

int ByteToWord24(unsigned char byte) {
  return (static_cast<int>(byte) - 128) << 16;
}

int WordToWord24(int word) {
  return word << 8;
}

}   // namespace



Sample::Sample()
  : volume_left_(32767), volume_right_(32767),
    volume_max_(32767),
    envelope_(0), envelope_max_(0),
    muted_(false), enables_envelope_(false) {}


int Sample::volume_left() const {
  if (IsMuted()) return 0;
  return volume_left_;
}

int Sample::volume_right() const {
  if (IsMuted()) return 0;
  return volume_right_;
}

int Sample::volume_max() const {
  return volume_max_;
}

void Sample::set_volume(int vol) {
  set_volume(vol, vol);
}

void Sample::set_volume(int l, int r) {
  volume_left_ = l;
  volume_right_ = r;
}

void Sample::set_volume_max(int vol) {
  if (vol == 0) return;
  volume_max_ = vol;
}



bool Sample::enables_envelope() const { return enables_envelope_; }
int Sample::envelope() const {
  if (enables_envelope_ == false) return 0;
  return envelope_;
}
int Sample::envelope_max() const {
  if (enables_envelope_ == false) return 0;
  return envelope_max_;
}
void Sample::set_envelope(int env) {
  envelope_ = env;
  enables_envelope_ = true;
}
void Sample::set_envelope_max(int env) {
  envelope_max_ = env;
}



void Sample::GetStereo16(int* l, int* r) const {
  rennyAssert(l != nullptr && r != nullptr);
  const int a = GetRaw16();
  *l = static_cast<long>(a) * volume_left_ / volume_max_;
  *r = static_cast<long>(a) * volume_right_ / volume_max_;
}


void Sample::GetStereo16(short* dest) const {
  rennyAssert(dest != nullptr);
  int l, r;
  GetStereo16(&l, &r);
  dest[0] = CLIP16(l);
  dest[1] = CLIP16(r);
}




unsigned char Sample8::GetRaw8() const {
  return sample_;
}

int Sample8::GetRaw16() const {
  return ByteToWord(sample_);
}

int Sample8::GetRaw24() const {
  return ByteToWord24(sample_);
}

void Sample8::Set8(unsigned char s) {
  sample_ = s;
}

void Sample8::Set16(int s) {
  sample_ = WordToByte(s);
}

void Sample8::Set24(int s) {
  sample_ = Word24ToByte(s);
}




unsigned char Sample16::GetRaw8() const {
  return WordToByte(sample_);
}

int Sample16::GetRaw16() const {
  return sample_;
}

int Sample16::GetRaw24() const {
  return WordToWord24(sample_);
}

void Sample16::Set8(unsigned char s) {
  sample_ = ByteToWord(s);
}

void Sample16::Set16(int s) {
  sample_ = s;
}

void Sample16::Set24(int s) {
  sample_ = Word24ToWord(s);
}


////////////////////////////////////////////////////////////////////////
// SoundSequence class functions
////////////////////////////////////////////////////////////////////////

SampleSequence::SampleSequence()
  : vol_left_(1.0f), vol_right_(1.0f),
    muted_(false), enables_env_(false), env_max_(0)
{}

void SampleSequence::ClearData() {
  samples_.clear();
}

void SampleSequence::volume(int seq_no, float* left, float* right) const {
  rennyAssert(seq_no < static_cast<int>(samples_.size()));
  const SampleEx& sample = samples_.at(seq_no);
  if (left)  *left  = sample.vol_left_;
  if (right) *right = sample.vol_right_;
}

void SampleSequence::set_volume(float left, float right) {
  vol_left_  = left;
  vol_right_ = right;
}

void SampleSequence::set_volume(int seq_no, float left, float right) {
  rennyAssert(seq_no < static_cast<int>(samples_.size()));
  SampleEx& sample = samples_.at(seq_no);
  sample.vol_left_  = left;
  sample.vol_right_ = right;
}

void SampleSequence::Pushf(float sample, int env) {
  SampleEx s;
  s.sample_ = sample;
  s.vol_left_  = vol_left_;
  s.vol_right_ = vol_right_;
  s.env_ = env;
  samples_.push_back(s);
}

void SampleSequence::Push16i(int sample, int env) {
  SampleEx s;
  s.sample_ = static_cast<float>(sample) / 32768.0f;
  s.vol_left_  = vol_left_;
  s.vol_right_ = vol_right_;
  s.env_ = env;
  samples_.push_back(s);
}

void SampleSequence::Getf(int seq_no, float* left, float* right) const {
  rennyAssert(seq_no < static_cast<int>(samples_.size()));
  const SampleEx& sample = samples_.at(seq_no);
  if (left)  *left  = sample.sample_ * sample.vol_left_;
  if (right) *right = sample.sample_ * sample.vol_right_;
}

void SampleSequence::Get16i(int seq_no, int* left, int* right) const {
  rennyAssert(seq_no < static_cast<int>(samples_.size()));
  const SampleEx& sample = samples_.at(seq_no);
  if (left)  *left  = static_cast<int>(sample.sample_ * sample.vol_left_  * 32768.0f);
  if (right) *right = static_cast<int>(sample.sample_ * sample.vol_right_ * 32768.0f);
}

int SampleSequence::GetEnv(int seq_no) const {
  rennyAssert(seq_no < static_cast<int>(samples_.size()));
  return enables_env_ ? samples_.at(seq_no).env_ : 0;
}


////////////////////////////////////////////////////////////////////////
// SoundBlock class functions
////////////////////////////////////////////////////////////////////////


SoundBlock::SoundBlock(int channel_number) : samples_(channel_number), rvb_is_enabled_(false) {}
SoundBlock::~SoundBlock() {}

size_t SoundBlock::sample_length() const {
  size_t ret = 0;
  for (const auto& ch : samples_) {
    auto l = ch.sample_length();
    if (ret < l) ret = l;
  }
  return ret;
}

void SoundBlock::ChangeChannelCount(int new_channel_count) {
  if (new_channel_count != static_cast<int>(samples_.size())) {
    samples_.assign(new_channel_count, SampleSequence());
  }
}

void SoundBlock::GetStereo16(int seq_no, short* dest) const {
  float l = 0.0f, r = 0.0f;
  float tmp_l, tmp_r;
  const unsigned int ch_count = channel_count();
  for (unsigned int i = 0; i < ch_count; ++i) {
    Ch(i).Getf(seq_no, &tmp_l, &tmp_r);
    l += tmp_l;
    r += tmp_r;
  }

  if (ReverbIsEnabled()) {
    ReverbCh(0).Getf(seq_no, &tmp_l, &tmp_r);
    l += tmp_l;
    r += tmp_r;
    ReverbCh(1).Getf(seq_no, &tmp_l, &tmp_r);
    l += tmp_l;
    r += tmp_r;
  }

  dest[0] = CLIP16(l*32768);
  dest[1] = CLIP16(r*32768);
}

void SoundBlock::Clear() {
  const unsigned int ch_count = channel_count();
  for (unsigned int i = 0; i < ch_count; ++i) {
    // Ch(i).Set16(0);
    Ch(i).ClearData();
  }
  rvb_sample_[0].ClearData();
  rvb_sample_[1].ClearData();
}

void SoundBlock::Reset() {
  output_ = nullptr;
  const unsigned int ch_count = channel_count();
  for (unsigned int i = 0; i < ch_count; ++i) {
    SampleSequence& s = Ch(i);
    // s.Set16(0);
    s.ClearData();
    s.set_volume(1.0f, 1.0f);
  }
  rvb_sample_[0].ClearData();
  rvb_sample_[1].ClearData();
}



SoundDevice* SoundBlock::output() {
  return output_;
}


void SoundBlock::set_output(SoundDevice* output) {
  output_ = output;
}


bool SoundBlock::NotifyDevice() {
  if (output_ == nullptr) return false;
  output_->OnUpdate(this);
  return true;
}


class SoundBlock16 : public SoundBlock {
public:
  explicit SoundBlock16(int channel_number = 2) : samples_(channel_number) {
    if (channel_number == 2) {
      samples_[0].set_volume(32767, 0);
      samples_[1].set_volume(0, 32767);
    }
  }

  unsigned int channel_count() const { return samples_.size(); }
  unsigned int block_size() const { return 2 * channel_count(); }

  Sample& Ch(int ch) { return samples_.at(ch); }

private:
  wxVector<Sample16> samples_;
};



SoundInfo::SoundInfo() {}
SoundInfo::~SoundInfo() {}


void SoundInfo::set_tags(const Tag &tags) {
  for (Tag::const_iterator it = tags.begin(); it != tags.end(); ++it) {
    const wxString& key = it->first;
    if (key == "title") {
      title_ = it->second;
    } else if (key == "artist") {
      artist_ = it->second;
    } else if (key == "album" || key == "game") {
      album_ = it->second;
    } else if (key == "year") {
      year_ = it->second;
    } else if (key == "genre") {
      genre_ = it->second;
    } else if (key == "comment") {
      comment_ = it->second;
    } else if (key == "copyright") {
      copyright_ = it->second;
    } else if (key == "length") {
      length_ = it->second;
    } else {
      others_[key] = it->second;
    }
  }
}


bool SoundData::Advance(SoundBlock *dest) {
  if (dest == nullptr) return false;
  dest->Clear();
  bool ret = DoAdvance(dest);
  if (ret) {
    pos_++;
    /*
    if (pos_ % GetSamplingRate() == 0) {
      rennyLogDebug("SoundData", "%d seconds...", pos_ / GetSamplingRate());
    }
    */
  }
  return ret;
}
