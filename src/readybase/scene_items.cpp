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

#include "scene_items.hpp"

// local:
#include "Properties.hpp"

// VTK:
#include <vtkColorSeries.h>
#include <vtkColorTransferFunction.h>
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkScalarBarActor.h>
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

vtkSmartPointer<vtkScalarsToColors> GetColorMap(const Properties& render_settings)
{
    const float low = render_settings.GetProperty("low").GetFloat();
    const float high = render_settings.GetProperty("high").GetFloat();

    vtkSmartPointer<vtkColorTransferFunction> lut = vtkSmartPointer<vtkColorTransferFunction>::New();
    lut->SetColorSpaceToHSV();
    lut->HSVWrapOff();
    float r, g, b;
    render_settings.GetProperty("color_low").GetColor(r, g, b);
    lut->AddRGBPoint(low, r, g, b);
    render_settings.GetProperty("color_high").GetColor(r, g, b);
    lut->AddRGBPoint(high, r, g, b);
    return lut;
}

void SetDefaultRenderSettings(Properties& render_settings)
{
    render_settings.DeleteAllProperties();
    render_settings.AddProperty(Property("surface_color", "color", 1.0f, 1.0f, 1.0f)); // RGB [0,1]
    render_settings.AddProperty(Property("color_low", "color", 0.0f, 0.0f, 1.0f));
    render_settings.AddProperty(Property("color_high", "color", 1.0f, 0.0f, 0.0f));
    render_settings.AddProperty(Property("show_color_scale", true));
    render_settings.AddProperty(Property("show_multiple_chemicals", true));
    render_settings.AddProperty(Property("active_chemical", "chemical", "a"));
    render_settings.AddProperty(Property("low", 0.0f));
    render_settings.AddProperty(Property("high", 1.0f));
    render_settings.AddProperty(Property("vertical_scale_1D", 30.0f));
    render_settings.AddProperty(Property("vertical_scale_2D", 15.0f));
    render_settings.AddProperty(Property("contour_level", 0.25f));
    render_settings.AddProperty(Property("cap_contour", true));
    render_settings.AddProperty(Property("invert_contour_cap", false));
    render_settings.AddProperty(Property("use_wireframe", false));
    render_settings.AddProperty(Property("show_cell_edges", false));
    render_settings.AddProperty(Property("show_bounding_box", true));
    render_settings.AddProperty(Property("show_chemical_label", true));
    render_settings.AddProperty(Property("slice_3D", true));
    render_settings.AddProperty(Property("slice_3D_axis", "axis", "z"));
    render_settings.AddProperty(Property("slice_3D_position", 0.5f)); // [0,1]
    render_settings.AddProperty(Property("show_displacement_mapped_surface", true));
    render_settings.AddProperty(Property("color_displacement_mapped_surface", false));
    render_settings.AddProperty(Property("use_image_interpolation", true));
    render_settings.AddProperty(Property("timesteps_per_render", 100));
    render_settings.AddProperty(Property("show_phase_plot", false));
    render_settings.AddProperty(Property("phase_plot_x_axis", "chemical", "a"));
    render_settings.AddProperty(Property("phase_plot_y_axis", "chemical", "b"));
    render_settings.AddProperty(Property("phase_plot_z_axis", "chemical", "c"));
}
