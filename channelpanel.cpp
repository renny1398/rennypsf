#include "channelpanel.h"
#include <wx/panel.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
//#include <wx/gauge.h>
#include "SoundManager.h"
#include "app.h"
#include "spu/spu.h"


////////////////////////////////////////////////////////////////////////
// Mute button
////////////////////////////////////////////////////////////////////////


MuteButton::MuteButton(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : wxPanel(parent, id, pos, size, style, name), muted_(false), entered_(false)
{
    Connect(wxID_ANY, wxEVT_ENTER_WINDOW, wxMouseEventHandler(MuteButton::onEnter));
    Connect(wxID_ANY, wxEVT_LEAVE_WINDOW, wxMouseEventHandler(MuteButton::onLeave));
    Connect(wxID_ANY, wxEVT_PAINT, wxPaintEventHandler(MuteButton::onPaint));
}


void MuteButton::render(wxDC &dc)
{
    int width, height;
    GetSize(&width, &height);

    wxVisualAttributes attr = this->GetDefaultAttributes();

    wxPen blackPen(*wxBLACK, 1);
    wxPen whitePen(*wxWHITE, 1);
    wxPen grayPen(wxColour(128, 128, 128));

    dc.SetBrush(attr.colBg);

    if (entered_) {
        dc.SetPen(grayPen);
    } else {
        dc.SetPen(blackPen);
    }
    dc.DrawRectangle(0, 0, width, height);

    if (entered_) {
        dc.SetPen(grayPen);
    } else {
        dc.SetPen(whitePen);
    }
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.DrawLabel(this->GetLabel(), wxRect(0, 0, width, height), wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
}


void MuteButton::onPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    render(dc);
}

void MuteButton::paintNow()
{
    wxClientDC dc(this);
    render(dc);
}


void MuteButton::onEnter(wxMouseEvent& WXUNUSED(event))
{
    entered_ = true;
    paintNow();
}

void MuteButton::onLeave(wxMouseEvent& WXUNUSED(event))
{
    entered_ = false;
    paintNow();
}


////////////////////////////////////////////////////////////////////////
// Volume bar
////////////////////////////////////////////////////////////////////////

VolumeBar::VolumeBar(wxWindow *parent, wxOrientation orientation, int max)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(512, 6)), orientation_(orientation), max_(max)
{
    value_ = 0;
}

wxBEGIN_EVENT_TABLE(VolumeBar, wxPanel)
EVT_PAINT(VolumeBar::paintEvent)
wxEND_EVENT_TABLE()


void VolumeBar::paintEvent(wxPaintEvent &)
{
    wxPaintDC dc(this);
    render(dc);
}

void VolumeBar::paintNow()
{
    wxClientDC dc(this);
    render(dc);
}


void VolumeBar::render(wxDC &dc)
{
    wxASSERT(orientation_ == wxHORIZONTAL);

    int barWidth, barHeight;
    GetSize(&barWidth, &barHeight);

    const int blockNumber = 80;
    const int blockgapWidth = barWidth / blockNumber;
    const int gapWidth = 2;
    const int blockWidth = blockgapWidth - gapWidth;

    const int gapHeight = 1;
    const int blockHeight = barHeight - gapHeight*2;

    int val = value_ * barWidth / max_;

    wxPen greenPen(*wxGREEN, 1);
    wxPen yellowPen(*wxYELLOW, 1);
    wxPen blackPen(*wxBLACK, 1);

    for (int i = 0; i < blockNumber; i++) {
        if (i < blockNumber * 3 / 4) {
            dc.SetPen(greenPen);
            dc.SetBrush(*wxGREEN_BRUSH);
        } else {
            dc.SetPen(yellowPen);
            dc.SetBrush(*wxYELLOW_BRUSH);
        }
        int actBlockWidth = (i*blockgapWidth+blockWidth <= val) ? blockWidth : val % blockgapWidth;
        dc.DrawRectangle(i*blockgapWidth, gapHeight, actBlockWidth, blockHeight);

        dc.SetPen(blackPen);
        dc.SetBrush(*wxBLACK_BRUSH);
        if (val <= i*blockgapWidth+blockWidth) break;
        dc.DrawRectangle(i*blockgapWidth+blockWidth, gapHeight, gapWidth, blockHeight);
    }
    dc.DrawRectangle(val, gapHeight, barWidth-val, blockHeight);
}


////////////////////////////////////////////////////////////////////////
// Keyboard Widget
////////////////////////////////////////////////////////////////////////


int KeyboardWidget::calcKeyboardWidth()
{
    int numOctaves = octaveMax_ - octaveMin_ - 1;
    int keyboardWidth = keyWidth_ * (7 * numOctaves + 1) + 1;
    return keyboardWidth;
}


