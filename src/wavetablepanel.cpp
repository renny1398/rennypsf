#include "wavetablepanel.h"
//#include "spu/soundbank.h"
//#include "spu/spu.h"
#include "app.h"


////////////////////////////////////////////////////////////////////////
// Wavetable List Control
////////////////////////////////////////////////////////////////////////


WavetableList::WavetableList(wxWindow *parent) :
  wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT, wxDefaultValidator, wxT("WavetableList"))
{
  wxListCtrl::InsertColumn(COLUMN_INDEX_MUTED, wxT(" "));
  SetColumnWidth(COLUMN_INDEX_MUTED, 24);

  wxListItem itemOffset;
  itemOffset.SetId(COLUMN_INDEX_OFFSET);
  itemOffset.SetWidth(80);
  itemOffset.SetText(_("Offset"));
  wxListCtrl::InsertColumn(COLUMN_INDEX_OFFSET, itemOffset);

  wxListItem itemLength;
  itemLength.SetId(COLUMN_INDEX_LENGTH);
  itemLength.SetWidth(64);
  itemLength.SetText(_("Length"));
  wxListCtrl::InsertColumn(COLUMN_INDEX_LENGTH, itemLength);

  wxListItem itemLoop;
  itemLoop.SetId(COLUMN_INDEX_LOOP);
  itemLoop.SetWidth(64);
  itemLoop.SetText(_("Loop"));
  wxListCtrl::InsertColumn(COLUMN_INDEX_LOOP, itemLoop);

  wxListItem itemFFT;
  itemFFT.SetId(COLUMN_INDEX_FFT);
  itemFFT.SetWidth(80);
  itemFFT.SetText(_("FFT"));
  wxListCtrl::InsertColumn(COLUMN_INDEX_FFT, itemFFT);

  wxEvtHandler::Connect(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, (wxObjectEventFunction)&WavetableList::onListRightClick, 0, this);

  // create a popup menu
  menuPopup_.Append(ID_EXPORT_WAVE, _("&Export"));
  wxEvtHandler::Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&WavetableList::onPopupClick, 0, this);

  // receive key input
  wxEvtHandler::Bind(wxEVT_CHAR, &WavetableList::OnChar, this);
}


long WavetableList::Find(int number) {
  const int count = wxListCtrl::GetItemCount();
  for (long i = 0; i < count; i++) {
    long ofs;
    wxString str = wxListCtrl::GetItemText(i, COLUMN_INDEX_OFFSET);
    str.ToLong(&ofs);
    if (ofs == number) {
      return i;
    }
  }
  return wxNOT_FOUND;
}




void WavetableList::OnAdd(ToneInfo* tone)
{
  const unsigned int offset = tone->number;
  // const unsigned int length = tone->length;
  // const unsigned int loop = tone->loop;

  // offset 4096 may be dummy
  if (offset == 4096) return;

  const int itemCount = wxListCtrl::GetItemCount();
  int i = 0;
  while (i < itemCount) {
    long ofs;
    GetItemText(i, COLUMN_INDEX_OFFSET).ToLong(&ofs);
    if (offset < ofs) break;
    i++;
  }

  wxString strOffset;
  strOffset << offset;

  wxListCtrl::InsertItem(i, strOffset);
  wxListCtrl::SetItem(i, COLUMN_INDEX_MUTED, wxT(" "));
  wxListCtrl::SetItem(i, COLUMN_INDEX_OFFSET, strOffset);
}


void WavetableList::OnModify(ToneInfo* tone)
{
  const unsigned int offset = tone->number;
  const unsigned int length = tone->length;
  const unsigned int loop = tone->loop;

  wxString strOffset;
  strOffset << offset;

  long itemId = Find(offset);
  if (itemId == wxNOT_FOUND) {
    return;
  }

  wxString strLength;
  strLength << length;
  wxListCtrl::SetItem(itemId, COLUMN_INDEX_LENGTH, strLength);

  wxString strLoop;
  strLoop << loop;
  wxListCtrl::SetItem(itemId, COLUMN_INDEX_LOOP, strLoop);

  float freq = tone->pitch;
  if (freq >= 1.0) {
    wxString strFreq;
    strFreq << freq;
    wxListCtrl::SetItem(itemId, COLUMN_INDEX_FFT, strFreq);
  }
}


void WavetableList::OnRemove(ToneInfo* tone)
{
  const unsigned int offset = tone->number;
  // wxString strOffset;
  // strOffset << offset;
  long item = Find(offset);
  if (item == wxNOT_FOUND) {
    return;
  }
  wxListCtrl::DeleteItem(item);
}


