#pragma once
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/vector.h>

class wxBoxSizer;
class wxStaticText;
class wxGauge;

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
        wxStaticText* channelText;
        wxBoxSizer* volumeSizer;
        wxGauge* volumeLeft;
        wxGauge* volumeRight;
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
