#pragma once

#include <wx/vector.h>
#include <wx/file.h>
#include "SoundFormatInfo.h"



////////////////////////////////////////////////////////////////////////
// Sample classes
////////////////////////////////////////////////////////////////////////


class ISampleListener;

class Sample {
public:
  Sample();
  virtual ~Sample() {}

  virtual unsigned int bit_count() const = 0;
  unsigned int byte_count() const { return bit_count() / 8; }

  int volume_left() const;
  int volume_right() const;
  int volume_max() const;
  void set_volume(int);
  void set_volume(int l, int r);
  void set_volume_max(int);

  bool enables_envelope() const;
  int envelope() const;
  int envelope_max() const;
  void set_envelope(int);
  void set_envelope_max(int);

  void GetStereo16(int* l, int* r) const;
  void GetStereo16(short* dest) const;

  virtual unsigned char GetRaw8() const = 0;
  virtual int GetRaw16() const = 0;
  virtual int GetRaw24() const = 0;
  // virtual float Get32f() const = 0;

  virtual void Set8(unsigned char) = 0;
  virtual void Set16(int) = 0;
  virtual void Set24(int) = 0;
  // virtual void Set32f(float) = 0;

  void Mute() { muted_ = true; }
  void Unmute() { muted_ = false; }
  bool IsMuted() const { return muted_; }

  void AddListener(ISampleListener* listener);
  void RemoveListener(ISampleListener* listener);

private:
  int volume_left_;
  int volume_right_;
  int volume_max_;

  int envelope_;
  int envelope_max_;

  bool muted_;
  bool enables_envelope_;

  wxVector<ISampleListener*> listeners_;
};


class ISampleListener {
  friend class Sample;
public:
  virtual ~ISampleListener() {}
protected:
  virtual void OnUpdate(const Sample*) = 0;
};




class Sample8 : public Sample {
public:
  Sample8() : sample_(128) {}
  Sample8(unsigned char s) : sample_(s) {}
  unsigned int bit_count() const { return 8; }

  unsigned char GetRaw8() const;
  int GetRaw16() const;
  int GetRaw24() const;
  void Set8(unsigned char);
  void Set16(int);
  void Set24(int);

private:
  unsigned char sample_;
};



class Sample16 : public Sample {
public:
  Sample16() : sample_(0) {}
  Sample16(int s) : sample_(s) {}
  unsigned int bit_count() const { return 16; }

  unsigned char GetRaw8() const;
  int GetRaw16() const;
  int GetRaw24() const;
  void Set8(unsigned char);
  void Set16(int);
  void Set24(int);

private:
  int sample_;
};




////////////////////////////////////////////////////////////////////////
// SoundBlock classes
////////////////////////////////////////////////////////////////////////


class SoundDeviceDriver;

#include <wx/sharedptr.h>
#include <wx/weakref.h>

class SoundBlock {
public:
  SoundBlock(int WXUNUSED(channel_number) = 2);
  virtual ~SoundBlock();

  virtual unsigned int channel_count() const = 0;
  virtual unsigned int block_size() const = 0;

  virtual Sample& Ch(int) = 0;
  const Sample& Ch(int) const;

  //virtual void GetStereo16(int* l, int* r) const = 0;
  // virtual void GetStereo24(int* l, int* r) const = 0;

  void GetStereo16(short*) const;

  void Reset();

  const wxSharedPtr<SoundDeviceDriver>& output() const;
  void set_output(const wxSharedPtr<SoundDeviceDriver>&);

private:
  wxSharedPtr<SoundDeviceDriver> output_;

};



#include "note.h"


class SoundData;
/*
class SoundFormatListener {
public:
  ~SoundFormatListener() {}

protected:
  virtual void OnUpdate(SoundFormat* sf);
  virtual void OnDestroy(SoundFormat* sf);

  friend class SoundFormat;
};
*/

class Soundbank;


////////////////////////////////////////////////////////////////////////
// SoundInfo classes
////////////////////////////////////////////////////////////////////////

#include <wx/tracker.h>
#include <wx/hashmap.h>

class SoundInfo : public wxTrackable {
public:
  WX_DECLARE_STRING_HASH_MAP(wxString, Tag);

  SoundInfo();
  ~SoundInfo();

  const wxString& title() const { return title_; }
  void set_title(const wxString& title) { title_ = title; }

  const wxString& artist() const { return artist_; }
  void set_artist(const wxString& artist) { artist_ = artist; }

  const wxString& album() const { return album_; }
  void set_album(const wxString& album) { album_ = album; }

  const wxString& year() const { return year_; }
  void set_year(const wxString& year) { year_ = year; }

  const wxString& genre() const { return genre_; }
  void set_genre(const wxString& genre) { genre_ = genre; }

  const wxString& comment() const { return comment_; }
  void set_comment(const wxString& comment) { comment_ = comment; }

  const wxString& copyright() const { return copyright_; }
  void set_copyright(const wxString& copyright) { copyright_ = copyright; }

  const wxString& length() const { return length_; }
  void set_length(const wxString& length) { length_ = length; }

  const Tag& others() const { return others_; }
  void set_tags(const Tag& tags);

private:
  wxString title_;
  wxString artist_;
  wxString album_;
  wxString year_;
  wxString genre_;
  wxString comment_;
  wxString copyright_;
  // int len_minutes_;
  // int len_seconds_;
  wxString length_;

  Tag others_;
};


////////////////////////////////////////////////////////////////////////
// SoundData classes
////////////////////////////////////////////////////////////////////////

/*!
 @brief A base class for Sound Format (?sf).
*/
class SoundData : public wxTrackable
{
public:
  //! A virtual desctructor.
  virtual ~SoundData();

  //! Get the instance of Sample correspond to the channel number.
  Sample& Ch(int);
  //! Get the constant instalce of Sample correspond to the channel number.
  const Sample& Ch(int) const;

  //! Check if the sound is loaded.
  bool IsLoaded() const;

  void GetTag(const wxString &key, wxString *value) const;
  void SetTag(const wxString &key, const wxString &value);

  //! Play the sound.
  bool Play(const wxSharedPtr<SoundDeviceDriver>&);
  //! Stop the sound if it is playing.
  bool Stop();

  //! Returns the reference of Soundbank.
  virtual Soundbank& soundbank() = 0;

  const wxString& GetFileName() const;
  virtual unsigned int GetSamplingRate() const = 0;

  Note GetNote(int ch) const;

  // const Wavetable& wavetable() const;

protected:
  //! A pure virtual function called from Play().
  virtual bool DoPlay() = 0;
  //! A pure virtual function called from Stop().
  virtual bool DoStop() = 0;

  //! Get the sound block correspond to the instance.
  virtual SoundBlock& sound_block() = 0;

  wxFile file_;
  wxString path_;
  bool infoLoaded_;

private:
  SoundFormatInfo m_info;
};


inline bool SoundData::IsLoaded() const {
  return infoLoaded_;
}

inline const wxString& SoundData::GetFileName() const {
  return path_;
}
