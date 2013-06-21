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



class VolumeBar : public wxPanel
{
public:
    VolumeBar(wxWindow* parent, wxOrientation orientation, int max);

    void paintEvent(wxPaintEvent&);
    void paintNow();

protected:
    void render(wxDC& dc);

public:
    int GetValue() const;
    void SetValue(int value);

private:
    wxOrientation orientation_;
    int value_;
    int max_;


    wxDECLARE_EVENT_TABLE();
};


WX_DECLARE_HASH_SET(int, wxIntegerHash, wxIntegerEqual, IntSet);


#include "spu/spu.h"


wxDECLARE_EVENT(wxEVENT_SPU_CHANNEL_NOTE_ON, wxCommandEvent);
wxDECLARE_EVENT(wxEVENT_SPU_CHANNEL_NOTE_OFF, wxCommandEvent);


class wxRect;


class KeyboardWidget : public wxPanel, public SPU::ChannelInfoListener
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
    void PaintPressedKeys(const IntSet& keys);
    void PaintReleasedKeys(const IntSet& keys);

protected:
    int calcKeyboardWidth();

    void OnNoteOn(wxCommandEvent& event);
    void OnNoteOff(wxCommandEvent& event);

    virtual void OnNoteOn(const SPU::ChannelInfo &ch);
    virtual void OnNoteOff(const SPU::ChannelInfo &ch);

private:
    int keyWidth_, keyHeight_;
    int octaveMin_, octaveMax_;

    IntSet pressedKeys_;
    bool muted_;

    static const int defaultOctaveMin = -1;
    static const int defaultOctaveMax = 9;

    wxDECLARE_EVENT_TABLE();
};



inline int VolumeBar::GetValue() const {
    return value_;
}

inline void VolumeBar::SetValue(int value) {
    value_ = value;
}


#include <wx/event.h>

class ChannelPanel: public wxPanel
{
public:
    ChannelPanel(wxWindow* parent);

    void ChangeChannelNumber(int n);

protected:
    void Update();

    void onChangeLoopIndex(wxCommandEvent& event);

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
    };

    class DrawTimer: public wxTimer {
    public:
        DrawTimer(wxWindow* parent);
        void Notify();
    private:
        wxWindow* parent;
    };

    wxBoxSizer* wholeSizer;
    wxVector<ChannelElement> elements;
    DrawTimer timer;

    wxDECLARE_EVENT_TABLE();
};
