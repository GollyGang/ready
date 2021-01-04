/*  Copyright 2011-2021 The Ready Bunch

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
#include "scene_items.hpp"

// VTK:
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkScalarBarActor.h>
#include <vtkSmartPointer.h>
#include <vtkTextProperty.h>

void AddScalarBar(vtkRenderer* pRenderer,vtkScalarsToColors* lut)
{
    vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
    scalar_bar->SetLookupTable(lut);
    scalar_bar->SetOrientationToHorizontal();
    scalar_bar->SetWidth(0.5);
    scalar_bar->SetHeight(0.15);
    scalar_bar->SetPosition(0.25,0.01);
    scalar_bar->SetMaximumHeightInPixels(80);
    // workaround for http://www.paraview.org/Bug/view.php?id=14561
    scalar_bar->SetTitle("M");
    scalar_bar->GetTitleTextProperty()->SetOpacity(0);
    scalar_bar->SetTitleRatio(0.7);
    pRenderer->AddActor2D(scalar_bar);
}
