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
#include "app.hpp"          // for wxGetApp
#include "frame.hpp"
#include "vtk_pipeline.hpp"
#include "utils.hpp"
#include "IO_XML.hpp"
#include "RulePanel.hpp"
#include "HelpPanel.hpp"
#include "PatternsPanel.hpp"
#include "IDs.hpp"

// readybase:
#include "GrayScott_slow.hpp"
#include "GrayScott_slow_3D.hpp"
#include "OpenCL_nDim.hpp"

// local resources:
#include "appicon16.xpm"

// wxWidgets:
#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <wx/filename.h>
#include <wx/font.h>
#include <wx/dir.h>
#include <wx/dnd.h>         // for wxFileDropTarget
#include <wx/clipbrd.h>     // for wxTheClipboard

// wxVTK: (local copy)
#include "wxVTKRenderWindowInteractor.h"

// STL:
#include <fstream>
#include <string>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkSmartPointer.h>
#include <vtkImageAppendComponents.h>

#ifdef __WXMAC__
    #include <Carbon/Carbon.h>    // for GetCurrentProcess, etc
#endif

wxString PaneName(int id)
{
    return wxString::Format(_T("%d"),id);
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_IDLE(MyFrame::OnIdle)
    EVT_SIZE(MyFrame::OnSize)
    EVT_CLOSE(MyFrame::OnClose)
    // file menu
    EVT_MENU(wxID_NEW, MyFrame::OnNewPattern)
    EVT_MENU(wxID_OPEN, MyFrame::OnOpenPattern)
    EVT_MENU(wxID_SAVE, MyFrame::OnSavePattern)
    EVT_MENU(wxID_EXIT, MyFrame::OnQuit)
    // edit menu
    EVT_MENU(wxID_CUT, MyFrame::OnCut)
    EVT_MENU(wxID_COPY, MyFrame::OnCopy)
    EVT_MENU(wxID_PASTE, MyFrame::OnPaste)
    EVT_UPDATE_UI(wxID_PASTE, MyFrame::OnUpdatePaste)
    EVT_MENU(wxID_CLEAR, MyFrame::OnClear)
    EVT_MENU(wxID_SELECTALL, MyFrame::OnSelectAll)
    // view menu
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::RulePane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::RulePane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::HelpPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::HelpPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::RestoreDefaultPerspective, MyFrame::OnRestoreDefaultPerspective)
    EVT_MENU(ID::Screenshot, MyFrame::OnScreenshot)
    EVT_MENU(ID::ChangeActiveChemical, MyFrame::OnChangeActiveChemical)
    // action menu
    EVT_MENU(ID::Step, MyFrame::OnStep)
    EVT_UPDATE_UI(ID::Step, MyFrame::OnUpdateStep)
    EVT_MENU(ID::RunStop, MyFrame::OnRunStop)
    EVT_UPDATE_UI(ID::RunStop, MyFrame::OnUpdateRunStop)
    EVT_MENU(ID::Reset, MyFrame::OnReset)
    EVT_UPDATE_UI(ID::Reset, MyFrame::OnUpdateReset)
    EVT_MENU(ID::InitWithBlobInCenter, MyFrame::OnInitWithBlobInCenter)
    EVT_MENU(ID::SelectOpenCLDevice, MyFrame::OnSelectOpenCLDevice)
    EVT_MENU(ID::OpenCLDiagnostics, MyFrame::OnOpenCLDiagnostics)
    // help menu
    EVT_MENU(wxID_HELP, MyFrame::OnHelp)
    EVT_MENU(ID::HelpQuick, MyFrame::OnHelp)
    EVT_MENU(ID::HelpTips, MyFrame::OnHelp)
    EVT_MENU(ID::HelpKeyboard, MyFrame::OnHelp)
    EVT_MENU(ID::HelpMouse, MyFrame::OnHelp)
    EVT_MENU(ID::HelpFile, MyFrame::OnHelp)
    EVT_MENU(ID::HelpEdit, MyFrame::OnHelp)
    EVT_MENU(ID::HelpView, MyFrame::OnHelp)
    EVT_MENU(ID::HelpAction, MyFrame::OnHelp)
    EVT_MENU(ID::HelpHelp, MyFrame::OnHelp)
    EVT_MENU(ID::HelpRefs, MyFrame::OnHelp)
    EVT_MENU(ID::HelpFormats, MyFrame::OnHelp)
    EVT_MENU(ID::HelpProblems, MyFrame::OnHelp)
    EVT_MENU(ID::HelpChanges, MyFrame::OnHelp)
    EVT_MENU(ID::HelpCredits, MyFrame::OnHelp)
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
END_EVENT_TABLE()

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title),
       pVTKWindow(NULL),system(NULL),
       is_running(false),
       timesteps_per_render(100),
       frames_per_second(0.0),
       million_cell_generations_per_second(0.0),
       iOpenCLPlatform(0),iOpenCLDevice(0),
       iActiveChemical(1)
{
    this->SetIcon(wxICON(appicon16));
    this->aui_mgr.SetManagedWindow(this);

    this->InitializeMenus();

    CreateStatusBar(1);
    SetStatusText(_("Ready"));

    this->InitializePatternsPane();
    this->InitializeRulePane();
    this->InitializeHelpPane();
    this->InitializeRenderPane();
    
    this->default_perspective = this->aui_mgr.SavePerspective();
    this->LoadSettings();
    this->aui_mgr.Update();

    // initialize an RD system to get us started
    this->OpenFile("Patterns/CPU-only/grayscott.vti");
}

