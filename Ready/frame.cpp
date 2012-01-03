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

   // we can use IDs higher than this for our own purposes
   Dummy = wxID_HIGHEST+1,

   // view menu
   GrayScott2DDemo,
   GrayScott2DOpenCLDemo,
   GrayScott3DDemo,
   GrayScott3DOpenCLDemo,
   PatternsPane,
   KernelPane,
   CanvasPane,
   SystemSettingsPane,
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
    EVT_MENU(ID::GrayScott2DDemo,MyFrame::OnDemo)
    EVT_MENU(ID::GrayScott2DOpenCLDemo,MyFrame::OnDemo)
    EVT_MENU(ID::GrayScott3DDemo,MyFrame::OnDemo)
    EVT_MENU(ID::GrayScott3DOpenCLDemo,MyFrame::OnDemo)
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::KernelPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::KernelPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::CanvasPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::CanvasPane, MyFrame::OnUpdateViewPane)
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
    this->LoadDemo(ID::GrayScott3DDemo);
}

void MyFrame::InitializeMenus()
{
    // add menus
    wxMenuBar *menuBar = new wxMenuBar();
    {   // file menu:
        wxMenu *menu = new wxMenu;
        menu->Append(wxID_ANY, _("&Open\tCtrl-O"), _("Open a pattern"));
        menu->AppendSeparator();
        menu->Append(wxID_ANY, _("&Save\tCtrl-S"), _("Save the current pattern"));
        menu->AppendSeparator();
        menu->Append(ID::Quit, _("E&xit\tAlt-X"), _("Quit this program"));
        menuBar->Append(menu, _("&File"));
    }
    {   // view menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::GrayScott2DDemo,_("GrayScott 2D demo"),_("Show a 2D demo"));
        menu->Append(ID::GrayScott2DOpenCLDemo,_("GrayScott 2D OpenCL demo"),_("Show a 2D OpenCL demo"));
        menu->Append(ID::GrayScott3DDemo,_("GrayScott 3D demo"),_("Show a 3D demo"));
        menu->Append(ID::GrayScott3DOpenCLDemo,_("GrayScott 3D OpenCL demo"),_("Show a 3D OpenCL demo"));
        menu->AppendSeparator();
        menu->AppendCheckItem(ID::PatternsPane, _("&Patterns"), _("View the patterns pane"));
        menu->AppendCheckItem(ID::KernelPane, _("&Kernel"), _("View the kernel pane"));
        menu->AppendCheckItem(ID::CanvasPane, _("&Canvas"), _("View the canvas pane"));
        menu->AppendSeparator();
        menu->Append(ID::RestoreDefaultPerspective,_("&Restore default layout"),_("Put the windows back where they were"));
        menu->AppendSeparator();
        menu->Append(ID::Screenshot, _("Save &screenshot"), _("Save a screenshot of the current view"));
        menuBar->Append(menu, _("&View"));
    }
    {   // settings menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::SelectOpenCLDevice,_("&Select OpenCL device to use"),_("Choose which OpenCL device to run on"));
        menu->AppendSeparator();
        menu->Append(ID::OpenCLDiagnostics,_("Open&CL diagnostics"),_("Show the available OpenCL devices and their attributes"));
        menuBar->Append(menu,_("&Settings"));
    }
    {   // actions menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::Step,_("&Step\tSPACE"),_("Advance the simulation by a single timestep"));
        menu->Append(ID::Run,_("&Run\tF5"),_("Start running the simulation"));
        menu->Append(ID::Stop,_("St&op\tF6"),_("Stop running the simulation"));
        menu->AppendSeparator();
        menu->Append(ID::InitWithBlobInCenter,_("Init with random blob"),_("Re-start with a random blob in the middle"));
        menuBar->Append(menu,_("&Actions"));
    }
    {   // help menu:
        wxMenu *menu = new wxMenu;
        menu->Append(ID::About, _("&About...\tF1"), _("Show about dialog"));
        menuBar->Append(menu, _("&Help"));
    }
    SetMenuBar(menuBar);
}

void MyFrame::InitializePanes()
{
    // a patterns file structure pane (currently just a placeholder)
    this->aui_mgr.AddPane(this->CreatePatternsCtrl(), wxAuiPaneInfo()
                  .Name(PaneName(ID::PatternsPane))
                  .Caption(_("Patterns Pane"))
                  .Bottom()
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
                      );
    }
    // a 'settings' pane (just a placeholder)
    this->aui_mgr.AddPane(new wxTextCtrl(this,wxID_ANY), 
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::SystemSettingsPane))
                  .Caption(_("System"))
                  .Top()
                  );
    // the VTK window goes in the center pane (always visible) - got problems when had in a floating pane
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
    title << _("Ready ") << _T(STR(READY_VERSION));
    info.SetName(title);
    info.SetDescription(_("A program for exploring reaction-diffusion systems.\n\nReady is free software, distributed under the GPLv3 license."));
    info.SetCopyright(_T("(C) 2011 The Ready Bunch"));
    info.AddDeveloper(_T("Tim Hutton"));
    info.AddDeveloper(_T("Robert Munafo"));
    info.AddDeveloper(_T("Andrew Trevorrow"));
    info.AddDeveloper(_T("Tom Rokicki"));
    wxAboutBox(info);
}

