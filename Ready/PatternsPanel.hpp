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

// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/dirctrl.h>    // for wxGenericDirCtrl
#include <wx/treectrl.h>   // for wxTreeCtrl, wxTreeEvent, wxTreeItemId

// local:
class MyFrame;

// our patterns panel lets users click on pattern files
class PatternsPanel : public wxPanel
{
    public:

        PatternsPanel(MyFrame* parent,wxWindowID id);

        void DoIdleChecks();    // called from MyFrame's OnIdle handler
        void BuildTree();       // build tree of pattern files
        bool TreeHasFocus();    // tree ctrl has keyboard focus?
        
        // return false if key event should be passed to default handler
        bool DoKey(int key, int mods);

    private:

        void AppendDir(const wxString& indir, wxTreeCtrl* treectrl, wxTreeItemId root);
        void DeselectTree(wxTreeCtrl* treectrl, wxTreeItemId root);

        void OnTreeSelChanged(wxTreeEvent& event);
        void OnTreeExpand(wxTreeEvent& event);
        void OnTreeCollapse(wxTreeEvent& event);
        void OnTreeClick(wxMouseEvent& event);

        void OnSetFocus(wxFocusEvent& event);

    private:

        MyFrame* frame;   // link to parent frame

        wxGenericDirCtrl* patternctrl;

        #ifdef __WXMSW__
            bool call_unselect;
            wxString editpath;
            bool ignore_selection;
        #endif
        bool edit_file;

        DECLARE_EVENT_TABLE()
};
