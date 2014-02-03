#include "PSF.h"
#include "psx/psx.h"
#include "spu/spu.h"
#include <wx/msgout.h>

#include <wx/file.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include <wx/hashmap.h>


PSF::PSF(PSX::Composite *psx)
  : psx_(psx) {
  if (psx == NULL) {
    psx_ = new PSX::Composite();
  }
}



PSF::~PSF()
{
  while (!m_libs.empty()) {
    PSF* psf = m_libs.back();
    delete psf;
    m_libs.pop_back();
  }
  delete psx_;
}


WX_DECLARE_HASH_MAP(int, wxString, wxIntegerHash, wxIntegerEqual, IntStrHash);

void PSF::LoadLibrary() {
  wxString directory, filename, ext;
  wxFileName::SplitPath(path_, &directory, &filename, &ext);
  IntStrHash libname_hash;

  for (int i = 1; i < 10; i++) {
    wxString _libN, libname;
    if (i <= 1) {
      _libN.assign("_lib");
    } else {
      _libN.sprintf("_lib%d", i);
    }
    GetTag(_libN, &libname);
    if (libname.IsEmpty() == false) {
      libname_hash[i] = directory + '/' + libname;
    }
  }

  for (IntStrHash::iterator it = libname_hash.begin(), it_end = libname_hash.end(); it != it_end; ++it) {
    PSF *psflib = LoadLib(it->second);
    if (psflib) {
      m_libs.push_back(psflib);
    }
  }

}



void PSF::Init()
{
  psx_->Init();
}

void PSF::Reset()
{
  psx_->Reset();
}


#include "app.h"

bool PSF::DoPlay()
{
  LoadBinary();
  m_thread = psx_->Run();
  return true;
}


bool PSF::DoStop()
{
  psx_->Terminate();
  m_thread = 0;
  wxMessageOutputDebug().Printf(wxT("PSF is stopped."));
  return true;
}




PSF1::PSF1(PSX::Composite *psx)
  : PSF(psx)
{
}


bool PSF1::LoadBinary()
{
  if (file_.IsOpened() == false) {
    wxMessageOutputStderr().Printf(wxT("file '%s' can't be opened."), path_);
    return 0;
  }

  wxFileInputStream stream(file_);

  stream.SeekI(m_ofsBinary);
  // char *compressed = new char[preloaded_psf->m_lenBinary];
  char *uncompressed = new char[0x800 + 0x200000];    // header + UserMemory
  wxZlibInputStream zlib_stream(stream);
  zlib_stream.Read(uncompressed, 0x800);

  PSF::PSXEXEHeader& header = *(reinterpret_cast<PSF::PSXEXEHeader*>(uncompressed));
  if (::memcmp(header.signature, "PS-X EXE", 8) != 0) {
    const wxString str_sign(header.signature, 8);
    wxMessageOutputStderr().Printf(wxT("Uncompressed binary is not PS-X EXE! (%s)"), str_sign);
    return 0;
  }

  LoadLibrary();

  if (m_libs.empty() == false) {
    PSF1* psflib = dynamic_cast<PSF1*>(m_libs.at(0));
    header.pc0 = psflib->m_header.pc0;
    header.gp0 = psflib->m_header.gp0;
    header.sp0 = psflib->m_header.sp0;
  }

  psx_->R3000ARegs().GPR.PC = header.pc0;
  psx_->R3000ARegs().GPR.GP = header.gp0;
  psx_->R3000ARegs().GPR.SP = header.sp0;

  zlib_stream.Read(uncompressed + 0x800, 0x200000);
  // psx_->InitMemory();
  psx_->Memcpy(header.text_addr, uncompressed + 0x800, header.text_size);

  m_header = header;

  // wxMessageOutputDebug().Printf("PC0 = 0x%08x, GP0 = 0x%08x, SP0 = 0x%08x", psx_->R3000ARegs().GPR.PC, psx_->R3000ARegs().GPR.GP, psx_->R3000ARegs().GPR.SP);
  wxMessageOutputDebug().Printf("PSF File '%s' is loaded.", path_);
  delete [] uncompressed;
  return true;
}


#include "PSFLoader.h"

PSF *PSF1::LoadLib(const wxString &path)
{
  PSF1Loader loader;
  PSF1 *psflib = dynamic_cast<PSF1*>( loader.LoadInfo(path, psx_) );
  if (psflib->LoadBinary() == false) {
    return 0;
  }

  for (wxVector<PSF*>::const_iterator it = psflib->m_libs.begin(), it_end = psflib->m_libs.end(); it != it_end; ++it) {
    m_libs.push_back(*it);
  }

  return psflib;
}




PSF2Entry::PSF2Entry(PSF2Directory *parent, const char *name)
  : parent_(*parent), name_(name) {}


const wxString PSF2Entry::GetFullPath() const {
  if (IsRoot()) return name_;
  return parent_.GetFullPath() + '/' + name_;
}