KeyboardWidget::KeyboardWidget(wxWindow* parent, int keyWidth, int keyHeight) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(513, keyHeight)),
    octaveMin_(defaultOctaveMin), octaveMax_(defaultOctaveMax), muted_(false)
{
    if (keyWidth < 7) keyWidth_ = 6;
    else keyWidth_ = 8;
    keyHeight_ = keyHeight;

    wxWindow::SetSize(calcKeyboardWidth(), keyHeight_);
    // wxSize size = wxWindow::GetSize();
    // wxMessageOutputDebug().Printf(wxT("(%d, %d)"), size.GetWidth(), size.GetHeight());
}


wxBEGIN_EVENT_TABLE(KeyboardWidget, wxPanel)
EVT_PAINT(KeyboardWidget::paintEvent)
wxEND_EVENT_TABLE()


void KeyboardWidget::paintEvent(wxPaintEvent &)
{
    wxPaintDC dc(this);
    render(dc);
}

void KeyboardWidget::paintNow()
{
    wxClientDC dc(this);
    render(dc);
}


void KeyboardWidget::render(wxDC &dc)
{
    wxASSERT(keyWidth_ == 8);

    const int numOctaves = octaveMax_ - octaveMin_ - 1;

    wxPen blackPen(*wxBLACK, 1);
    dc.SetPen(blackPen);
    dc.SetBrush(*wxWHITE_BRUSH);

    for (int i = 0; i < numOctaves * 7 + 1; i++) {
        dc.DrawRectangle(i*keyWidth_, 0, keyWidth_+1, keyHeight_);
    }

    dc.SetBrush(*wxBLACK_BRUSH);
    for (int i = 0; i < numOctaves * 7; i++) {
        switch (i % 7) {
        case 0: /* FALLTHRU */
        case 1: /* FALLTHRU */
        case 3: /* FALLTHRU */
        case 4: /* FALLTHRU */
        case 5:
            dc.DrawRectangle(i*keyWidth_+6, 0, 6, keyHeight_/2);
            break;
        }
    }

    if (pressedKeys_.empty()) return;

    if (muted_) {
        dc.SetPen(wxPen(*wxBLUE, 1));
        dc.SetBrush(*wxBLUE_BRUSH);
    } else {
        dc.SetPen(wxPen(*wxRED, 1));
        dc.SetBrush(*wxRED_BRUSH);
    }

    for (IntSet::const_iterator it = pressedKeys_.begin(); it != pressedKeys_.end(); ++it) {
        int i = *it / 12;
        int j = *it % 12;
        int x, y;
        switch (j) {
        case 0:
            x = (i * 7 + 0) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;
        case 2:
            x = (i * 7 + 1) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;
        case 4:
            x = (i * 7 + 2) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;
        case 5:
            x = (i * 7 + 3) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;
        case 7:
            x = (i * 7 + 4) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;
        case 9:
            x = (i * 7 + 5) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;
        case 11:
            x = (i * 7 + 6) * keyWidth_ + 2;
            y = keyHeight_/2;
            break;

        case 1:
            x = (i * 7 + 0) * keyWidth_ + 6;
            y = 0;
            break;
        case 3:
            x = (i * 7 + 1) * keyWidth_ + 6;
            y = 0;
            break;
        case 6:
            x = (i * 7 + 3) * keyWidth_ + 6;
            y = 0;
            break;
        case 8:
            x = (i * 7 + 4) * keyWidth_ + 6;
            y = 0;
            break;
        case 10:
            x = (i * 7 + 5) * keyWidth_ + 6;
            y = 0;
            break;
        }
    }
}


bool KeyboardWidget::PressKey(int keyIndex)
{
    int keyMax = (octaveMax_ - octaveMin_ - 1) * 12;
    if (keyMax < keyIndex) return false;
    pressedKeys_.insert(keyIndex);
    return true;
}


bool KeyboardWidget::PressKey(double pitch)
{
    // A4 = 440 Hz
    int index = round( ( log(pitch/440.0)/log(2.0) + 5.75 ) * 12.0 );
    if (index < 0 || 120 < index) return false;
    pressedKeys_.insert(index);
    return true;
}


void KeyboardWidget::ReleaseKey()
{
    pressedKeys_.clear();
}


void KeyboardWidget::ReleaseKey(int keyIndex)
{
    pressedKeys_.erase(keyIndex);
}


////////////////////////////////////////////////////////////////////////
// Channel Panel
////////////////////////////////////////////////////////////////////////

