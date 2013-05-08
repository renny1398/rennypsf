#include "channelframe.h"
#include "channelpanel.h"
#include <wx/sizer.h>

ChannelFrame::ChannelFrame(wxWindow* parent): wxFrame(parent, wxID_ANY, _("Channel View"), wxDefaultPosition, wxSize(400, 720))
{
    ChannelPanel* panel = new ChannelPanel(this);
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    SetBackgroundColour(*wxBLACK);
    ClearBackground();

    panel->SetSizer(sizer);
    SetAutoLayout(true);
    //sizer->Fit(this);
}
