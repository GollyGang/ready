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

// VTK:
#include <vtkSmartPointer.h>
#include <vtkXMLGenericDataObjectReader.h>
#include <vtkImageData.h>
#include <vtkPointData.h>

// readybase:
#include <AbstractRD.hpp>
#include <OpenCL_utils.hpp>
#include <IO_XML.hpp>
#include <Properties.hpp>
#include <GrayScottImageRD.hpp>
#include <FormulaOpenCLImageRD.hpp>
#include <FullKernelOpenCLImageRD.hpp>
#include <GrayScottMeshRD.hpp>
#include <FormulaOpenCLMeshRD.hpp>
#include <FullKernelOpenCLMeshRD.hpp>

// -------------------------------------------------------------------------------------------------------------

void InitializeDefaultRenderSettings(Properties &render_settings);
void ReadFromFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                  AbstractRD *&system,Properties &render_settings);
void ReadFromImageDataFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                           AbstractRD *&system,Properties &render_settings);
void ReadFromUnstructuredGridFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                                  AbstractRD *&system,Properties &render_settings);

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
        cout << "A command-line utility to run Ready patterns without the GUI.\n\nUsage:\n\n" << argv[0] << " <input_file> <output_file>\n";
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

    // read the file
    AbstractRD *system;
    cout << "Loading file...\n";
    try {
        ReadFromFile(argv[1],is_opencl_available,opencl_platform,opencl_device,system,render_settings);
    }
    catch(const exception& e)
    {
        cout << "Error loading file:\n" << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // do something with the file
    cout << "Running the simulation for 1000 steps...\n";
    try {
        system->Update(1000); // TODO: command-line option
    }
    catch(const exception& e)
    {
        cout << "Error running simulation:\n" << e.what() << "\n";
        return EXIT_FAILURE;
    }

    // save something out
    cout << "Saving file...\n";
    try {
        system->SaveFile(argv[2],render_settings);
    }
    catch(const exception& e)
    {
        cout << "Error saving file:\n" << e.what() << "\n";
        return EXIT_FAILURE;
    }

    delete system;
}

// -------------------------------------------------------------------------------------------------------------

void ReadFromFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                  AbstractRD *&system,Properties &render_settings)
{
    // TODO: code duplication in these Read*() functions from frame.cpp, should merge
    vtkSmartPointer<vtkXMLGenericDataObjectReader> generic_reader = vtkSmartPointer<vtkXMLGenericDataObjectReader>::New();
    bool parallel;
    int data_type = generic_reader->ReadOutputType(filename,parallel);
    switch(data_type)
    {
        case VTK_IMAGE_DATA: 
            ReadFromImageDataFile(filename,is_opencl_available,opencl_platform,opencl_device,
                                  system,render_settings); 
            break;
        case VTK_UNSTRUCTURED_GRID: 
            ReadFromUnstructuredGridFile(filename,is_opencl_available,opencl_platform,opencl_device,
                                         system,render_settings); 
            break;
        default: 
            throw runtime_error("Unsupported data type or file read error");
    }
}

// -------------------------------------------------------------------------------------------------------------

void ReadFromImageDataFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                           AbstractRD *&system,Properties &render_settings)
{
    vtkSmartPointer<RD_XMLImageReader> reader = vtkSmartPointer<RD_XMLImageReader>::New();
    reader->SetFileName(filename);
    reader->Update();
    vtkImageData *image = reader->GetOutput();

    string type = reader->GetType();
    string name = reader->GetName();

    ImageRD* image_system;
    if(type=="inbuilt")
    {
        if(name=="Gray-Scott")
            image_system = new GrayScottImageRD();
        else 
            throw runtime_error("Unsupported inbuilt implementation: "+name);
    }
    else if(type=="formula")
    {
        if(!is_opencl_available) 
            throw runtime_error("Pattern requires OpenCL");
        image_system = new FormulaOpenCLImageRD(opencl_platform,opencl_device);
    }
    else if(type=="kernel")
    {
        if(!is_opencl_available) 
            throw runtime_error("Pattern requires OpenCL");
        image_system = new FullKernelOpenCLImageRD(opencl_platform,opencl_device);
    }
    else throw runtime_error("Unsupported rule type: "+type);
    bool warn_to_update;
    image_system->InitializeFromXML(reader->GetRDElement(),warn_to_update);
    if(warn_to_update)
        cout << "(This file is from a more recent version of Ready. You should download a newer version.)\n";

    // render settings
    vtkSmartPointer<vtkXMLDataElement> xml_render_settings = reader->GetRDElement()->FindNestedElementWithName("render_settings");
    if(xml_render_settings) // optional
        render_settings.OverwriteFromXML(xml_render_settings);

    int dim[3];
    image->GetDimensions(dim);
    int nc = image->GetNumberOfScalarComponents() * image->GetPointData()->GetNumberOfArrays();
    image_system->SetDimensions(dim[0],dim[1],dim[2]);
    image_system->SetNumberOfChemicals(nc);
    if(reader->ShouldGenerateInitialPatternWhenLoading())
        image_system->GenerateInitialPattern();
    else
        image_system->CopyFromImage(image);
    system = image_system;
}

// -------------------------------------------------------------------------------------------------------------

void ReadFromUnstructuredGridFile(const char *filename,bool is_opencl_available,int opencl_platform,int opencl_device,
                                  AbstractRD *&system,Properties &render_settings)
{
    vtkSmartPointer<RD_XMLUnstructuredGridReader> reader = vtkSmartPointer<RD_XMLUnstructuredGridReader>::New();
    reader->SetFileName(filename);
    reader->Update();
    vtkUnstructuredGrid *ugrid = reader->GetOutput();

    string type = reader->GetType();
    string name = reader->GetName();

    MeshRD* mesh_system;
    if(type=="inbuilt")
    {
        if(name=="Gray-Scott")
            mesh_system = new GrayScottMeshRD();
        else 
            throw runtime_error("Unsupported inbuilt implementation: "+name);
    }
    else if(type=="formula")
    {
        if(!is_opencl_available) 
            throw runtime_error("Pattern requires OpenCL");
        mesh_system = new FormulaOpenCLMeshRD(opencl_platform,opencl_device);
    }
    else if(type=="kernel")
    {
        if(!is_opencl_available) 
            throw runtime_error("Pattern requires OpenCL");
        mesh_system = new FullKernelOpenCLMeshRD(opencl_platform,opencl_device);
    }
    else throw runtime_error("Unsupported rule type: "+type);

    bool warn_to_update;
    mesh_system->InitializeFromXML(reader->GetRDElement(),warn_to_update);
    if(warn_to_update)
        cout << "(This file is from a more recent version of Ready. You should download a newer version.)\n";

    mesh_system->CopyFromMesh(ugrid);
    // render settings
    vtkSmartPointer<vtkXMLDataElement> xml_render_settings = reader->GetRDElement()->FindNestedElementWithName("render_settings");
    if(xml_render_settings) // optional
        render_settings.OverwriteFromXML(xml_render_settings);

    if(reader->ShouldGenerateInitialPatternWhenLoading())
        mesh_system->GenerateInitialPattern();

    system = mesh_system;
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