void MyFrame::InitializeMenus()
{
    wxMenuBar *menuBar = new wxMenuBar();
    {   // file menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_NEW, _("New Pattern...\tCtrl-N"), _("Create a new pattern"));
        menu->AppendSeparator();
        menu->Append(wxID_OPEN, _("Open Pattern...\tCtrl-O"), _("Choose a pattern file to open"));
        menu->AppendSeparator();
        menu->Append(wxID_SAVE, _("Save Pattern...\tCtrl-S"), _("Save the current pattern"));
        menu->Append(ID::Screenshot, _("Save Screenshot...\tF3"), _("Save a screenshot of the current view"));
        menu->AppendSeparator();
        menu->Append(wxID_EXIT);
        menuBar->Append(menu, _("&File"));
    }
    {   // edit menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_CUT, _("Cut\tCtrl-X"), _("Cut the selection and save it to the clipboard"));
        menu->Append(wxID_COPY, _("Copy\tCtrl-C"), _("Copy the selection to the clipboard"));
        menu->Append(wxID_PASTE, _("Paste\tCtrl-V"), _("Paste in the contents of the clipboard"));
        menu->Append(wxID_CLEAR, _("Clear"), _("Clear the selection"));
        menu->AppendSeparator();
        menu->Append(wxID_SELECTALL, _("Select All\tCtrl-A"), _("Select everything"));
        menuBar->Append(menu, _("&Edit"));
    }
    {   // view menu:
        wxMenu *menu = new wxMenu;
        menu->AppendCheckItem(ID::PatternsPane, _("&Patterns Pane"), _("View the patterns pane"));
        menu->AppendCheckItem(ID::RulePane, _("&Rule Pane"), _("View the rule pane"));
        menu->AppendCheckItem(ID::HelpPane, _("&Help Pane\tF1"), _("View the help pane"));
        menu->AppendSeparator();
        menu->Append(ID::RestoreDefaultPerspective, _("&Restore Default Layout"), _("Put the windows back where they were"));
        menu->AppendSeparator();
        menu->Append(ID::ChangeActiveChemical, _("&Change Active Chemical..."), _("Change which chemical is being visualized"));
        menuBar->Append(menu, _("&View"));
    }
    {   // action menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::Step, _("&Step\tF4"), _("Advance the simulation by a single timestep"));
        menu->Append(ID::RunStop, _("&Run\tF5"), _("Start or stop running the simulation"));
        menu->AppendSeparator();
        menu->Append(ID::Reset, _("Reset\tCtrl-R"), _("Go back to the starting pattern"));
        menu->Append(ID::InitWithBlobInCenter, _("Random &Blob"), _("Re-start with a random blob in the middle"));
        menu->AppendSeparator();
        menu->Append(ID::SelectOpenCLDevice, _("Select OpenCL &Device..."), _("Choose which OpenCL device to run on"));
        menu->Append(ID::OpenCLDiagnostics, _("Show Open&CL Diagnostics..."), _("Show the available OpenCL devices and their attributes"));
        menuBar->Append(menu, _("&Action"));
    }
    {   // help menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_HELP,        _("Contents"));
        menu->Append(ID::HelpQuick,    _("Quick Start"));
        menu->Append(ID::HelpTips,     _("Hints and Tips"));
        menu->Append(ID::HelpKeyboard, _("Keyboard Shortcuts"));
        menu->Append(ID::HelpMouse,    _("Mouse Shortcuts"));
        menu->AppendSeparator();
        menu->Append(ID::HelpFile,     _("File Menu"));
        menu->Append(ID::HelpEdit,     _("Edit Menu"));
        menu->Append(ID::HelpView,     _("View Menu"));
        menu->Append(ID::HelpAction,   _("Action Menu"));
        menu->Append(ID::HelpHelp,     _("Help Menu"));
        menu->AppendSeparator();
        menu->Append(ID::HelpRefs,     _("References"));
        menu->Append(ID::HelpFormats,  _("File Formats"));
        menu->Append(ID::HelpProblems, _("Known Problems"));
        menu->Append(ID::HelpChanges,  _("Changes"));
        menu->Append(ID::HelpCredits,  _("Credits"));
        menu->AppendSeparator();
        menu->Append(wxID_ABOUT,       _("&About Ready"));
        menuBar->Append(menu, _("&Help"));
    }
    SetMenuBar(menuBar);
}

