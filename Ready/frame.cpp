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
#include "frame.hpp"
#include "vtk_pipeline.hpp"
#include "utils.hpp"

// readybase:
#include "GrayScott_slow.hpp"
#include "GrayScott_slow_3D.hpp"
#include "OpenCL2D_2Chemicals.hpp"
#include "OpenCL3D_2Chemicals.hpp"

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

// IDs for the controls and the menu commands
namespace ID { enum {

   // some IDs have special values
   Quit = wxID_EXIT,
   About = wxID_ABOUT,
   Help = wxID_HELP,

   // we can use IDs higher than this for our own purposes
   Dummy = wxID_HIGHEST+1,

   // view menu
   PatternsPane,
   KernelPane,
   CanvasPane,
   HelpPane,
   RestoreDefaultPerspective,
   Screenshot,

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
    EVT_MENU(ID::Quit,  MyFrame::OnQuit)
    // view menu
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::KernelPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::KernelPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::HelpPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::HelpPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::RestoreDefaultPerspective,MyFrame::OnRestoreDefaultPerspective)
    EVT_MENU(ID::Screenshot,MyFrame::OnScreenshot)
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
    EVT_MENU(ID::About, MyFrame::OnAbout)
    EVT_MENU(ID::Help, MyFrame::OnHelp)
    // controls
    EVT_TREE_SEL_CHANGED(ID::PatternsTree,MyFrame::OnPatternsTreeSelChanged)
END_EVENT_TABLE()

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title),
       pVTKWindow(NULL),system(NULL),
       is_running(true),
       timesteps_per_render(100),
       frames_per_second(0.0),
       million_cell_generations_per_second(0.0),
       iOpenCLPlatform(0),iOpenCLDevice(0)
{
    this->SetIcon(wxICON(appicon16));
    this->aui_mgr.SetManagedWindow(this);

    this->InitializeMenus();

    CreateStatusBar(1);
    SetStatusText(_("Ready"));

    this->InitializePanes();
    
    this->default_perspective = this->aui_mgr.SavePerspective();
    this->LoadSettings();
    this->aui_mgr.Update();

    // initialize an RD system to get us started
    this->LoadDemo(1);
}

void MyFrame::InitializeMenus()
{
    // add menus
    wxMenuBar *menuBar = new wxMenuBar();
    {   // file menu:
        wxMenu *menu = new wxMenu;
        //menu->Append(wxID_ANY, _("&Open\tCtrl-O"), _("Open a pattern"));
        //menu->AppendSeparator();
        //menu->Append(wxID_ANY, _("&Save\tCtrl-S"), _("Save the current pattern"));
        //menu->AppendSeparator();
        menu->Append(ID::Quit, _("E&xit\tAlt-F4"), _("Quit this program"));
        menuBar->Append(menu, _("&File"));
    }
    {   // view menu:
        wxMenu *menu = new wxMenu;
        menu->AppendCheckItem(ID::PatternsPane, _("&Patterns pane"), _("View the patterns pane"));
        menu->AppendCheckItem(ID::KernelPane, _("&Kernel pane"), _("View the kernel pane"));
        menu->AppendCheckItem(ID::HelpPane, _("&Help pane"), _("View the help pane"));
        menu->AppendSeparator();
        menu->Append(ID::RestoreDefaultPerspective,_("&Restore default layout"),_("Put the windows back where they were"));
        menu->AppendSeparator();
        menu->Append(ID::Screenshot, _("Save &screenshot...\tF3"), _("Save a screenshot of the current view"));
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
        menu->Append(ID::Help, _("&Help...\tF1"), _("Show information about how to use Ready"));
        menu->AppendSeparator();
        menu->Append(ID::About, _("&About..."), _("Show about dialog"));
        menuBar->Append(menu, _("&Help"));
    }
    SetMenuBar(menuBar);
}

