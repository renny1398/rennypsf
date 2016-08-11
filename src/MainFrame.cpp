#include "MainFrame.h"
#include "app.h"

#include <wx/filefn.h>
#include <wx/filedlg.h>

#include "Playlist.h"

#include "common/SoundFormat.h"

//#include "spu/spu.h"


///////////////////////////////////////////////////////////////////////
// EVENT TABLE
///////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)

EVT_MENU(ID_OPEN, MainFrame::OnOpen)
EVT_MENU(wxID_EXIT, MainFrame::OnExit)
EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)

EVT_BUTTON(ID_PLAY, MainFrame::OnPlay)
EVT_BUTTON(ID_STOP, MainFrame::OnStop)
#ifdef __WXDEBUG__
EVT_BUTTON(ID_DEBUG, MainFrame::OnDebug)
#endif
wxEND_EVENT_TABLE()



MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size)
{
    wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_OPEN, _("&Open"), _("Open a PSF file."));
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append(menuFile, _("&File"));
	menuBar->Append(menuHelp, _("&Help"));

	SetMenuBar(menuBar);

	CreateStatusBar();

	// create sizer
    wxBoxSizer *frame_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *playinfo_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *playcontrol_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *playbuttons_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *fileinfo_sizer = new wxStaticBoxSizer(new wxStaticBox(this, wxID_ANY, _("File information")), wxVERTICAL);

	// play controller
    wxButton *play_button = new wxButton(this, ID_PLAY, _("Play"), wxDefaultPosition);
    wxButton *pause_button = new wxButton(this, ID_PAUSE, _("Pause"), wxDefaultPosition);
    wxButton *stop_button = new wxButton(this, ID_STOP, _("Stop"), wxDefaultPosition);
#ifdef __WXDEBUG__
    wxButton* debugButton = new wxButton(this, ID_DEBUG, _("Debug"), wxDefaultPosition);
#endif

	playbuttons_sizer->Add(play_button, 1, wxEXPAND);
	playbuttons_sizer->Add(pause_button, 1, wxEXPAND);
	playbuttons_sizer->Add(stop_button, 1, wxEXPAND);
#ifdef __WXDEBUG__
    playbuttons_sizer->Add(debugButton, 1, wxEXPAND);
#endif

    playcontrol_sizer->Add(new wxSlider(this, ID_SEEKBAR, 0, 0, 1023), 1, wxEXPAND);
	playcontrol_sizer->Add(playbuttons_sizer, 1, wxEXPAND);

	//*title,*artist,*game,*year,*genre,*psfby,*comment,*copyright
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Title:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Artist:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Game:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Year:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Genre:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("PSF by:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Comment:")), 1, wxEXPAND);
    fileinfo_sizer->Add(new wxStaticText(this, wxID_ANY, _("Copyright:")), 1, wxEXPAND);

	playinfo_sizer->Add(playcontrol_sizer, 2, wxEXPAND);
	playinfo_sizer->Add(fileinfo_sizer, 1, wxEXPAND);
	frame_sizer->Add(playinfo_sizer, 1, wxEXPAND);

	// channels

    // playlist
    file_treectrl = new PSFPlaylist(this, ID_PLAYLIST);
	frame_sizer->Add(file_treectrl, 1, wxEXPAND);

	// set sizer
	SetSizer(frame_sizer);
	SetAutoLayout(true);
	frame_sizer->Fit(this);

    // test
    // DragAcceptFiles(true);
}



void MainFrame::OnOpen(wxCommandEvent& WXUNUSED(event))
{
    const wxString psf_wildcard = "PSF files (*.psf;*.minipsf)|*.psf;*.minipsf";
    const wxString psf2_wildcard = "PSF2 files (*.psf2;*.minipsf2)|*.psf2;*.minipsf2";

    wxFileDialog file_dialog(this, _("Open PSF file"), "", "", psf_wildcard + '|' + psf2_wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (file_dialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    wxString path = file_dialog.GetPath();
    file_treectrl->Append(path);
}

void MainFrame::OnExit(wxCommandEvent& WXUNUSED(event))
{
	// Spu.Shutdown();
	Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxT("rennypsf v1.0"), wxT("About rennypsf"), wxOK | wxICON_INFORMATION);
}



void MainFrame::OnPlay(wxCommandEvent& WXUNUSED(event))
{
    PSFPlaylistItem *item = file_treectrl->GetSelectedItem();
    if (item == 0) return;

    SoundData *sound = item->GetSound();
    rennyAssert(sound != NULL);
    //if (sound->Load() == false) return;

    wxGetApp().Play(sound);
}

void MainFrame::OnStop(wxCommandEvent& WXUNUSED(event))
{
    wxGetApp().Stop();
}


////////////////////////////////////////////////////////////////
// for Debug mode
////////////////////////////////////////////////////////////////

#ifdef __WXDEBUG__
#include "debugframe.h"

void MainFrame::OnDebug(wxCommandEvent& WXUNUSED(event))
{
    // DebugFrame
    DebugFrame* debugFrame = new DebugFrame(this, 0);
    debugFrame->Show(true);
}

#endif
