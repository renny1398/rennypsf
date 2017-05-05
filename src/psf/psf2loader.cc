#include "psf/psfloader.h"
#include "psf/psf.h"
#include "common/debug.h"
#include <wx/wfstream.h>
#include <wx/zstream.h>

////////////////////////////////////////////////////////////////////////
/// PSF2 Entry Class
////////////////////////////////////////////////////////////////////////


PSF2Entry::PSF2Entry(PSF2Directory *parent, const char *name)
  : parent_(parent), name_(name) {}


bool PSF2Entry::IsRoot() const {
  return parent_ == nullptr;
}

PSF2Directory* PSF2Entry::Parent() {
  return parent_;
}

const wxString& PSF2Entry::GetName() const {
  return name_;
}

const wxString PSF2Entry::GetFullPath() const {
  if (IsRoot()) return name_;
  return parent_->GetFullPath() + '/' + name_;
}


PSF2Entry* PSF2Entry::Find(const wxString &path) {

  wxString next;
  wxString curr = path.BeforeFirst('/', &next);

  if (IsFile() == true) {
    if (curr != name_) {
      return nullptr;
    }
    return this;
  }

  if (IsRoot() == false) {
    if (curr != name_) {
      return nullptr;
    }
    if (next.empty() == false) {
      return this;
    }
  }

  PSF2Directory* dir = this->directory();
  if (dir == nullptr) {
    return nullptr;
  }

  wxVector<PSF2Entry*>::iterator itr = dir->children_.begin();
  const wxVector<PSF2Entry*>::iterator itrEnd = dir->children_.end();

  while (itr != itrEnd) {
    PSF2Entry* ret;
    if (IsRoot()) {
      ret = (*itr)->Find(curr);
    } else {
      ret = (*itr)->Find(next);
    }
    if (ret != nullptr) {
      return ret;
    }
    ++itr;
  }
  return nullptr;
}


PSF2Directory* PSF2Entry::GetRoot() {
  if (IsRoot()) {
    return directory();
  }
  return parent_->GetRoot();
}


////////////////////////////////////////////////////////////////////////
/// PSF2 File Class
////////////////////////////////////////////////////////////////////////


PSF2File::PSF2File(PSF2Directory *parent, const char *name)
  : PSF2Entry(parent, name), data_(nullptr), size_(0) {}


PSF2File::PSF2File(PSF2Directory *parent, const char *name, wxScopedArray<unsigned char>& data, int size)
  : PSF2Entry(parent, name), data_(nullptr) {
  data_.swap(data);
  size_ = size;
}


const unsigned char* PSF2File::GetData() const {
  return data_.get();
}


size_t PSF2File::GetSize() const {
  return size_;
}



////////////////////////////////////////////////////////////////////////
/// PSF2 Directory Class
////////////////////////////////////////////////////////////////////////


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


bool PSF2Directory::LoadFile(wxFileInputStream *stream, const char *filename, int uncompressed_size, int block_size) {

  const int X = (uncompressed_size + block_size - 1) / block_size;
  wxScopedArray<unsigned char> uncompressed_data(new unsigned char[uncompressed_size]);

  stream->SeekI(4*X, wxFromCurrent);

  wxZlibInputStream zlib_stream(*stream);
  zlib_stream.Read(&uncompressed_data[0], uncompressed_size);

  PSF2File* entry = new PSF2File(this, filename, uncompressed_data, uncompressed_size);
  AddEntry(entry);

  rennyLogDebug("PSF2Directory", "Loaded psf2:%s", static_cast<const char*>(entry->GetFullPath()));

  return true;
}


bool PSF2Directory::LoadEntries(wxFileInputStream *stream, wxFileOffset offset) {

  rennyAssert(stream != nullptr);
  const wxFileOffset base_offset = stream->TellI();
  stream->SeekI(offset, wxFromCurrent);

  int entry_num;
  stream->Read(&entry_num, 4);

  for (int i = 0; i < entry_num; i++) {

    char filename[37];
    stream->Read(filename, 36);
    filename[36] = '\0';

    int child_offset;
    stream->Read(&child_offset, 4);

    int uncompressed_size;
    stream->Read(&uncompressed_size, 4);

    int block_size;
    stream->Read(&block_size, 4);

    const wxFileOffset curr_offset = stream->TellI();

    if (uncompressed_size == 0 && block_size == 0) {
      if (child_offset == 0) {
        PSF2File* const entry = new PSF2File(this, filename);
        AddEntry(entry);
        continue;
      }
      PSF2Directory* const entry = new PSF2Directory(this, filename);
      stream->SeekI(base_offset, wxFromStart);
      entry->LoadEntries(stream, child_offset);
      stream->SeekI(curr_offset, wxFromStart);
      AddEntry(entry);
      continue;
    }
    stream->SeekI(base_offset + child_offset, wxFromStart);
    LoadFile(stream, filename, uncompressed_size, block_size);
    stream->SeekI(curr_offset, wxFromStart);
  }

  stream->SeekI(base_offset, wxFromStart);
  return true;
}


////////////////////////////////////////////////////////////////////////
/// PSF2 Loader Class
////////////////////////////////////////////////////////////////////////


PSF2Loader::PSF2Loader(int fd, const wxString &filename)
  : PSFLoader(fd, filename) {
}

PSF2Loader* PSF2Loader::Instance(int fd, const wxString &filename) {
  return new PSF2Loader(fd, filename);
}


bool PSF2Loader::LoadPSF2Entries(PSF2Directory *root) {

  rennyAssert(root->IsRoot());

  wxFileInputStream stream(file());
  stream.SeekI(reserved_area_ofs(), wxFromStart);
  return root->LoadEntries(&stream, 0);
}


PSF2* PSF2Loader::LoadDataEx() {

  LoadInfo();
  if (ref_data_ != nullptr) {
    // TODO: support reload
    return ref_data_;
  }

  PSF2Directory* root = new PSF2Directory(nullptr, "/");
  if (LoadPSF2Entries(root) == false) {
    return nullptr;
  }

  LoadLibraries();

  wxVector<PSFLoader*>::iterator it = psflib_begin();
  const wxVector<PSFLoader*>::const_iterator it_end = psflib_end();
  for (; it != it_end; ++it) {
    PSF2Loader* const loader = dynamic_cast<PSF2Loader*>(*it);
    if (loader != nullptr) {
      loader->LoadInfo();
      loader->LoadPSF2Entries(root);
    }
  }

  PSF2Entry* irx_entry = root->Find(wxT("psf2.irx"));
  if (irx_entry == nullptr) {
    rennyLogError("PSF2Loader", "psf2.irx is not found.");
    return nullptr;
  }
  PSF2File* psf2irx = irx_entry->file();
  if (psf2irx == nullptr) {
    rennyLogError("PSF2Loader", "psf2.irx is not a file.");
    return nullptr;
  }

  PSF2* p_psf = new PSF2(psf2irx);
  if (p_psf->IsOk() == false) {
    delete p_psf;
    return nullptr;
  }

  ref_data_ = p_psf;
  return p_psf;
}


SoundData* PSF2Loader::LoadData() {
  return LoadDataEx();
}
