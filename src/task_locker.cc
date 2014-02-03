#include "task_locker.h"
#include <wx/thread.h>


class TaskLockerManager {
public:
  TaskLockerManager();

  void BeginFirstTask();
  void EndFirstTask();
  void BeginSecondTask();
  void EndSecondTask();

private:
  wxMutex shared_mutex_;
  wxCondition shared_cond_;

  bool first_task_begun_;
  bool first_task_ended_;
  bool second_task_begun_;
  bool second_task_ended_;

  wxDECLARE_NO_COPY_CLASS(TaskLockerManager);
};


TaskLockerManager::TaskLockerManager()
  : shared_cond_(shared_mutex_),
    first_task_begun_(false), first_task_ended_(false),
    second_task_begun_(false), second_task_ended_(false) {}


void TaskLockerManager::BeginFirstTask() {
  shared_mutex_.Lock();
  if (first_task_ended_ == true && second_task_begun_ == false) {
    do {
      shared_cond_.Wait();
    } while (second_task_begun_ == false);
  }
  first_task_begun_ = true;
  first_task_ended_ = false;
  shared_mutex_.Unlock();
}


void TaskLockerManager::EndFirstTask() {
  shared_mutex_.Lock();
  first_task_begun_ = false;
  first_task_ended_ = true;
  shared_cond_.Broadcast();
  shared_mutex_.Unlock();
}


void TaskLockerManager::BeginSecondTask() {
  shared_mutex_.Lock();
  while (first_task_ended_ == false) {
    shared_cond_.Wait();
  }
  second_task_begun_ = true;
  second_task_ended_ = false;
  shared_cond_.Broadcast();
  shared_mutex_.Unlock();
}


void TaskLockerManager::EndSecondTask() {
  shared_mutex_.Lock();
  second_task_begun_ = false;
  second_task_ended_ = true;
  shared_mutex_.Unlock();
}

/**
 * TaskLocker constructions
 */
TaskLocker::TaskLocker(wxSharedPtr<TaskLockerManager>& manager) : manager_(manager) {}
FirstTaskLocker::FirstTaskLocker(wxSharedPtr<TaskLockerManager>& manager) : TaskLocker(manager) {}
SecondTaskLocker::SecondTaskLocker(wxSharedPtr<TaskLockerManager>& manager) : TaskLocker(manager) {}


const wxSharedPtr<TaskLockerManager>& TaskLocker::manager() {
  return manager_;
}


void FirstTaskLocker::Lock() {
  manager()->BeginFirstTask();
}

void FirstTaskLocker::Unlock() {
  manager()->EndFirstTask();
}

void SecondTaskLocker::Lock() {
  manager()->BeginSecondTask();
}

void SecondTaskLocker::Unlock() {
  manager()->EndSecondTask();
}



TaskLockerFactory::TaskLockerFactory()
  : manager_(new TaskLockerManager) {}


FirstTaskLocker TaskLockerFactory::CreateFirstTaskLocker() {
  return FirstTaskLocker(manager_);
}

SecondTaskLocker TaskLockerFactory::CreateSecondTaskLocker() {
  return SecondTaskLocker(manager_);
}
