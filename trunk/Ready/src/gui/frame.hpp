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
class PatternsPanel;
class InfoPanel;
class HelpPanel;
class wxVTKRenderWindowInteractor;

// readybase
#include "Properties.hpp"
class AbstractRD;

/// The wxFrame-derived top-level window for the Ready GUI.
class MyFrame : public wxFrame
{
    public:

        MyFrame(const wxString& title);
        ~MyFrame();

        // interface with PatternsPanel
        void OpenFile(const wxString& path, bool remember = true);
        void EditFile(const wxString& path);
        bool UserWantsToCancelWhenAskedIfWantsToSave();

        // interface with InfoPanel
        AbstractRD* GetCurrentRDSystem() { return system; }
        void SetRuleName(std::string s);
        void SetDescription(std::string s);
        void SetParameter(int iParam,float val);
        void SetParameterName(int iParam,std::string s);
        void SetFormula(std::string s);
        void SetNumberOfChemicals(int n);
        bool SetDimensions(int x,int y,int z);
        void SetBlockSize(int x,int y,int z);
        Properties& GetRenderSettings() { return this->render_settings; }
        void RenderSettingsChanged();

        // interface with Preferences dialog
        void ShowPrefsDialog(const wxString& page = wxEmptyString);
        void UpdateMenuAccelerators();

        // handle keyboard shortcuts not appearing in menu items
        void OnKeyDown(wxKeyEvent& event);
        void OnChar(wxKeyEvent& event);
        void ProcessKey(int key, int modifiers);

        bool IsFullScreen() { return this->fullscreen; }

    private:

        // File menu
        void OnNewPattern(wxCommandEvent& event);
        void OnOpenPattern(wxCommandEvent& event);
        void OnReloadFromDisk(wxCommandEvent& event);
        void OnSavePattern(wxCommandEvent& event);
        void OnScreenshot(wxCommandEvent& event);
        void OnStartRecording(wxCommandEvent& event);
        void OnUpdateStartRecording(wxUpdateUIEvent& event);
        void OnStopRecording(wxCommandEvent& event);
        void OnUpdateStopRecording(wxUpdateUIEvent& event);
        void OnAddMyPatterns(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnImportMesh(wxCommandEvent& event);
        void OnExportMesh(wxCommandEvent& event);
        void OnExportImage(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);

        // Open Recent submenu
        void OnOpenRecent(wxCommandEvent& event);
        void AddRecentPattern(const wxString& path);
        void OpenRecentPattern(int id);
        void ClearMissingPatterns();
        void ClearAllPatterns();

        // Edit menu
        void OnCut(wxCommandEvent& event);
        void OnCopy(wxCommandEvent& event);
        void OnPaste(wxCommandEvent& event);
        void OnUpdatePaste(wxUpdateUIEvent& event);
        void OnClear(wxCommandEvent& event);
        void OnSelectAll(wxCommandEvent& event);
        void OnSelectPointerTool(wxCommandEvent& event);
        void OnUpdateSelectPointerTool(wxUpdateUIEvent& event);
        void OnSelectPencilTool(wxCommandEvent& event);
        void OnUpdateSelectPencilTool(wxUpdateUIEvent& event);
        void OnSelectBrushTool(wxCommandEvent& event);
        void OnUpdateSelectBrushTool(wxUpdateUIEvent& event);
        void OnSelectPickerTool(wxCommandEvent& event);
        void OnUpdateSelectPickerTool(wxUpdateUIEvent& event);

        // View menu
        void OnFullScreen(wxCommandEvent& event);
        void OnFitPattern(wxCommandEvent& event);
        void OnWireframe(wxCommandEvent& event);
        void OnUpdateWireframe(wxUpdateUIEvent& event);
        void OnToggleViewPane(wxCommandEvent& event);
        void OnUpdateViewPane(wxUpdateUIEvent& event);
        void OnRestoreDefaultPerspective(wxCommandEvent& event);
        void OnChangeActiveChemical(wxCommandEvent& event);

        // Action menu
        void OnStep(wxCommandEvent& event);
        void OnUpdateStep(wxUpdateUIEvent& event);
        void OnRunStop(wxCommandEvent& event);
        void OnUpdateRunStop(wxUpdateUIEvent& event);
        void OnRunFaster(wxCommandEvent& event);
        void OnRunSlower(wxCommandEvent& event);
        void OnChangeRunningSpeed(wxCommandEvent& event);
        void OnReset(wxCommandEvent& event);
        void OnUpdateReset(wxUpdateUIEvent& event);
        void OnGenerateInitialPattern(wxCommandEvent& event);
        void OnBlank(wxCommandEvent& event);
        void OnAddParameter(wxCommandEvent& event);
        void OnUpdateAddParameter(wxUpdateUIEvent& event);
        void OnDeleteParameter(wxCommandEvent& event);
        void OnUpdateDeleteParameter(wxUpdateUIEvent& event);
        void OnViewFullKernel(wxCommandEvent& event);
        void OnUpdateViewFullKernel(wxUpdateUIEvent& event);
        void OnSelectOpenCLDevice(wxCommandEvent& event);
        void OnOpenCLDiagnostics(wxCommandEvent& event);

        // Help menu
        void OnAbout(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);

        // other event handlers
        void OnActivate(wxActivateEvent& event);
        void OnIdle(wxIdleEvent& event);
        void OnSize(wxSizeEvent& event);
        void OnClose(wxCloseEvent& event);

        // internal functions

        void InitializeMenus();
        void InitializeToolbars();
        void InitializePatternsPane();
        void InitializeInfoPane();
        void UpdateInfoPane();
        void InitializeHelpPane();
        void InitializeRenderPane();
        void InitializeDefaultRenderSettings();
        void LoadSettings();
        void SaveSettings();
        void CheckFocus();
        void EnableAllMenus(bool enable);
       
        void SetCurrentRDSystem(AbstractRD* system);
        void UpdateWindows();
        void UpdateWindowTitle();
        void UpdateToolbars();
        void SetStatusBarText();
        void RecordFrame();

        wxString SavePatternDialog();   // return empty path if user cancels
        void SaveFile(const wxString& path);

    private:

        // using wxAUI for window management
        wxAuiManager aui_mgr;
        wxString default_perspective;

        // VTK does the rendering
        wxVTKRenderWindowInteractor *pVTKWindow;

        // current system being simulated (in future we might want more than one)
        AbstractRD *system;

        // panes:
        PatternsPanel *patterns_panel;
        InfoPanel *info_panel;
        HelpPanel *help_panel;
        // toolbars:
        wxAuiToolBar *file_toolbar,*action_toolbar,*paint_toolbar;

        // settings:
        Properties render_settings;

        // following are used when running a simulation:
        bool is_running;
        double frames_per_second, million_cell_generations_per_second;
        int num_steps;
        int steps_since_last_render;
        double accumulated_time;
        bool do_one_render;

        // used when recording frames to disk
        bool is_recording,record_data_image;
        std::string recording_prefix,recording_extension;
        int iRecordingFrame;

        static const int MAX_TIMESTEPS_PER_RENDER = 1e8;

        int realkey;  // used to pass info from OnKeyDown to OnChar

        bool fullscreen;    // in full screen mode?
        bool is_opencl_available;
        
        static const char *opencl_not_available_message;

        enum TCursorType { POINTER, PENCIL, BRUSH, PICKER } CurrentCursor;

        DECLARE_EVENT_TABLE()
};
