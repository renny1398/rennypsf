#ifndef NDEBUG

#include "common/debug.h"
#include <wx/vector.h>
#include <wx/frame.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/event.h>
#include <wx/thread.h>
#include <wx/utils.h>


namespace {

const wxString str_log_levels[RennyDebug::kLogLevelMax] =
    {"", "Debug", "Info", "Notice", "Warning",
     "Error", "Critical", "Alert", "Emergency"};

}


class RennyDebugListCtrl : public wxListCtrl {

public:
  RennyDebugListCtrl(wxWindow* parent);

  void Log(const wxString& level, const wxString& instance, const wxString& msg, const wxString& created_on);

protected:
  enum Column {
    kColumnLevel = 0,
    kColumnInstance,
    kColumnMessage,
    kColumnCreatedOn
  };

  wxString OnGetItemText(long item, long column) const;

private:
  struct Item {
    wxString level;
    wxString instance;
    wxString msg;
    wxString created_on;
    Item(const wxString& level, const wxString& instance, const wxString& msg, const wxString& created_on)
      : level(level), instance(instance), msg(msg), created_on(created_on) {}
  };
  wxVector<Item> items_;
  wxMutex mutex_;
};


RennyDebugListCtrl::RennyDebugListCtrl(wxWindow *parent)
  : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL) {
  InsertColumn(0, wxT("Log Level"), wxLIST_FORMAT_LEFT, 64);
  InsertColumn(1, wxT("Instance"), wxLIST_FORMAT_LEFT, 144);
  InsertColumn(2, wxT("Message"), wxLIST_FORMAT_LEFT, 628);
  InsertColumn(3, wxT("Created On"), wxLIST_FORMAT_LEFT, 192);
}

void RennyDebugListCtrl::Log(const wxString &level, const wxString &instance, const wxString &msg, const wxString &created_on) {
  mutex_.Lock();
  items_.push_back(Item(level, instance, msg, created_on));
  mutex_.Unlock();
  SetItemCount(items_.size());
}

wxString RennyDebugListCtrl::OnGetItemText(long item, long column) const {
  switch (column) {
  case kColumnLevel:
    return items_.at(item).level;
  case kColumnInstance:
    return items_.at(item).instance;
  case kColumnMessage:
    return items_.at(item).msg;
  case kColumnCreatedOn:
    return items_.at(item).created_on;
  default:
    return wxString("");
  }
}


RennyDebug::RennyDebug(wxWindow* parent)
  : frame_(new wxFrame(parent, wxID_ANY, wxT("Renny Debug Window"), wxDefaultPosition, wxSize(1024, 768))) {

  wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);

  list_ctrl_ = new RennyDebugListCtrl(frame_);
  vert_sizer->Add(list_ctrl_, 1, wxEXPAND);

  frame_->Bind(wxEVT_CLOSE_WINDOW, &RennyDebug::OnClose, this);
}

RennyDebug::~RennyDebug() {
  if (instance_ != NULL) {
    delete instance_;
  }
}

RennyDebug* RennyDebug::instance_ = NULL;

RennyDebug* RennyDebug::Instance() {
  return instance_;
}

void RennyDebug::CreateWindow(wxWindow* parent) {
  if (instance_ == NULL) {
    instance_ = new RennyDebug(parent);
  } else if (instance_->frame_ == NULL) {
    instance_->~RennyDebug();
    instance_ = new(instance_) RennyDebug(parent);
  }
}

void RennyDebug::DestroyWindow() {
  if (instance_ != NULL && instance_->frame_ != NULL) {
    instance_->frame_->Destroy();
  }
}

bool RennyDebug::IsWindowCreated() const {
  return frame_ != NULL;
}

void RennyDebug::ShowWindow() {
  if (frame_) {
    frame_->Show(true);
  }
}

void RennyDebug::HideWindow() {
  if (frame_) {
    frame_->Hide();
  }
}

void RennyDebug::OnClose(wxCloseEvent &event) {
  if (event.CanVeto() == false) {
    frame_->Destroy();
  } else {
    frame_->Hide();
  }
}


void RennyDebug::Log(LogLevel log_level, const wxString& instance_name, const wxString& msg) {
  wxASSERT(log_level < LogLevel::kMax);
  const wxString& str_log_level = str_log_levels[log_level];

  if (frame_ != NULL && list_ctrl_ != NULL) {
    list_ctrl_->Log(str_log_level, instance_name, msg, wxNow());
  } else {
    if (kLogLevelWarning <= log_level) {
      wxMessageOutputStderr().Printf(wxT("[%s] %s: %s"), str_log_level, instance_name, msg);
    } else {
      wxMessageOutputDebug().Printf(wxT("[%s] %s: %s"), str_log_level, instance_name, msg);
    }
  }
}

void RennyDebug::LogError(const wxString& instance_name, const wxString& msg) {
  Log(kLogLevelError, instance_name, msg);
}

void RennyDebug::LogWarning(const wxString& instance_name, const wxString& msg) {
  Log(kLogLevelWarning, instance_name, msg);
}

void RennyDebug::LogInfo(const wxString& instance_name, const wxString& msg) {
  Log(kLogLevelInfo, instance_name, msg);
}

void RennyDebug::LogDebug(const wxString& instance_name, const wxString& msg) {
  Log(kLogLevelDebug, instance_name, msg);
}


#include <wx/app.h>

// extern "C" {

void rennyCreateDebugWindow(wxWindow* parent) {
  RennyDebug::CreateWindow(parent);

}

void rennyDestroyDebugWindow() {
  RennyDebug::DestroyWindow();
}

void rennyShowDebugWindow() {
  RennyDebug::Instance()->ShowWindow();
}

void rennyHideDebugWindow() {
  RennyDebug::Instance()->HideWindow();
}

void rennyLogError(const char* instance_name, const char* msg_format, ...) {
  va_list arg;
  va_start(arg, msg_format);
  wxString msg(msg_format);
  msg.PrintfV(msg, arg);
  va_end(arg);

  RennyDebug* instance = RennyDebug::Instance();
  if (instance != NULL) {
    RennyDebug::Instance()->LogError(instance_name, msg);
  }
}

void rennyLogWarning(const char* instance_name, const char* msg_format, ...) {
  va_list arg;
  va_start(arg, msg_format);
  wxString msg(msg_format);
  msg.PrintfV(msg, arg);
  va_end(arg);

  RennyDebug* instance = RennyDebug::Instance();
  if (instance != NULL) {
    RennyDebug::Instance()->LogWarning(instance_name, msg);
  }
}

void rennyLogInfo(const char* instance_name, const char* msg_format, ...) {
  va_list arg;
  va_start(arg, msg_format);
  wxString msg(msg_format);
  msg.PrintfV(msg, arg);
  va_end(arg);

  RennyDebug* instance = RennyDebug::Instance();
  if (instance != NULL) {
    RennyDebug::Instance()->LogInfo(instance_name, msg);
  }
}

void rennyLogDebug(const char* instance_name, const char* msg_format, ...) {
  va_list arg;
  va_start(arg, msg_format);
  wxString msg(msg_format);
  msg.PrintfV(msg, arg);
  va_end(arg);

  RennyDebug* instance = RennyDebug::Instance();
  if (instance != NULL) {
    RennyDebug::Instance()->LogDebug(instance_name, msg);
  }
}

// }


#endif
