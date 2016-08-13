#include "common/command.h"
#include "common/SoundManager.h"
#include "common/debug.h"
#include <wx/string.h>
#include <string>
#include <iostream>
#include <sstream>

#include "common/Sound.h"
#include "app.h"
#include "common/soundbank.h"

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

    wxString file_path(file_path_);
    for (wxString::iterator it = file_path.begin(); it != file_path.end(); ++it) {
      if (*it == '\\') {
        file_path.erase(it, it+1);
      }
    }

    SoundLoader* loader = SoundLoader::Instance(file_path);
    if (loader == NULL) {
      return false;
    }

    // SoundInfo* info = loader->LoadInfo();
    SoundData *sound = loader->LoadData();
    if (sound == 0) {
      std::cout << "Failed to load a sound file." << std::endl;
      return false;
    }

    SoundDevice* sdd = wxGetApp().GetSoundManager();
    if (sdd == NULL) {
      wxGetApp().SetSoundDevice(new WaveOutAL);
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
   SoundDevice* sdd = wxGetApp().GetSoundManager();
    if (sdd == NULL) {
      rennyLogWarning("SetVolumeCommand", "SoundDevice is not initialized.");
      return false;
    }
    sdd->SetVolume(vol);
    return true;
  }
};


class ShowSoundbank : public Command {
public:
  ShowSoundbank(const std::string& p) : Command(p) {}

  bool Execute() {
    wxSharedPtr<SoundData> sf = wxGetApp().GetPlayingSound();
    if (sf == 0) {
      std::cout << "No playing sound." << std::endl;
      return false;
    }
    Soundbank& sb = sf->soundbank();

    std::cout << "  id   | length | loop   " << std::endl;
    std::cout << "-------|--------|--------" << std::endl;

    for (Soundbank::InstrumentMap::iterator itr = sb.begin(); itr != sb.end(); ++itr) {
      const int id = itr->first;
      const unsigned int length = itr->second->length();
      const int loop = itr->second->loop();
      std::printf(" %5d | %6d | %6d \n", id, length, loop);
    }
    return true;
  }
};


class MuteInstrument : public Command {
public:
  MuteInstrument(const std::string& p) : Command(p) {}

  bool Execute() {
    const std::string& str_id = params().at(0);
    if (str_id.empty()) return false;
    wxSharedPtr<SoundData> sf = wxGetApp().GetPlayingSound();
    if (sf == 0) {
      std::cout << "No playing sound." << std::endl;
      return false;
    }
    Soundbank& sb = sf->soundbank();
    int id = std::stoi(str_id);
    Instrument& inst = sb.instrument(id);
    if (inst.length() == 0) {
      std::cout << "'" << id << "' is invalid instrument id." << std::endl;
      return false;
    }
    bool is_muted = inst.IsMuted();
    if (is_muted) {
      inst.Unmute();
      std::cout << "Unmuted ";
    } else {
      inst.Mute();
      std::cout << "Muted ";
    }
    std::cout << "instrument '" << id << "'." << std::endl;
    return true;
  }
};


class MuteChannel : public Command {
public:
  MuteChannel(const std::string& p) : Command(p) {}

  bool Execute() {
    const std::string& str_ch = params().at(0);
    if (str_ch.empty()) return false;
    wxSharedPtr<SoundData> sf = wxGetApp().GetPlayingSound();
    if (sf == 0) {
      std::cout << "No playing sound." << std::endl;
      return false;
    }
    int ch = std::stoi(str_ch);
    Sample& sample = sf->Ch(ch);
    if (sample.IsMuted()) {
      sample.Unmute();
    } else {
      sample.Mute();
    }
    return true;
  }
};



class ExitCommand : public Command {
public:
  ExitCommand(const std::string& p) : Command(p) {}

  bool Execute() {
    wxGetApp().ExitMainLoop();
    return true;
  }
};




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
  if (cmd == "show-soundbank") {
    return new ShowSoundbank(params);
  }
  if (cmd == "mute-inst") {
    return new MuteInstrument(params);
  }
  if (cmd == "mute-channel") {
      return new MuteChannel(params);
  }
  if (cmd == "exit") {
    return new ExitCommand(params);
  }

  return NULL;
}


CommandFactory* CommandFactory::GetInstance() {
  static CommandFactory instance;
  return &instance;
}
