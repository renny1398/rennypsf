#pragma once
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/timer.h>
#include <wx/vector.h>

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

    void render(wxDC& dc);

    int GetValue() const;
    void SetValue(int value);

private:
    wxOrientation orientation_;
    int value_;
    int max_;


    DECLARE_EVENT_TABLE()
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
        // wxGauge* volumeLeft;
        // wxGauge* volumeRight;
        VolumeBar *volumeLeft;
        VolumeBar *volumeRight;
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
