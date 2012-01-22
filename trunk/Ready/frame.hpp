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
#include <wx/aui/aui.h>

// local:
class RulePanel;
class PatternsPanel;

// STL:
#include <vector>

class wxVTKRenderWindowInteractor;
class BaseRD;

class MyFrame : public wxFrame
{
    public:

        MyFrame(const wxString& title);
        ~MyFrame();

        // interface with PatternsPanel
        void OpenFile(const wxString& path, bool remember = true);
        void EditFile(const wxString& path);
        bool UserWantsToCancelWhenAskedIfWantsToSave();

        // interface with RulePanel
        void SetRuleName(std::string s);
        void SetRuleDescription(std::string s);
        void SetPatternDescription(std::string s);
        void SetParameter(int iParam,float val);
        void SetParameterName(int iParam,std::string s);
        void SetFormula(std::string s);

    private:

        // file menu
        void OnNewPattern(wxCommandEvent& event);
        void OnOpenPattern(wxCommandEvent& event);
        void OnSavePattern(wxCommandEvent& event);
        void OnScreenshot(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);

        // edit menu
        void OnCut(wxCommandEvent& event);
        void OnCopy(wxCommandEvent& event);
        void OnPaste(wxCommandEvent& event);
        void OnUpdatePaste(wxUpdateUIEvent& event);
        void OnClear(wxCommandEvent& event);
        void OnSelectAll(wxCommandEvent& event);

        // view menu
        void OnToggleViewPane(wxCommandEvent& event);
        void OnUpdateViewPane(wxUpdateUIEvent& event);
        void OnRestoreDefaultPerspective(wxCommandEvent& event);
        void OnChangeActiveChemical(wxCommandEvent& event);

        // action menu
        void OnStep(wxCommandEvent& event);
        void OnUpdateStep(wxUpdateUIEvent& event);
        void OnRunStop(wxCommandEvent& event);
        void OnUpdateRunStop(wxUpdateUIEvent& event);
        void OnReset(wxCommandEvent& event);
        void OnUpdateReset(wxUpdateUIEvent& event);
        void OnInitWithBlobInCenter(wxCommandEvent& event);
        void OnSelectOpenCLDevice(wxCommandEvent& event);
        void OnOpenCLDiagnostics(wxCommandEvent& event);

        // help menu
        void OnAbout(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);

        // other event handlers
        void OnIdle(wxIdleEvent& event);
        void OnSize(wxSizeEvent& event);
        void OnClose(wxCloseEvent& event);

        // internal functions

        void InitializeMenus();
        void InitializePatternsPane();
        void InitializeRulePane();
        void UpdateRulePane();
        void InitializeHelpPane();
        void InitializeRenderPane();
        void LoadSettings();
        void SaveSettings();
        void LoadDemo(int iDemo);
       
        void SetCurrentRDSystem(BaseRD* system);
        void UpdateWindows();
        void UpdateWindowTitle();
        void SetStatusBarText();

        void SaveFile(const wxString& path);

    private:

        // using wxAUI for window management
        wxAuiManager aui_mgr;

        // VTK does the rendering
        wxVTKRenderWindowInteractor *pVTKWindow;

        // current system being simulated (in future we might want more than one)
        BaseRD *system;

        // rule pane things:
        RulePanel *rule_panel;

        // patterns pane things:
        PatternsPanel *patterns_panel;

        // settings:

        bool is_running;
        int timesteps_per_render;
        double frames_per_second,million_cell_generations_per_second;

        wxString default_perspective;
        wxString last_used_screenshot_folder;
        int iActiveChemical;

        int iOpenCLPlatform,iOpenCLDevice;

        DECLARE_EVENT_TABLE()
};
