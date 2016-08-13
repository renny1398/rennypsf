#include "common/SoundLoader.h"
#include "common/debug.h"
#include <wx/string.h>
#include <wx/file.h>
#include <map>

namespace {
  typedef std::map<wxString, SoundLoader::InstanceFunc> InstanceFuncMap;
  InstanceFuncMap instance_funcs_;
}




bool SoundLoader::RegisterInstanceFunc(InstanceFunc func, const char *signature) {
  instance_funcs_[wxString(signature)] = func;
  return true;
}


#include "psf/psfloader.h"
#include "vorbis/vorbis.h"


SoundLoader* SoundLoader::Instance(const wxString &filename) {

  static bool is_inited(false);
  if (is_inited == false) {

    RegisterInstanceFunc((InstanceFunc)&VorbisLoader::Instance, "OggS");
    RegisterInstanceFunc((InstanceFunc)&PSF1Loader::Instance, "PSF\x1");
    RegisterInstanceFunc((InstanceFunc)&PSF2Loader::Instance, "PSF\x2");

    is_inited = true;
  }

  wxFile file(filename, wxFile::read);
  if (file.IsOpened() == false) {
    return NULL;
  }

  char buff[4];
  file.Read(buff, 4);
  const wxString signature(buff, 4);
  InstanceFuncMap::iterator it = instance_funcs_.find(signature);
  if (it == instance_funcs_.end()) {
    rennyLogError("SoundLoader", "Failed to generate a loader. (signature = %s)", static_cast<const char*>(signature));
    file.Close();
    return NULL;
  }

  int fd = file.Detach();
  return it->second(fd, filename);
}


SoundLoader::SoundLoader() {
}
