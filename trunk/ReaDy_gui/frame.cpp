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

// local:
#include "frame.hpp"

// local resources:
#include "appicon16.xpm"

// wxWidgets:
#include <wx/artprov.h>
#include <wx/imaglist.h>
#include <wx/config.h>
#include <wx/aboutdlg.h>

// wxVTK:
#include "wxVTKRenderWindowInteractor.h"

// VTK:
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkPerlinNoise.h>
#include <vtkSampleFunction.h>
#include <vtkContourFilter.h>
#include <vtkProperty.h>

// STL:
#include <fstream>
#include <string>
using namespace std;

// IDs for the controls and the menu commands
namespace ID { enum {

   Quit = wxID_EXIT,
   About = wxID_ABOUT,

   PatternsPane = wxID_HIGHEST+1,
   KernelPane,
   CanvasPane,

}; };

wxString PaneName(int id)
{
    return wxString::Format(_T("%d"),id);
}

BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(ID::Quit,  MyFrame::OnQuit)
    EVT_MENU(ID::PatternsPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::PatternsPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::KernelPane, MyFrame::OnToggleViewPane)
    EVT_UPDATE_UI(ID::KernelPane, MyFrame::OnUpdateViewPane)
    EVT_MENU(ID::About, MyFrame::OnAbout)
END_EVENT_TABLE()

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
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
        viewMenu->AppendCheckItem(ID::PatternsPane, _("&Patterns"), _("View the patterns pane"));
        viewMenu->AppendCheckItem(ID::KernelPane, _("&Kernel"), _("View the kernel pane"));
        viewMenu->AppendSeparator();
        viewMenu->Append(wxID_ANY, _("Save &screenshot"), _("Save a screenshot of the current view"));
        menuBar->Append(viewMenu, _("&View"));
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

    // initialise a VTK pipeline and connect it to the rendering window
    this->InitializeVTKPipeline();
    
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

    // center pane (always visible)
    this->aui_mgr.AddPane(this->pVTKWindow, wxAuiPaneInfo()
                  .Name(PaneName(ID::CanvasPane))
                  .CenterPane()
                  .PaneBorder(false)
                  );

    this->LoadSettings();
}

void MyFrame::InitializeVTKPipeline()
{
    // wxVTKRenderWindowInteractor inherits from wxWindow *and* vtkRenderWindowInteractor
    this->pVTKWindow = new wxVTKRenderWindowInteractor(this, wxID_ANY);

    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    this->pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    // make a 3D perlin noise scene, just as a placeholder
    // (VTK can do 2D rendering too, for image display)
    {
        // a function that returns a value at every point in space
        vtkSmartPointer<vtkPerlinNoise> perlinNoise = vtkSmartPointer<vtkPerlinNoise>::New();
        perlinNoise->SetFrequency(2, 1.25, 1.5);
        perlinNoise->SetPhase(0, 0, 0);

        // samples the function at a 3D grid of points
        vtkSmartPointer<vtkSampleFunction> sample = vtkSmartPointer<vtkSampleFunction>::New();
        sample->SetImplicitFunction(perlinNoise);
        sample->SetSampleDimensions(65, 65, 20);
        sample->ComputeNormalsOff();
    
        // turns the 3d grid of sampled values into a polygon mesh for rendering,
        // by making a surface that contours the volume at a specified level
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInputConnection(sample->GetOutputPort());
        surface->SetValue(0, 0.0);

        // a mapper converts scene objects to graphics primitives
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(surface->GetOutputPort());
        mapper->ScalarVisibilityOff();

        // an actor determines how a scene object is displayed
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(1,1,0.9);  
        actor->GetProperty()->SetAmbient(0.1);
        actor->GetProperty()->SetDiffuse(0.6);
        actor->GetProperty()->SetSpecular(0.3);
        actor->GetProperty()->SetSpecularPower(30);

        // add the actor to the renderer's scene
        pRenderer->AddActor(actor);
    }

    // set the background color
    pRenderer->GradientBackgroundOn();
    pRenderer->SetBackground(0,0.4,0.6);
    pRenderer->SetBackground2(0,0.2,0.3);
    
    // change the interactor style to a trackball
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    this->pVTKWindow->SetInteractorStyle(is);

    // left mouse: rotates the camera around the focal point, as if the scene is a trackball
    // right mouse: move up and down to zoom in and out
    // scroll wheel: zoom in and out
    // shift+left mouse: pan
    // ctrl+left mouse: roll
    // shift+ctrl+left mouse: move up and down to zoom in and out
    // 'r' : reset the view to make everything visible
    // 'w' : switch to wireframe view
    // 's' : switch to surface view
}

void MyFrame::LoadSettings()
{
    // (uses registry or files, depending on platform)
    wxConfig config(_T("ReaDy"));
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
    wxConfig config(_T("ReaDy"));
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
}

void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxAboutDialogInfo info;
    info.SetName(_("ReaDy"));
    info.SetDescription(_("A program for exploring reaction-diffusion systems.\n\nReaDy is free software, distributed under the GPLv3 license."));
    info.SetCopyright(_T("(C) 2011 The ReaDy Bunch"));
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
