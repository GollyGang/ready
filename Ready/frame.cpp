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

// readybase:
#include "GrayScott_slow.hpp"
#include "GrayScott_slow_3D.hpp"
#include "OpenCL2D_2Chemicals.hpp"

// local resources:
#include "appicon16.xpm"

// wxWidgets:
#include <wx/artprov.h>
#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/aboutdlg.h>
#include <wx/filename.h>

// wxVTK: (local copy)
#include "wxVTKRenderWindowInteractor.h"

// STL:
#include <fstream>
#include <string>
using namespace std;

// OpenCL: (local copy)
#include "cl.hpp"

// VTK:
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkSmartPointer.h>

// IDs for the controls and the menu commands
namespace ID { enum {

   Quit = wxID_EXIT,
   About = wxID_ABOUT,

   PatternsPane = wxID_HIGHEST+1,
   KernelPane,
   CanvasPane,
   SystemSettingsPane,

   GrayScott2DDemo,
   GrayScott2DOpenCLDemo,
   GrayScott3DDemo,

   Step,
   Run,
   Stop,

   OpenCLDiagnostics,
   Screenshot,

}; };

wxString PaneName(int id)
{
    return wxString::Format(_T("%d"),id);
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_IDLE(MyFrame::OnIdle)
    EVT_SIZE(MyFrame::OnSize)
    // menu commands
    EVT_MENU(ID::Quit,  MyFrame::OnQuit)
    EVT_MENU(ID::About, MyFrame::OnAbout)
    EVT_MENU(ID::OpenCLDiagnostics,MyFrame::OnOpenCLDiagnostics)
    EVT_MENU(ID::Screenshot,MyFrame::OnScreenshot)
    EVT_MENU(ID::GrayScott2DDemo,MyFrame::OnGrayScott2DDemo)
    EVT_MENU(ID::GrayScott2DOpenCLDemo,MyFrame::OnGrayScott2DOpenCLDemo)
    EVT_MENU(ID::GrayScott3DDemo,MyFrame::OnGrayScott3DDemo)
    EVT_MENU(ID::Step,MyFrame::OnStep)
    EVT_UPDATE_UI(ID::Step,MyFrame::OnUpdateStep)
    EVT_MENU(ID::Run,MyFrame::OnRun)
    EVT_UPDATE_UI(ID::Run,MyFrame::OnUpdateRun)
    EVT_MENU(ID::Stop,MyFrame::OnStop)
    EVT_UPDATE_UI(ID::Stop,MyFrame::OnUpdateStop)
    // allow panes to be turned on and off from the menu:
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::KernelPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::KernelPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::CanvasPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::CanvasPane, MyFrame::OnUpdateViewPane)
END_EVENT_TABLE()

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title),
       pVTKWindow(NULL),system(NULL),
       is_running(true)
{
    this->SetIcon(wxICON(appicon16));
    this->aui_mgr.SetManagedWindow(this);

    // add menus
    wxMenuBar *menuBar = new wxMenuBar();
    {
        wxMenu *fileMenu = new wxMenu;
        fileMenu->Append(wxID_ANY, _("&Open\tCtrl-O"), _("Open a pattern"));
        fileMenu->AppendSeparator();
        fileMenu->Append(wxID_ANY, _("&Save\tCtrl-S"), _("Save the current pattern"));
        fileMenu->AppendSeparator();
        fileMenu->Append(ID::Quit, _("E&xit\tAlt-X"), _("Quit this program"));
        menuBar->Append(fileMenu, _("&File"));
    }
    {
        wxMenu *viewMenu = new wxMenu;
        viewMenu->Append(ID::GrayScott2DDemo,_("GrayScott &2D demo"),_("Show a 2D demo"));
        viewMenu->Append(ID::GrayScott2DOpenCLDemo,_("GrayScott &2D OpenCL demo"),_("Show a 2D OpenCL demo"));
        viewMenu->Append(ID::GrayScott3DDemo,_("GrayScott &3D demo"),_("Show a 3D demo"));
        viewMenu->AppendSeparator();
        viewMenu->AppendCheckItem(ID::PatternsPane, _("&Patterns"), _("View the patterns pane"));
        viewMenu->AppendCheckItem(ID::KernelPane, _("&Kernel"), _("View the kernel pane"));
        viewMenu->AppendCheckItem(ID::CanvasPane, _("&Canvas"), _("View the canvas pane"));
        viewMenu->AppendSeparator();
        viewMenu->Append(ID::Screenshot, _("Save &screenshot"), _("Save a screenshot of the current view"));
        menuBar->Append(viewMenu, _("&View"));
    }
    {
        wxMenu *settingsMenu = new wxMenu;
        settingsMenu->Append(ID::Step,_("&Step\tSPACE"),_("Advance the simulation by a single timestep"));
        settingsMenu->Append(ID::Run,_("&Run\tF5"),_("Start running the simulation"));
        settingsMenu->Append(ID::Stop,_("St&op\tF6"),_("Stop running the simulation"));
        settingsMenu->AppendSeparator();
        settingsMenu->Append(ID::OpenCLDiagnostics,_("Open&CL diagnostics"),_("Show the available OpenCL devices and their attributes"));
        menuBar->Append(settingsMenu,_("&Settings"));
    }
    {
        wxMenu *helpMenu = new wxMenu;
        helpMenu->Append(ID::About, _("&About...\tF1"), _("Show about dialog"));
        menuBar->Append(helpMenu, _("&Help"));
    }
    SetMenuBar(menuBar);

    // add status bar
    CreateStatusBar(2);
    SetStatusText(_("Ready"));

    // create a VTK window
    vtkObject::GlobalWarningDisplayOff(); // (can turn on for debugging)
    this->pVTKWindow = new wxVTKRenderWindowInteractor(this,wxID_ANY);

    // initialize an RD system to get us started
    try {
        GrayScott_slow_3D *gs = new GrayScott_slow_3D();
        gs->Allocate(30,25,20);
        gs->InitWithBlobInCenter();
        this->SetCurrentRDSystem(gs); // connects it to the VTK window
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_("Error during RD system initialization: %s"),e.what()));
        this->Destroy();
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during RD system initialization"));
        this->Destroy();
    }
    
    // load a kernel text (just as a demo, doesn't do anything)
    string dummy_kernel_text;
    {
        std::string kfn = CL_SOURCE_DIR; // (defined in CMakeLists.txt to be the source folder)
        kfn += "/gray_scott_kernel.cl";
        ifstream in(kfn.c_str());
        while(in.good())
        {
            string s;
            getline(in,s);
            dummy_kernel_text += s + "\n";
        }
    }

    // side panes (can be turned on and off, and rearranged)
    this->aui_mgr.AddPane(this->CreatePatternsCtrl(), wxAuiPaneInfo()
                  .Name(PaneName(ID::PatternsPane))
                  .Caption(_("Patterns Pane"))
                  .Bottom()
                  .BestSize(300,300)
                  .Layer(0) // layer 0 is the innermost ring around the central pane
                  );
    this->aui_mgr.AddPane(new wxTextCtrl(this,wxID_ANY,
                        wxString(dummy_kernel_text.c_str(),wxConvUTF8),
                        wxDefaultPosition,wxDefaultSize,
                        wxTE_MULTILINE), 
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::KernelPane))
                  .Caption(_("Kernel Pane"))
                  .Right()
                  .BestSize(400,400)
                  .Layer(1) // layer 1 is further towards the edge
                  );
    this->aui_mgr.AddPane(new wxTextCtrl(this,wxID_ANY), 
                  wxAuiPaneInfo()
                  .Name(PaneName(ID::SystemSettingsPane))
                  .Caption(_("System"))
                  .Top()
                  );
    // center pane (always visible)
    this->aui_mgr.AddPane(this->pVTKWindow, wxAuiPaneInfo()
                  .Name(PaneName(ID::CanvasPane))
                  .CenterPane()
                  .BestSize(400,400)
                  );

    this->LoadSettings();
    this->aui_mgr.Update();
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
    info.SetName(_("Ready"));
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
    wxBusyCursor busy;
    wxString report;
    {
        // Get available OpenCL platforms
        vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        report << _("Found ") << platforms.size() << _(" platform(s):\n");

        for(unsigned int iPlatform=0;iPlatform<platforms.size();iPlatform++)
        {
            report << _("Platform ") << iPlatform+1 << _T(":\n");
            string info;
            for(int i=CL_PLATFORM_PROFILE;i<=CL_PLATFORM_EXTENSIONS;i++)
            {
                platforms[iPlatform].getInfo(i,&info);
                report << wxString(info.c_str(),wxConvUTF8) << _T("\n");
            }

            // create a context using this platform and the GPU
            cl_context_properties cps[3] = { 
                CL_CONTEXT_PLATFORM, 
                (cl_context_properties)(platforms[iPlatform])(), 
                0 
            };
            cl::Context context( CL_DEVICE_TYPE_ALL, cps);

            // Get a list of devices on this platform
            vector<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();
            report << _("\nFound ") << devices.size() << _(" device(s) on this platform.\n");
            for(unsigned int iDevice=0;iDevice<devices.size();iDevice++)
            {
                report << _("Device ") << iDevice+1 << _T(":\n");
                for(unsigned int i=CL_DEVICE_NAME;i<=CL_DEVICE_EXTENSIONS;i++)
                {
                    if(devices[iDevice].getInfo(i,&info) == CL_SUCCESS)
                        report << wxString(info.c_str(),wxConvUTF8) << _T("\n");
                }
            }
            report << _T("\n");
        }
    }
    wxMessageBox(report);
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
        wxFileName::SplitPath(filename,NULL,NULL,&extension);
        if(filename.empty()) return; // user cancelled
        // validate
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