void MyFrame::InitializePatternsPane()
{
    this->patterns_panel = new PatternsPanel(this,wxID_ANY);
    this->aui_mgr.AddPane(this->patterns_panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::PatternsPane))
                  .Caption(_("Patterns Pane"))
                  .Left()
                  .BestSize(300,300)
                  .Layer(0) // layer 0 is the innermost ring around the central pane
                  );
}

void MyFrame::InitializeRulePane()
{
    this->rule_panel = new RulePanel(this,wxID_ANY);
    this->aui_mgr.AddPane(this->rule_panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::RulePane))
                  .Caption(_("Rule Pane"))
                  .Right()
                  .BestSize(400,400)
                  .Layer(1) // layer 1 is further towards the edge
                  .Hide()
                  );
}

void MyFrame::UpdateRulePane()
{
    this->rule_panel->Update(this->system);
}

void MyFrame::InitializeHelpPane()
{
    this->help_panel = new HelpPanel(this,wxID_ANY);
    this->aui_mgr.AddPane(this->help_panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::HelpPane))
                  .Caption(_("Help Pane"))
                  .Right()
                  .BestSize(400,400)
                  .Hide()
                  );
}

// -----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP

// derive a simple class for handling dropped files
class DnDFile : public wxFileDropTarget
{
public:
    DnDFile() {}
    virtual bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames);
};

bool DnDFile::OnDropFiles(wxCoord, wxCoord, const wxArrayString& filenames)
{
    MyFrame* frameptr = wxGetApp().currframe;

    // bring app to front
    #ifdef __WXMAC__
        ProcessSerialNumber process;
        if ( GetCurrentProcess(&process) == noErr )
            SetFrontProcess(&process);
    #endif
    #ifdef __WXMSW__
        SetForegroundWindow( (HWND)frameptr->GetHandle() );
    #endif
    frameptr->Raise();
   
    size_t numfiles = filenames.GetCount();
    for ( size_t n = 0; n < numfiles; n++ ) {
        frameptr->OpenFile(filenames[n]);
    }
    return true;
}

#endif // wxUSE_DRAG_AND_DROP

// -----------------------------------------------------------------------------

void MyFrame::InitializeRenderPane()
{
    // for now the VTK window goes in the center pane (always visible) - we got problems when had in a floating pane
    vtkObject::GlobalWarningDisplayOff(); // (can turn on for debugging)
    this->pVTKWindow = new wxVTKRenderWindowInteractor(this,wxID_ANY);
    this->aui_mgr.AddPane(this->pVTKWindow, wxAuiPaneInfo()
                  .Name(PaneName(ID::CanvasPane))
                  .CenterPane()
                  .BestSize(400,400)
                  );
    #if wxUSE_DRAG_AND_DROP
        // let users drag-and-drop pattern files onto the render pane
        this->pVTKWindow->SetDropTarget(new DnDFile());
    #endif
}

void MyFrame::LoadSettings()
{
    // (uses registry or files, depending on platform)
    wxConfig config(_T("Ready"));
    wxString str;
    if(config.Read(_T("WindowLayout"),&str))
        this->aui_mgr.LoadPerspective(str);
    int x=800,y=600;
    config.Read(_T("AppWidth"),&x);
    config.Read(_T("AppHeight"),&y);
    this->SetSize(x,y);
    x=200; y=200;
    config.Read(_T("AppX"),&x);
    config.Read(_T("AppY"),&y);
    this->SetPosition(wxPoint(x,y));
    config.Read(_T("iOpenCLPlatform"),&this->iOpenCLPlatform);
    config.Read(_T("iOpenCLDevice"),&this->iOpenCLDevice);
    config.Read(_T("LastUsedScreenshotFolder"),&this->last_used_screenshot_folder);
}

