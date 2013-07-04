#include "wavetablepanel.h"
#include "spu/soundbank.h"
#include "spu/spu.h"


////////////////////////////////////////////////////////////////////////
// Wavetable List Control
////////////////////////////////////////////////////////////////////////


WavetableList::WavetableList(wxWindow *parent) :
    wxListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT, wxDefaultValidator, wxT("WavetableList"))
{
    wxListItem itemOffset;
    itemOffset.SetId(1);
    itemOffset.SetWidth(80);
    itemOffset.SetText(_("Offset"));
    wxListCtrl::InsertColumn(0, itemOffset);

    wxListItem itemLength;
    itemLength.SetId(2);
    itemLength.SetWidth(64);
    itemLength.SetText(_("Length"));
    wxListCtrl::InsertColumn(1, itemLength);

    wxListItem itemLoop;
    itemLoop.SetId(3);
    itemLoop.SetWidth(64);
    itemLoop.SetText(_("Loop"));
    wxListCtrl::InsertColumn(2, itemLoop);

    wxListItem itemFFT;
    itemFFT.SetId(3);
    itemFFT.SetWidth(64);
    itemFFT.SetText(_("FFT"));
    wxListCtrl::InsertColumn(3, itemFFT);

    wxEvtHandler::Connect(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK, (wxObjectEventFunction)&WavetableList::onListRightClick, 0, this);

    // create a popup menu
    menuPopup_.Append(ID_EXPORT_WAVE, _("&Export"));
    wxEvtHandler::Connect(wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&WavetableList::onPopupClick, 0, this);
}


void WavetableList::OnAdd(SPU::SamplingTone* tone)
{
    const unsigned int offset = tone->GetAddr();
//    const unsigned int length = tone->GetLength() * 16 / 28;
//    const unsigned int loop = tone->GetLoopIndex() * 16 / 28;

    int itemCount = wxListCtrl::GetItemCount();
    wxString strOffset;
    strOffset << offset;
    wxListCtrl::InsertItem(itemCount, strOffset);

    wxListCtrl::SetItem(itemCount, 0, strOffset);
}


void WavetableList::OnModify(SPU::SamplingTone* tone)
{
    const unsigned int offset = tone->GetAddr();
    const unsigned int length = tone->GetLength() * 16 / 28;
    const unsigned int loop = tone->GetLoopOffset() * 16 / 28;

    wxString strOffset;
    strOffset << offset;

    long itemId = wxListCtrl::FindItem(-1, strOffset);
    if (itemId == wxNOT_FOUND) {
        return;
    }

    wxString strLength;
    strLength << length;
    wxListCtrl::SetItem(itemId, 1, strLength);

    wxString strLoop;
    strLoop << loop;
    wxListCtrl::SetItem(itemId, 2, strLoop);

    double freq = tone->GetFreq();
    if (freq >= 1.0) {
        wxString strFreq;
        strFreq << freq;
        wxListCtrl::SetItem(itemId, 3, strFreq);
    }
}


void WavetableList::OnRemove(SPU::SamplingTone* tone)
{
    const unsigned int offset = tone->GetAddr();
    wxString strOffset;
    strOffset << offset;
    long item = wxListCtrl::FindItem(0, strOffset);
    if (item == wxNOT_FOUND) {
        return;
    }
    wxListCtrl::DeleteItem(item);
}



void WavetableList::onPopupClick(wxCommandEvent &event)
{
    using namespace SPU;
    SamplingTone *tone = reinterpret_cast<SamplingTone*>(static_cast<wxMenu *>(event.GetEventObject())->GetClientData());
    wxMessageOutputDebug().Printf(wxT("offset: %d"), tone->GetAddr());
    switch (event.GetId()) {
    case ID_EXPORT_WAVE:
        ExportTone(tone);
        break;
    default:
        break;
    }
}


#include <wx/file.h>

void WavetableList::ExportTone(SPU::SamplingTone *tone)
{
    using namespace SPU;

    wxString strFileName;
    strFileName << tone->GetAddr() << ".wav";

    wxFile file(strFileName, wxFile::write);

    int length = 0;

    file.Write("RIFF", 4);
    file.Write(&length, 4);
    file.Write("WAVEfmt ", 8);

    const int channelNumber = 1;
    const int bitNumber = 16;
    const int samplingRate = 44100;
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

    length = tone->GetLength();
    for (int i = 0; i < length; i++) {
        int s = tone->At(i);
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
  /*
    wxListItem item = event.GetItem();
    long offset;
    item.GetText().ToLong(&offset);
    if (offset < 0 || 0x80000 <= offset) return;
    SPU::SamplingTone *tone = Spu.SoundBank_.GetSamplingTone(offset);
    menuPopup_.SetClientData(tone);
    PopupMenu(&menuPopup_);
  */
}


////////////////////////////////////////////////////////////////////////
// Wavetable Panel
////////////////////////////////////////////////////////////////////////


wxBEGIN_EVENT_TABLE(WavetablePanel, wxPanel)
EVT_COMMAND(wxID_ANY, wxEVENT_SPU_ADD_TONE, WavetablePanel::onAdd)
EVT_COMMAND(wxID_ANY, wxEVENT_SPU_MODIFY_TONE, WavetablePanel::onModify)
EVT_COMMAND(wxID_ANY, wxEVENT_SPU_REMOVE_TONE, WavetablePanel::onRemove)
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
}



void WavetablePanel::onAdd(wxCommandEvent& event)
{
    SPU::SamplingTone* tone = (SPU::SamplingTone*)event.GetClientData();
    listCtrl_->OnAdd(tone);
}


void WavetablePanel::onModify(wxCommandEvent& event)
{
    SPU::SamplingTone* tone = (SPU::SamplingTone*)event.GetClientData();
    listCtrl_->OnModify(tone);
}

void WavetablePanel::onRemove(wxCommandEvent& event)
{
    SPU::SamplingTone* tone = (SPU::SamplingTone*)event.GetClientData();
    listCtrl_->OnRemove(tone);
}
