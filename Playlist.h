#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/treectrl.h>
#include <wx/dnd.h>


class MainFrame;
class PSFFileDropTarget;
class SoundLoader;
class SoundFormat;
class PSFPlaylistItem;


class PSFPlaylist: public wxTreeCtrl
{
public:
	PSFPlaylist(wxWindow* parent, wxWindowID id);
    ~PSFPlaylist();

    wxTreeItemId Append(const wxString& text, int image = -1, int selImage = -1, PSFPlaylistItem* data = 0);
    PSFPlaylistItem* GetSelectedItem() const;

protected:
    void Selected(wxTreeEvent &event);
    void Activated(wxTreeEvent &event);

	wxDECLARE_EVENT_TABLE();

private:
	wxTreeItemId rootId;
	PSFFileDropTarget *fdt;

};


class PSFPlaylistItem: public wxTreeItemData
{
public:
    PSFPlaylistItem(const wxString& fullpath, const wxString& filename);
    PSFPlaylistItem(const wxString& fullpath, const wxString& name, const wxString& ext);
    ~PSFPlaylistItem();

	const wxString& GetFullPath() const;
	const wxString& GetFileName() const;
	const wxString& GetExt() const;

    bool IsAvailable() const;

    SoundFormat* LoadSound();

    friend class PSFPlaylist;

protected:
    bool PreloadSound(const wxString& path, const wxString& m_ext);

private:
    SoundLoader *m_loader;
    // wxString fullpath;
    wxString m_filename;
    wxString m_ext;
    SoundFormat *m_sound;
};


class PSFFileDropTarget: public wxFileDropTarget
{
	//MainFrame *frame;
	PSFPlaylist *playlist;
public:
	PSFFileDropTarget(PSFPlaylist *playlist): playlist(playlist) {}
	bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
};



inline wxTreeItemId PSFPlaylist::Append(const wxString &text, int image, int selImage, PSFPlaylistItem *data)
{
    return AppendItem(rootId, text, image, selImage, data);
}
