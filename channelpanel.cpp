#include "channelpanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include "SoundManager.h"
#include "app.h"

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
        element.channelText = new wxStaticText(this, -1, channelCaption, wxDefaultPosition, wxDefaultSize, 0);
        element.channelSizer->Add(element.channelText);
        element.volumeSizer = new wxBoxSizer(wxVERTICAL);
        element.volumeLeft = new wxGauge(this, -1, 1024, wxDefaultPosition, wxSize(256, 12), wxGA_HORIZONTAL | wxGA_SMOOTH);
        element.volumeRight = new wxGauge(this, -1, 1024, wxDefaultPosition, wxSize(256, 12), wxGA_HORIZONTAL | wxGA_SMOOTH);
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
    SoundManager& soundManager = wxGetApp().GetSoundManager();
    int i = 0;
    for (wxVector<ChannelElement>::iterator it = elements.begin(), it_end = elements.end(); it != it_end; ++it) {
        int currLeft = it->volumeLeft->GetValue();
        int currRight = it->volumeRight->GetValue();
        int nextLeft = soundManager.GetEnvelopeVolume(i);
        int nextRight = soundManager.GetEnvelopeVolume(i);
        i++;
        if (currLeft == nextLeft && currRight == nextRight) continue;
        it->volumeLeft->SetValue(nextLeft);
        it->volumeRight->SetValue(nextRight);
        if (i == 1) {
            wxMessageOutputDebug().Printf("Volume = 0x%08x", nextLeft);
        }
    }
}