void MyFrame::SaveSettings()
{
    // (uses registry or files, depending on platform)
    wxConfig config(_T("Ready"));
    config.Write(_T("WindowLayout"),this->aui_mgr.SavePerspective());
    config.Write(_T("AppWidth"),this->GetSize().x);
    config.Write(_T("AppHeight"),this->GetSize().y);
    config.Write(_T("AppX"),this->GetPosition().x);
    config.Write(_T("AppY"),this->GetPosition().y);
    config.Write(_T("iOpenCLPlatform"),this->iOpenCLPlatform);
    config.Write(_T("iOpenCLDevice"),this->iOpenCLDevice);
    config.Write(_T("LastUsedScreenshotFolder"),this->last_used_screenshot_folder);
}

MyFrame::~MyFrame()
{
    this->SaveSettings(); // save the current settings so it starts up the same next time
    this->aui_mgr.UnInit();
    this->pVTKWindow->Delete();
    delete this->system;
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo info;
    wxString title;
    title << _T("Ready ") << _T(STR(READY_VERSION));
    info.SetName(title);
    info.SetDescription(_("A program for exploring reaction-diffusion systems.\n\nReady is free software, distributed under the GPLv3 license."));
    info.SetCopyright(_T("(C) 2011, 2012 The Ready Bunch"));
    info.AddDeveloper(_T("Tim Hutton"));
    info.AddDeveloper(_T("Robert Munafo"));
    info.AddDeveloper(_T("Andrew Trevorrow"));
    info.AddDeveloper(_T("Tom Rokicki"));
    wxAboutBox(info);
}

void MyFrame::OnCut(wxCommandEvent& event)
{
    // TODO: action depends on which pane has focus
    event.Skip();
}

void MyFrame::OnCopy(wxCommandEvent& event)
{
    // TODO: action depends on which pane has focus
    event.Skip();
}

void MyFrame::OnPaste(wxCommandEvent& event)
{
    // TODO: action depends on which pane has focus
    event.Skip();
}

// AKT TODO!!! export this routine from utils???
static bool ClipboardHasText()
{
   bool hastext = false;
   #ifdef __WXGTK__
      // avoid re-entrancy bug in wxGTK 2.9.x
      if (wxTheClipboard->IsOpened()) return false;
   #endif
   if (wxTheClipboard->Open()) {
      hastext = wxTheClipboard->IsSupported(wxDF_TEXT);
      if (!hastext) {
         // TODO: we'll try to convert bitmap data to our pattern format???
         hastext = wxTheClipboard->IsSupported(wxDF_BITMAP);
      }
      wxTheClipboard->Close();
   }
   return hastext;
}

void MyFrame::OnUpdatePaste(wxUpdateUIEvent& event)
{
    event.Enable(ClipboardHasText());
}

void MyFrame::OnClear(wxCommandEvent& event)
{
    // TODO: action depends on which pane has focus
    event.Skip();
}

void MyFrame::OnSelectAll(wxCommandEvent& event)
{
    // TODO: action depends on which pane has focus
    event.Skip();
}

void MyFrame::OnToggleViewPane(wxCommandEvent &event)
{
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(event.GetId()));
    if(!pane.IsOk()) return;
    pane.Show(!pane.IsShown());
    this->aui_mgr.Update();
}

void MyFrame::OnUpdateViewPane(wxUpdateUIEvent& event)
{
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(event.GetId()));
    if(!pane.IsOk()) return;
    event.Check(pane.IsShown());
    this->aui_mgr.Update();
}

void MyFrame::OnOpenCLDiagnostics(wxCommandEvent &event)
{
    // TODO: merge this with SelectOpenCLDevice?
    wxBusyCursor busy;
    wxMessageBox(wxString(OpenCL_RD::GetOpenCLDiagnostics().c_str(),wxConvUTF8));
}

void MyFrame::OnSize(wxSizeEvent& event)
{
    // trigger a redraw
    if(this->pVTKWindow) this->pVTKWindow->Refresh(false);
    
    // need this to move and resize status bar in Mac app
    event.Skip();
}

