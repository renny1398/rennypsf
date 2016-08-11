#include "wavetable.h"
#include "common/debug.h"


int Tone::id() const { return id_; }
int Tone::length() const { return length_; }
int Tone::loop_index() const { return loop_index_; }


void Tone::set_id(int id) { id_ = id; }
void Tone::loop_index(int i) { rennyAssert(i < length_); loop_index_ = i; }


double Tone::freq() const { return freq_; }
void Tone::set_freq(double freq) { freq_ = freq; }


const Tone& Wavetable::tone(int id) const {
  Wavetable* this_casted = const_cast<Wavetable*>(this);
  return this_casted->tone(id);
}
