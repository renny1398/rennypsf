#include "common/debug.h"
#include <wx/frame.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include <wx/event.h>
#include <wx/thread.h>
#include <wx/utils.h>
#include <vector>
#include <deque>

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
  wxListItemAttr* OnGetItemAttr(long item) const;

private:
  struct Item {
    wxString level;
    wxString instance;
    wxString message;
    wxString created_on;
    Item(const wxString& lvl, const wxString& ins, const wxString& msg, const wxString& crtd_on)
      : level(lvl), instance(ins), message(msg), created_on(crtd_on) {}
  };
  std::deque<Item> items_;
  std::vector<Item> item_acc_;
  wxMutex mutex_;

  mutable wxListItemAttr attr_debug_;
  mutable wxListItemAttr attr_info_;
  mutable wxListItemAttr attr_warning_;
  mutable wxListItemAttr attr_error_;
};


RennyDebugListCtrl::RennyDebugListCtrl(wxWindow *parent)
  : wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_VIRTUAL),
    attr_debug_(wxColour(0, 0, 0), wxColour(0x80, 0x80, 0x80), wxFont()),
    attr_info_(wxColour(0x00, 0x52, 0x9b), wxColour(0xbd, 0xe5, 0xf8), wxFont()),
    attr_warning_(wxColour(0x9f, 0x60, 0x00), wxColour(0xfe, 0xef, 0xb3), wxFont()),
    attr_error_(wxColour(0xb1, 0x00, 0x09), wxColour(0xfd, 0xe4, 0xe1), wxFont())
{
  InsertColumn(0, wxT("Log Level"), wxLIST_FORMAT_LEFT, 64);
  InsertColumn(1, wxT("Instance"), wxLIST_FORMAT_LEFT, 144);
  InsertColumn(2, wxT("Message"), wxLIST_FORMAT_LEFT, 628);
  InsertColumn(3, wxT("Created On"), wxLIST_FORMAT_LEFT, 192);
}

void RennyDebugListCtrl::Log(const wxString &level, const wxString &instance, const wxString &msg, const wxString &created_on) {
  mutex_.Lock();
  items_.push_back(Item(level, instance, msg, created_on));
  if (items_.size() > 1000) {
    item_acc_.push_back(items_.front());
    items_.pop_front();
  }
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
    return items_.at(item).message;
  case kColumnCreatedOn:
    return items_.at(item).created_on;
  default:
    return wxString("");
  }
}

wxListItemAttr* RennyDebugListCtrl::OnGetItemAttr(long item) const {
  const wxString& level = items_.at(item).level;
  if (level == str_log_levels[RennyDebug::kLogLevelInfo]) {
    return &attr_info_;
  }
  if (level == str_log_levels[RennyDebug::kLogLevelWarning]) {
    return &attr_warning_;
  }
  if (level == str_log_levels[RennyDebug::kLogLevelError]) {
    return &attr_error_;
  }
  return &attr_debug_;
}


RennyDebug::RennyDebug(wxWindow* parent)
  : frame_(new wxFrame(parent, wxID_ANY, wxT("Renny Debug Window"), wxDefaultPosition, wxSize(1024, 768))),
    log_file_("rennypsf_log.txt", wxFile::write) {

  wxBoxSizer* vert_sizer = new wxBoxSizer(wxVERTICAL);

  list_ctrl_ = new RennyDebugListCtrl(frame_);
  vert_sizer->Add(list_ctrl_, 1, wxEXPAND);

  frame_->Bind(wxEVT_CLOSE_WINDOW, &RennyDebug::OnClose, this);
}

RennyDebug::~RennyDebug() {
  if (instance_ != nullptr) {
    delete instance_;
  }
  log_file_.Close();
}

RennyDebug* RennyDebug::instance_ = nullptr;

RennyDebug* RennyDebug::Instance() {
  return instance_;
}

void RennyDebug::CreateWindow(wxWindow* parent) {
  if (instance_ == nullptr) {
    instance_ = new RennyDebug(parent);
  } else if (instance_->frame_ == nullptr) {
    instance_->~RennyDebug();
    instance_ = new(instance_) RennyDebug(parent);
  }
}

void RennyDebug::DestroyWindow() {
  if (instance_ != nullptr && instance_->frame_ != nullptr) {
    instance_->frame_->Destroy();
  }
}

bool RennyDebug::IsWindowCreated() const {
  return frame_ != nullptr;
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
  rennyAssert(log_level < kLogLevelMax);
  const wxString& str_log_level = str_log_levels[log_level];

  wxString str_print;
  str_print.sprintf(wxT("[%s] %s: %s"), str_log_level, instance_name, msg);

  if (frame_ != nullptr && list_ctrl_ != nullptr) {
    list_ctrl_->Log(str_log_level, instance_name, msg, wxNow());
  } else {
    if (kLogLevelWarning <= log_level) {
      wxMessageOutputStderr().Printf(str_print);
    } else {
      wxMessageOutputDebug().Printf(str_print);
    }
  }
  if (log_file_.IsOpened()) {
    log_file_.Write(str_print);
    log_file_.Write(wxT("\n"));
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

#ifndef NDEBUG
void RennyDebug::LogDebug(const wxString& instance_name, const wxString& msg) {
  Log(kLogLevelDebug, instance_name, msg);
}
#endif


#include <wx/app.h>

extern "C" {

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
  if (instance != nullptr) {
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
  if (instance != nullptr) {
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
  if (instance != nullptr) {
    RennyDebug::Instance()->LogInfo(instance_name, msg);
  }
}

#ifndef NDEBUG
void rennyLogDebug(const char* instance_name, const char* msg_format, ...) {
  va_list arg;
  va_start(arg, msg_format);
  wxString msg(msg_format);
  msg.PrintfV(msg, arg);
  va_end(arg);

  RennyDebug* instance = RennyDebug::Instance();
  if (instance != nullptr) {
    RennyDebug::Instance()->LogDebug(instance_name, msg);
  }
}

#endif
}