void MyFrame::OnScreenshot(wxCommandEvent& event)
{
    // find an unused filename
    const wxString default_filename_root = _("Ready_screenshot_");
    const wxString default_filename_ext = _T("png");
    int unused_value = 0;
    wxString filename;
    wxString extension,folder;
    folder = this->last_used_screenshot_folder;
    do {
        filename = default_filename_root;
        filename << wxString::Format(_("%04d."),unused_value) << default_filename_ext;
        unused_value++;
    } while(::wxFileExists(folder+_T("/")+filename));

    // ask the user for confirmation
    bool accepted = true;
    do {
        filename = wxFileSelector(_("Specify the screenshot filename:"),folder,filename,default_filename_ext,
            _("PNG files (*.png)|*.png|JPG files (*.jpg)|*.jpg"),
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if(filename.empty()) return; // user cancelled
        // validate
        wxFileName::SplitPath(filename,&folder,NULL,&extension);
        if(extension!=_T("png") && extension!=_T("jpg"))
        {
            wxMessageBox(_("Unsupported format"));
            accepted = false;
        }
    } while(!accepted);

    this->last_used_screenshot_folder = folder;

    vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
    screenshot->SetInput(this->pVTKWindow->GetRenderWindow());

    vtkSmartPointer<vtkImageWriter> writer;
    if(extension==_T("png")) writer = vtkSmartPointer<vtkPNGWriter>::New();
    else if(extension==_T("jpg")) writer = vtkSmartPointer<vtkJPEGWriter>::New();
    writer->SetFileName(filename.mb_str());
    writer->SetInputConnection(screenshot->GetOutputPort());
    writer->Write();
}

void MyFrame::SetCurrentRDSystem(BaseRD* sys)
{
    delete this->system;
    this->system = sys;
    this->iActiveChemical = min(this->iActiveChemical,this->system->GetNumberOfChemicals()-1);
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->iActiveChemical);
    this->UpdateWindows();
}

void MyFrame::UpdateWindowTitle()
{
    wxString name = this->system->GetFilename();
    if (name.IsEmpty()) {
        // this should probably never happen
        name = _("unknown");
    } else {
        // just show file's name, not full path
        // AKT TODO!!! perhaps we should make this optional (via flag in Prefs > File)???
        name = name.AfterLast(wxFILE_SEP_PATH);
    }
    
    if (this->system->IsModified()) {
        // prepend asterisk to indicate the current system has been modified
        // (this is consistent with Golly and other Win/Linux apps)
        name = _T("*") + name;
    }
    
    #ifdef __WXMAC__
        // Mac apps don't show app name in window title
        this->SetTitle(name);
    #else
        // Win/Linux apps usually append the app name to the file name
        this->SetTitle(name + _T(" - Ready"));
    #endif
}

void MyFrame::UpdateWindows()
{
    this->SetStatusBarText();
    this->UpdateRulePane();
    this->UpdateWindowTitle();
    this->Refresh(false);
}

void MyFrame::OnStep(wxCommandEvent &event)
{
    try {
        this->system->Update(1);
    }
    catch(const exception& e)
    {
        wxMessageBox(_("Fatal error: ")+wxString(e.what(),wxConvUTF8));
        this->Destroy();
    }
    catch(...)
    {
        wxMessageBox(_("Unknown fatal error"));
        this->Destroy();
    }
    this->SetStatusBarText();
    Refresh(false);
}

void MyFrame::OnUpdateStep(wxUpdateUIEvent& event)
{
    event.Enable(!this->is_running);
}

void MyFrame::OnRunStop(wxCommandEvent &event)
{
    if (this->is_running) {
        this->is_running = false;
        this->SetStatusBarText();
    } else {
        this->is_running = true;
    }
    Refresh(false);
}

void MyFrame::OnUpdateRunStop(wxUpdateUIEvent& event)
{
    wxMenuBar* mbar = GetMenuBar();
    if (mbar) {
        if (this->is_running) {
            mbar->SetLabel(ID::RunStop, _("Stop\tF5"));
        } else {
            mbar->SetLabel(ID::RunStop, _("Run\tF5"));
        }
    }
}

void MyFrame::OnReset(wxCommandEvent &event)
{
    wxMessageBox(_("TODO!!!"));
    /* presumably do something like this:
    if (this->system->GetTimestepsTaken() > 0) {
        // restore pattern and other info saved by SaveStartingPattern() which
        // was called in OnStep/OnRunStop when GetTimestepsTaken() was 0
        this->RestoreStartingPattern();
        this->is_running = false;
        this->UpdateWindows();
    }
    */
}

void MyFrame::OnUpdateReset(wxUpdateUIEvent& event)
{
    event.Enable(this->system->GetTimestepsTaken() > 0);
}

void MyFrame::OnIdle(wxIdleEvent& event)
{
    this->patterns_panel->DoIdleChecks();
    
    // we drive our game loop by onIdle events
    if(this->is_running)
    {
        int n_cells = this->system->GetX() * this->system->GetY() * this->system->GetZ();
   
        double time_before = get_time_in_seconds();
   
        try 
        {
            this->system->Update(this->timesteps_per_render); // TODO: user controls speed
        }
        catch(const exception& e)
        {
            wxMessageBox(_("Fatal error: ")+wxString(e.what(),wxConvUTF8));
            this->Destroy();
        }
        catch(...)
        {
            wxMessageBox(_("Unknown fatal error"));
            this->Destroy();
        }
   
        double time_after = get_time_in_seconds();
        this->frames_per_second = this->timesteps_per_render / (time_after - time_before);
        this->million_cell_generations_per_second = this->frames_per_second * n_cells / 1e6;
   
        this->pVTKWindow->Refresh(false);
        this->SetStatusBarText();
   
        wxMilliSleep(30);
   
        event.RequestMore(); // trigger another onIdle event
    }
    
    event.Skip();
}