PSF2Entry* PSF2Entry::Find(const wxString &path) {

  wxString next;
  wxString curr = path.BeforeFirst('/', &next);

  if (IsFile() == true) {
    if (curr != name_) {
      return NULL;
    }
    return this;
  }

  if (this->IsRoot() == false) {
    if (curr != name_) {
      return NULL;
    }
    if (next.empty() == false) {
      return this;
    }
  }

  PSF2Directory* dir = dynamic_cast<PSF2Directory*>(this);
  wxVector<PSF2Entry*>::iterator itr = dir->children_.begin();
  const wxVector<PSF2Entry*>::iterator itrEnd = dir->children_.end();

  while (itr != itrEnd) {
    PSF2Entry* ret;
    if (IsRoot()) {
      ret = (*itr)->Find(curr);
    } else {
      ret = (*itr)->Find(next);
    }
    if (ret != NULL) {
      return ret;
    }
    ++itr;
  }
  return NULL;
}



PSF2File::PSF2File(PSF2Directory *parent, const char *name)
  : PSF2Entry(parent, name), data_(NULL), size_(0) {}


PSF2File::PSF2File(PSF2Directory *parent, const char *name, wxScopedArray<unsigned char>& data, int size)
  : PSF2Entry(parent, name), data_(NULL) {
  data_.swap(data);
  size_ = size;
}


const unsigned char* PSF2File::GetData() const {
  return data_.get();
}



PSF2Directory::PSF2Directory(PSF2Directory *parent, const char *name)
  : PSF2Entry(parent, name) {}


PSF2Directory::~PSF2Directory() {
  while (children_.empty() == false) {
    PSF2Entry* const entry = children_.back();
    children_.pop_back();
    delete entry;
  }
}


void PSF2Directory::AddEntry(PSF2Entry *entry) {
  if (entry->IsRoot()) return;
  children_.push_back(entry);
}



PSF2RootDirectory::PSF2RootDirectory()
  : PSF2Directory(this, "/") {}



PSF2::PSF2(PSX::Composite *psx)
  : PSF(psx), load_addr_(0x23f00) {}


void PSF2::ReadFile(wxFileInputStream *stream, PSF2Directory *parent, const char *filename, int uncompressed_size, int block_size) {

  const int X = (uncompressed_size + block_size - 1) / block_size;
  wxScopedArray<unsigned char> uncompressed_data(new unsigned char[uncompressed_size]);

  stream->SeekI(4*X, wxFromCurrent);

  wxZlibInputStream zlib_stream(*stream);
  zlib_stream.Read(&uncompressed_data[0], uncompressed_size);

  PSF2File* entry = new PSF2File(parent, filename, uncompressed_data, uncompressed_size);
  parent->AddEntry(entry);

  wxMessageOutputDebug().Printf(wxT("Found psf2:%s"), entry->GetFullPath());
}



void PSF2::ReadDirectory(wxFileInputStream *stream, PSF2Directory *parent) {

  int entry_num;
  stream->Read(&entry_num, 4);

  for (int i = 0; i < entry_num; i++) {

    char filename[37];
    stream->Read(filename, 36);
    filename[36] = '\0';

    int offset;
    stream->Read(&offset, 4);

    int uncompressed_size;
    stream->Read(&uncompressed_size, 4);

    int block_size;
    stream->Read(&block_size, 4);

    const int current_pointer = stream->TellI();

    if (uncompressed_size == 0 && block_size == 0) {
      if (offset == 0) {
        PSF2File* const entry = new PSF2File(parent, filename);
        parent->AddEntry(entry);
        continue;
      }
      PSF2Directory* const entry = new PSF2Directory(parent, filename);
      stream->SeekI(m_ofsReservedArea + offset, wxFromStart);
      ReadDirectory(stream, entry);
      stream->SeekI(current_pointer, wxFromStart);
      parent->AddEntry(entry);
      continue;
    }
    stream->SeekI(m_ofsReservedArea + offset, wxFromStart);
    ReadFile(stream, parent, filename, uncompressed_size, block_size);
    stream->SeekI(current_pointer, wxFromStart);
  }
}


