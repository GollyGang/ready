/*  Copyright 2011, The Ready Bunch

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
#include <wx/aui/aui.h>
#include <wx/treectrl.h>

class wxVTKRenderWindowInteractor;
class BaseRD;

class MyFrame : public wxFrame
{
public:

    MyFrame(const wxString& title);
    ~MyFrame();

private:

    // using wxAUI for window management
    wxAuiManager aui_mgr;

    // VTK does the rendering
    wxVTKRenderWindowInteractor *pVTKWindow;

    // the kernel pane allows the user to edit the code of the current RD system
    wxTextCtrl *kernel_pane;

    // current system being simulated (in future we might want more than one)
    BaseRD *system;

    bool is_running;
    int timesteps_per_render;
    double frames_per_second,million_cell_generations_per_second;

    wxString default_perspective;

private:

    // file menu

    void OnQuit(wxCommandEvent& event);

    // view menu

    void OnDemo(wxCommandEvent& event);
    void OnToggleViewPane(wxCommandEvent& event);
    void OnUpdateViewPane(wxUpdateUIEvent& event);
    void OnRestoreDefaultPerspective(wxCommandEvent& event);
    void OnScreenshot(wxCommandEvent& event);

    // actions menu

    void OnStep(wxCommandEvent& event);
    void OnUpdateStep(wxUpdateUIEvent& event);
    void OnRun(wxCommandEvent& event);
    void OnUpdateRun(wxUpdateUIEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnUpdateStop(wxUpdateUIEvent& event);
    void OnOpenCLDiagnostics(wxCommandEvent& event);

    // help menu

    void OnAbout(wxCommandEvent& event);

    // other event handlers

    void OnIdle(wxIdleEvent& event);
    void OnSize(wxSizeEvent& event);

    // internal functions

    void LoadSettings();
    void SaveSettings();
    void LoadDemo(int iDemo);
   
    void SetCurrentRDSystem(BaseRD* system);
    void SetStatusBarText();

    wxTreeCtrl* CreatePatternsCtrl();

    DECLARE_EVENT_TABLE()
};
