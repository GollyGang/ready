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

    // current system being simulated (in future we might want more than one)
    BaseRD *system;

    bool is_running;

private:

    // menu event handlers

    void OnQuit(wxCommandEvent& event);

    void OnGrayScott2DDemo(wxCommandEvent& event);
    void OnGrayScott3DDemo(wxCommandEvent& event);

    void OnToggleViewPane(wxCommandEvent& event);
    void OnUpdateViewPane(wxUpdateUIEvent& event);
    void OnScreenshot(wxCommandEvent& event);

    void OnOpenCLDiagnostics(wxCommandEvent& event);

    void OnStep(wxCommandEvent& event);
    void OnUpdateStep(wxUpdateUIEvent& event);
    void OnRun(wxCommandEvent& event);
    void OnUpdateRun(wxUpdateUIEvent& event);
    void OnStop(wxCommandEvent& event);
    void OnUpdateStop(wxUpdateUIEvent& event);

    void OnAbout(wxCommandEvent& event);

    // other event handlers

    void OnIdle(wxIdleEvent& event);
    void OnSize(wxSizeEvent& event);

    // internal functions

    void LoadSettings();
    void SaveSettings();
   
    void SetCurrentRDSystem(BaseRD* system);
    void SetStatusBarText();

    wxTreeCtrl* CreatePatternsCtrl();

    DECLARE_EVENT_TABLE()
};

