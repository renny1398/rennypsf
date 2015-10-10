#include "common/command.h"
#include "SoundManager.h"
#include <wx/string.h>
#include <string>
#include <iostream>
#include <sstream>

#include "Sound.h"
#include "app.h"

namespace {

}


Command::Command(const std::string &params) {
  std::stringstream oss(params);
  std::string buffer;
  std::string last;
  while (std::getline(oss, buffer, ' ')) {
    if (*(last.rbegin()) == '\\') {
      last.pop_back();
      last.push_back(' ');
      last.append(buffer);
      params_.pop_back();
      params_.push_back(last);
    } else {
      params_.push_back(buffer);
      last = buffer;
    }
  }
}



class PlayCommand : public Command {
public:
  PlayCommand(const std::string& p)
    : Command(p), file_path_(params().at(0)) {
  }

  bool Execute() {
    wxString ext;
    for (std::string::const_reverse_iterator itr = file_path_.rbegin(); itr != file_path_.rend(); ++itr) {
      if (*itr == '.') break;
      ext.insert(ext.begin(), *itr);
    }

    SoundLoader* loader = SoundLoaderFactory::GetInstance()->GetLoader(ext);
    if (loader == NULL) {
      std::cout << "Failed to generate a loader for " << ext << ".\n";
      return false;
    }

    wxString file_path(file_path_);
    for (wxString::iterator it = file_path.begin(); it != file_path.end(); ++it) {
      if (*it == '\\') {
        file_path.erase(it, it+1);
      }
    }

    SoundFormat *sound = loader->LoadInfo(file_path);
    if (sound == 0) {
      std::cout << "Failed to load a sound file." << std::endl;
      return false;
    }

    wxSharedPtr<SoundDeviceDriver> sdd = wxGetApp().GetSoundManager();
    if (sdd == 0) {
      sdd.reset(new WaveOutAL);
      wxGetApp().SetSoundDevice(sdd);
    }
    // sound->Play(sdd);
    wxGetApp().Play(sound);

    return true;
  }

private:
  const std::string& file_path_;
};



class StopCommand : public Command {
public:
  StopCommand(const std::string& p) : Command(p) {}

  bool Execute() {
    // wxSharedPtr<SoundDeviceDriver> sdd = wxGetApp().GetSoundManager();
    // if (sdd != 0) sdd->Stop();
    wxGetApp().Stop();
    return true;
  }
};


class SetVolumeCommand : public Command {
public:
  SetVolumeCommand(const std::string& p) : Command(p) {}

  bool Execute() {
    const std::string& param = params().at(0);
    if (param.empty()) return false;
    float vol = std::stof(param);
    wxSharedPtr<SoundDeviceDriver> sdd = wxGetApp().GetSoundManager();
    if (sdd == 0) {
      wxMessageOutputDebug().Printf(wxT("Warning: SoundDevice is not initialized."));
      return false;
    }
    sdd->SetVolume(vol);
    return true;
  }
};

/*
class ExitCommand : public Command {
public:
  ExitCommand(const std::string& p) : Command(p) {}

  vool Execute() {

  }
};
*/



Command* CommandFactory::CreateCommand(const std::string &line) {

  wxString cmd;
  std::string params;
  for (std::string::const_iterator itr = line.begin(); itr != line.end(); ++itr) {
    if (*itr == ' ') {
      for (++itr; itr != line.end(); ++itr) {
        params.push_back(*itr);
      }
      break;
    }
    cmd.Append(*itr);
  }

  if (cmd == "play") {
    return new PlayCommand(params);
  }
  if (cmd == "stop") {
    return new StopCommand(params);
  }
  if (cmd == "set-volume") {
    return new SetVolumeCommand(params);
  }
  if (cmd == "exit") {
    return NULL;
  }

  return NULL;
}


CommandFactory* CommandFactory::GetInstance() {
  static CommandFactory instance;
  return &instance;
}
