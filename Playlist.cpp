#include "Playlist.h"
#include "MainFrame.h"
#include "Sound.h"

#include <wx/filename.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>


PSFPlaylist::PSFPlaylist(wxWindow* parent, wxWindowID id)
    : wxTreeCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_TWIST_BUTTONS | wxTR_HIDE_ROOT)
{
    rootId = AddRoot(wxT("Root"));  // "Root" is dummy

	// register folder and file icons
	wxImageList *treeImage = new wxImageList(16,16);
	wxBitmap idx0 = wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER);
	wxBitmap idx1 = wxArtProvider::GetBitmap(wxART_NEW, wxART_OTHER);
	treeImage->Add(idx0);
	treeImage->Add(idx1);
	AssignImageList(treeImage);

//	fdt = new PSFFileDropTarget(this);
//	SetDropTarget(fdt);
    DragAcceptFiles(true);
}

PSFPlaylist::~PSFPlaylist()
{
}


PSFPlaylistItem* PSFPlaylist::GetSelectedItem() const
{
    PSFPlaylistItem *item = dynamic_cast<PSFPlaylistItem*>(GetItemData(GetSelection()));
    return item;
}


bool PSFPlaylist::Append(const wxString &file_name)
{
    if (wxDirExists(file_name)) {
        wxPuts(wxT("WARNING: we cannot open a directory yet."));
        return false;
    }
    wxString path, name, ext;
    wxFileName::SplitPath(file_name, &path, &name, &ext);
    if (ext.IsEmpty()) return false;

    PSFPlaylistItem *item = wxNEW PSFPlaylistItem(file_name, name, ext);
    if (item->IsAvailable()) {
        AppendItem(item->GetFileName(), 1, 1, item);
        return true;
    }
    delete item;
    return false;
}



wxBEGIN_EVENT_TABLE(PSFPlaylist, wxTreeCtrl)
EVT_TREE_SEL_CHANGED(ID_PLAYLIST, PSFPlaylist::Selected)
EVT_TREE_ITEM_ACTIVATED(ID_PLAYLIST, PSFPlaylist::Activated)
EVT_DROP_FILES(PSFPlaylist::OnDropFiles)
wxEND_EVENT_TABLE()


void PSFPlaylist::Selected(wxTreeEvent &event)
{
    PSFPlaylistItem *item = dynamic_cast<PSFPlaylistItem*>(GetItemData(GetSelection()));
    wxASSERT_MSG(item != 0, "failed to dynamic-cast wxTreeItemData into PSFPlaylistItem");

    wxMessageOutputDebug().Printf("'%s' is seledted.", item->GetFileName());
}


void PSFPlaylist::Activated(wxTreeEvent& event)
{
    wxCommandEvent play_event(wxEVT_COMMAND_BUTTON_CLICKED, ID_PLAY);
    wxPostEvent(this, play_event);
}


void PSFPlaylist::OnDropFiles(wxDropFilesEvent &event)
{
    wxMessageOutputDebug().Printf("OnDropFiles");
}


PSFPlaylistItem::PSFPlaylistItem(const wxString& fullpath, const wxString& filename)
    : m_loader(0), m_sound(0)
{
    m_filename = filename;
    m_ext = filename.Mid(filename.Find(wxT('.'), true)+1);
    PreloadSound(fullpath, m_ext);
}

PSFPlaylistItem::PSFPlaylistItem(const wxString& fullpath, const wxString& name, const wxString& ext)
    : m_loader(0), m_sound(0)
{
    m_filename = name + wxT('.') + ext;
    m_ext = ext;
    PreloadSound(fullpath, ext);
}

PSFPlaylistItem::~PSFPlaylistItem()
{
    SoundFormat *sound = m_sound;
    if (sound) {
        if (sound->IsLoaded() == true) {
            delete sound;
        }
        delete m_loader;
    }
}


inline const wxString& PSFPlaylistItem::GetFullPath() const {
    return m_loader->GetPath();
}

inline const wxString& PSFPlaylistItem::GetFileName() const {
    return m_filename;
}

inline const wxString& PSFPlaylistItem::GetExt() const {
    return m_ext;
}


inline bool PSFPlaylistItem::IsAvailable() const {
    return (m_sound != 0);
}


SoundFormat* PSFPlaylistItem::LoadSound()
{
    SoundFormat *sound = m_loader->Load();
    m_sound = sound;
    return sound;
}



bool PSFPlaylistItem::PreloadSound(const wxString &path, const wxString &ext)
{
    SoundLoader *loader = m_loader;
    SoundFormat *sound = m_sound;

    if (loader == 0) {
        SoundLoaderFactory *factory = SoundLoaderFactory::GetInstance();
        loader = factory->GetLoader(ext);
        if (loader == 0) {
            return false;
        }
    }
    m_loader = loader;

    if (sound == 0) {
        sound = loader->Preload(path);
        if (sound == 0) {
            return false;
        }
    }
    m_sound = sound;

    return true;
}


/*
bool PSFFileDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) {
    wxMessageOutputDebug().Printf("OnDropFile");
    int i = 0;
	do {
		if (wxDirExists(filenames[i])) {
			wxPuts(wxT("WARNING: we cannot open a directory yet."));
		} else {
			// append an item to the list
			wxString path, name, ext;
            wxFileName::SplitPath(filenames[i], &path, &name, &ext);
			if (ext.IsEmpty()) continue;
            // if (ExtIsAvailable(ext) == false) continue;

            PSFPlaylistItem *item = wxNEW PSFPlaylistItem(filenames[i], name, ext);
            if (item->IsAvailable()) {
                playlist->Append(item->GetFileName(), 1, 1, item);
            } else {
                delete item;
            }
		}
	} while ((++i) < filenames.GetCount());
	return true;
}


wxDragResult PSFFileDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult defResult)
{
    wxMessageOutputDebug().Printf("OnEnter");
    return OnDragOver(x, y, defResult);
}
*/
