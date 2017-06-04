#include "psf/psx/iop.h"
#include "psf/psx/r3000a.h"
#include "psf/psf.h"
#include "common/debug.h"
#include <wx/vector.h>
#include <wx/regex.h>

// for debug
#include <sstream>
#include <iomanip>
#include "psf/psx/disassembler.h"

namespace psx {


IOP::IOP(PSX* psx) :
  Component(psx), UserMemoryAccessor(psx), root_(nullptr),
#ifndef NDEBUG
  p_disasm_(&psx->Disasm()),
#endif
  load_addr_(0x23f00) {
#ifndef NDEBUG
  rennyAssert(p_disasm_ != nullptr);
#endif
}

IOP::~IOP() {
  if (root_) {
    delete root_;
    root_ = nullptr;
  }
}


PSXAddr IOP::load_addr() const {
  return load_addr_;
}

void IOP::set_load_addr(PSXAddr load_addr) {
  load_addr_ = load_addr;
}


void IOP::SetRootDirectory(const PSF2Directory *root) {
  root_ = root;
}


unsigned int IOP::LoadELF(PSF2File* psf2irx) {

  if (psf2irx == nullptr) {
    return 0xffffffff;
  }
  const unsigned char* data = psf2irx->GetData();

  if (load_addr_ & 3) {
    load_addr_ &= ~3;
    load_addr_ += 4;
  }

  if ((data[0] != 0x7f) || (data[1] != 'E') ||
      (data[2] != 'L') || (data[3] != 'F') ) {
    rennyLogError("PSF2", "%s is not ELF file.", static_cast<const char*>(psf2irx->GetName()));
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
      Psx().Memcpy(load_addr_ + addr, &data[offset], size);
      totallen += size;
      break;

    case 2:
      break;

    case 3:
      break;

    case 8:
      Psx().Memset(load_addr_ + addr, 0, size);
      totallen += size;
      break;

    case 9:
      for (unsigned int rec = 0; rec < (size/8); rec++) {
        uint32_t offs, info, target, temp, val, vallo;
        static uint32_t hi16offs = 0, hi16target = 0;

        offs = *(reinterpret_cast<const unsigned int*>(data + offset + (rec*8)));
        info = *(reinterpret_cast<const unsigned int*>(data + offset + (rec*8) + 4));
        target = BFLIP32(psxMu32val(load_addr_ + offs));

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

          psxMu32ref(load_addr_ + hi16offs) = BFLIP32(hi16target);
          break;

        default:
          break;
        }

        psxMu32ref(load_addr_ + offs) = BFLIP32(target);
      }
      break;

    case 0x70000000:
      // do_iopmod
      rennyLogWarning("IOP::LoadELF", "Not implemented do_iopmod().");
      break;

    default:
      break;
    }
    shent += shentsize;
  }

  entry += load_addr_;
  entry |= 0x80000000;
  load_addr_ += totallen;

  rennyLogDebug("IOP::LoadELF", "%s Load Entry = 0x%08x", psf2irx->GetName().c_str().AsChar(), entry);
  return entry;
}



const wxString IOP::sprintf(const wxString& format, uint32_t param1, uint32_t param2, uint32_t param3) const {

  wxRegEx re_esc("\\x1b");
  wxRegEx re_isnum("%[0-9.]*[xXdDcCuU]");
  wxRegEx re_isstr("%s");

  /*
  if (re_esc.IsValid() == false) {
    rennyLogError("IOP_sprintf", "ESC RegEx is invalid.");
    return wxString("");
  } else if (re_isnum.IsValid() == false) {
    rennyLogError("IOP_sprintf", "IsNum RegEx is invalid.");
    return wxString("");
  } else if (re_isstr.IsValid() == false) {
    rennyLogError("IOP_sprintf", "IsStr RegEx is invalid.");
    return wxString("");
  }
  */

  wxString msg(format);
  re_esc.ReplaceAll(&msg, wxT("[ESC]"));

  const uint32_t params[3] = { param1, param2, param3 };

  wxVector<uint32_t> printf_params;
  for (int i = 0; i < 3; i++) {
    size_t start, len;
    size_t idx_isnum = 0xffffffff, idx_isstr = 0xffffffff;
    size_t len_isnum = 0;
    if (re_isnum.Matches(msg)) {
      re_isnum.GetMatch(&start, &len, 0);
      // rennyLogDebug("IOP_sprintf", "Number is matched @ %d", start);
      idx_isnum = start;
      len_isnum = len;
    }
    if (re_isstr.Matches(msg)) {
      re_isstr.GetMatch(&start, &len, 0);
      // rennyLogDebug("IOP_sprintf", "String is matched @ %d", start);
      idx_isstr = start;
    }
    if (idx_isnum < idx_isstr) {
      printf_params.push_back(params[i]);
    } else if (idx_isstr < idx_isnum) {
      const wxString str_param(const_cast<IOP*>(this)->psxMu8ptr(params[i]));
      // rennyLogDebug("IOP_sprintf", "String parameter (%d): %s", i, static_cast<const char*>(str_param));
      re_isstr.ReplaceFirst(&msg, str_param);
      // rennyLogDebug("IOP_sprintf", static_cast<const char*>(msg));
      // wxMessageOutputDebug().Output(msg);
    } else {
      break;
    }
  }

  for (size_t i = printf_params.size(); i < 3; i++) {
    printf_params.push_back(0);
  }

  msg.Printf(msg, printf_params[0], printf_params[1], printf_params[2]);
  return msg;
}



