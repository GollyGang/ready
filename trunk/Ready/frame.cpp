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
#include "app.hpp"      // for wxGetApp
#include "frame.hpp"
#include "vtk_pipeline.hpp"
#include "utils.hpp"
#include "IO_XML.hpp"

// readybase:
#include "GrayScott_slow.hpp"
#include "GrayScott_slow_3D.hpp"
#include "OpenCL_nDim.hpp"

// local resources:
#include "appicon16.xpm"

// wxWidgets:
#include <wx/artprov.h>
#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <wx/filename.h>
#include <wx/font.h>
#include <wx/html/htmlwin.h>
#include <wx/dir.h>
#include <wx/dnd.h>           // for wxFileDropTarget

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

// IDs for the controls and the menu commands
namespace ID { enum {

   // we can use IDs higher than this for our own purposes
   Dummy = wxID_HIGHEST+1,

   // view menu
   PatternsPane,
   RulePane,
   CanvasPane,
   HelpPane,
   RestoreDefaultPerspective,
   Screenshot,
   ChangeActiveChemical,

   // settings menu
   SelectOpenCLDevice,
   OpenCLDiagnostics,

   // actions menu
   Step,
   Run,
   Stop,
   ReplaceProgram,
   InitWithBlobInCenter,

   // controls
   PatternsTree,

}; };

wxString PaneName(int id)
{
    return wxString::Format(_T("%d"),id);
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_IDLE(MyFrame::OnIdle)
    EVT_SIZE(MyFrame::OnSize)
    // file menu
    EVT_MENU(wxID_OPEN, MyFrame::OnOpenPattern)
    EVT_MENU(wxID_SAVE, MyFrame::OnSavePattern)
    EVT_MENU(wxID_EXIT,  MyFrame::OnQuit)
    // view menu
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::RulePane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::RulePane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::HelpPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::HelpPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::RestoreDefaultPerspective,MyFrame::OnRestoreDefaultPerspective)
    EVT_MENU(ID::Screenshot,MyFrame::OnScreenshot)
    EVT_MENU(ID::ChangeActiveChemical,MyFrame::OnChangeActiveChemical)
    // settings menu
    EVT_MENU(ID::SelectOpenCLDevice,MyFrame::OnSelectOpenCLDevice)
    EVT_MENU(ID::OpenCLDiagnostics,MyFrame::OnOpenCLDiagnostics)
    // actions menu
    EVT_MENU(ID::Step,MyFrame::OnStep)
    EVT_UPDATE_UI(ID::Step,MyFrame::OnUpdateStep)
    EVT_MENU(ID::Run,MyFrame::OnRun)
    EVT_UPDATE_UI(ID::Run,MyFrame::OnUpdateRun)
    EVT_MENU(ID::Stop,MyFrame::OnStop)
    EVT_UPDATE_UI(ID::Stop,MyFrame::OnUpdateStop)
    EVT_BUTTON(ID::ReplaceProgram,MyFrame::OnReplaceProgram)
    EVT_UPDATE_UI(ID::ReplaceProgram,MyFrame::OnUpdateReplaceProgram)
    EVT_MENU(ID::InitWithBlobInCenter,MyFrame::OnInitWithBlobInCenter)
    // help menu
    EVT_MENU(wxID_ABOUT, MyFrame::OnAbout)
    EVT_MENU(wxID_HELP, MyFrame::OnHelp)
    // controls
    EVT_TREE_SEL_CHANGED(ID::PatternsTree,MyFrame::OnPatternsTreeSelChanged)
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
    this->LoadDemo(1);
}

