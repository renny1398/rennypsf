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
  // void LoadLibrary();
  // virtual PSF *LoadLib(const wxString &path) = 0;


protected:
  // virtual bool LoadBinary() = 0;

  SoundBlock& sound_block() {
    // TODO: multicore
    return psx_->Spu().core(0).Voices();
  }

  PSX::Composite* psx_;

  // PSXEXEHeader m_header;
  // void *m_memory;
  PSX::R3000A::InterpreterThread *m_thread;

  //wxString m_path;
  // uint32_t m_version;
  // wxFileOffset m_ofsReservedArea;
  // wxFileOffset m_ofsBinary;
  // uint32_t m_lenReservedArea;
  // uint32_t m_lenBinary;
  // uint32_t m_crc32Binary;

  // wxVector<PSF*> m_libs;
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
  // bool LoadBinary();
  // PSF *LoadLib(const wxString &path);
};




class PSF2Directory;

class PSF2Entry {
public:
  PSF2Entry(PSF2Directory* parent, const char* name);

  virtual ~PSF2Entry() {}

  virtual bool IsFile() const = 0;
  virtual bool IsDirectory() const = 0;
  virtual bool IsRoot() const = 0;

  PSF2Directory& Parent() {
    return parent_;
  }

  const wxString& GetName() const {
    return name_;
  }

  const wxString GetFullPath() const;

  PSF2Entry* Find(const wxString& path);

protected:
  PSF2Directory& parent_;
  wxString name_;
};


class PSF2File : public PSF2Entry {
public:
  PSF2File(PSF2Directory *parent, const char* name);
  PSF2File(PSF2Directory *parent, const char* name, wxScopedArray<unsigned char>& data, int size);
  virtual bool IsFile() const { return true; }
  virtual bool IsDirectory() const { return false; }
  virtual bool IsRoot() const { return false; }

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
  virtual bool IsRoot() const { return false; }

  void AddEntry(PSF2Entry* entry);

private:
  wxVector<PSF2Entry*> children_;

  friend class PSF2Entry;
};


class PSF2RootDirectory : public PSF2Directory {
public:
  PSF2RootDirectory();
  virtual bool IsFile() const { return false; }
  virtual bool IsDirectory() const { return true; }
  virtual bool IsRoot() const { return true; }
};




class wxFileInputStream;

class PSF2 : public PSF {
public:
  PSF2(PSX::Composite* psx);
  // bool Play();
  // bool Stop();
  unsigned int GetSamplingRate() const { return 48000; }
protected:
  // bool LoadBinary();
  // bool LoadBinary(PSF2RootDirectory*);
  // PSF *LoadLib(const wxString &path);
  // unsigned int LoadELF(PSF2Entry *psf2irx);

  // void ReadFile(wxFileInputStream* stream, PSF2Directory* parent, const char* filename, int uncompressed_size, int block_size);
  // void ReadDirectory(wxFileInputStream* stream, PSF2Directory* parent);

private:
  unsigned int load_addr_;
};

