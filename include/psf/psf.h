#pragma once
#include "common/SoundFormat.h"
#include "psf/psx/psx.h"
#include <stdint.h>
#include <wx/file.h>
#include <wx/ptr_scpd.h>
#include <wx/weakref.h>


class PSF: public SoundData
{
public:
  explicit PSF(uint32_t version);
  virtual ~PSF();

  Soundbank& soundbank();

  void Init();
  void Reset();

  virtual bool DoPlay();
  // bool Pause();
  virtual bool DoStop();
  // bool Seek();
  // bool Tell();

  bool Open(SoundBlock* block);
  bool Close();
  bool DoAdvance(SoundBlock *dest);

  bool ChangeOutputSamplingRate(uint32_t rate);

  friend class PSFLoader;

protected:
  psx::PSX* psx_;

private:
  uint32_t unprocessed_cycles_;
};


class PSF1: public PSF
{
public:
  PSF1(uint32_t pc0, uint32_t gp0, uint32_t sp0);
  unsigned int GetSamplingRate() const { return 44100; }

  friend class PSF1Loader;

protected:
  void PSXMemCpy(psx::PSXAddr dest, void* src, int length);
};


class wxFileInputStream;
class PSF2File;
class PSF2Directory;

class PSF2Entry : public wxTrackable {
public:
  PSF2Entry(PSF2Directory* parent, const char* name);

  virtual ~PSF2Entry() {}

  virtual bool IsFile() const = 0;
  virtual bool IsDirectory() const = 0;

  const PSF2File* file() const {
    return const_cast<PSF2Entry*>(this)->file();
  }
  const PSF2Directory* directory() const {
    return const_cast<PSF2Entry*>(this)->directory();
  }

  bool IsRoot() const;

  virtual PSF2File* file() = 0;
  virtual PSF2Directory* directory() = 0;

  PSF2Directory* Parent();
  const wxString& GetName() const;
  const wxString GetFullPath() const;
  PSF2Entry* Find(const wxString& path);
  PSF2Directory* GetRoot();

protected:
  PSF2Directory* parent_;
  wxString name_;
};


class PSF2File : public PSF2Entry {
public:
  PSF2File(PSF2Directory *parent, const char* name);
  PSF2File(PSF2Directory *parent, const char* name, wxScopedArray<unsigned char>& data, int size);
  virtual bool IsFile() const { return true; }
  virtual bool IsDirectory() const { return false; }

  PSF2File* file() { return this; }
  PSF2Directory* directory() { return nullptr; }

  const unsigned char* GetData() const;
  size_t GetSize() const;

private:
  wxScopedArray<unsigned char> data_;
  size_t size_;
};


class PSF2Directory : public PSF2Entry {
public:
  PSF2Directory(PSF2Directory *parent, const char *name);
  ~PSF2Directory();

  virtual bool IsFile() const { return false; }
  virtual bool IsDirectory() const { return true; }

  PSF2File* file() { return nullptr; }
  PSF2Directory* directory() { return this; }

  void AddEntry(PSF2Entry* entry);

  bool LoadFile(wxFileInputStream* stream, const char *filename, int uncompressed_size, int block_size);
  bool LoadEntries(wxFileInputStream* stream, wxFileOffset offset);

private:
  wxVector<PSF2Entry*> children_;

  friend class PSF2Entry;
};


class PSF2 : public PSF {
public:
  PSF2(PSF2File* psf2irx);
  ~PSF2();
  bool IsOk() const;
  // bool Play();
  // bool Stop();
  unsigned int GetSamplingRate() const { return 48000; }
};
