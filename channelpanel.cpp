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


MuteButton::MuteButton(wxWindow* parent, int channel_number) :
  wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(50, 25)), muted_(false), entered_(false)
{
  channel_number_ = channel_number;
  // Connect(wxID_ANY, wxEVT_ENTER_WINDOW, wxMouseEventHandler(MuteButton::onEnter));
  // Connect(wxID_ANY, wxEVT_LEAVE_WINDOW, wxMouseEventHandler(MuteButton::onLeave));
  // Connect(wxID_ANY, wxEVT_PAINT, wxPaintEventHandler(MuteButton::onPaint));

  wxEvtHandler::Bind(wxEVT_PAINT, &MuteButton::onPaint, this);
  wxEvtHandler::Bind(wxEVT_LEFT_DOWN, &MuteButton::onClick, this);
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

  if (muted_) {
    dc.SetPen(grayPen);
  } else {
    dc.SetPen(blackPen);
  }
  dc.SetBrush(*wxWHITE_BRUSH);
  const wxString label = wxString::Format(wxT("ch.%2d"), channel_number_);
  wxSize str_size = dc.GetTextExtent(label);
  dc.DrawText(label, (width - str_size.GetWidth()) / 2, (height - str_size.GetHeight()) / 2);
  // dc.DrawLabel(label, wxRect(0, 0, width, height), wxALIGN_CENTER_VERTICAL | wxALIGN_CENTER_HORIZONTAL);
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


void MuteButton::onClick(wxMouseEvent &)
{
  muted_ = !muted_;
  if (muted_) {
    wxGetApp().GetSoundManager()->Mute(channel_number_);
    SetForegroundColour(wxColour(128, 128, 128));
    // wxMessageOutputDebug().Printf(wxT("Mute on ch.%d"), channel_number_);
  } else {
    wxGetApp().GetSoundManager()->Unmute(channel_number_);
    SetForegroundColour(wxColour(0, 0, 0));
    // wxMessageOutputDebug().Printf(wxT("Mute off ch.%d"), channel_number_);
  }
  Refresh(false);
}






////////////////////////////////////////////////////////////////////////
// Volume bar
////////////////////////////////////////////////////////////////////////

VolumeBar::VolumeBar(wxWindow *parent, wxOrientation orientation, int ch)
  : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(512, 4)), orientation_(orientation)
{
  ch_ = ch;
  value_ = 0;

  // wxEvtHandler::Bind(wxEVT_CHANGE_VELOCITY, &VolumeBar::OnChangeVelocity, this);
  wxGetApp().GetSoundManager()->AddListener(this, ch);
}

wxBEGIN_EVENT_TABLE(VolumeBar, wxPanel)
EVT_PAINT(VolumeBar::paintEvent)
wxEND_EVENT_TABLE()


void VolumeBar::paintEvent(wxPaintEvent &)
{
  wxPaintDC dc(this);

  wxASSERT(orientation_ == wxHORIZONTAL);

  int barWidth, barHeight;
  GetSize(&barWidth, &barHeight);

  const int blockNumber = 64;
  const int blockgapWidth = barWidth / blockNumber;
  const int gapWidth = 1;
  const int blockWidth = blockgapWidth - gapWidth;

  const int gapHeight = 1;
  const int blockHeight = barHeight - gapHeight*2;

  int val = static_cast<int>(value_ * barWidth);

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


void VolumeBar::OnChangeVelocity(const NoteInfo &note) {
    const float prev_vel = GetValue();
    if (note.velocity != prev_vel) {
        SetValue(note.velocity);
        int width, height;
        GetSize(&width, &height);
        wxRect rect;
        if (prev_vel <= note.velocity) {
            rect.SetLeft(prev_vel);
            rect.SetRight(note.velocity);
        } else {
            rect.SetLeft(note.velocity);
            rect.SetRight(prev_vel);
        }
        rect.SetTop(0);
        rect.SetBottom(height);
        // RefreshRect(rect);
    }
}




////////////////////////////////////////////////////////////////////////
// Keyboard Widget
////////////////////////////////////////////////////////////////////////


wxDEFINE_EVENT(wxEVENT_SPU_CHANNEL_NOTE_ON, wxCommandEvent);
wxDEFINE_EVENT(wxEVENT_SPU_CHANNEL_NOTE_OFF, wxCommandEvent);

wxBEGIN_EVENT_TABLE(KeyboardWidget, wxPanel)
EVT_PAINT(KeyboardWidget::paintEvent)
wxEND_EVENT_TABLE()


int KeyboardWidget::calcKeyboardWidth()
{
  int numOctaves = octaveMax_ - octaveMin_ - 1;
  int keyboardWidth = keyWidth_ * (7 * numOctaves + 1) + 1;
  return keyboardWidth;
}


KeyboardWidget::KeyboardWidget(wxWindow* parent, int ch, int keyWidth, int keyHeight) :
  wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(513, keyHeight)),
  octaveMin_(defaultOctaveMin), octaveMax_(defaultOctaveMax), muted_(false), update_key_(false)
{
  if (keyWidth < 7) keyWidth_ = 6;
  else keyWidth_ = 8;
  keyHeight_ = keyHeight;

  wxWindow::SetSize(calcKeyboardWidth(), keyHeight_);
  // wxSize size = wxWindow::GetSize();
  // wxMessageOutputDebug().Printf(wxT("(%d, %d)"), size.GetWidth(), size.GetHeight());

  SetBackgroundColour(*wxWHITE);

  wxEvtHandler::Bind(wxEVT_NOTE_ON, &KeyboardWidget::OnNoteOn, this);
  wxEvtHandler::Bind(wxEVT_NOTE_OFF, &KeyboardWidget::OnNoteOff, this);
  wxEvtHandler::Bind(wxEVT_CHANGE_PITCH, &KeyboardWidget::OnChangePitch, this);

  wxGetApp().GetSoundManager()->AddListener(this, ch);
}



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


