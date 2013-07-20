#include "channelframe.h"
#include "channelpanel.h"
#include "wavetablepanel.h"
#include <wx/sizer.h>
#include "spu/spu.h"

ChannelFrame::ChannelFrame(wxWindow* parent): wxFrame(parent, wxID_ANY, _("Channel View"), wxDefaultPosition, wxSize(400, 720))
{
  // SetBackgroundColour(*wxBLACK);
  // ClearBackground();

  ChannelPanel* panel = new ChannelPanel(this);
  wxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

  wxStaticBoxSizer *wavetableBoxSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, _("Wavetable"));
  WavetablePanel *wavetablePanel = new WavetablePanel(this);
  wavetableBoxSizer->Add(wavetablePanel, 0, wxEXPAND);

  sizer->Add(panel);
  sizer->Add(wavetableBoxSizer, 0, wxEXPAND);
  this->SetSizer(sizer);
  SetAutoLayout(true);
  sizer->Fit(this);
}