void MyFrame::InitializeMenus()
{
    wxMenuBar *menuBar = new wxMenuBar();
    {   // file menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_OPEN);
        menu->AppendSeparator();
        menu->Append(wxID_SAVE);
        menu->AppendSeparator();
        menu->Append(wxID_EXIT);
        menuBar->Append(menu, _("&File"));
    }
    {   // view menu:
        wxMenu *menu = new wxMenu;
        menu->AppendCheckItem(ID::PatternsPane, _("&Patterns pane"), _("View the patterns pane"));
        menu->AppendCheckItem(ID::RulePane, _("&Rule pane"), _("View the rule pane"));
        menu->AppendCheckItem(ID::HelpPane, _("&Help pane"), _("View the help pane"));
        menu->AppendSeparator();
        menu->Append(ID::RestoreDefaultPerspective,_("&Restore default layout"),_("Put the windows back where they were"));
        menu->AppendSeparator();
        menu->Append(ID::Screenshot, _("Save &screenshot...\tF3"), _("Save a screenshot of the current view"));
        menu->AppendSeparator();
        menu->Append(ID::ChangeActiveChemical,_("&Change active chemical"),_("Change which chemical is being visualized"));
        menuBar->Append(menu, _("&View"));
    }
    {   // settings menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::SelectOpenCLDevice,_("&Select OpenCL device to use..."),_("Choose which OpenCL device to run on"));
        menu->AppendSeparator();
        menu->Append(ID::OpenCLDiagnostics,_("Open&CL diagnostics..."),_("Show the available OpenCL devices and their attributes"));
        menuBar->Append(menu,_("&Settings"));
    }
    {   // actions menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::Step,_("&Step\tF4"),_("Advance the simulation by a single timestep"));
        menu->Append(ID::Run,_("&Run\tF5"),_("Start running the simulation"));
        menu->Append(ID::Stop,_("St&op\tF6"),_("Stop running the simulation"));
        menu->AppendSeparator();
        menu->Append(ID::InitWithBlobInCenter,_("Init with random blob"),_("Re-start with a random blob in the middle"));
        menuBar->Append(menu,_("&Actions"));
    }
    {   // help menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_HELP,_("&Help...\tF1"),_("Show information about how to use Ready"));
        menu->AppendSeparator();
        menu->Append(wxID_ABOUT);
        menuBar->Append(menu, _("&Help"));
    }
    SetMenuBar(menuBar);
}

void FillTreeWithFilenames(wxTreeCtrl* tree,wxTreeItemId root,wxString folder,wxString filename_template)
{
    wxArrayString as;
    wxDir::GetAllFiles(folder,&as,filename_template,wxDIR_FILES);
    for(int i=0;i<as.size();i++)
        tree->AppendItem(root,wxFileName(as[i]).GetFullName(),1);
    // recurse down into each subdirectory
    wxDir dir(folder);
    wxString folder_name;
    bool cont = dir.GetFirst(&folder_name,wxALL_FILES_PATTERN,wxDIR_DIRS);
    while(cont)
    {
        wxTreeItemId subfolder = tree->AppendItem(root,folder_name,0);
        FillTreeWithFilenames(tree,subfolder,folder+_T("/")+folder_name,filename_template);
        tree->Expand(subfolder);
        cont = dir.GetNext(&folder_name);
    }
}

void MyFrame::InitializePatternsPane()
{
    patterntree = new wxTreeCtrl(this, ID::PatternsTree,
                                 wxPoint(0,0), wxSize(240,250),
                                 wxTR_DEFAULT_STYLE | wxNO_BORDER | wxTR_HIDE_ROOT);
    
    wxImageList* imglist = new wxImageList(16, 16, true, 2);
    imglist->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16,16)));
    imglist->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16)));
    patterntree->AssignImageList(imglist);

    wxTreeItemId root = patterntree->AddRoot(_(""), 0);
    // add inbuilt patterns
    {
        wxTreeItemId inbuilt = patterntree->AppendItem(root,_("Inbuilt patterns"),0);
        {
            wxTreeItemId demos = patterntree->AppendItem(inbuilt, wxT("CPU demos"), 0);
            this->demo_ids.push_back(patterntree->AppendItem(demos, wxT("Gray-Scott 2D"), 1));
            this->demo_ids.push_back(patterntree->AppendItem(demos, wxT("Gray-Scott 3D"), 1));
            patterntree->Expand(demos);
        }
        {
            wxTreeItemId demos = patterntree->AppendItem(inbuilt, wxT("OpenCL demos"), 0);
            this->demo_ids.push_back(patterntree->AppendItem(demos, wxT("Gray-Scott 1D"), 1));
            this->demo_ids.push_back(patterntree->AppendItem(demos, wxT("Gray-Scott 2D"), 1));
            this->demo_ids.push_back(patterntree->AppendItem(demos, wxT("Gray-Scott 3D"), 1));
            patterntree->Expand(demos);
        }
        patterntree->Expand(inbuilt);
    }
    // add patterns from patterns folder
    {
        // remember id of patterns folder for use in OnPatternsTreeSelChanged
        // AKT TODO!!! change svn folder name to "Patterns"???
        wxString foldername = _("patterns");
        patternroot = patterntree->AppendItem(root,foldername, 0);
        FillTreeWithFilenames(patterntree,patternroot,foldername,_T("*"));
        patterntree->Expand(patternroot);
    }

    this->aui_mgr.AddPane(patterntree, wxAuiPaneInfo()
                  .Name(PaneName(ID::PatternsPane))
                  .Caption(_("Patterns Pane"))
                  .Left()
                  .BestSize(300,300)
                  .Layer(0) // layer 0 is the innermost ring around the central pane
                  );
}

