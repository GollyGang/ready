/*  Copyright 2011, The ReaDy Bunch

    This file is part of ReaDy.

    ReaDy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ReaDy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ReaDy. If not, see <http://www.gnu.org/licenses/>.         */

// wxWidgets:
#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif
#include <wx/aui/aui.h>
#include <wx/treectrl.h>

class wxVTKRenderWindowInteractor;

class MyFrame : public wxFrame
{
public:

    MyFrame(const wxString& title);
    ~MyFrame();

    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void OnToggleViewPane(wxCommandEvent& event);
    void OnUpdateViewPane(wxUpdateUIEvent& event);

private:

    // using wxAUI for window management
    wxAuiManager aui_mgr;

    // VTK does the rendering
    wxVTKRenderWindowInteractor *pVTKWindow;

private:

    void InitializeVTKPipeline();
    void LoadSettings();
    void SaveSettings();

    wxTreeCtrl* CreatePatternsCtrl();

    DECLARE_EVENT_TABLE()
};

