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

// STL:
#include <iostream>
using namespace std;

// stdlib:
#include <stdlib.h>

// readybase:
#include <SystemFactory.hpp>
#include <Properties.hpp>
#include <OpenCL_utils.hpp>
#include <AbstractRD.hpp>

// -------------------------------------------------------------------------------------------------------------

void InitializeDefaultRenderSettings(Properties &render_settings);

// -------------------------------------------------------------------------------------------------------------
/*
        A demonstration of using Ready as a processing back-end.

        Currently it just loads a file, runs it for 1000 timesteps and then saves out the result. If we actually want
        this utility to do something useful then we can add more command-line options.
*/
// -------------------------------------------------------------------------------------------------------------

int main(int argc,char *argv[])
{
    if(argc!=3)
    {
        cout << "A command-line utility to run Ready patterns without the GUI.\nUsage:   " << argv[0] << " <input_file> <output_file>\n";
        return EXIT_FAILURE;
    }

    bool is_opencl_available = OpenCL_utils::IsOpenCLAvailable();
    if(is_opencl_available)
        cout << "OpenCL found.\n";
    else
        cout << "OpenCL not found.\n";
    int opencl_platform = 0; // TODO: command-line option
    int opencl_device = 0; // TODO: command-line option

    Properties render_settings("render_settings");
    InitializeDefaultRenderSettings(render_settings);

    AbstractRD *system;
    try 
    {
        // read the file
        cout << "Loading file...\n";
        bool warn_to_update;
        system = SystemFactory::CreateFromFile(argv[1],is_opencl_available,opencl_platform,opencl_device,render_settings,warn_to_update);
        if(warn_to_update)
            cout << "This pattern was created with a newer version of Ready. You should update your copy.\n";

        // do something with the file
        cout << "Running the simulation for 1000 steps...\n";
        system->Update(1000); // TODO: command-line option

        // save something out
        cout << "Saving file...\n";
        system->SaveFile(argv[2],render_settings);
    }
    catch(const exception& e)
    {
        cout << "Error:\n" << e.what() << "\n";
        return EXIT_FAILURE;
    }

    delete system;
    return EXIT_SUCCESS;
}

// -------------------------------------------------------------------------------------------------------------

void InitializeDefaultRenderSettings(Properties &render_settings)
{
    // TODO: code duplication here from frame.cpp, not sure how best to merge
    render_settings.DeleteAllProperties();
    render_settings.AddProperty(Property("surface_color","color",1.0f,1.0f,1.0f)); // RGB [0,1]
    render_settings.AddProperty(Property("color_low","color",0.0f,0.0f,1.0f));
    render_settings.AddProperty(Property("color_high","color",1.0f,0.0f,0.0f));
    render_settings.AddProperty(Property("show_color_scale",true));
    render_settings.AddProperty(Property("show_multiple_chemicals",true));
    render_settings.AddProperty(Property("active_chemical","chemical","a"));
    render_settings.AddProperty(Property("low",0.0f));
    render_settings.AddProperty(Property("high",1.0f));
    render_settings.AddProperty(Property("vertical_scale_1D",30.0f));
    render_settings.AddProperty(Property("vertical_scale_2D",15.0f));
    render_settings.AddProperty(Property("contour_level",0.25f));
    render_settings.AddProperty(Property("use_wireframe",false));
    render_settings.AddProperty(Property("show_cell_edges",false));
    render_settings.AddProperty(Property("show_bounding_box",true));
    render_settings.AddProperty(Property("slice_3D",true));
    render_settings.AddProperty(Property("slice_3D_axis","axis","z"));
    render_settings.AddProperty(Property("slice_3D_position",0.5f)); // [0,1]
    render_settings.AddProperty(Property("show_displacement_mapped_surface",true));
    render_settings.AddProperty(Property("color_displacement_mapped_surface",true));
    render_settings.AddProperty(Property("use_image_interpolation",true));
    render_settings.AddProperty(Property("timesteps_per_render",100));
}

// -------------------------------------------------------------------------------------------------------------
