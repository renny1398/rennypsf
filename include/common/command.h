#ifndef COMMAND_H_
#define COMMAND_H_

#include <string>
#include <wx/vector.h>

class Command {

public:
  Command(const std::string&);
  virtual ~Command() {}

  virtual bool Execute() = 0;

protected:
  const wxVector<std::string>& params() const {
    return params_;
  }

private:
  wxVector<std::string> params_;
};


class CommandFactory {

public:
  Command* CreateCommand(const std::string&);

  static CommandFactory* GetInstance();

protected:
  CommandFactory() = default;
  ~CommandFactory() = default;
  Command& operator=(const Command&);
};


#endif  // COMMAND_H_