bool IOP::stdio(uint32_t call_num) {

  const uint32_t a0 = R3000ARegs().GPR.A0;
  const uint32_t a1 = R3000ARegs().GPR.A1;
  const uint32_t a2 = R3000ARegs().GPR.A2;
  const uint32_t a3 = R3000ARegs().GPR.A3;

  switch (call_num) {
  case 4: // printf
    do {
      wxString out = sprintf(psxMs8ptr(a0), a1, a2, a3);
      if (out.Find("start") != wxNOT_FOUND) {
        // rennyLogInfo("IOP::stdio(printf)", "Start Dumping.");
        // Disasm().StartOutputToFile();
      } else if (out.Find("end") != wxNOT_FOUND) {
        // rennyLogInfo("IOP::stdio(printf)", "End Dumping.");
        // Disasm().StopOutputToFile();
      }
      rennyLogInfo("IOP::stdio(printf)", static_cast<const char*>(out));
    } while (false);
    return true;
  default:
    rennyLogError("IOP::stdio", "Unhandled service %d.", call_num);
    return false;
  }
}


bool IOP::sysmem(uint32_t call_num) {

  const uint32_t a0 = R3000ARegs().GPR.A0;
  uint32_t a1 = R3000ARegs().GPR.A1;
  const uint32_t a2 = R3000ARegs().GPR.A0;
  const uint32_t a3 = R3000ARegs().GPR.A0;  // dummy

  switch (call_num) {
  case 4: // AllocMemory
    {
      PSXAddr new_alloc_addr = load_addr_;
      if (new_alloc_addr & 0xf) {
        new_alloc_addr &= 0xfffffff0;
        new_alloc_addr += 0x10;
      }
      if (a1 & 0xf) {
        a1 &= 0xfffffff0;
        a1 += 0x10;
      }
      load_addr_ = new_alloc_addr;
      rennyLogDebug("IOP::sysmem", "AllocMemory(%d, %d, 0x%x) = 0x%08x", a0, a1, a2, new_alloc_addr | 0x80000000);
      R3000ARegs().GPR.V0 = new_alloc_addr;
    }
    return true;

  case 5: // FreeMemory
    rennyLogDebug("IOP::sysmem", "FreeMemory(0x%08x)", a0);
    return true;

  case 7: // QueryMaxFreeMemSize
    rennyLogDebug("IOP::sysmem", "QueryMaxFreeMemSize");
    R3000ARegs().GPR.V0 = (2*1024*1024) - load_addr_;
    return true;

  case 8: // QueryTotalFreeMemSize
    rennyLogDebug("IOP::sysmem", "QueryTotalFreeMemSize");
    R3000ARegs().GPR.V0 = (2*1024*1024) - load_addr_;
    return true;

  case 14:  // Kprintf
    {
      wxString out = IOP::sprintf(wxString(psxMs8ptr(a0) + (a0 & 3)), a1, a2, a3);
      out.Replace(wxT("\x1b"), wxT("]"));
      rennyLogDebug("IOP::sysmem", "KTTY: %s", static_cast<const char*>(out));
    }
    return true;

  default:
    rennyLogError("IOP::sysmem", "Unhandled service %d.", call_num);
    return false;
  }
}


