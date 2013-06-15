#pragma once
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
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
    MuteButton(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name);

private:
    void onEnter(wxMouseEvent&);
    void onLeave(wxMouseEvent&);
    void onPaint(wxPaintEvent&);

    void paintNow();
    void render(wxDC&);

private:
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

class KeyboardWidget : public wxPanel
{
public:
    KeyboardWidget(wxWindow* parent, int keyWidth, int keyHeight);

public:
    bool PressKey(int keyIndex);
    bool PressKey(double pitch);
    void ReleaseKey();
    void ReleaseKey(int keyIndex);

protected:
    void paintEvent(wxPaintEvent&);
    void paintNow();
    void render(wxDC& dc);

protected:
    int calcKeyboardWidth();

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


class ChannelPanel: public wxPanel
{
public:
    ChannelPanel(wxWindow* parent);

    void ChangeChannelNumber(int n);

protected:
    void Update();

private:
    struct ChannelElement {
        int ich;
        wxBoxSizer* channelSizer;
        // wxStaticText* channelText;
        MuteButton* muteButton;
        wxBoxSizer* volumeSizer;
        VolumeBar *volumeLeft;
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
};





////////////////////////////////////////////////////////////////////////
// Wavetable List Control Panel
////////////////////////////////////////////////////////////////////////


struct WavetableInfo
{
    unsigned int offset;
    unsigned int length;
    unsigned int loop;
};


#include "spu/soundbank.h"

class WavetableList : public wxListCtrl, public SPU::SoundBankListener
{
public:
    WavetableList(wxWindow* parent);

protected:
    void onAdd(SPU::SoundBank*, SPU::SamplingTone *tone);
    void onModify(SPU::SoundBank*, SPU::SamplingTone *tone);
    void onRemove(SPU::SoundBank*, SPU::SamplingTone *tone);

    void onListRightClick(wxListEvent& event);
    void onPopupClick(wxCommandEvent& event);

    void ExportTone(SPU::SamplingTone* tone);

private:
    // map<Tone, wavetable>
    wxMenu menuPopup_;
    static const int ID_EXPORT_WAVE = 1001;
};
