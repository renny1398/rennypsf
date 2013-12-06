#pragma once

#include "stdwx.h"

class wxTreeCtrl;
class SoundFormat;
class PSFPlaylist;

class MainFrame: public wxFrame
{
public:
	MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);

private:
	// Menu Event
	void OnOpen(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);

	void DragAndDrop(wxCommandEvent& event);

    // Button Event
    void OnPlay(wxCommandEvent& event);
    void OnStop(wxCommandEvent& event);

    // for Debug mode
#ifdef __WXDEBUG__
    void OnDebug(wxCommandEvent& event);
#endif

    PSFPlaylist *file_treectrl;

	wxDECLARE_EVENT_TABLE();
};


enum {
	ID_OPEN = 101,

    ID_SHOW_KEYBOARDS = 250,

	ID_PLAY = 301,
	ID_PAUSE,
	ID_STOP,
    ID_DEBUG,

	ID_SEEKBAR = 501,
	ID_PLAYLIST,

	//*title,*artist,*game,*year,*genre,*psfby,*comment,*copyright
	ID_INFO_TITLE = 901,
	ID_INFO_ARTIST,
	ID_INFO_GAME,
	ID_INFO_YEAR,
    ID_INFO_GENRE,
	ID_INFO_PSFBY,
	ID_INFO_COMMENT,
	ID_INFO_COPYRIGHT
};