unsigned int PSF2::LoadELF(PSF2Entry* psf2irx) {

  PSF2File* irx = dynamic_cast<PSF2File*>(psf2irx);
  if (irx == NULL) {
    return 0xffffffff;
  }
  const unsigned char* data = irx->GetData();

  if (load_addr_ & 3) {
    load_addr_ &= ~3;
    load_addr_ += 4;
  }

  if ((data[0] != 0x7f) || (data[1] != 'E') ||
      (data[2] != 'L') || (data[3] != 'F') ) {
    wxMessageOutputDebug().Printf(wxT("%s is not ELF file."), irx->GetName());
    return 0xffffffff;
  }

  uint32_t entry = *(reinterpret_cast<const unsigned int*>(data + 24));
  uint32_t shoff = *(reinterpret_cast<const unsigned int*>(data + 32));

  uint32_t shentsize = *(reinterpret_cast<const unsigned short*>(data + 46));
  uint32_t shnum = *(reinterpret_cast<const unsigned short*>(data + 48));

  uint32_t shent = shoff;
  uint32_t totallen = 0;

  for (unsigned int i = 0; i < shnum; i++) {
    uint32_t type = *(reinterpret_cast<const unsigned int*>(data + shent + 4));
    uint32_t addr = *(reinterpret_cast<const unsigned int*>(data + shent + 12));
    uint32_t offset = *(reinterpret_cast<const unsigned int*>(data + shent + 16));
    uint32_t size = *(reinterpret_cast<const unsigned int*>(data + shent + 20));

    switch (type) {
    case 0:
      break;

    case 1:
      psx_->Memcpy(load_addr_ + addr, &data[offset], size);
      totallen += size;
      break;

    case 2:
      break;

    case 3:
      break;

    case 8:
      psx_->Memset(load_addr_ + addr, 0, size);
      totallen += size;
      break;

    case 9:
      for (unsigned int rec = 0; rec < (size/8); rec++) {
        uint32_t offs, info, target, temp, val, vallo;
        static uint32_t hi16offs = 0, hi16target = 0;

        offs = *(reinterpret_cast<const unsigned int*>(data + offset + (rec*8)));
        info = *(reinterpret_cast<const unsigned int*>(data + offset + (rec*8) + 4));
        target = BFLIP32(psx_->U32M_ref(load_addr_ + offs));

        switch (info & 0xff) {
        case 2:
          target += load_addr_;
          break;

        case 4:
          temp = (target & 0x03ffffff);
          target &= 0xfc000000;
          temp += (load_addr_ >> 2);
          target |= temp;
          break;

        case 5:
          hi16offs = offs;
          hi16target = target;
          break;

        case 6:
          vallo = ((target & 0xffff) ^ 0x8000) - 0x8000;

          val = ((hi16target & 0xffff) << 16) + vallo;
          val += load_addr_;

          val = ((val >> 16) + ((val & 0x8000) != 0)) & 0xffff;

          hi16target = (hi16target & ~0xffff) | val;

          val = load_addr_ + vallo;
          target = (target & ~0xffff) | (val & 0xffff);

          psx_->U32M_ref(load_addr_ + hi16offs) = BFLIP32(hi16target);
          break;

        default:
          break;
        }

        psx_->U32M_ref(load_addr_ + offs) = BFLIP32(target);
      }
      break;

    case 0x70000000:
      // do_iopmod
      break;

    default:
      break;
    }
    shent += shentsize;
  }

  entry += load_addr_;
  entry |= 0x80000000;
  load_addr_ += totallen;

  wxMessageOutputDebug().Printf(wxT("entry PC = %08x"), entry);

  return entry;
}


PSF *PSF2::LoadLib(const wxString &path)
{
  PSFLoader loader;
  PSF2 *psflib = dynamic_cast<PSF2*>( loader.LoadInfo(path) );

  for (wxVector<PSF*>::const_iterator it = psflib->m_libs.begin(), it_end = psflib->m_libs.end(); it != it_end; ++it) {
    m_libs.push_back(*it);
  }

  return psflib;
}


bool PSF2::LoadBinary(PSF2RootDirectory* root) {

  if (file_.IsOpened() == false) return false;

  wxFileInputStream stream(file_);
  stream.SeekI(m_ofsReservedArea);

  ReadDirectory(&stream, root);

  return true;
}


bool PSF2::LoadBinary() {
  PSF2RootDirectory* root = new PSF2RootDirectory();
  if (LoadBinary(root) == false) {
    return false;
  }

  LoadLibrary();

  wxVector<PSF*>::iterator itr = m_libs.begin();
  const wxVector<PSF*>::iterator itrEnd = m_libs.end();
  while (itr != itrEnd) {
    PSF2* const child = dynamic_cast<PSF2*>(*itr);
    child->LoadBinary(root);
    ++itr;
  }

  PSF2Entry* irx = root->Find(wxT("psf2.irx"));
  if (irx == NULL) {
    wxMessageOutputStderr().Printf(wxT("psf2.irx is not found."));
    return false;
  }

  psx_->R3000ARegs().GPR.PC = LoadELF(irx);
  psx_->R3000ARegs().GPR.SP = 0x801ffff0;
  psx_->R3000ARegs().GPR.FP = 0x801ffff0;
  if (psx_->R3000ARegs().GPR.PC == 0xffffffff) {
    return false;
  }

  psx_->R3000ARegs().GPR.RA = 0x80000000;
  psx_->R3000ARegs().GPR.A0 = 2;
  psx_->R3000ARegs().GPR.A1 = 0x80000004;
  psx_->U32M_ref(4) = BFLIP32(0x80000008);
  psx_->U32M_ref(0) = BFLIP32(PSX::R3000A::OPCODE_HLECALL);
  ::strcpy(psx_->S8M_ptr(8), "psf2:/");

  return true;
}