#include <wx/gdicmn.h>

void KeyboardWidget::CalcKeyRect(int key, wxRect *rect)
{
  int i = key / 12;
  int j = key % 12;
  int x, y, w, h;
  switch (j) {
  case 0:
    x = (i * 7 + 0) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;
  case 2:
    x = (i * 7 + 1) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;
  case 4:
    x = (i * 7 + 2) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;
  case 5:
    x = (i * 7 + 3) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;
  case 7:
    x = (i * 7 + 4) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;
  case 9:
    x = (i * 7 + 5) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;
  case 11:
    x = (i * 7 + 6) * keyWidth_ + 1;
    y = keyHeight_/2;
    w = 7;
    h = keyHeight_ / 2 - 1;
    break;

  case 1:
    x = (i * 7 + 0) * keyWidth_ + 6;
    y = 0;
    w = 6;
    h = keyHeight_ / 2;
    break;
  case 3:
    x = (i * 7 + 1) * keyWidth_ + 6;
    y = 0;
    w = 6;
    h = keyHeight_ / 2;
    break;
  case 6:
    x = (i * 7 + 3) * keyWidth_ + 6;
    y = 0;
    w = 6;
    h = keyHeight_ / 2;
    break;
  case 8:
    x = (i * 7 + 4) * keyWidth_ + 6;
    y = 0;
    w = 6;
    h = keyHeight_ / 2;
    break;
  case 10:
    x = (i * 7 + 5) * keyWidth_ + 6;
    y = 0;
    w = 6;
    h = keyHeight_ / 2;
    break;
  }

  rect->SetLeft(x);
  rect->SetTop(y);
  rect->SetWidth(w);
  rect->SetHeight(h);
}


void KeyboardWidget::PaintPressedKeys(const IntSet &keys, wxPaintDC* paint_dc)
{
  update_key_ = false;

//   if (pressedKeys_.empty()) return;

  wxDC* dc;
  if (paint_dc) {
    dc = paint_dc;
  } else {
    dc = new wxClientDC(this);
  }

  if (muted_) {
    dc->SetPen(wxPen(*wxBLUE, 1));
    dc->SetBrush(*wxBLUE_BRUSH);
  } else {
    dc->SetPen(wxPen(*wxRED, 1));
    dc->SetBrush(*wxRED_BRUSH);
  }

//  for (IntSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
  while (keys_to_press_.empty() == false) {
    const int key_index = keys_to_press_.back();
    wxRect rect;
    CalcKeyRect(key_index, &rect);
    dc->DrawRectangle(rect);
    keys_to_press_.pop_back();
    pressedKeys_.insert(key_index);
  }

  if (paint_dc == NULL) {
    delete dc;
  }
}


void KeyboardWidget::PaintReleasedKeys(wxPaintDC* paint_dc)
{
  wxDC* dc;
  if (paint_dc) {
    dc = paint_dc;
  } else {
    dc = new wxClientDC(this);
  }

  while (keys_to_release_.empty() == false) {
    const int key_index = keys_to_release_.back();
    wxRect rect;
    CalcKeyRect(key_index, &rect);
    if (rect.GetY() == 0) {
      dc->SetPen(wxPen(*wxBLACK, 1));
      dc->SetBrush(*wxBLACK_BRUSH);
    } else {
      dc->SetPen(wxPen(*wxWHITE, 1));
      dc->SetBrush(*wxWHITE_BRUSH);
    }
    dc->DrawRectangle(rect);
    keys_to_release_.pop_back();
    pressedKeys_.erase(key_index);
  }

  if (paint_dc == NULL) {
    delete dc;
  }
}


void KeyboardWidget::render(wxDC &dc)
{
  if (update_key_ == false) {

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
  }

  PaintPressedKeys(pressedKeys_);
}