wxTreeCtrl* MyFrame::CreatePatternsCtrl()
{
    wxTreeCtrl* tree = new wxTreeCtrl(this, wxID_ANY,
                                      wxPoint(0,0), wxSize(240,250),
                                      wxTR_DEFAULT_STYLE | wxNO_BORDER);

    wxImageList* imglist = new wxImageList(16, 16, true, 2);
    imglist->Add(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(16,16)));
    imglist->Add(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(16,16)));
    tree->AssignImageList(imglist);

    // TODO: parse the Patterns folder; code below is just a placeholder!

    wxTreeItemId root = tree->AddRoot(_("Patterns folder"), 0);
    wxArrayTreeItemIds items;

    items.Add(tree->AppendItem(root, wxT("Gray-Scott"), 0));
    items.Add(tree->AppendItem(root, wxT("FitzHugh-Nagumo"), 0));

    int i, count;
    for (i = 0, count = items.Count(); i < count; ++i)
    {
        wxTreeItemId id = items.Item(i);
        tree->AppendItem(id, wxT("Dots_demo.txt"), 1);
        tree->AppendItem(id, wxT("Stripes_demo.txt"), 1);
        tree->AppendItem(id, wxT("Waves_demo.txt"), 1);
    }

    tree->Expand(root);
    tree->Expand(items.front());

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
    wxString filename,extension;
    bool accepted = true;
    do {
        filename = wxFileSelector(_("Specify the screenshot filename:"),_T("."),_("Ready_screenshot_00.png"),_T("png"),
            _("PNG files (*.png)|*.png|JPG files (*.jpg)|*.jpg"),
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
        if(filename.empty()) return; // user cancelled
        // validate
        wxFileName::SplitPath(filename,NULL,NULL,&extension);
        if(extension!=_T("png") && extension!=_T("jpg"))
        {
            wxMessageBox(_("Unsupported format"));
            accepted = false;
        }
    } while(!accepted);

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
    InitializeVTKPipeline(this->pVTKWindow,this->system);
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

    int n_cells = this->system->GetImageToRender()->GetDimensions()[0]
                * this->system->GetImageToRender()->GetDimensions()[1]
                * this->system->GetImageToRender()->GetDimensions()[2];

    double time_before = get_time_in_seconds();

    try {
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

void MyFrame::OnDemo(wxCommandEvent& event)
{
    this->LoadDemo(event.GetId());
}

void MyFrame::LoadDemo(int iDemo)
{
    try {
        switch(iDemo) 
        {
            case ID::GrayScott2DDemo:
                {
                    GrayScott_slow *gs = new GrayScott_slow();
                    gs->Allocate(80,50);
                    gs->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(gs);
                }
                break;
            case ID::GrayScott3DDemo: 
                {
                    GrayScott_slow_3D *gs = new GrayScott_slow_3D();
                    gs->Allocate(30,25,20);
                    gs->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(gs);
                }
                break;
            case ID::GrayScott2DOpenCLDemo:
                {
                    OpenCL2D_2Chemicals *s = new OpenCL2D_2Chemicals();
                    s->SetPlatform(this->iOpenCLPlatform);
                    s->SetDevice(this->iOpenCLDevice);
                    s->Allocate(128,64);
                    s->InitWithBlobInCenter();
                    this->SetCurrentRDSystem(s);
                }
                break;
            case ID::GrayScott3DOpenCLDemo:
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
    try {
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
    int iSelection;
    int np = OpenCL_RD::GetNumberOfPlatforms();
    for(int ip=0;ip<np;ip++)
    {
        int nd = OpenCL_RD::GetNumberOfDevices(ip);
        for(int id=0;id<nd;id++)
        {
            if(ip==this->iOpenCLPlatform && id==this->iOpenCLDevice)
                iSelection = choices.size();
            wxString s = OpenCL_RD::GetPlatformDescription(ip);
            s << " : " << OpenCL_RD::GetDeviceDescription(ip,id);
            choices.Add(s);
        }
    }
    wxSingleChoiceDialog dlg(this,_("Select the OpenCL device to use:"),_("Select OpenCL device"),
        choices);
    dlg.SetSelection(iSelection);
    if(dlg.ShowModal()!=wxID_OK) return;
    iSelection = dlg.GetSelection();
    int dc = 0;
    for(int ip=0;ip<np;ip++)
    {
        int nd = OpenCL_RD::GetNumberOfDevices(ip);
        if(iSelection < nd)
        {
            this->iOpenCLPlatform = ip;
            this->iOpenCLDevice = iSelection;
            break;
        }
        iSelection -= nd;
    }
    // TODO: how to tell the current system that the desired platform/device may have changed?
    // (will currently only take effect the next time the user loads a new system)
}
