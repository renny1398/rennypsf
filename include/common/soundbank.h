#ifndef SOUNDBANK_H_
#include <wx/hashmap.h>


class Instrument;

class InstrumentDataIterator {
 public:
  InstrumentDataIterator();
  InstrumentDataIterator(const Instrument* p_inst, bool is_loop);
  ~InstrumentDataIterator();

  bool HasNext() const;
  int Next();

 private:
  const Instrument* p_inst_;
  bool is_loop_;
  int index_;
  mutable int data_;
};


class Instrument {
 public:

  static const int kInvalidData = 0x80000000;

  Instrument();
  virtual ~Instrument() {}

  virtual int id() const = 0;
  virtual int at(int i) const = 0;
  // virtual short* data() const = 0;
  virtual unsigned int length() const = 0;
  virtual int loop() const = 0;

  void Mute();
  void Unmute();
  bool IsMuted() const;

  InstrumentDataIterator Iterator(bool is_loop) const;

  // int sampling_rate() const;
  double freq() const;
  void set_freq(double);

 private:
  bool is_muted_;
  double freq_;
};


class Soundbank;

class SoundbankObserver {
 public:
  virtual ~SoundbankObserver() {}
  virtual void Update(const Soundbank&) = 0;
  virtual void Update(const Instrument&) = 0;
};


class Soundbank {
 public:
  WX_DECLARE_HASH_MAP(int, Instrument*, wxIntegerHash, wxIntegerEqual, InstrumentMap);

  Soundbank();
  ~Soundbank();

  const Instrument& instrument(int id) const;
  Instrument& instrument(int id);
  void set_instrument(Instrument*);

  InstrumentMap::iterator begin() { return insts_.begin(); }
  InstrumentMap::iterator end() { return insts_.end(); }

  void Clear();

  void AttachObserver(SoundbankObserver*);
  void DetachObserver(SoundbankObserver*);
  void NotifyObserver();
  void NotifyObserver(int id);

 private:
  InstrumentMap insts_;
  SoundbankObserver* observer_;
};

#endif // SOUNDBANK_H_
