#pragma once
#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include "spu/soundbank.h"


////////////////////////////////////////////////////////////////////////
// Wavetable List Control Panel
////////////////////////////////////////////////////////////////////////


struct WavetableInfo
{
    unsigned int offset;
    unsigned int length;
    unsigned int loop;
};


class WavetableList : public wxListCtrl
{
public:
    WavetableList(wxWindow* parent);

    void OnAdd(SPU::SamplingTone* tone);
    void OnModify(SPU::SamplingTone* tone);
    void OnRemove(SPU::SamplingTone* tone);


protected:
    void onListRightClick(wxListEvent& event);
    void onPopupClick(wxCommandEvent& event);

    void ExportTone(SPU::SamplingTone* tone);

private:
    // map<Tone, wavetable>
    wxMenu menuPopup_;

    static const int ID_EXPORT_WAVE = 1001;
};



class WavetablePanel : public wxPanel
{
public:
    WavetablePanel(wxWindow* parent);

protected:
    void onAdd(wxCommandEvent& event);
    void onModify(wxCommandEvent& event);
    void onRemove(wxCommandEvent& event);

private:
    WavetableList* listCtrl_;
    wxSizer* mainSizer_;

    wxDECLARE_EVENT_TABLE();
};
