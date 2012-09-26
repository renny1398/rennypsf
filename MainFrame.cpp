#include "MainFrame.h"
#include "app.h"

#include <wx/filefn.h>


#include "Playlist.h"

#include "SoundFormat.h"


///////////////////////////////////////////////////////////////////////
// EVENT TABLE
///////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)

EVT_MENU(ID_OPEN, MainFrame::OnOpen)
EVT_MENU(wxID_EXIT, MainFrame::OnExit)
EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)

EVT_BUTTON(ID_PLAY, MainFrame::OnPlay)
EVT_BUTTON(ID_STOP, MainFrame::OnStop)

wxEND_EVENT_TABLE()



MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size)
{
    wxMenu *menuFile = wxNEW wxMenu;
	menuFile->Append(ID_OPEN, _("&Open"), _("Open a PSF file."));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = wxNEW wxMenu;
	menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = wxNEW wxMenuBar;
	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(menuHelp, _("&Help"));

	SetMenuBar(menuBar);

	CreateStatusBar();

	// create sizer
    wxBoxSizer *frame_sizer = wxNEW wxBoxSizer(wxVERTICAL);
    wxBoxSizer *playinfo_sizer = wxNEW wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *playcontrol_sizer = wxNEW wxBoxSizer(wxVERTICAL);
    wxBoxSizer *playbuttons_sizer = wxNEW wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *fileinfo_sizer = wxNEW wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("File information")), wxVERTICAL);

	// play controller
    wxButton *play_button = wxNEW wxButton(this, ID_PLAY, _("Play"), wxDefaultPosition);
    wxButton *pause_button = wxNEW wxButton(this, ID_PAUSE, _("Pause"), wxDefaultPosition);
    wxButton *stop_button = wxNEW wxButton(this, ID_STOP, _("Stop"), wxDefaultPosition);

	playbuttons_sizer->Add(play_button, 1, wxEXPAND);
	playbuttons_sizer->Add(pause_button, 1, wxEXPAND);
	playbuttons_sizer->Add(stop_button, 1, wxEXPAND);

    playcontrol_sizer->Add(wxNEW wxSlider(this, ID_SEEKBAR, 0, 0, 1023), 1, wxEXPAND);
	playcontrol_sizer->Add(playbuttons_sizer, 1, wxEXPAND);

	//*title,*artist,*game,*year,*genre,*psfby,*comment,*copyright
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Title:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Artist:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Game:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Year:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Genre:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("PSF by:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Comment:")), 1, wxEXPAND);
    fileinfo_sizer->Add(wxNEW wxStaticText(this, wxID_ANY, _("Copyright:")), 1, wxEXPAND);

	playinfo_sizer->Add(playcontrol_sizer, 2, wxEXPAND);
	playinfo_sizer->Add(fileinfo_sizer, 1, wxEXPAND);
	frame_sizer->Add(playinfo_sizer, 1, wxEXPAND);

	// channels

    // playlist
    file_treectrl = wxNEW PSFPlaylist(this, ID_PLAYLIST);
	frame_sizer->Add(file_treectrl, 1, wxEXPAND);

	// set sizer
	SetSizer(frame_sizer);
	SetAutoLayout(true);
	frame_sizer->Fit(this);
}



void MainFrame::OnOpen(wxCommandEvent &event)
{
}

void MainFrame::OnExit(wxCommandEvent &event)
{
	Close(true);
}

void MainFrame::OnAbout(wxCommandEvent &event)
{
	wxMessageBox(wxT("rennypsf v1.0"), wxT("About rennypsf"), wxOK | wxICON_INFORMATION);
}



void MainFrame::OnPlay(wxCommandEvent &event)
{
    PSFPlaylistItem *item = file_treectrl->GetSelectedItem();
    if (item == 0) return;

    SoundFormat *sound = item->LoadSound();
    if (sound == 0) return;

    wxGetApp().Play(sound);
}

void MainFrame::OnStop(wxCommandEvent &event)
{
    wxGetApp().Stop();
}