bool IOP::modload(uint32_t call_num) {

  const uint32_t a0 = R3000ARegs().GPR.A0;
  uint32_t a1 = R3000ARegs().GPR.A1;
  const uint32_t a2 = R3000ARegs().GPR.A2;

  switch (call_num) {    
  case 7:	// LoadStartModule
    {
      wxString module_name = psxMs8ptr(a0 + 8); // len("aofile:/") == 8
      // wxString str1 = psxMs8ptr(a2);

      uint32_t new_alloc_addr = load_addr_;
      if (new_alloc_addr & 0xf) {
        new_alloc_addr &= 0xfffffff0;
        new_alloc_addr += 0x10;
      }
      load_addr_ = new_alloc_addr + 2048;

      PSF2Entry* module_entry = (const_cast<PSF2Directory*>(root_)->Find(module_name));
      if (module_entry != nullptr) {

        PSF2File* module = module_entry->file();
        uint32_t start = LoadELF(module);

        if (start != 0xffffffff) {
          uint32_t args[20], numargs = 1, argofs;
          uint8_t *argwalk = psxMu8ptr(a2);

          args[0] = a0; // arg0: module name
          argofs = 0;

          if (a1 > 0) {
            args[numargs++] = a2;

            while (a1) {
              if ((*argwalk == 0) && (a1 > 1)) {
                args[numargs++] = a2 + argofs + 1;
              }
              argwalk++;
              argofs++;
              a1--;
            }
          }

          for (uint32_t i = 0; i < numargs; i++) {
            psxMu32ref(new_alloc_addr + i*4) = BFLIP32(args[i]);
          }

          // for debug
          std::stringstream ss;
          ss << "(";
          for (decltype(numargs) i = 1; i < numargs; ++i) {
            ss << "0x" << std::hex << psxMu32val(new_alloc_addr + i*4);
            if (i+1 != numargs) {
              ss << ", ";
            }
          }
          ss << ")";
          rennyLogDebug("IOP::modload", "LoadStartModule: %s %s",
                        static_cast<const char*>(module_name),
                        static_cast<const char*>(ss.str().c_str()));

          R3000ARegs().GPR.A0 = numargs;
          R3000ARegs().GPR.A1 = 0x80000000 | new_alloc_addr;

          R3000ARegs().PC = start/* - 4*/;
          R3000a().LeaveRAAlone();
#ifndef NDEBUG
          wxString disasm_out;
          disasm_out.sprintf("PC := 0x%08X", start);
          p_disasm_->OutputStringToFile(disasm_out);
#endif
        }
      }
    }
    return true;
  case 4: // ReBootStart
  case 5: // LoadModuleAddress
  case 6: // LoadModule
  case 8: // StartModule
  case 9: // LoadModuleBufferAddess
  case 10:// LoadModuleBuffer
  default:
    rennyLogError("IOP::modload", "Unhandled service %d.", call_num);
    return false;
  }
}


