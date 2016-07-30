#pragma once

#include <wx/string.h>
#include <wx/weakref.h>
#include <stdint.h>

#ifndef NDEBUG

class wxFrame;
class RennyDebugListCtrl;
class wxCloseEvent;

class RennyDebug {
  
protected:
  RennyDebug(wxWindow* parent);
  ~RennyDebug();
  
public:
  static RennyDebug* Instance();
  static void CreateWindow(wxWindow* parent);
  static void DestroyWindow();

  bool IsWindowCreated() const;
  void ShowWindow();
  void HideWindow();

  void OnClose(wxCloseEvent& event);

  enum LogLevel {
    kLogLevelNone = 0,
    kLogLevelDebug,
    kLogLevelInfo,
    kLogLevelNotice,
    kLogLevelWarning,
    kLogLevelError,
    kLogLevelCritical,
    kLogLevelAlert,
    kLogLevelEmergency,
    kLogLevelMax
  };

  void Log(LogLevel log_level, const wxString& instance_name, const wxString& msg);
  
  void LogError(const wxString& instance_name, const wxString& msg);
  void LogWarning(const wxString& instance_name, const wxString& msg);
  void LogInfo(const wxString& instance_name, const wxString& msg);
  void LogDebug(const wxString& instance_name, const wxString& msg);

private:
  static RennyDebug* instance_;

  wxWeakRef<wxFrame> frame_;
  wxWeakRef<RennyDebugListCtrl> list_ctrl_;
};

// extern "C" {

void rennyCreateDebugWindow(wxWindow* parent);
void rennyDestroyDebugWindow();
void rennyShowDebugWindow();
void rennyHideDebugWindow();

// void rennyEnableLogging();
// void rennyDisableLogging();

void rennyLogError(const char* instance_name, const char* msg_format, ...);
void rennyLogWarning(const char* instance_name, const char* msg_format, ...);
void rennyLogInfo(const char* instance_name, const char* msg_format, ...);
void rennyLogDebug(const char* instance_name, const char* msg_format, ...);

// }

#else

#define rennyCreateDebugWindow()
#define rennyDestroyDebugWindow()
#define rennyShowDebugWindow()
#define rennyHideDebugWindow()

// #define rennyEnableLogging()
// #define rennyDisableLogging()

#define rennyLogError(instance_name, msg_format, ...)
#define rennyLogWarning(instance_name, msg_format, ...)
#define rennyLogInfo(instance_name, msg_format, ...)
#define rennyLogDebug(instance_name, msg_format, ...)

#endif