ChannelPanel::ChannelPanel(wxWindow *parent)
    : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, "channel_panel"),
      timer(this)
{
    wholeSizer = new wxBoxSizer(wxVERTICAL);

    for (int i = 0; i < 24; i++) {
        ChannelElement element;
        element.ich = i;
        element.channelSizer = new wxBoxSizer(wxHORIZONTAL);
        wxString channelCaption;
        channelCaption.sprintf("ch.%02d", i);
        // element.channelText = new wxStaticText(this, -1, channelCaption, wxDefaultPosition, wxDefaultSize, 0);
        element.muteButton = new MuteButton(this, wxID_ANY, wxDefaultPosition, wxSize(50, 25), 0, channelCaption);
        element.muteButton->SetLabel(channelCaption);
        element.channelSizer->Add(element.muteButton);
        element.volumeSizer = new wxBoxSizer(wxVERTICAL);
        element.volumeSizer->Add(new KeyboardWidget(this, 8, 20), wxFIXED_MINSIZE);

        element.volumeLeft = new VolumeBar(this, wxHORIZONTAL, 2048);
        element.volumeSizer->Add(element.volumeLeft);

        element.channelSizer->Add(element.volumeSizer, wxFIXED_MINSIZE);
        wholeSizer->Add(element.channelSizer);
        elements.push_back(element);
    }

    SetSizer(wholeSizer);
    SetAutoLayout(true);
    wholeSizer->Fit(this);

    timer.Start(50, wxTIMER_CONTINUOUS);
}


ChannelPanel::DrawTimer::DrawTimer(wxWindow* parent): parent(parent)
{
}


void ChannelPanel::DrawTimer::Notify()
{
    parent->Update();
}


void ChannelPanel::Update()
{
    SoundDevice *soundDevice = wxGetApp().GetSoundManager();
    int i = 0;
    for (wxVector<ChannelElement>::iterator it = elements.begin(), it_end = elements.end(); it != it_end; ++it) {
        int currVol = it->volumeLeft->GetValue();
        int nextVol = soundDevice->GetEnvelopeVolume(i++);
        if (currVol == nextVol) continue;
        it->volumeLeft->SetValue(nextVol);
        it->volumeLeft->Refresh();
    }
}



////////////////////////////////////////////////////////////////////////
// Wavetable List Control Panel
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


void WavetableList::onAdd(SPU::SoundBank*, SPU::SamplingTone *tone)
{
    const unsigned int offset = tone->GetSPUOffset();
//    const unsigned int length = tone->GetLength() * 16 / 28;
//    const unsigned int loop = tone->GetLoopIndex() * 16 / 28;

    int itemCount = wxListCtrl::GetItemCount();
    wxString strOffset;
    strOffset << offset;
    wxListCtrl::InsertItem(itemCount, strOffset);

    wxListCtrl::SetItem(itemCount, 0, strOffset);

 /*
    if (length == 0) return;

    wxString strLength;
    strLength << length;
    wxListCtrl::SetItem(itemCount, 2, strLength);

    wxString strLoop;
    strLoop << loop;
    wxListCtrl::SetItem(itemCount, 3, strLoop);
    */
}


void WavetableList::onModify(SPU::SoundBank*, SPU::SamplingTone *tone)
{
    const unsigned int offset = tone->GetSPUOffset();
    const unsigned int length = tone->GetLength() * 16 / 28;
    const unsigned int loop = tone->GetLoopIndex() * 16 / 28;

    wxString strOffset;
    strOffset << offset;

    long itemId = wxListCtrl::FindItem(-1, strOffset);
    if (itemId == wxNOT_FOUND) {
        return;
    }

/*
    wxListItem item;

    for (int i = 0; i < wxListCtrl::GetItemCount(); i++) {
        item.SetId(i);
        item.SetMask(wxLIST_MASK_TEXT);
        wxListCtrl::GetItem(item);
        wxMessageOutputDebug().Printf(item.GetText());
        if (item.GetText() == strOffset) break;
    }
    if (item.GetText() != strOffset) return;

    long itemId = item.GetId();

    wxMessageOutputDebug().Printf(wxT("onModify"));
*/
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

void WavetableList::onRemove(SPU::SoundBank*, SPU::SamplingTone *tone)
{
    const unsigned int offset = tone->GetSPUOffset();
    wxString strOffset;
    strOffset << offset;
    long item = wxListCtrl::FindItem(0, strOffset);
    if (item == wxNOT_FOUND) {
        return;
    }
    wxListCtrl::DeleteItem(item);
}


void WavetableList::onListRightClick(wxListEvent &event)
{
    wxListItem item = event.GetItem();
    long offset;
    item.GetText().ToLong(&offset);
    if (offset < 0 || 0x80000 <= offset) return;
    SPU::SamplingTone *tone = Spu.SoundBank_.GetSamplingTone(offset);
    menuPopup_.SetClientData(tone);
    PopupMenu(&menuPopup_);
}

void WavetableList::onPopupClick(wxCommandEvent &event)
{
    using namespace SPU;
    SamplingTone *tone = reinterpret_cast<SamplingTone*>(static_cast<wxMenu *>(event.GetEventObject())->GetClientData());
    wxMessageOutputDebug().Printf(wxT("offset: %d"), tone->GetSPUOffset());
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
    strFileName << tone->GetSPUOffset() << ".wav";

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