void MyFrame::OnGrayScott2DDemo(wxCommandEvent& event)
{
    try {
        GrayScott_slow *gs = new GrayScott_slow();
        gs->Allocate(80,50);
        gs->InitWithBlobInCenter();
        this->SetCurrentRDSystem(gs);
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_("Error during RD system initialization: %s"),e.what()));
        this->Destroy();
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during RD system initialization"));
        this->Destroy();
    }
}

void MyFrame::OnGrayScott2DOpenCLDemo(wxCommandEvent& event)
{
    try { 
        OpenCL2D_2Chemicals *s = new OpenCL2D_2Chemicals();
        s->Allocate(512,512);
        s->InitWithBlobInCenter();
        this->SetCurrentRDSystem(s);
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_("Error during RD system initialization: %s"),e.what()));
        this->Destroy();
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during RD system initialization"));
        this->Destroy();
    }
}

void MyFrame::OnGrayScott3DDemo(wxCommandEvent& event)
{
    try {
        GrayScott_slow_3D *gs = new GrayScott_slow_3D();
        gs->Allocate(15,15,15);
        gs->InitWithBlobInCenter();
        this->SetCurrentRDSystem(gs);
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_("Error during RD system initialization: %s"),e.what()));
        this->Destroy();
    }
    catch(...)
    {
        wxMessageBox(_("Unknown error during RD system initialization"));
        this->Destroy();
    }
}

void MyFrame::SetCurrentRDSystem(BaseRD* sys)
{
    delete this->system;
    this->system = sys;
    InitializeVTKPipeline(this->pVTKWindow,this->system);
    this->SetStatusBarText();
}

void MyFrame::OnStep(wxCommandEvent &event)
{
    try {
        this->system->Update(1);
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_("Fatal error: %s"),e.what()));
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

    try {
        this->system->Update(100); // TODO: user controls speed
    }
    catch(const exception& e)
    {
        wxMessageBox(wxString::Format(_("Fatal error: %s"),e.what()));
        this->Destroy();
    }
    catch(...)
    {
        wxMessageBox(_("Unknown fatal error"));
        this->Destroy();
    }

    // TODO: compute wallclock speed

    this->SetStatusBarText();
    this->Refresh(false);

    wxMilliSleep(30);

    event.RequestMore(); // trigger another onIdle event
}

void MyFrame::SetStatusBarText()
{
    // TODO: display wallclock speed
    wxString txt;
    if(this->is_running) txt << _("Running. ");
    else txt << _("Stopped. ");
    txt << _("Timesteps: ") << this->system->GetTimestepsTaken();
    SetStatusText(txt);
}
