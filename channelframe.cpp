#include "channelframe.h"
#include "channelpanel.h"
#include "wavetablepanel.h"
#include <wx/sizer.h>
#include "spu/spu.h"

ChannelFrame::ChannelFrame(wxWindow* parent): wxFrame(parent, wxID_ANY, _("Channel View"), wxDefaultPosition, wxSize(400, 720))
{
    ChannelPanel* panel = new ChannelPanel(this);
    wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

    SetBackgroundColour(*wxBLACK);
    ClearBackground();

    WavetablePanel *wavetablePanel = new WavetablePanel(this);

    sizer->Add(panel);
    sizer->Add(wavetablePanel, wxGROW);
    this->SetSizer(sizer);
    SetAutoLayout(true);
    sizer->Fit(this);
}