void MyFrame::SetStatusBarText()
{
    wxString txt;
    if(this->is_running) txt << _("Running.");
    else txt << _("Stopped.");
    txt << _(" Timesteps: ") << this->system->GetTimestepsTaken();
    txt << _T("   (") << wxString::Format(_T("%.0f"),this->frames_per_second) 
        << _(" computed frames per second, ")
        << wxString::Format(_T("%.0f"),this->million_cell_generations_per_second) 
        << _T(" mcgs)");
    SetStatusText(txt);
}

void MyFrame::OnRestoreDefaultPerspective(wxCommandEvent& event)
{
    this->aui_mgr.LoadPerspective(this->default_perspective);
}

void MyFrame::OnInitWithBlobInCenter(wxCommandEvent& event)
{
    this->system->InitWithBlobInCenter();
    this->UpdateWindows();
}

void MyFrame::OnSelectOpenCLDevice(wxCommandEvent& event)
{
    // TODO: merge this with GetOpenCL diagnostics?
    wxArrayString choices;
    int iOldSelection;
    int np;
    try 
    {
        np = OpenCL_RD::GetNumberOfPlatforms();
    }
    catch(const exception& e)
    {
        wxMessageBox(_("OpenCL not available: ")+
            wxString(e.what(),wxConvUTF8));
        return;
    }
    catch(...)
    {
        wxMessageBox(_("OpenCL not available"));
        return;
    }
    for(int ip=0;ip<np;ip++)
    {
        int nd = OpenCL_RD::GetNumberOfDevices(ip);
        for(int id=0;id<nd;id++)
        {
            if(ip==this->iOpenCLPlatform && id==this->iOpenCLDevice)
                iOldSelection = (int)choices.size();
            wxString s(OpenCL_RD::GetPlatformDescription(ip).c_str(),wxConvUTF8);
            s << _T(" : ") << wxString(OpenCL_RD::GetDeviceDescription(ip,id).c_str(),wxConvUTF8);
            choices.Add(s);
        }
    }
    wxSingleChoiceDialog dlg(this,_("Select the OpenCL device to use:"),_("Select OpenCL device"),
        choices);
    dlg.SetSelection(iOldSelection);
    if(dlg.ShowModal()!=wxID_OK) return;
    int iNewSelection = dlg.GetSelection();
    if(iNewSelection != iOldSelection)
        wxMessageBox(_("The selected device will be used the next time an OpenCL pattern is loaded."));
    int dc = 0;
    for(int ip=0;ip<np;ip++)
    {
        int nd = OpenCL_RD::GetNumberOfDevices(ip);
        if(iNewSelection < nd)
        {
            this->iOpenCLPlatform = ip;
            this->iOpenCLDevice = iNewSelection;
            break;
        }
        iNewSelection -= nd;
    }
    // TODO: hot-change the current RD system
}

void MyFrame::OnHelp(wxCommandEvent &event)
{
    int id = event.GetId();
    switch (id)
    {
        case wxID_HELP:         this->help_panel->ShowHelp(_("Help/index.html")); break;
        case ID::HelpQuick:     this->help_panel->ShowHelp(_("Help/quickstart.html")); break;
        case ID::HelpTips:      this->help_panel->ShowHelp(_("Help/tips.html")); break;
        case ID::HelpKeyboard:  this->help_panel->ShowHelp(SHOW_KEYBOARD_SHORTCUTS); break;
        case ID::HelpMouse:     this->help_panel->ShowHelp(_("Help/mouse.html")); break;
        case ID::HelpFile:      this->help_panel->ShowHelp(_("Help/file.html")); break;
        case ID::HelpEdit:      this->help_panel->ShowHelp(_("Help/edit.html")); break;
        case ID::HelpView:      this->help_panel->ShowHelp(_("Help/view.html")); break;
        case ID::HelpAction:    this->help_panel->ShowHelp(_("Help/action.html")); break;
        case ID::HelpHelp:      this->help_panel->ShowHelp(_("Help/help.html")); break;
        case ID::HelpRefs:      this->help_panel->ShowHelp(_("Help/refs.html")); break;
        case ID::HelpFormats:   this->help_panel->ShowHelp(_("Help/formats.html")); break;
        case ID::HelpProblems:  this->help_panel->ShowHelp(_("Help/problems.html")); break;
        case ID::HelpChanges:   this->help_panel->ShowHelp(_("Help/changes.html")); break;
        case ID::HelpCredits:   this->help_panel->ShowHelp(_("Help/credits.html")); break;
        default:
            wxMessageBox(_("Bug: Unexpected ID in OnHelp!"));
            return;
    }
    
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(ID::HelpPane));
    if(!pane.IsOk()) return;
    pane.Show();
    this->aui_mgr.Update();
}

