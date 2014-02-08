#ifndef TASK_LOCKER_H_
#define TASK_LOCKER_H_

#include <wx/sharedptr.h>


class TaskLockerManager;
class wxMutex;
class wxCondition;


class TaskLocker {
public:
  TaskLocker(wxSharedPtr<TaskLockerManager>&);
  virtual ~TaskLocker() {}
  virtual void Lock() = 0;
  virtual void Unlock() = 0;
protected:
  const wxSharedPtr<TaskLockerManager>& manager();
private:
  const wxSharedPtr<TaskLockerManager> manager_;
};


class FirstTaskLocker : public TaskLocker {
public:
  FirstTaskLocker(wxSharedPtr<TaskLockerManager>&);
  void Lock();
  void Unlock();
};


class SecondTaskLocker : public TaskLocker {
public:
  SecondTaskLocker(wxSharedPtr<TaskLockerManager>&);
  void Lock();
  void Unlock();
};


class TaskLockerFactory {
public:
  TaskLockerFactory();
  FirstTaskLocker CreateFirstTaskLocker();
  SecondTaskLocker CreateSecondTaskLocker();
private:
  wxSharedPtr<TaskLockerManager> manager_;
};


#endif // TASK_LOCKER_H_