void WavetableList::OnChar(wxKeyEvent &event) {
  int key = event.GetKeyCode();
  switch (key) {
  case 'M':
  case 'm':
/*
    {
      long item = -1;
      do {
        item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == -1) break;
        wxString str_ofs = wxListCtrl::GetItemText(item, COLUMN_INDEX_OFFSET);
        long ofs;
        str_ofs.ToLong(&ofs);
        // bool muted = wxGetApp().GetSoundManager()->SwitchToneMuted(ofs);
        const wxSharedPtr<SoundFormat>& sf = wxGetApp().GetPlayingSound();
        if (sf == NULL) continue;
        bool muted =
        if (muted == true) {
          wxListCtrl::SetItem(item, COLUMN_INDEX_MUTED, wxT("M"));
        } else {
          wxListCtrl::SetItem(item, COLUMN_INDEX_MUTED, wxT(" "));
        }
      } while (true);
    }*/
    return;
  }
}




void WavetableList::onPopupClick(wxCommandEvent &event)
{
  using namespace SPU;
  Instrument *tone = reinterpret_cast<Instrument*>(static_cast<wxMenu *>(event.GetEventObject())->GetClientData());
  wxMessageOutputDebug().Printf(wxT("offset: %d"), tone->id());
  switch (event.GetId()) {
  case ID_EXPORT_WAVE:
    ExportTone(tone);
    break;
  default:
    break;
  }
}


#include <wx/file.h>

void WavetableList::ExportTone(Instrument *tone)
{
  using namespace SPU;

  wxString strFileName;
  strFileName << tone->id() << ".wav";

  wxFile file(strFileName, wxFile::write);

  int length = 0;

  file.Write("RIFF", 4);
  file.Write(&length, 4);
  file.Write("WAVEfmt ", 8);

  const int channelNumber = 1;
  const int bitNumber = 16;
  const int samplingRate = 44100; // TODO: sampling rate
  const int blockSize = (bitNumber/8) * channelNumber;
  const int dataRate = samplingRate * blockSize;

  const int fmtSize = 16;
  const int fmtId = 1;
  file.Write(&fmtSize, 4);
  file.Write(&fmtId, 2);
  file.Write(&channelNumber, 2);
  file.Write(&samplingRate, 4);
  file.Write(&dataRate, 4);
  file.Write(&blockSize, 2);
  file.Write(&bitNumber, 2);

  file.Write("data", 4);
  file.Write(&length, 4);

  length = tone->length();
  for (int i = 0; i < length; i++) {
    int s = tone->at(i);
    file.Write(&s, 2);
  }

  length *= 2;
  file.Seek(0x28, wxFromStart);
  file.Write(&length, 4);
  length += 0x24;
  file.Write(&length, 4);

  file.Close();

  wxMessageOutputDebug().Printf(wxT("Saved as %s"), strFileName);
}


void WavetableList::onListRightClick(wxListEvent &event)
{
  wxListItem item = event.GetItem();
  long offset;
  item.GetText().ToLong(&offset);
  if (offset < 0 || 0x80000 <= offset) return;
//  SPU::SamplingTone *tone = Spu.SoundBank_.GetSamplingTone(offset);
//  menuPopup_.SetClientData(tone);
//  PopupMenu(&menuPopup_);
}


////////////////////////////////////////////////////////////////////////
// Wavetable Panel
////////////////////////////////////////////////////////////////////////


wxBEGIN_EVENT_TABLE(WavetablePanel, wxPanel)
EVT_COMMAND(wxID_ANY, wxEVT_ADD_TONE, WavetablePanel::onAdd)
EVT_COMMAND(wxID_ANY, wxEVT_CHANGE_TONE, WavetablePanel::onModify)
EVT_COMMAND(wxID_ANY, wxEVT_REMOVE_TONE, WavetablePanel::onRemove)
wxEND_EVENT_TABLE()


#include <wx/sizer.h>

WavetablePanel::WavetablePanel(wxWindow *parent) :
  wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
  wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
  WavetableList *listCtrl = new WavetableList(this);
  mainSizer->Add(listCtrl, wxEXPAND);

  this->SetSizer(mainSizer);

  listCtrl_ = listCtrl;
  mainSizer_ = mainSizer;

  // Spu.SoundBank_.AddListener(this);
  // wxGetApp().GetSoundManager()->AddToneListener(this);
}



void WavetablePanel::onAdd(wxCommandEvent& event)
{
  ToneInfo* tone = (ToneInfo*)event.GetClientData();
  listCtrl_->OnAdd(tone);
  delete tone;
}


void WavetablePanel::onModify(wxCommandEvent& event)
{
  ToneInfo* tone = (ToneInfo*)event.GetClientData();
  listCtrl_->OnModify(tone);
  delete tone;
}

void WavetablePanel::onRemove(wxCommandEvent& event)
{
  ToneInfo* tone = (ToneInfo*)event.GetClientData();
  listCtrl_->OnRemove(tone);
  delete tone;
}
