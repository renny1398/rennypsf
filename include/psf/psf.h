#pragma once

#include "SoundFormat.h"
#include "psf/psx/psx.h"
#include <stdint.h>
#include <wx/file.h>
#include <wx/ptr_scpd.h>


class PSF: public SoundData
{
public:
  explicit PSF(PSX::Composite* psx = NULL);
  virtual ~PSF();

  Soundbank& soundbank();

  void Init();
  void Reset();

  virtual bool DoPlay();
  // bool Pause();
  virtual bool DoStop();
  // bool Seek();
  // bool Tell();

  friend class PSFLoader;

protected:

  SoundBlock& sound_block() {
    // TODO: multicore
    return psx_->Spu().core(0).Voices();
  }

  PSX::Composite* psx_;

  PSX::R3000A::InterpreterThread *m_thread;
};


class PSF1: public PSF
{
public:
  // PSF1(PSX::Composite* psx = NULL);
  PSF1(uint32_t pc0, uint32_t gp0, uint32_t sp0);
  unsigned int GetSamplingRate() const { return 44100; }

  friend class PSF1Loader;

protected:
  void PSXMemCpy(PSX::PSXAddr dest, void* src, int length);
};


class wxFileInputStream;
class PSF2Directory;

class PSF2Entry : public wxTrackable {
public:
  PSF2Entry(PSF2Directory* parent, const char* name);

  virtual ~PSF2Entry() {}

  virtual bool IsFile() const = 0;
  virtual bool IsDirectory() const = 0;
  bool IsRoot() const;

  PSF2Directory* Parent();
  const wxString& GetName() const;
  const wxString GetFullPath() const;
  PSF2Entry* Find(const wxString& path);

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

  const unsigned char* GetData() const;

private:
  wxScopedArray<unsigned char> data_;
  int size_;
};


class PSF2Directory : public PSF2Entry {
public:
  PSF2Directory(PSF2Directory *parent, const char *name);
  ~PSF2Directory();

  virtual bool IsFile() const { return false; }
  virtual bool IsDirectory() const { return true; }

  void AddEntry(PSF2Entry* entry);

  bool LoadFile(wxFileInputStream* stream, const char *filename, int uncompressed_size, int block_size);
  bool LoadEntries(wxFileInputStream* stream, wxFileOffset offset);

private:
  wxVector<PSF2Entry*> children_;

  friend class PSF2Entry;
};


class PSF2 : public PSF {
public:
  PSF2(PSF2Entry* psf2irx);
  bool IsOk() const;
  // bool Play();
  // bool Stop();
  unsigned int GetSamplingRate() const { return 48000; }
protected:
  unsigned int LoadELF(PSF2Entry *psf2irx);

private:
  unsigned int load_addr_;
};