void MyFrame::InitializePanes()
{
    // a patterns file structure pane (currently just has some hard-wired demos)
    this->aui_mgr.AddPane(this->CreatePatternsCtrl(), wxAuiPaneInfo()
                  .Name(PaneName(ID::PatternsPane))
                  .Caption(_("Patterns Pane"))
                  .Left()
                  .BestSize(300,300)
                  .Layer(0) // layer 0 is the innermost ring around the central pane
                  );
    // a kernel-editing pane
    {
        wxPanel *panel = new wxPanel(this,wxID_ANY);
        this->kernel_pane = new wxTextCtrl(panel,wxID_ANY,
                            _T(""),
                            wxDefaultPosition,wxDefaultSize,
                            wxTE_MULTILINE | wxTE_RICH2 | wxTE_DONTWRAP | wxTE_PROCESS_TAB );
        this->kernel_pane->SetFont(wxFont(9,wxFONTFAMILY_TELETYPE,wxFONTSTYLE_NORMAL,wxFONTWEIGHT_BOLD));
        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(new wxButton(panel,ID::ReplaceProgram,_("Compile")),wxSizerFlags(0).Align(wxALIGN_RIGHT));
        sizer->Add(this->kernel_pane,wxSizerFlags(1).Expand());
        panel->SetSizer(sizer);
        this->aui_mgr.AddPane(panel,
                      wxAuiPaneInfo()
                      .Name(PaneName(ID::KernelPane))
                      .Caption(_("Kernel Pane"))
                      .Right()
                      .BestSize(400,400)
                      .Layer(1) // layer 1 is further towards the edge
                      .Hide()
                      );
    }
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
            "<p>Ready " STR(READY_VERSION) " is an early release of a program to explore <a href=\"http://en.wikipedia.org/wiki/Reaction–diffusion_system\">reaction-diffusion</a> systems.</p>"
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
    // for now the VTK window goes in the center pane (always visible) - we got problems when had in a floating pane
    vtkObject::GlobalWarningDisplayOff(); // (can turn on for debugging)
    this->pVTKWindow = new wxVTKRenderWindowInteractor(this,wxID_ANY);
    this->aui_mgr.AddPane(this->pVTKWindow, wxAuiPaneInfo()
                  .Name(PaneName(ID::CanvasPane))
                  .CenterPane()
                  .BestSize(400,400)
                  );
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

wxTreeCtrl* MyFrame::CreatePatternsCtrl()
{
    wxTreeCtrl* tree = new wxTreeCtrl(this, ID::PatternsTree,
                                      wxPoint(0,0), wxSize(240,250),
                                      wxTR_DEFAULT_STYLE | wxNO_BORDER);

    wxImageList* imglist = new wxImageList(16, 16, true, 2);
    imglist->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16,16)));
    imglist->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16)));
    tree->AssignImageList(imglist);

    wxTreeItemId root = tree->AddRoot(_("Patterns"), 0);
    {
        wxTreeItemId demos = tree->AppendItem(root, wxT("CPU demos"), 0);
        this->demo_ids.push_back(tree->AppendItem(demos, wxT("Gray-Scott 2D"),1));
        this->demo_ids.push_back(tree->AppendItem(demos, wxT("Gray-Scott 3D"), 1));
        tree->Expand(demos);
    }
    {
        wxTreeItemId demos = tree->AppendItem(root, wxT("OpenCL demos"), 0);
        this->demo_ids.push_back(tree->AppendItem(demos, wxT("Gray-Scott 2D"), 1));
        this->demo_ids.push_back(tree->AppendItem(demos, wxT("Gray-Scott 3D"), 1));
        tree->Expand(demos);
    }
    tree->Expand(root);
    // TODO: parse the Patterns folder and add those instead of these hard-wired demos

    return tree;
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
    InitializeVTKPipeline(this->pVTKWindow,this->system,1); // TODO: allow user to change which chemical is being visualized
    this->UpdateWindows();
}

void MyFrame::UpdateWindows()
{
    this->SetStatusBarText();
    // fill the kernel pane
    if(this->system->HasEditableProgram())
    {
        this->kernel_pane->SetValue(wxString(this->system->GetProgram().c_str(),wxConvUTF8));
        this->kernel_pane->Enable(true);
    }
    else
    {
        this->kernel_pane->SetValue(_T(""));
        this->kernel_pane->Enable(false);
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
                    s->Allocate(80,50);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 1: 
                {
                    GrayScott_slow_3D *s = new GrayScott_slow_3D();
                    s->Allocate(30,25,20);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 2:
                {
                    OpenCL2D_2Chemicals *s = new OpenCL2D_2Chemicals();
                    s->SetPlatform(this->iOpenCLPlatform);
                    s->SetDevice(this->iOpenCLDevice);
                    s->Allocate(128,64);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case 3:
                {
                    OpenCL3D_2Chemicals *s = new OpenCL3D_2Chemicals();
                    s->SetPlatform(this->iOpenCLPlatform);
                    s->SetDevice(this->iOpenCLDevice);
                    s->Allocate(64,64,64);
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
        this->system->TestProgram(string(this->kernel_pane->GetValue().mb_str()));
    }
    catch(const runtime_error& e)
    {
        wxMessageBox(_("Kernel program did not compile:\n\n")+wxString(e.what(),wxConvUTF8));
        return;
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during test compile"));
        return;
    }
    // program compiled successfully
    this->system->SetProgram(string(this->kernel_pane->GetValue().mb_str()));
    this->kernel_pane->SetModified(false);
}

void MyFrame::OnUpdateReplaceProgram(wxUpdateUIEvent& event)
{
    event.Enable(this->system->HasEditableProgram() && this->kernel_pane->IsModified());
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
    for(int i=0;i<(int)this->demo_ids.size();i++)
        if(event.GetItem()==this->demo_ids[i])
            this->LoadDemo(i);
}

void MyFrame::OnHelp(wxCommandEvent &event)
{
    wxAuiPaneInfo &pane = this->aui_mgr.GetPane(PaneName(ID::HelpPane));
    if(!pane.IsOk()) return;
    pane.Show();
    this->aui_mgr.Update();
}