void MyFrame::OnSavePattern(wxCommandEvent &event)
{
    wxString filename = wxFileSelector(_("Specify the output filename:"),wxEmptyString,_("pattern.vti"),_T("vti"),
        _("VTK image files (*.vti)|*.vti"),wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if(filename.empty()) return; // user cancelled

    SaveFile(filename);

    this->system->SetFilename(string(filename.mb_str()));
    this->system->SetModified(false);
    this->UpdateWindowTitle();
}

void MyFrame::SaveFile(const wxString& path)
{
    wxBusyCursor busy;

    vtkSmartPointer<vtkImageAppendComponents> iac = vtkSmartPointer<vtkImageAppendComponents>::New();
    for(int i=0;i<this->system->GetNumberOfChemicals();i++)
        iac->AddInput(this->system->GetImage(i));
    iac->Update();

    vtkSmartPointer<RD_XMLWriter> iw = vtkSmartPointer<RD_XMLWriter>::New();
    iw->SetSystem(this->system);
    iw->SetInputConnection(iac->GetOutputPort());
    iw->SetFileName(path.mb_str());
    iw->Write();
}

void MyFrame::OnNewPattern(wxCommandEvent &event)
{
    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;
    
    // TODO: call something like this->system->InitPattern() instead???
    // or call InitWithBlobInCenter if Prefs > File has "Start with random blob" option ticked???
    this->system->InitWithBlobInCenter();

    this->is_running = false;
    this->system->SetFilename("untitled");
    this->system->SetModified(false);
    this->UpdateWindows();
}

void MyFrame::OnOpenPattern(wxCommandEvent &event)
{
    if(UserWantsToCancelWhenAskedIfWantsToSave()) return;
    wxString filename = wxFileSelector(_("Specify the input filename:"),wxEmptyString,_("pattern.vti"),_T("vti"),
        _("VTK image files (*.vti)|*.vti"),wxFD_OPEN);
    if(filename.empty()) return; // user cancelled
    OpenFile(filename);
}

void MyFrame::OpenFile(const wxString& path, bool remember)
{
    /* AKT TODO!!!
    if (IsHTMLFile(path)) {
        // show HTML file in help pane
        ShowHelp(path);
        return;
    }
    
    if (IsTextFile(path)) {
        // open text file in user's preferred text editor
        EditFile(path);
        return;
    }
    */

    if (remember) /* AKT TODO!!! AddRecentPattern(path) */;
    
    // load pattern file
    BaseRD *target_system = NULL;
    try
    {
        wxBusyCursor busy;

        vtkSmartPointer<RD_XMLReader> iw = vtkSmartPointer<RD_XMLReader>::New();
        iw->SetFileName(path.mb_str());
        iw->Update();

        string type = iw->GetType();
        if(type=="inbuilt")
        {
            string name = iw->GetName();
            if(name=="Gray-Scott")
                target_system = new GrayScott_slow();
            else if(name=="Gray-Scott_3D")
                target_system = new GrayScott_slow_3D();
            else 
                throw runtime_error("Unsupported inbuilt implementation: "+name);
            iw->SetSystemFromXMLWithoutFormula(target_system);
        }
        else if(type=="formula")
        {
            {
                // to load type="formula" pattern files the implementation must support editable kernels, which for now means OpenCL_nDim
                // TODO: detect if opencl is available, abort if not
                OpenCL_nDim *s = new OpenCL_nDim();
                s->SetPlatform(this->iOpenCLPlatform);
                s->SetDevice(this->iOpenCLDevice);
                target_system = s;
            }
            iw->SetSystemFromXMLWithFormula(target_system);
        }

        int dim[3];
        iw->GetOutput()->GetDimensions(dim);
        int nc = iw->GetOutput()->GetNumberOfScalarComponents();
        target_system->Allocate(dim[0],dim[1],dim[2],nc);
        target_system->CopyFromImage(iw->GetOutput());
        target_system->SetFilename(string(path.mb_str())); // TODO: display filetitle only (user option?)
        target_system->SetModified(false);
        this->SetCurrentRDSystem(target_system);
    }
    catch(const exception& e)
    {
        wxMessageBox(_("Failed to open file: ")+wxString(e.what(),wxConvUTF8));
        delete target_system;
        return;
    }
    catch(...)
    {
        wxMessageBox(_("Failed to open file"));
        delete target_system;
        return;
    }
}

void MyFrame::EditFile(const wxString& path)
{
    wxMessageBox(_("This file will eventually be opened in your text editor:\n") + path);
    /* AKT TODO!!!
    // prompt user if text editor hasn't been set yet
    if (texteditor.IsEmpty()) {
        ChooseTextEditor(this, texteditor);
        if (texteditor.IsEmpty()) return;
    }
    
    // open given file in user's preferred text editor
    wxString cmd;
    #ifdef __WXMAC__
        cmd = wxString::Format(wxT("open -a \"%s\" \"%s\""), texteditor.c_str(), path.c_str());
    #else
        // Windows or Unix
        cmd = wxString::Format(wxT("\"%s\" \"%s\""), texteditor.c_str(), path.c_str());
    #endif
    wxExecute(cmd, wxEXEC_ASYNC);
    */
}

void MyFrame::OnChangeActiveChemical(wxCommandEvent &event)
{
    wxArrayString choices;
    for(int i=0;i<this->system->GetNumberOfChemicals();i++)
        choices.Add(wxString::Format(_T("%d"),i+1));
    wxSingleChoiceDialog dlg(this,_("Select the chemical to render:"),_("Select active chemical"),
        choices);
    dlg.SetSelection(this->iActiveChemical);
    if(dlg.ShowModal()!=wxID_OK) return;
    this->iActiveChemical = dlg.GetSelection();
    InitializeVTKPipeline(this->pVTKWindow,this->system,this->iActiveChemical);
    this->UpdateWindows();
    // TODO: might have some visualization based on more than one chemical
}

void MyFrame::SetRuleName(string s)
{
    this->system->SetRuleName(s);
    this->UpdateWindowTitle();
    // TODO: update anything else that needs to know
}

void MyFrame::SetRuleDescription(string s)
{
    this->system->SetRuleDescription(s);
    this->UpdateWindowTitle();
    // TODO: update anything else that needs to know
}

void MyFrame::SetPatternDescription(string s)
{
    this->system->SetPatternDescription(s);
    this->UpdateWindowTitle();
    // TODO: update anything else that needs to know
}

void MyFrame::SetParameter(int iParam,float val)
{
    this->system->SetParameterValue(iParam,val);
    this->UpdateWindowTitle();
    // TODO: update anything else that needs to know
}

void MyFrame::SetParameterName(int iParam,std::string s)
{
    this->system->SetParameterName(iParam,s);
    this->UpdateWindowTitle();
    // TODO: update anything else that needs to know
}

void MyFrame::SetFormula(std::string s)
{
    this->system->SetFormula(s);
    this->UpdateWindowTitle();
    // TODO: update anything else that needs to know
}

// AKT TODO!!! export this routine from utils???
static int SaveChanges(const wxString& query, const wxString& msg)
{
    #ifdef __WXMAC__
        // create a standard looking Mac dialog
        wxMessageDialog dialog(wxGetActiveWindow(), msg, query,
                               wxCENTER | wxNO_DEFAULT | wxYES_NO | wxCANCEL |
                               wxICON_INFORMATION);
        
        // change button order to what Mac users expect to see
        dialog.SetYesNoCancelLabels("Cancel", "Save", "Don't Save");
       
        switch ( dialog.ShowModal() )
        {
            case wxID_YES:    return wxCANCEL;  // Cancel
            case wxID_NO:     return wxYES;     // Save
            case wxID_CANCEL: return wxNO;      // Don't Save
            default:          return wxCANCEL;  // should never happen
        }
    #else
        // Windows/Linux
        return wxMessageBox(msg, query, wxICON_QUESTION | wxYES_NO | wxCANCEL,
                            wxGetActiveWindow());
    #endif
}

bool MyFrame::UserWantsToCancelWhenAskedIfWantsToSave()
{
    if(!this->system->IsModified()) return false;
    
    int ret = SaveChanges(_("Save the current system?"),_("If you don't save, your changes will be lost."));
    if(ret==wxCANCEL) return true;
    if(ret==wxNO) return false;

    // ret == wxYES
    wxString filename = wxFileSelector(_("Specify the output filename:"),wxEmptyString,_("pattern.vti"),_T("vti"),
        _("VTK image files (*.vti)|*.vti"),wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if(filename.empty()) return true; // user cancelled

    SaveFile(filename);
    
    return false;
}

void MyFrame::OnClose(wxCloseEvent& event)
{
    if(event.CanVeto() && this->UserWantsToCancelWhenAskedIfWantsToSave()) return;
    event.Skip();
}
