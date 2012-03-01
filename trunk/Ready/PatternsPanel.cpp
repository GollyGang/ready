/*  Copyright 2011, 2012 The Ready Bunch

    This file is part of Ready.

    Ready is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ready is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ready. If not, see <http://www.gnu.org/licenses/>.         */

// local:
#include "PatternsPanel.hpp"
#include "frame.hpp"
#include "IDs.hpp"
#include "prefs.hpp"        // for patterndir, userdir

// wxWidgets:
#include <wx/filename.h>    // for wxFileName

#if defined(__WXMAC__) && wxCHECK_VERSION(2,9,0)
    // ControlDown has been changed to mean Command key down
    #define ControlDown RawControlDown
#endif

BEGIN_EVENT_TABLE(PatternsPanel, wxPanel)
    EVT_TREE_SEL_CHANGED(wxID_TREECTRL, PatternsPanel::OnTreeSelChanged)
#ifdef __WXMSW__
    EVT_TREE_ITEM_EXPANDED(wxID_TREECTRL, PatternsPanel::OnTreeExpand)
    EVT_TREE_ITEM_COLLAPSED(wxID_TREECTRL, PatternsPanel::OnTreeCollapse)
#endif
END_EVENT_TABLE()

PatternsPanel::PatternsPanel(MyFrame* parent,wxWindowID id) 
    : wxPanel(parent,id), frame(parent)
{
    #ifdef __WXMSW__
        call_unselect = false;      // DoIdleChecks needs to call Unselect?
        editpath = wxEmptyString;   // DoIdleChecks calls EditFile if this isn't empty
        ignore_selection = false;   // ignore spurious selection?
    #endif
    edit_file = false;              // edit the clicked file?

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

    patternctrl = NULL;             // for 1st call of OnTreeSelChanged
    patternctrl = new wxGenericDirCtrl(this, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize,
                                       #ifdef __WXMSW__
                                           // speed up a bit
                                           wxDIRCTRL_DIR_ONLY | wxNO_BORDER,
                                       #else
                                           wxNO_BORDER,
                                       #endif
                                       wxEmptyString   // see all file types
                                      );

    sizer->Add(patternctrl,wxSizerFlags(1).Expand());
    this->SetSizer(sizer);

    #ifdef __WXMSW__
        // now remove wxDIRCTRL_DIR_ONLY so we'll see files
        patternctrl->SetWindowStyle(wxNO_BORDER);
    #endif

    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();

    #if defined(__WXGTK__)
        // make sure background is white when using KDE's GTK theme
        #if wxCHECK_VERSION(2,9,0)
            treectrl->SetBackgroundStyle(wxBG_STYLE_ERASE);
        #else
            treectrl->SetBackgroundStyle(wxBG_STYLE_COLOUR);
        #endif
        treectrl->SetBackgroundColour(*wxWHITE);
        // reduce indent a bit
        treectrl->SetIndent(8);
    #elif defined(__WXMAC__)
        // reduce font size
        wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        font.SetPointSize(12);
        treectrl->SetFont(font);
    #elif defined(__WXMSW__)
        // reduce indent a lot on Windows
        treectrl->SetIndent(4);
    #endif
    
    BuildTree();

    // install event handler to detect control/right-click on a file
    treectrl->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(PatternsPanel::OnTreeClick), NULL, this);
    treectrl->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(PatternsPanel::OnTreeClick), NULL, this);
    #ifdef __WXMSW__
        // fix double-click problem
        treectrl->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(PatternsPanel::OnTreeClick), NULL, this);
    #endif

    // install event handlers to detect keyboard shortcuts when treectrl has focus
    treectrl->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(MyFrame::OnKeyDown), NULL, frame);
    treectrl->Connect(wxEVT_CHAR, wxKeyEventHandler(MyFrame::OnChar), NULL, frame);

    #ifdef __WXMSW__
        treectrl->Connect(wxEVT_SET_FOCUS, wxFocusEventHandler(PatternsPanel::OnSetFocus),NULL,this);
    #endif
}

void PatternsPanel::DoIdleChecks()
{
    #ifdef __WXMSW__
        if (call_unselect) {
            // deselect file so user can click the same item again
            patternctrl->GetTreeCtrl()->Unselect();
            call_unselect = false;
        }
        if (!editpath.IsEmpty()) {
            frame->EditFile(editpath);
            editpath.Clear();
        }
    #endif
}

void PatternsPanel::AppendDir(const wxString& indir, wxTreeCtrl* treectrl, wxTreeItemId root)
{
    wxString dir = indir;
    if (dir.Last() == wxFILE_SEP_PATH) dir.Truncate(dir.Length()-1);
    wxDirItemData* diritem = new wxDirItemData(dir, dir, true);
    wxTreeItemId id = treectrl->AppendItem(root, dir.AfterLast(wxFILE_SEP_PATH), 0, 0, diritem);
    if ( diritem->HasFiles() || diritem->HasSubDirs() ) {
        treectrl->SetItemHasChildren(id);
        treectrl->Expand(id);
        
        // also expand any subfolders, but only one level
        if ( diritem->HasSubDirs() ) {
            wxTreeItemIdValue cookie;
            wxTreeItemId nextid = treectrl->GetFirstChild(id, cookie);
            while ( nextid.IsOk() ) {
                if ( treectrl->ItemHasChildren(nextid) ) {
                    treectrl->Expand(nextid);
                }
                nextid = treectrl->GetNextChild(id, cookie);
            }
        }
    }
}

