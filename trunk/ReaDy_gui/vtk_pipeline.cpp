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
#include <vtkImageData.h>

// STL:
#include <stdexcept>
using namespace std;

void InitializeVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
{
    switch(system->GetDimensionality())
    {
        case 2: 
            Initialize2DVTKPipeline(pVTKWindow,system);
            break;
        case 3: 
            Initialize3DVTKPipeline(pVTKWindow,system);
            break;
        default:
            throw runtime_error("Unsupported RD system");
    }
}

void Initialize2DVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
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
        lut->SetTableRange(-1.0, 1.0);
        lut->SetHueRange(0.6, 0.0);

        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInput(system->GetVTKImage());
      
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

void Initialize3DVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system)
{
    // TODO
}