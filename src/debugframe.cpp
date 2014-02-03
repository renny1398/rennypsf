#include "debugframe.h"

#include <wx/listctrl.h>
#include <wx/sizer.h>

///////////////////////////////////////////////////////////////////////
// Event Table
///////////////////////////////////////////////////////////////////////
/*
wxBEGIN_EVENT_TABLE(DebugFrame, wxFrame)

//EVT_MENT(wxID_OPEN, MainFrame::OnOpen)

wxEND_EVENT_TABLE()
*/

DebugFrame::DebugFrame(wxFrame* parent, const PSF *psf)
    : wxFrame(parent, wxID_ANY, wxT("Rennypsf Debugger (line PSFLab)"), wxDefaultPosition, wxDefaultSize)
{
    wxListCtrl* disasmListCtrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);

    wxListItem listItem;
    listItem.SetId(0);
    listItem.SetText(_("Address"));
    listItem.SetWidth(50);
    disasmListCtrl->SetColumn(0, listItem);

    listItem.SetId(1);
    listItem.SetText(_("Instruction"));
    listItem.SetWidth(100);
    disasmListCtrl->SetColumn(1, listItem);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(disasmListCtrl);

    this->SetSizer(mainSizer);
    mainSizer->Fit(this);
}
