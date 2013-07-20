#pragma once
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/vector.h>
#include <wx/hashset.h>
#include <wx/hashmap.h>

class wxBoxSizer;
class wxStaticText;
// class wxGauge;


class MuteButton : public wxPanel
{
public:
  MuteButton(wxWindow* parent, int channel_number);

private:
  void onEnter(wxMouseEvent&);
  void onLeave(wxMouseEvent&);
  void onPaint(wxPaintEvent&);
  void onClick(wxMouseEvent&);

  void paintNow();
  void render(wxDC&);

private:
  int channel_number_;
  bool muted_;
  bool entered_;
};


#include "SoundManager.h"

class VolumeBar : public wxPanel, public SoundDriverListener
{
public:
  VolumeBar(wxWindow* parent, wxOrientation orientation, int ch);

  float GetValue() const;
  void SetValue(float value);

protected:
  void paintEvent(wxPaintEvent&);
  void OnChangeVelocity(const NoteInfo& note);

private:
  wxOrientation orientation_;
  int ch_;
  float value_;


  wxDECLARE_EVENT_TABLE();
};


WX_DECLARE_HASH_SET(int, wxIntegerHash, wxIntegerEqual, IntSet);


#include "spu/spu.h"


wxDECLARE_EVENT(wxEVENT_SPU_CHANNEL_NOTE_ON, wxCommandEvent);
wxDECLARE_EVENT(wxEVENT_SPU_CHANNEL_NOTE_OFF, wxCommandEvent);


class wxRect;
class wxPaintDC;

class KeyboardWidget : public wxPanel, public SoundDriverListener
{
public:
  KeyboardWidget(wxWindow* parent, int ch, int keyWidth, int keyHeight);

public:
  bool PressKey(int keyIndex);
  bool PressKey(double pitch);
  void ReleaseKey();
  void ReleaseKey(int keyIndex);

protected:
  void paintEvent(wxPaintEvent&);
  void paintNow();
  void render(wxDC& dc);

  void CalcKeyRect(int key, wxRect* rect);
  void PaintPressedKeys(const IntSet& keys, wxPaintDC *paint_dc = NULL);
  void PaintReleasedKeys(wxPaintDC *paint_dc = NULL);

protected:
  int calcKeyboardWidth();

  void OnNoteOn(wxCommandEvent& event);
  void OnNoteOff(wxCommandEvent& event);
  void OnChangePitch(wxCommandEvent& event);

private:
  int keyWidth_, keyHeight_;
  int octaveMin_, octaveMax_;

  IntSet pressedKeys_;
  bool muted_;
  wxVector<int> keys_to_press_;
  wxVector<int> keys_to_release_;
  bool update_key_;

  static const int defaultOctaveMin = -1;
  static const int defaultOctaveMax = 9;

  wxDECLARE_EVENT_TABLE();
};



inline float VolumeBar::GetValue() const {
  return value_;
}

inline void VolumeBar::SetValue(float value) {
  value_ = value;
}


#include <wx/stattext.h>

class RateText : public wxStaticText, public SoundDriverListener {

public:
  RateText(wxWindow* parent, int ch);

protected:
  // void OnPaint(wxPaintEvent &event);
  void OnChangePitch(const NoteInfo& note);

private:
  int rate_;
};




#include <wx/event.h>

class ChannelPanel: public wxPanel
{
public:
  ChannelPanel(wxWindow* parent);

  void ChangeChannelNumber(int n);

  // void onChangeLoopIndex(wxCommandEvent& event);

private:
  struct ChannelElement {
    int ich;
    wxBoxSizer* channelSizer;
    // wxStaticText* channelText;
    MuteButton* muteButton;
    KeyboardWidget* keyboard;
    wxBoxSizer* volumeSizer;
    VolumeBar *volumeLeft;
    wxStaticText* textToneOffset;
    wxStaticText* textToneLoop;
    wxStaticText* rate_text;
  };

  wxBoxSizer* wholeSizer;
  wxVector<ChannelElement> elements;
};
