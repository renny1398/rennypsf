#include "channelpanel.h"
#include <wx/panel.h>
#include <wx/dc.h>
#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
//#include <wx/gauge.h>
#include "SoundManager.h"
#include "app.h"


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
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(256, 12)), orientation_(orientation), max_(max)
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

    const int blockNumber = 20;
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
    wxSize size = wxWindow::GetSize();
    wxMessageOutputDebug().Printf(wxT("(%d, %d)"), size.GetWidth(), size.GetHeight());
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
        element.muteButton = new MuteButton(this, wxID_ANY, wxDefaultPosition, wxSize(60, 60), 0, channelCaption);
        element.muteButton->SetLabel(channelCaption);
        element.channelSizer->Add(element.muteButton);
        element.volumeSizer = new wxBoxSizer(wxVERTICAL);
        element.volumeSizer->Add(new KeyboardWidget(this, 8, 20));
        element.volumeLeft = new VolumeBar(this, wxHORIZONTAL, 65536 * 64);
        element.volumeRight = new VolumeBar(this, wxHORIZONTAL, 65536 * 64) ;
        element.volumeSizer->Add(element.volumeLeft);
        element.volumeSizer->Add(element.volumeRight);
        element.channelSizer->Add(element.volumeSizer);
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
        int currLeft = it->volumeLeft->GetValue();
        int currRight = it->volumeRight->GetValue();
        int nextLeft, nextRight;
        soundDevice->GetEnvelopeVolume(i, &nextLeft, &nextRight);
        i++;
        if (currLeft == nextLeft && currRight == nextRight) continue;
        it->volumeLeft->SetValue(nextLeft);
        it->volumeRight->SetValue(nextRight);
        it->volumeLeft->Refresh();
        it->volumeRight->Refresh();
    }
}
