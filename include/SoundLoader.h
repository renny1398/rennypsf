#ifndef SOUNDLOADER_H
#define SOUNDLOADER_H

#include <wx/tracker.h>

class SoundInfo;
class SoundData;
class wxString;

class SoundLoader: public wxTrackable
{
public:
  SoundLoader();
  virtual ~SoundLoader() {}

  static SoundLoader* Instance(const wxString& filename);

  // virtual const wxString &GetPath() const = 0;

  virtual SoundInfo *LoadInfo() = 0;
  virtual SoundData *LoadData() = 0;

  typedef SoundLoader *(*InstanceFunc)(int fd, const wxString& filename);
  static bool RegisterInstanceFunc(InstanceFunc func, const char* signature);
};

#endif // SOUNDLOADER_H