bool IOP::ioman(uint32_t call_num) {

  const uint32_t a0 = R3000ARegs().GPR.A0;
  const uint32_t a1 = R3000ARegs().GPR.A1;
  const uint32_t a2 = R3000ARegs().GPR.A2;
  // const uint32_t a3 = R3000ARegs().GPR.A3;

  switch (call_num) {
  case 4: // open
    {
      int handle = -1;
      for (int i = 0; files_.begin() + i != files_.end(); i++) {
        if (files_[i].file == nullptr) {
          handle = i;
          break;
        }
      }
      if (handle == -1) {
        files_.push_back(File());
        handle = files_.size() - 1;
      }

      wxString filename(psxMs8ptr(a0));
      filename = filename.Mid(filename.find_first_of(":/") + 2, wxString::npos);
      PSF2File* file = dynamic_cast<PSF2File*>(const_cast<PSF2Directory*>(root_)->Find(filename));

      if (file == nullptr) {
        rennyLogError("IOP::ioman(open)", "Cannot open a file '%s'", static_cast<const char*>(filename));
        R3000ARegs().GPR.V0 = 0xffffffff;
        return false;
      }

      rennyLogDebug("IOP::ioman(open)", "Open a file '%s'", static_cast<const char*>(filename));
      files_[handle].file = file;
      files_[handle].pos = 0;
      R3000ARegs().GPR.V0 = handle;
    }
    return true;

  case 5: // close
    {
      const size_t n = files_.size();
      if (n <= a0) return true;
      if (n - 1 == a0) {
        files_.pop_back();
        return true;
      }
      files_[a0].file = nullptr;
      files_[a0].pos = 0;
    }
    return true;

  case 6: // read
    {
      rennyLogDebug("IOP::ioman(read)", "read(%d, 0x%05x, %d)", a0, a1, a2);
      if (files_.size() <= a0) {
        R3000ARegs().GPR.V0 = 0;
        return false;
      }
      PSF2File* file = files_[a0].file;
      if (file == nullptr) {
        R3000ARegs().GPR.V0 = 0;
        return false;
      }
      size_t& pos = files_[a0].pos;
      size_t size = a2;
      if (file->GetSize() - pos < size) {
        size = file->GetSize() - pos;
      }
      Psx().Memcpy(a1, file->GetData(), size);
      pos += size;
      R3000ARegs().GPR.V0 = size;
    }
    return true;

  case 8: // lseek
    switch (a2) {
    case 0: // SEEK_SET
      files_[a0].pos = a1;
      break;
    case 1: // SEEK_CUR
      files_[a0].pos += static_cast<int>(a1);
      break;
    case 2: // SEEK_END
      // files_[a0].pos = files_[a0].file->GetSize() - a1;
      files_[a0].pos = files_[a0].file->GetSize() + static_cast<int>(a1);
      break;
    default:
      return false;
    }
    R3000ARegs().GPR.V0 = files_[a0].pos;
    rennyLogDebug("IOP::ioman(lseek)", "lseek(%d, 0x%08x, %d) -> 0x%08x",
                  a0, a1, a2, files_[a0].pos);
    return true;

  case 20:  // AddDrv
    rennyLogDebug("IOP::ioman", "Call AddDrv.");
    R3000ARegs().GPR.V0 = 0;
    return true;

  case 21:  // DelDrv
    rennyLogDebug("IOP::ioman", "Call DelDrv.");
    R3000ARegs().GPR.V0 = 0;
    return true;

  case 7:   // write
  case 9:   // ioctl
  case 10:  // remove
  case 11:  // mkdir
  case 12:  // rmdir
  case 13:  // dopen
  case 14:  // dclose
  case 15:  // dread
  case 16:  // getstat
  case 17:  // chstat
  case 18:  // format
  default:
    rennyLogError("IOP::ioman", "Unhandled service %d.", call_num);
    return false;
  }
}


bool IOP::Call(PSXAddr pc, uint32_t call_num) {

  PSXAddr scan = pc & 0x0fffffff;
  while ((psxMu32val(scan) != BFLIP32(0x41e00000)) && (scan >= 0x10000)) {
    scan -= 4;
  }

  if (psxMu32val(scan) != BFLIP32(0x41e00000)) { // scan < 0x10000
    rennyLogError("IOP", "Couldn't find IOP link signature.");
    return false;
  }

  scan += 12;	// skip '0x41e00000', zero and version

  const wxString module_name(reinterpret_cast<const char*>(psxMu32ptr(scan)), 8);

  bool ret = false;
  rennyLogDebug("IOP(Debug)", "module_name: %s (length = %ld)",
                static_cast<const char*>(module_name), module_name.length());
#ifndef NDEBUG
  wxString str_callinfo("IOP: call ");
  str_callinfo.Append(module_name);
  p_disasm_->OutputStringToFile(str_callinfo);
#endif
  if (module_name.IsSameAs(wxString("stdio\0\0\0", 8))) {
    ret = stdio(call_num);
  } else if (module_name.IsSameAs(wxString("sysmem\0\0", 8))) {
    ret = sysmem(call_num);
  } else if (module_name.IsSameAs(wxString("modload\0", 8))) {
    ret = modload(call_num);
  } else if (module_name.IsSameAs(wxString("ioman\0\0\0", 8))) {
    ret = ioman(call_num);
  } else {
    rennyLogError("IOP", "Unhandled service %d for module %s.", call_num, static_cast<const char*>(module_name));
    return false;
  }
  return ret;
}

} // namespace psx
