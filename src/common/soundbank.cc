#include "common/soundbank.h"
#include "common/debug.h"


InstrumentDataIterator::InstrumentDataIterator()
  : p_inst_(0), is_loop_(false), index_(0), data_(Instrument::kInvalidData) {}

InstrumentDataIterator::InstrumentDataIterator(const Instrument *p_inst, bool is_loop)
  : p_inst_(p_inst), is_loop_(is_loop && (p_inst->loop() >= 0)), index_(0), data_(Instrument::kInvalidData) {
}

InstrumentDataIterator::~InstrumentDataIterator() {}


bool InstrumentDataIterator::HasNext() const {
  if (p_inst_ == 0) return false;
  if (p_inst_->IsMuted()) return false;
  int data = p_inst_ ? p_inst_->at(index_) : Instrument::kInvalidData;
  data_ = data;
  return (data != Instrument::kInvalidData);
}


int InstrumentDataIterator::Next() {
  if (p_inst_ == 0) {
    return Instrument::kInvalidData;
  }
  int ret = data_;
  int i = index_;
  if (ret == Instrument::kInvalidData) {
    ret = p_inst_->at(i);
  } else {
    data_ = Instrument::kInvalidData;
  }
  ++i;
  if (static_cast<int>(p_inst_->length()) <= i && is_loop_) {
    i = p_inst_->loop();
  }
  index_ = i;
  return ret;
}


Instrument::Instrument() : is_muted_(false), freq_(0.0) {}


void Instrument::Mute() {
  is_muted_ = true;
}


void Instrument::Unmute() {
  is_muted_ = false;
}


bool Instrument::IsMuted() const {
  return is_muted_;
}


InstrumentDataIterator Instrument::Iterator(bool is_loop) const {
  return InstrumentDataIterator(this, is_loop);
}


double Instrument::freq() const {
  return freq_;
}


void Instrument::set_freq(double f) {
  freq_ = f;
}


class NullInstrument : public Instrument {
public:
  NullInstrument() {}
  int id() const { return -1; }
  int at(int /*i*/) const { return kInvalidData; }
  unsigned int length() const { return 0; }
  int loop() const { return -1; }
};


Soundbank::Soundbank() {}

Soundbank::~Soundbank() {
  Clear();
}


const Instrument& Soundbank::instrument(int id) const {
  return *insts_.find(id)->second;
}


Instrument& Soundbank::instrument(int id) {
  static NullInstrument null_inst;
  InstrumentMap::iterator itr = insts_.find(id);
  if (itr == insts_.end()) {
    return null_inst;
  }
  return *(itr->second);
}


void Soundbank::set_instrument(Instrument* p_inst) {
  rennyAssert(p_inst != 0);
  insts_[p_inst->id()] = p_inst;
}


void Soundbank::Clear() {
  InstrumentMap::iterator itr = insts_.begin();
  const InstrumentMap::iterator itr_end = insts_.end();
  for (; itr != itr_end; ++itr) {
    delete itr->second;
    itr->second = 0;
  }
  insts_.clear();
}


void Soundbank::AttachObserver(SoundbankObserver* observer) {
  observer_ = observer;
}


void Soundbank::DetachObserver(SoundbankObserver *) {
  observer_ = 0;
}


void Soundbank::NotifyObserver() {
  if (observer_) {
    observer_->Update(*this);
  }
}


void Soundbank::NotifyObserver(int id) {
  if (observer_) {
    observer_->Update(instrument(id));
  }
}
