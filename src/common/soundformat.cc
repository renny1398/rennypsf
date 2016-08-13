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
  return volume_left_;
}

int Sample::volume_right() const {
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
  rennyAssert(l != NULL && r != NULL);
  const int a = GetRaw16();
  *l = static_cast<long>(a) * volume_left_ / volume_max_;
  *r = static_cast<long>(a) * volume_right_ / volume_max_;
}


void Sample::GetStereo16(short* dest) const {
  rennyAssert(dest != NULL);
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




/*
class Sample24 : public Sample {
public:
  Sample24() : sample_(0) {}
  Sample24(int s) : sample_(s) {}
  unsigned int bit_number() const { return 24; }

  unsigned char GetRaw8() const { return Word24ToByte(sample_); }
  int GetRaw16() const { return Word24ToWord(sample_); }
  int GetRaw24() const { return sample_; }
  void Set8(unsigned char s) { sample_ = ByteToWord24(s); }
  void Set16(int s) { sample_ = WordToWord24(s); }
  void Set24(int s) { sample_ = s; }

private:
  int sample_;
};
*/



SoundBlock::SoundBlock(int) {}
SoundBlock::~SoundBlock() {}


const Sample& SoundBlock::Ch(int ch) const {
  SoundBlock* const this_casted = const_cast<SoundBlock*>(this);
  return this_casted->Ch(ch);
}


void SoundBlock::GetStereo16(short* dest) const {
  int l = 0, r = 0;
  int tmp_l, tmp_r;
  const unsigned int ch_count = channel_count();
  for (unsigned int i = 0; i < ch_count; ++i) {
    Ch(i).GetStereo16(&tmp_l, &tmp_r);
    l += tmp_l;
    r += tmp_r;
  }
  dest[0] = CLIP16(l);
  dest[1] = CLIP16(r);
}


void SoundBlock::Reset() {
  output_ = NULL;
  const unsigned int ch_count = channel_count();
  for (unsigned int i = 0; i < ch_count; ++i) {
    Sample& s = Ch(i);
    s.Set16(0);
    s.set_volume(32767);
    s.set_volume_max(32767);
  }
}



SoundDevice* SoundBlock::output() {
  return output_;
}


void SoundBlock::set_output(SoundDevice* output) {
  output_ = output;
}


bool SoundBlock::NotifyDevice() {
  if (output_ == NULL) return false;
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




SoundData::~SoundData() {}


Sample& SoundData::Ch(int ch) {
  return sound_block().Ch(ch);
}


const Sample& SoundData::Ch(int ch) const {
  SoundData* this_casted = const_cast<SoundData*>(this);
  return this_casted->Ch(ch);
}



bool SoundData::Play(SoundDevice* sdd) {
  sdd->Listen();
  sound_block().set_output(sdd);
  return DoPlay();
}


bool SoundData::Stop() {
  SoundBlock& sb = sound_block();
  SoundDevice* sdd = sb.output();
  sb.Reset();
  bool ret = DoStop();
  if (sdd != 0) {
    sdd->Stop();
  }
  return ret;
}


bool SoundData::NotifyDevice() {
  return sound_block().NotifyDevice();
}


class OrdinarySoundThread : public wxThread {
public:
  OrdinarySoundThread(OrdinarySoundData* data);
protected:
  wxThread::ExitCode Entry();
private:
  OrdinarySoundData* const data_;
};


OrdinarySoundThread::OrdinarySoundThread(OrdinarySoundData* data) : wxThread(wxTHREAD_JOINABLE), data_(data) {}

wxThread::ExitCode OrdinarySoundThread::Entry() {
  if (data_ == NULL) return NULL;
  while (!TestDestroy()) {
    if (data_->Advance() == false) break;
  }
  return NULL;
}


OrdinarySoundData::OrdinarySoundData()
: thread_(NULL) {}

OrdinarySoundData::~OrdinarySoundData() {
  DoStop();
  if (thread_) {
    delete thread_;
  }
}


bool OrdinarySoundData::DoPlay() {
  if (thread_ == NULL) {
    thread_ = new OrdinarySoundThread(this);
  }
  thread_->Create();

  // pos_ = 0L;
  // buffer_pos_ = 0;

  thread_->Run();
  return true;
}


bool OrdinarySoundData::DoStop() {
  if (thread_) {
    thread_->Delete();
    thread_->Wait();
  }
  return true;
}