void MyFrame::InitializeRulePane()
{
    wxPanel *panel = new wxPanel(this,wxID_ANY);
    this->rule_pane = new wxTextCtrl(panel,wxID_ANY,
                        _T(""),
                        wxDefaultPosition,wxDefaultSize,
                        wxTE_MULTILINE | wxTE_RICH2 | wxTE_DONTWRAP | wxTE_PROCESS_TAB );
    // TODO: provide UI for changing font size (ditto in Help pane)
    #ifdef __WXMAC__
        // need bigger font size on Mac
        this->rule_pane->SetFont(wxFont(12,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
    #else
        this->rule_pane->SetFont(wxFont(8,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
    #endif
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    //sizer->Add(new wxButton(panel,ID::ReplaceProgram,_("Compile")),wxSizerFlags(0).Align(wxALIGN_RIGHT));
    // TODO: kernel-editing temporarily disabled, can edit file instead for now
    sizer->Add(this->rule_pane,wxSizerFlags(1).Expand());
    panel->SetSizer(sizer);
    this->aui_mgr.AddPane(panel,
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::RulePane))
                  .Caption(_("Rule Pane"))
                  .Right()
                  .BestSize(400,400)
                  .Layer(1) // layer 1 is further towards the edge
                  .Hide()
                  );
}

void MyFrame::InitializeHelpPane()
{
    // http://wiki.wxwidgets.org/Calling_The_Default_Browser_In_WxHtmlWindow
    class HtmlWindow: public wxHtmlWindow
    {
    public:
        HtmlWindow(wxWindow *parent, wxWindowID id = -1,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxHW_SCROLLBAR_AUTO, const wxString& name = _T("htmlWindow"))
            : wxHtmlWindow(parent, id, pos, size, style, name) {}

        void OnLinkClicked(const wxHtmlLinkInfo& link)
        {
            if( link.GetHref().StartsWith(_T("http://")) || link.GetHref().StartsWith(_T("mailto://")) )
                wxLaunchDefaultBrowser(link.GetHref());
            else
                wxHtmlWindow::OnLinkClicked(link);
        }
    };
    // a help pane
    HtmlWindow *html = new HtmlWindow(this,wxID_ANY);
    html->SetPage(_("<html><body>"
        "<h3>Quick start guide</h3>"
        "<h5>1. Overview</h5>"
        "<p>Ready " STR(READY_VERSION) " is an early release of a program to explore <a href=\"http://en.wikipedia.org/wiki/Reaction-diffusion_system\">reaction-diffusion</a> systems.</p>"
        "<p>Click on the demos in the Patterns Pane to see some different systems."
        "<p>The <a href=\"http://en.wikipedia.org/wiki/OpenCL\">OpenCL</a> demos will only work if you've got OpenCL installed. Either install the latest drivers for your graphics card, "
        "or install one of the SDKs from <a href=\"http://developer.amd.com/appsdk\">AMD</a> or <a href=\"http://software.intel.com/en-us/articles/vcsource-tools-opencl-sdk/\">Intel</a> "
        "that will work with your CPU. Use the Settings menu commands to examine the OpenCL devices available."
        "<h5>2. Interacting with the rendered scene</h5>"
        "<p>From the Actions menu: Stop (F6) the system running, or start it running (F5), or take small Steps (F4)."
        "<p><b>left mouse:</b> rotates the camera around the focal point, as if the scene is a trackball"
        "<p><b>right mouse, or shift+ctrl+left mouse:</b> move up and down to zoom in and out"
        "<p><b>scroll wheel:</b> zoom in and out"
        "<p><b>shift+left mouse:</b> pan"
        "<p><b>ctrl+left mouse:</b> roll"
        "<p><b>'r':</b> reset the view to make everything visible"
        "<p><b>'w':</b> switch to wireframe view"
        "<p><b>'s':</b> switch to surface view</ul>"
        "<h5>3. Working with the windows</h5>"
        "<p>The Patterns Pane, Help Pane and Kernel Pane can be shown or hidden by using the commands on the View menu. By dragging the panes by their title bar you can dock them into the "
        "Ready frame in different positions or float them as separate windows."
        "<p>The Kernel Pane is only used when the current system is an OpenCL demo. It shows the current OpenCL kernel."
        "<h5>4. More help</h5>"
        "<p>Send an email to <a href=\"mailto://reaction-diffusion@googlegroups.com\">reaction-diffusion@googlegroups.com</a> if you have any problems, or want to get involved."
        "<p>See the text files in the installation folder for more information."
        "</body></html>")); // TODO: split out into html files
    this->aui_mgr.AddPane(html, 
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

void MyFrame::UpdateWindows()
{
    this->SetStatusBarText();
    // fill the rule pane
    if(this->system->HasEditableFormula())
    {
        this->rule_pane->SetValue(wxString(this->system->GetFormula().c_str(),wxConvUTF8));
        this->rule_pane->Enable(true);
    }
    else
    {
        this->rule_pane->SetValue(_T("(this implementation has no editable formula)"));
        this->rule_pane->Enable(false);
    }
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

void MyFrame::OnRun(wxCommandEvent &event)
{
    this->is_running = true;
    Refresh(false);
}

void MyFrame::OnStop(wxCommandEvent &event)
{
    this->is_running = false;
    this->SetStatusBarText();
    Refresh(false);
}

void MyFrame::OnUpdateStep(wxUpdateUIEvent& event)
{
    event.Enable(!this->is_running);
}

void MyFrame::OnUpdateRun(wxUpdateUIEvent& event)
{
    event.Enable(!this->is_running);
}

void MyFrame::OnUpdateStop(wxUpdateUIEvent& event)
{
    event.Enable(this->is_running);
}

void MyFrame::OnIdle(wxIdleEvent& event)
{
    // we drive our game loop by onIdle events
    if(!this->is_running) return;

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

void MyFrame::LoadDemo(int iDemo)
{
    try 
    {
        switch(iDemo) 
        {
            case 0:
                {
                    GrayScott_slow *s = new GrayScott_slow();
                    s->SetNumberOfChemicals(2);
                    s->Allocate(80,50,1,2);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 1: 
                {
                    GrayScott_slow_3D *s = new GrayScott_slow_3D();
                    s->SetNumberOfChemicals(2);
                    s->Allocate(30,25,20,2);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 2:
                {
                    OpenCL_nDim *s = new OpenCL_nDim();
                    s->SetNumberOfChemicals(2);
                    s->SetPlatform(this->iOpenCLPlatform);
                    s->SetDevice(this->iOpenCLDevice);
                    s->Allocate(128,1,1,2);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 3:
                {
                    OpenCL_nDim *s = new OpenCL_nDim();
                    s->SetNumberOfChemicals(2);
                    s->SetPlatform(this->iOpenCLPlatform);
                    s->SetDevice(this->iOpenCLDevice);
                    s->Allocate(128,64,1,2);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 4:
                {
                    OpenCL_nDim *s = new OpenCL_nDim();
                    s->SetNumberOfChemicals(2);
                    s->SetPlatform(this->iOpenCLPlatform);
                    s->SetDevice(this->iOpenCLDevice);
                    s->Allocate(64,64,64,2);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            default: throw runtime_error("MyFrame::LoadDemo : internal error: unknown demo ID");
        }
    }
    catch(const exception& e)
    {
        wxMessageBox(_("Error during RD system initialization: ")+
            wxString(e.what(),wxConvUTF8));
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during RD system initialization"));
    }
}

void MyFrame::OnRestoreDefaultPerspective(wxCommandEvent& event)
{
    this->aui_mgr.LoadPerspective(this->default_perspective);
}

void MyFrame::OnReplaceProgram(wxCommandEvent& event)
{
    try 
    {
        this->system->TestFormula(string(this->rule_pane->GetValue().mb_str()));
    }
    catch(const runtime_error& e)
    {
        wxMessageBox(_("Formula did not compile:\n\n")+wxString(e.what(),wxConvUTF8));
        return;
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during test compile"));
        return;
    }
    // program compiled successfully
    this->system->SetFormula(string(this->rule_pane->GetValue().mb_str()));
    this->rule_pane->SetModified(false);
}

void MyFrame::OnUpdateReplaceProgram(wxUpdateUIEvent& event)
{
    event.Enable(this->system->HasEditableFormula() && this->rule_pane->IsModified());
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

void MyFrame::OnPatternsTreeSelChanged(wxTreeEvent& event)
{
    wxTreeItemId id = event.GetItem();
    if (!id.IsOk()) return;
    
    // first check if an inbuilt demo was clicked
    for(int i=0;i<(int)this->demo_ids.size();i++)
        if(id==this->demo_ids[i]) {
            this->LoadDemo(i);
            return;
        }
    
    // return if folder was selected
    // (use GetItemImage rather than ItemHasChildren because folder might be empty)
    if (patterntree->GetItemImage(id) == 0) return;
    
    // AKT TODO!!! allow selected file to be loaded again
    // (see Golly's MainFrame::OnDirTreeSelection for the horrible hack)
    
    // build path to selected file
    wxString filepath = patterntree->GetItemText(id);
    wxTreeItemId parent;
    do {
        parent = patterntree->GetItemParent(id);
        if (!parent.IsOk()) break;   // play safe
        wxString folder = patterntree->GetItemText(parent) + _T("/");
        filepath = folder + filepath;
        id = parent;
    } while (parent != patternroot);
    
    // load selected file
    OpenFile(filepath);
}

void MyFrame::OnHelp(wxCommandEvent &event)
{
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

    vtkSmartPointer<vtkImageAppendComponents> iac = vtkSmartPointer<vtkImageAppendComponents>::New();
    for(int i=0;i<this->system->GetNumberOfChemicals();i++)
        iac->AddInput(this->system->GetImage(i));
    iac->Update();

    vtkSmartPointer<RD_XMLWriter> iw = vtkSmartPointer<RD_XMLWriter>::New();
    iw->SetSystem(this->system);
    iw->SetInputConnection(iac->GetOutputPort());
    iw->SetFileName(filename.mb_str());
    iw->Write();
}

void MyFrame::OnOpenPattern(wxCommandEvent &event)
{
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
    BaseRD *target_system;
    try
    {
        // to load pattern files the implementation must support editable kernels, which for now means OpenCL_nDim
        // TODO: detect if opencl is available, abort if not
        {
            OpenCL_nDim *s = new OpenCL_nDim();
            s->SetPlatform(this->iOpenCLPlatform);
            s->SetDevice(this->iOpenCLDevice);
            target_system = s;
        }

        vtkSmartPointer<RD_XMLReader> iw = vtkSmartPointer<RD_XMLReader>::New();
        iw->SetFileName(path.mb_str());
        iw->Update();
        iw->SetFromXML(target_system);

        int dim[3];
        iw->GetOutput()->GetDimensions(dim);
        int nc = iw->GetOutput()->GetNumberOfScalarComponents();
        target_system->Allocate(dim[0],dim[1],dim[2],nc);
        target_system->CopyFromImage(iw->GetOutput());
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