bool KeyboardWidget::PressKey(int keyIndex)
{
  // wxMessageOutputDebug().Printf(wxT("PressKey"));
  int keyMax = (octaveMax_ - octaveMin_ - 1) * 12;
  if (keyMax < keyIndex) return false;
  // pressedKeys_.insert(keyIndex);
  keys_to_press_.push_back(keyIndex);
  // paintNow();
  wxRect rect;
  CalcKeyRect(keyIndex, &rect);
  update_key_ = true;
  wxWindow::RefreshRect(rect, false);
  //PaintPressedKeys(pressedKeys_);
  return true;
}


bool KeyboardWidget::PressKey(double pitch)
{
  // A4 = 440 Hz
  int index = round( ( log(pitch/440.0)/log(2.0) + 5.75 ) * 12.0 );
  if (index < 0 || 120 < index) return false;
  return PressKey(index);
}


void KeyboardWidget::ReleaseKey()
{
/*
  while (pressedKeys_.empty() == false) {
    IntSet::iterator itr = pressedKeys_.begin();
  }
  */
  // pressedKeys_.clear();
  for (IntSet::iterator itr = pressedKeys_.begin(); itr !=pressedKeys_.end(); ++itr) {
    keys_to_release_.push_back(*itr);
  }

  PaintReleasedKeys();
}


void KeyboardWidget::ReleaseKey(int keyIndex)
{
  pressedKeys_.erase(keyIndex);
  // paintNow();
  wxRect rect;//
  CalcKeyRect(keyIndex, &rect);
  update_key_ = true;
  wxWindow::Refresh();
}



void KeyboardWidget::OnNoteOn(wxCommandEvent &event) {
  const NoteInfo* note = (NoteInfo*)event.GetClientData();
  PressKey(note->pitch);
  delete note;
}


void KeyboardWidget::OnNoteOff(wxCommandEvent& event) {
  const NoteInfo* note = (NoteInfo*)event.GetClientData();
  ReleaseKey();
  delete note;
}


void KeyboardWidget::OnChangePitch(wxCommandEvent &event) {
  const NoteInfo* note = (NoteInfo*)event.GetClientData();
  ReleaseKey();
  PressKey(note->pitch);
  delete note;
}



////////////////////////////////////////////////////////////////////////
// Rate text
////////////////////////////////////////////////////////////////////////


RateText::RateText(wxWindow *parent, int ch)
  : wxStaticText(parent, wxID_ANY, wxT("000000"), wxDefaultPosition)
{
  // wxEvtHandler::Bind(wxEVT_PAINT, &RateText::OnPaint, this);

  this->SetBackgroundColour(wxColor(255, 255, 255));

  // wxEvtHandler::Bind(wxEVT_CHANGE_PITCH, &RateText::OnChangeRate, this);
  wxGetApp().GetSoundManager()->AddListener(this, ch);
}

/*
void RateText::OnPaint(wxPaintEvent &event) {
  wxPaintDC dc(this);

  wxString str_rate = wxString::Format(wxT("%d"), rate_);
  dc.DrawText(str_rate, 0, 0);
}
*/


void RateText::OnChangePitch(const NoteInfo &note) {
  rate_ = note.rate;
  const wxString str_rate = wxString::Format(wxT("%d"), rate_);
  // SetLabel(str_rate);
}



////////////////////////////////////////////////////////////////////////
// Channel Panel
////////////////////////////////////////////////////////////////////////


ChannelPanel::ChannelPanel(wxWindow *parent)
//  : wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL, "channel_panel") {
: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL) {

  wholeSizer = new wxStaticBoxSizer(wxVERTICAL, this, _("Channel Information"));

  for (int i = 0; i < 24; i++) {
    ChannelElement element;
    element.ich = i;
    element.channelSizer = new wxBoxSizer(wxHORIZONTAL);

    element.muteButton = new MuteButton(this, i);
    element.channelSizer->Add(element.muteButton, 0, wxEXPAND);

    element.volumeSizer = new wxBoxSizer(wxVERTICAL);

    element.keyboard = new KeyboardWidget(this, i, 8, 20);
    element.volumeSizer->Add(element.keyboard, 0, wxTOP | wxFIXED_MINSIZE, 1);

    element.volumeLeft = new VolumeBar(this, wxHORIZONTAL, i);
    element.volumeSizer->Add(element.volumeLeft);
    element.channelSizer->Add(element.volumeSizer, 0, wxTOP | wxBOTTOM | wxFIXED_MINSIZE, 1);

    element.textToneOffset = new wxStaticText(this, wxID_ANY, wxT("0"), wxDefaultPosition, wxSize(48, -1));
    element.channelSizer->Add(element.textToneOffset);

    element.textToneLoop = new wxStaticText(this, wxID_ANY, wxT("0"), wxDefaultPosition, wxSize(40, -1));
    element.channelSizer->Add(element.textToneLoop);

    element.rate_text = new RateText(this, i);
    element.channelSizer->Add(element.rate_text);

    wholeSizer->Add(element.channelSizer);
    elements.push_back(element);
  }

  SetSizer(wholeSizer);
  SetAutoLayout(true);
  wholeSizer->Fit(this);

  // FitInside();
  // SetScrollRate(5, 5);

  // Spu.AddListener(this);
}
