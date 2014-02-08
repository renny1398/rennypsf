#ifndef WAVETABLE_H_
#define WAVETABLE_H_


#include <wx/vector.h>

class Tone {
public:
  virtual ~Tone() {}

  int id() const;
  int length() const;
  int loop_index() const;

  void set_id(int);
  void set_data(const short*, int);
  void loop_index(int);

  double freq() const;
  void set_freq(double);

  virtual int At(int) const;

  virtual int GetData(short*);

  void Mute();
  void Unmute();
  bool IsMuted() const;

  // Iterator
  //

private:
  int id_;
  int length_;
  int loop_index_;
  double freq_;
};




class Wavetable {
public:
  virtual ~Wavetable() {}

  void Init();
  void Reset();

  virtual Tone& tone(int) = 0;
  const Tone& tone(int) const;
};












#endif
