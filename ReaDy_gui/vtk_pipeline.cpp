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
#include "BaseRD_2D.hpp"
#include "BaseRD_3D.hpp"

// wxVTK: (local copy)
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
#include <vtkImageShiftScale.h>
#include <vtkImageActor.h>
#include <vtkProperty2D.h>
#include <vtkRendererCollection.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkImageMapToColors.h>

// STL:
#include <stdexcept>
using namespace std;

void InitializeVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
{
    switch(system->GetDimensionality())
    {
        case 2: 
            {
                BaseRD_2D* system_2d = dynamic_cast<BaseRD_2D*>(system);
                Initialize2DVTKPipeline(pVTKWindow,system_2d);
            }
            break;
        case 3: 
            {
                BaseRD_3D* system_3d = dynamic_cast<BaseRD_3D*>(system);
                Initialize3DVTKPipeline(pVTKWindow,system_3d);
            }
            break;
        default:
            throw runtime_error("Unsupported RD system");
    }
}

void Initialize2DVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD_2D* system)
{
    // TODO
}

void Initialize3DVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD_3D* system)
{
    // TODO
}

/*
void Show3DVTKDemo(wxVTKRenderWindowInteractor* pVTKWindow)
{
    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    pVTKWindow->GetRenderWindow()->GetRenderers()->RemoveAllItems();
    pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    // make a 3D perlin noise scene, just as a demo
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
    pVTKWindow->SetInteractorStyle(is);

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

void Show2DVTKDemo(wxVTKRenderWindowInteractor* pVTKWindow)
{
    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    pVTKWindow->GetRenderWindow()->GetRenderers()->RemoveAllItems();
    pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    // make a 2D perlin noise image, just as a placeholder
    {
        // a function that returns a value at every point in space
        vtkSmartPointer<vtkPerlinNoise> perlinNoise = vtkSmartPointer<vtkPerlinNoise>::New();
        perlinNoise->SetFrequency(2, 1.25, 1.5);
        perlinNoise->SetPhase(0, 0, 0);

        // sample the function at a 2D grid of points
        vtkSmartPointer<vtkSampleFunction> sample = vtkSmartPointer<vtkSampleFunction>::New();
        sample->SetImplicitFunction(perlinNoise);
        sample->SetSampleDimensions(300,300,1);
        sample->ComputeNormalsOff();

        // create a lookup table for mapping values to colors
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->SetRampToLinear();
        lut->SetScaleToLinear();
        lut->SetTableRange(-1.0, 1.0);
        lut->SetHueRange(0.6, 0.0);

        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInputConnection(sample->GetOutputPort());
      
        // an actor determines how a scene object is displayed
        vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInput(image_mapper->GetOutput());
        actor->InterpolateOff();

        // add the actor to the renderer's scene
        pRenderer->AddActor(actor);
    }

    // set the background color
    pRenderer->SetBackground(0,0,0);
    
    // change the interactor style to something suitable for 2D images
    vtkSmartPointer<vtkInteractorStyleImage> is = vtkSmartPointer<vtkInteractorStyleImage>::New();
    pVTKWindow->SetInteractorStyle(is);
}
*/