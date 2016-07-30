#pragma once
#include <wx/panel.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include "psf/spu/soundbank.h"
#include "common/SoundManager.h"


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

    void OnAdd(ToneInfo* tone);
    void OnModify(ToneInfo* tone);
    void OnRemove(ToneInfo* tone);

protected:
    long Find(int number);

    void OnChar(wxKeyEvent& event);

    void onListRightClick(wxListEvent& event);
    void onPopupClick(wxCommandEvent& event);

    void ExportTone(Instrument* tone);

private:
    enum COLUMN_INDEX {
      COLUMN_INDEX_MUTED,
      COLUMN_INDEX_OFFSET,
      COLUMN_INDEX_LENGTH,
      COLUMN_INDEX_LOOP,
      COLUMN_INDEX_FFT,
      COLUMN_INDEX_SIMILAR_TO
    };

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
