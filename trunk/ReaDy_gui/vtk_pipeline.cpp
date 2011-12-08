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
#include "vtk_pipeline.hpp"

// readybase:
#include "BaseRD.hpp"

// wxVTK: (local copy)
#include "wxVTKRenderWindowInteractor.h"

// VTK:
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>
#include <vtkContourFilter.h>
#include <vtkProperty.h>
#include <vtkImageShiftScale.h>
#include <vtkImageActor.h>
#include <vtkProperty2D.h>
#include <vtkRendererCollection.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>
#include <vtkImageData.h>
#include <vtkScalarBarActor.h>
#include <vtkCubeSource.h>
#include <vtkExtractEdges.h>

// STL:
#include <stdexcept>
using namespace std;

void InitializeVTKPipeline_2D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system);
void InitializeVTKPipeline_3D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system);

void InitializeVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
{
    assert(pVTKWindow);
    assert(system);

    switch(system->GetDimensionality())
    {
        case 2: InitializeVTKPipeline_2D(pVTKWindow,system); break;
        case 3: InitializeVTKPipeline_3D(pVTKWindow,system); break;
        default:
            throw runtime_error("InitializeVTKPipeline : Unsupported dimensionality");
    }
    pVTKWindow->Refresh(false);
}

void InitializeVTKPipeline_2D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
{
    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    pVTKWindow->GetRenderWindow()->GetRenderers()->RemoveAllItems();
    pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    // assemble the scene
    {
        // create a lookup table for mapping values to colors
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->SetRampToLinear();
        lut->SetScaleToLinear();
        lut->SetTableRange(0.0, 1.0);
        lut->SetHueRange(0.6, 0.0);

        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInput(system->GetVTKImage());
        image_mapper->SetActiveComponent(1);
        // TODO: will need to support many more ways of rendering
      
        // an actor determines how a scene object is displayed
        vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInput(image_mapper->GetOutput());
        actor->InterpolateOff();

        // add the actor to the renderer's scene
        pRenderer->AddActor(actor);

        // also add a scalar bar to show how the colors correspond to values
        {
            vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
            scalar_bar->SetLookupTable(lut);
            pRenderer->AddActor2D(scalar_bar);
        }
    }

    // set the background color
    pRenderer->SetBackground(0,0,0);
    
    // change the interactor style to something suitable for 2D images
    vtkSmartPointer<vtkInteractorStyleImage> is = vtkSmartPointer<vtkInteractorStyleImage>::New();
    pVTKWindow->SetInteractorStyle(is);
}

void InitializeVTKPipeline_3D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
{
    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    pVTKWindow->GetRenderWindow()->GetRenderers()->RemoveAllItems();
    pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    // contour the 3D volume and render as a polygonal surface
    {
        // turns the 3d grid of sampled values into a polygon mesh for rendering,
        // by making a surface that contours the volume at a specified level
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInput(system->GetVTKImage());
        surface->SetValue(0, 0.5);
        surface->SetArrayComponent(1);
        // TODO: will need to provide more ways of rendering (e.g. volume rendering)
        // TODO: allow user to control the contour level

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
    // also add the bounding box
    {
        vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
        box->SetBounds(system->GetVTKImage()->GetBounds());
        vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
        edges->SetInputConnection(box->GetOutputPort());
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(edges->GetOutputPort());
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0,0,0);  
        actor->GetProperty()->SetAmbient(1);
        pRenderer->AddActor(actor);
    }

    // set the background color
    pRenderer->GradientBackgroundOn();
    pRenderer->SetBackground(0,0.4,0.6);
    pRenderer->SetBackground2(0,0.2,0.3);
    
    // change the interactor style to a trackball
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    pVTKWindow->SetInteractorStyle(is);
}
