#ifndef COMMON_NOTE_H_
#define COMMON_NOTE_H_


#include <wx/string.h>

class Note {
public:
  Note(int key, float velocity = 1.0f) : key_(key), velocity_(velocity) {}

  int key() const { return key_; }
  wxString ToString() const;

  bool IsOn() const;
  bool IsOff() const;

private:
  int key_;
  float velocity_;
};

















#endif  // COMMON_NOTE_H_