void PatternsPanel::BuildTree()
{
    // delete old tree (except root)
    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
    wxTreeItemId root = patternctrl->GetRootId();
    treectrl->DeleteChildren(root);
   
    if ( wxFileName::DirExists(patterndir) ) {
        // append Ready's pattern folder as first child of root
        AppendDir(patterndir, treectrl, root);
    }

    if ( wxFileName::DirExists(userdir) ) {
        // append user's pattern folder as another child of root
        AppendDir(userdir, treectrl, root);
    }

    // select top folder so hitting left arrow can collapse it and won't cause an assert
    wxTreeItemIdValue cookie;
    wxTreeItemId id = treectrl->GetFirstChild(root, cookie);
    if (id.IsOk()) treectrl->SelectItem(id);
}

void PatternsPanel::DeselectTree(wxTreeCtrl* treectrl, wxTreeItemId root)
{
    // recursively traverse tree and reset each item background to white
    wxTreeItemIdValue cookie;
    wxTreeItemId id = treectrl->GetFirstChild(root, cookie);
    while ( id.IsOk() ) {
        wxColor currcolor = treectrl->GetItemBackgroundColour(id);
        if ( currcolor != *wxWHITE ) {
            // assume item is selected
            treectrl->SetItemBackgroundColour(id, *wxWHITE);
        }
        if ( treectrl->ItemHasChildren(id) ) {
            DeselectTree(treectrl, id);
        }
        id = treectrl->GetNextChild(root, cookie);
    }
}

void PatternsPanel::OnTreeExpand(wxTreeEvent& WXUNUSED(event))
{
    #ifdef __WXMSW__
        // avoid bug -- expanding/collapsing a folder can cause top visible item
        // to become selected and thus OnTreeSelChanged gets called
        ignore_selection = true;
    #endif
}

void PatternsPanel::OnTreeCollapse(wxTreeEvent& WXUNUSED(event))
{
    #ifdef __WXMSW__
        // avoid bug -- expanding/collapsing a folder can cause top visible item
        // to become selected and thus OnTreeSelChanged gets called
        ignore_selection = true;
    #endif
}

void PatternsPanel::OnTreeSelChanged(wxTreeEvent& event)
{
    if (patternctrl == NULL) return;   // ignore 1st call
    
    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) return;

    wxString filepath = patternctrl->GetFilePath();
    
    // deselect file/folder so this handler will be called if user clicks same item
    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
    #ifdef __WXMSW__
        // calling UnselectAll() or Unselect() here causes a crash
        if (ignore_selection) {
            // ignore spurious selection
            ignore_selection = false;
            call_unselect = true;
            return;
        }
    #else
        treectrl->UnselectAll();
    #endif

    if ( filepath.IsEmpty() ) {
        // user clicked on a folder name
        DeselectTree(treectrl, treectrl->GetRootItem());
        treectrl->SetItemBackgroundColour(id, *wxLIGHT_GREY);

    } else if (edit_file) {
        // open file in text editor
        #ifdef __WXMSW__
            // call EditFile in later DoIdleChecks to avoid right-click problem
            editpath = filepath;
        #else
            frame->EditFile(filepath);
        #endif
    
    } else {
        // user clicked on a pattern/html/txt file
        
        // reset background of previously selected file by traversing entire tree;
        // we can't just remember previously selected id because ids don't persist
        // after a folder has been collapsed and expanded
        DeselectTree(treectrl, treectrl->GetRootItem());

        // indicate the selected file
        treectrl->SetItemBackgroundColour(id, *wxLIGHT_GREY);

        frame->OpenFile(filepath);
    }

    #ifdef __WXMSW__
        // calling Unselect() here causes a crash so do later in DoIdleChecks
        call_unselect = true;
    #endif
}

void PatternsPanel::OnTreeClick(wxMouseEvent& event)
{
    // set global flag for testing in OnTreeSelChanged
    edit_file = event.ControlDown() || event.RightDown();
   
    #ifdef __WXMSW__
        // this handler gets called even if user clicks outside an item,
        // and in some cases can result in the top visible item becoming
        // selected, so we need to avoid that
        ignore_selection = false;
        if (patternctrl) {
            wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
            if (treectrl) {
                wxPoint pt = event.GetPosition();
                int flags;
                wxTreeItemId id = treectrl->HitTest(pt, flags);
                if (id.IsOk() && (flags & wxTREE_HITTEST_ONITEMLABEL ||
                                  flags & wxTREE_HITTEST_ONITEMICON)) {
                    // fix problem with right-click
                    if (event.RightDown()) {
                        treectrl->SelectItem(id, true);
                        // OnTreeSelChanged gets called a few times for some reason
                    }
                    // fix problem with double-click
                    if (event.LeftDClick()) ignore_selection = true;
                } else {
                    ignore_selection = true;
                }
            }
        }
    #endif
   
    event.Skip();
}

bool PatternsPanel::TreeHasFocus()
{
    wxTreeCtrl* treectrl = patternctrl->GetTreeCtrl();
    return treectrl && treectrl->HasFocus();
}

bool PatternsPanel::DoKey(int key, int mods)
{
    // first look for keys that should be passed to the default handler
    if ( mods == wxMOD_NONE ) {
        if ( key == WXK_UP || key == WXK_DOWN || key == WXK_LEFT || key == WXK_RIGHT ) {
            // let default handler see arrow keys (to select files or open/close folders)
            return false;
        }
    }
    
    // finally do other keyboard shortcuts
    frame->ProcessKey(key, mods);
    return true;
}

void PatternsPanel::OnSetFocus(wxFocusEvent& event)
{
    #ifdef __WXMSW__
        ignore_selection = true;
    #endif
}
