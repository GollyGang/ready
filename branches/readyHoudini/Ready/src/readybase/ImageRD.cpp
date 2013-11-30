/*  Copyright 2011, 2012, 2013 The Ready Bunch

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
#include "ImageRD.hpp"
#include "IO_XML.hpp"
#include "overlays.hpp"
#include "Properties.hpp"
#include "scene_items.hpp"
#include "utils.hpp"

// stdlib:
#include <math.h>
#include <stdlib.h>

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkActor.h>
#include <vtkAssignAttribute.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkContourFilter.h>
#include <vtkCubeAxesActor2D.h>
#include <vtkCubeSource.h>
#include <vtkCutter.h>
#include <vtkDataSetMapper.h>
#include <vtkExtractEdges.h>
#include <vtkFloatArray.h>
#include <vtkGeometryFilter.h>
#include <vtkImageActor.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageData.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper.h>
#include <vtkImageMirrorPad.h>
#include <vtkImageReslice.h>
#include <vtkImageThreshold.h>
#include <vtkImageToStructuredPoints.h>
#include <vtkImageWrapPad.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkMergeFilter.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkPointSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyDataNormals.h>
#include <vtkProperty.h>
#include <vtkProperty2D.h>
#include <vtkRearrangeFields.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor3D.h>
#include <vtkTextureMapToPlane.h>
#include <vtkThreshold.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWarpScalar.h>
#include <vtkWarpVector.h>
#include <vtkXMLUtilities.h>

// -------------------------------------------------------------------

ImageRD::ImageRD() 
    : xgap(5.0)
    , ygap(20.0)
    , image_top1D(2.0)
    , image_ratio1D(30.0)
{
    this->starting_pattern = vtkImageData::New();
    this->assign_attribute_filter = NULL;
    this->rearrange_fields_filter = NULL;
}

// ---------------------------------------------------------------------

ImageRD::~ImageRD()
{
    this->DeallocateImages();
    this->starting_pattern->Delete();
}

// ---------------------------------------------------------------------

void ImageRD::DeallocateImages()
{
    for(int iChem=0;iChem<(int)this->images.size();iChem++)
    {
        if(this->images[iChem])
            this->images[iChem]->Delete();
    }
}

// ---------------------------------------------------------------------

int ImageRD::GetArenaDimensionality() const
{
    assert(this->images.front());
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->images.front()->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

// ---------------------------------------------------------------------

float ImageRD::GetX() const
{
    return this->images.front()->GetDimensions()[0];
}

// ---------------------------------------------------------------------

float ImageRD::GetY() const
{
    return this->images.front()->GetDimensions()[1];
}

// ---------------------------------------------------------------------

float ImageRD::GetZ() const
{
    return this->images.front()->GetDimensions()[2];
}

// ---------------------------------------------------------------------

vtkImageData* ImageRD::GetImage(int iChemical) const
{ 
    return this->images[iChemical];
}

// ---------------------------------------------------------------------

void ImageRD::GetImage(vtkImageData *im) const
{ 
    vtkSmartPointer<vtkImageAppendComponents> iac = vtkSmartPointer<vtkImageAppendComponents>::New();
    for(int i=0;i<this->GetNumberOfChemicals();i++)
    {
        #if VTK_MAJOR_VERSION >= 6
            iac->AddInputData(this->GetImage(i));
        #else
            iac->AddInput(this->GetImage(i));
        #endif
    }
    iac->Update();
    im->DeepCopy(iac->GetOutput());
}

// ---------------------------------------------------------------------

void ImageRD::CopyFromImage(vtkImageData* im)
{
    int n_arrays = im->GetPointData()->GetNumberOfArrays();
    int n_components = im->GetNumberOfScalarComponents();

    bool has_named_arrays = true;
    for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
    {
        if(!im->GetPointData()->HasArray(GetChemicalName(iChem).c_str()))
        {
            has_named_arrays = false;
            break;
        }
    }

    if(has_named_arrays && n_components==1 && n_arrays==this->GetNumberOfChemicals())
    {
        // convert named array data to single-component data in multiple images
        for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
        {
            this->images[iChem]->SetExtent(im->GetExtent());
            this->images[iChem]->GetPointData()->SetScalars(im->GetPointData()->GetArray(GetChemicalName(iChem).c_str()));
        }
    }
    else if(n_arrays==1 && n_components==this->GetNumberOfChemicals()) 
    {
        // convert multi-component data to single-component data in multiple images
        vtkSmartPointer<vtkImageExtractComponents> iec = vtkSmartPointer<vtkImageExtractComponents>::New();
        #if VTK_MAJOR_VERSION >= 6
            iec->SetInputData(im);
        #else
            iec->SetInput(im);
        #endif
        for(int i=0;i<this->GetNumberOfChemicals();i++)
        {
            iec->SetComponents(i);
            iec->Update();
            this->images[i]->DeepCopy(iec->GetOutput());
        }
    }
    else   
        throw runtime_error("ImageRD::CopyFromImage : chemical count mismatch");

    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

void ImageRD::AllocateImages(int x,int y,int z,int nc)
{
    this->DeallocateImages();
    this->n_chemicals = nc;
    this->images.resize(nc);
    for(int i=0;i<nc;i++)
        this->images[i] = AllocateVTKImage(x,y,z);
    this->is_modified = true;
    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

/* static */ vtkImageData* ImageRD::AllocateVTKImage(int x,int y,int z)
{
    vtkImageData *im = vtkImageData::New();
    assert(im);
    #if VTK_MAJOR_VERSION >= 6
        im->SetDimensions(x,y,z);
        im->AllocateScalars(VTK_FLOAT,1);
    #else
        im->SetNumberOfScalarComponents(1);
        im->SetScalarTypeToFloat();
        im->SetDimensions(x,y,z);
        im->AllocateScalars();
    #endif
    if(im->GetDimensions()[0]!=x || im->GetDimensions()[1]!=y || im->GetDimensions()[2]!=z)
        throw runtime_error("ImageRD::AllocateVTKImage : Failed to allocate image data - dimensions too big?");
    return im;
}

// ---------------------------------------------------------------------

void ImageRD::GenerateInitialPattern()
{
    this->BlankImage();

    const int X = this->images.front()->GetDimensions()[0];
    const int Y = this->images.front()->GetDimensions()[1];
    const int Z = this->images.front()->GetDimensions()[2];

    for(int z=0;z<Z;z++)
    {
        for(int y=0;y<Y;y++)
        {
            for(int x=0;x<X;x++)
            {
                for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
                {
                    Overlay* overlay = this->initial_pattern_generator[iOverlay];

                    int iC = overlay->GetTargetChemical();
                    if(iC<0 || iC>=this->GetNumberOfChemicals())
                        continue; // best for now to silently ignore this overlay, because the user has no way of editing the overlays (short of editing the file)
                        //throw runtime_error("Overlay: chemical out of range: "+GetChemicalName(iC));

                    float *val = vtk_at(static_cast<float*>(this->GetImage(iC)->GetScalarPointer()),x,y,z,X,Y);
                    vector<float> vals(this->GetNumberOfChemicals());
                    for(int i=0;i<this->GetNumberOfChemicals();i++)
                        vals[i] = *vtk_at(static_cast<float*>(this->GetImage(i)->GetScalarPointer()),x,y,z,X,Y);
                    *val = overlay->Apply(vals,this,x,y,z);
                }
            }
        }
    }
    for(int i=0;i<(int)this->images.size();i++)
        this->images[i]->Modified();
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void ImageRD::BlankImage()
{
    for(int iImage=0;iImage<(int)this->images.size();iImage++)
    {
        this->images[iImage]->GetPointData()->GetScalars()->FillComponent(0,0.0);
        this->images[iImage]->Modified();
    }
    this->timesteps_taken = 0;
    this->undo_stack.clear();
}

// ---------------------------------------------------------------------

void ImageRD::Update(int n_steps)
{
    this->undo_stack.clear();
    this->InternalUpdate(n_steps);

    this->timesteps_taken += n_steps;

    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
        this->images[ic]->Modified();

    if(this->rearrange_fields_filter && this->assign_attribute_filter)
    {
        // manually update the rendering pipeline to allow for changing array names
        // TODO: this step shouldn't be necessary
        this->rearrange_fields_filter->Update();
        assign_attribute_filter->Assign(this->rearrange_fields_filter->GetOutput()->GetCellData()->GetArray(0)->GetName(), 
            vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeRenderPipeline(vtkRenderer* pRenderer,const Properties& render_settings)
{
    this->rearrange_fields_filter = NULL;
    this->assign_attribute_filter = NULL;

    switch(this->GetArenaDimensionality())
    {
        // TODO: merge the dimensionalities (often want one/more slices from lower dimensionalities)
        case 0:
        case 1: this->InitializeVTKPipeline_1D(pRenderer,render_settings); break;
        case 2: this->InitializeVTKPipeline_2D(pRenderer,render_settings); break;
        case 3: this->InitializeVTKPipeline_3D(pRenderer,render_settings); break;
        default:
            throw runtime_error("ImageRD::InitializeRenderPipeline : Unsupported dimensionality");
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeVTKPipeline_1D(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float vertical_scale_1D = render_settings.GetProperty("vertical_scale_1D").GetFloat();
    float r,g,b,low_hue,low_sat,low_val,high_hue,high_sat,high_val;
    render_settings.GetProperty("color_low").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&low_hue,&low_sat,&low_val);
    render_settings.GetProperty("color_high").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&high_hue,&high_sat,&high_val);
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();
    bool color_displacement_mapped_surface = render_settings.GetProperty("color_displacement_mapped_surface").GetBool();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());

    int iFirstChem=0,iLastChem=this->GetNumberOfChemicals();
    if(!show_multiple_chemicals) { iFirstChem = iActiveChemical; iLastChem = iFirstChem+1; }
    
    float scaling = vertical_scale_1D / (high-low); // vertical_scale gives the height of the graph in worldspace units
    
    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetSaturationRange(low_sat,high_sat);
    lut->SetHueRange(low_hue,high_hue);
    lut->SetValueRange(low_val,high_val);
    lut->Build();
    
    for(int iChem = iFirstChem; iChem < iLastChem; ++iChem)
    {
        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        #if VTK_MAJOR_VERSION >= 6
            image_mapper->SetInputData(this->GetImage(iChem));
        #else
            image_mapper->SetInput(this->GetImage(iChem));
        #endif
      
        // will convert the x*y 2D image to a x*y grid of quads
        const float image_height = this->GetX() / this->image_ratio1D;
        const float image_offset = this->image_top1D - image_height * 2.0f * (iChem - iFirstChem);
        vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
        plane->SetXResolution(this->GetX());
        plane->SetYResolution(this->GetY());
        plane->SetOrigin(0,image_offset-image_height,0);
        plane->SetPoint1(this->GetX(),image_offset-image_height,0);
        plane->SetPoint2(0,image_offset,0);

        vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
        texture->SetInputConnection(image_mapper->GetOutputPort());
        if(use_image_interpolation)
            texture->InterpolateOn();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(plane->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetTexture(texture);
        if(show_cell_edges)
            actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(0,0,0);
        actor->GetProperty()->LightingOff();
        pRenderer->AddActor(actor);
        
        if(this->GetNumberOfChemicals()>1)
        {
            vtkSmartPointer<vtkTextActor3D> label = vtkSmartPointer<vtkTextActor3D>::New();
            label->SetInput(GetChemicalName(iChem).c_str());
            const float text_label_offset = 20.0f;
            label->SetPosition(-text_label_offset,image_offset-image_height,0);
            label->PickableOff();
            label->SetScale(image_height/10.0);
            pRenderer->AddActor(label);
        }
    }

    // also add a scalar bar to show how the colors correspond to values
    if(show_color_scale)
    {
        AddScalarBar(pRenderer,lut);
    }

    // add a line graph for all the chemicals (active one highlighted)
    for(int iChemical=iFirstChem;iChemical<iLastChem;iChemical++)
    {
        vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
        #if VTK_MAJOR_VERSION >= 6
            plane->SetInputData(this->GetImage(iChemical));
        #else
            plane->SetInput(this->GetImage(iChemical));
        #endif
        vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
        warp->SetInputConnection(plane->GetOutputPort());
        warp->SetScaleFactor(-scaling);
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(warp->GetOutputPort());
        mapper->ScalarVisibilityOff();
        mapper->ImmediateModeRenderingOn();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetAmbient(1);
        if(iChemical==iActiveChemical)
            actor->GetProperty()->SetColor(1,1,1);
        else
            actor->GetProperty()->SetColor(0.5,0.5,0.5);
        actor->RotateX(90.0);
        actor->PickableOff();
        actor->SetPosition(0,-low*scaling+this->ygap,0);
        pRenderer->AddActor(actor);
    }
    
    // add an axis
    vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
    axis->SetCamera(pRenderer->GetActiveCamera());
    axis->SetBounds(0,0,this->ygap,(high-low)*scaling+this->ygap,0,0);
    axis->SetRanges(0,0,low,high,0,0);
    axis->UseRangesOn();
    axis->XAxisVisibilityOff();
    axis->ZAxisVisibilityOff();
    axis->SetYLabel("");
    axis->SetLabelFormat("%.2f");
    axis->SetInertia(10000);
    axis->SetCornerOffset(0);
    axis->SetNumberOfLabels(5);
    axis->PickableOff();
    pRenderer->AddActor(axis);

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        this->AddPhasePlot(pRenderer,scaling,low,high,0.0f,this->ygap*2 + scaling * (high-low),0.0f,iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeVTKPipeline_2D(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float vertical_scale_2D = render_settings.GetProperty("vertical_scale_2D").GetFloat();
    float r,g,b,low_hue,low_sat,low_val,high_hue,high_sat,high_val;
    render_settings.GetProperty("color_low").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&low_hue,&low_sat,&low_val);
    render_settings.GetProperty("color_high").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&high_hue,&high_sat,&high_val);
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    float surface_r,surface_g,surface_b;
    render_settings.GetProperty("surface_color").GetColor(surface_r,surface_g,surface_b);
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    bool show_displacement_mapped_surface = render_settings.GetProperty("show_displacement_mapped_surface").GetBool();
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool color_displacement_mapped_surface = render_settings.GetProperty("color_displacement_mapped_surface").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());
    
    float scaling = vertical_scale_2D / (high-low); // vertical_scale gives the height of the graph in worldspace units

    vtkFloatingPointType offset[3] = {0,0,0};

    int iFirstChem=0,iLastChem=this->GetNumberOfChemicals();
    if(!show_multiple_chemicals) { iFirstChem = iActiveChemical; iLastChem = iFirstChem+1; }
    
    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetSaturationRange(low_sat,high_sat);
    lut->SetHueRange(low_hue,high_hue);
    lut->SetValueRange(low_val,high_val);
    lut->Build();

    for(int iChem=iFirstChem;iChem<iLastChem;iChem++)
    {
        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        #if VTK_MAJOR_VERSION >= 6
            image_mapper->SetInputData(this->GetImage(iChem));
        #else
            image_mapper->SetInput(this->GetImage(iChem));
        #endif

        // will convert the x*y 2D image to a x*y grid of quads
        vtkSmartPointer<vtkPlaneSource> plane = vtkSmartPointer<vtkPlaneSource>::New();
        plane->SetXResolution(this->GetX());
        plane->SetYResolution(this->GetY());
        plane->SetOrigin(0,0,0);
        plane->SetPoint1(this->GetX(),0,0);
        plane->SetPoint2(0,this->GetY(),0);

        vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
        texture->SetInputConnection(image_mapper->GetOutputPort());
        if(use_image_interpolation)
            texture->InterpolateOn();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(plane->GetOutputPort());

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->SetPosition(offset);
        actor->SetTexture(texture);
        if(show_cell_edges)
            actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(0,0,0);
        actor->GetProperty()->LightingOff();
        pRenderer->AddActor(actor);

        if(show_displacement_mapped_surface)
        {
            vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
            #if VTK_MAJOR_VERSION >= 6
                plane->SetInputData(this->GetImage(iChem));
            #else
                plane->SetInput(this->GetImage(iChem));
            #endif
            vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
            warp->SetInputConnection(plane->GetOutputPort());
            warp->SetScaleFactor(scaling);
            vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
            normals->SetInputConnection(warp->GetOutputPort());
            normals->SplittingOff();
            vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            if(color_displacement_mapped_surface)
            {
                vtkSmartPointer<vtkTextureMapToPlane> tmap = vtkSmartPointer<vtkTextureMapToPlane>::New();
                tmap->SetOrigin(0,0,0);
                tmap->SetPoint1(this->GetX(),0,0);
                tmap->SetPoint2(0,this->GetY(),0);
                tmap->SetInputConnection(normals->GetOutputPort());
                mapper->SetInputConnection(tmap->GetOutputPort());
            }
            else
                mapper->SetInputConnection(normals->GetOutputPort());
            mapper->ScalarVisibilityOff();
            mapper->ImmediateModeRenderingOn();
            vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(surface_r,surface_g,surface_b);
            actor->GetProperty()->SetAmbient(0.1);
            actor->GetProperty()->SetDiffuse(0.7);
            actor->GetProperty()->SetSpecular(0.2);
            actor->GetProperty()->SetSpecularPower(3);
            if(use_wireframe)
                actor->GetProperty()->SetRepresentationToWireframe();
            actor->SetPosition(offset[0]+0.5,offset[1]+0.5+this->GetY()+this->ygap,offset[2]);
            actor->PickableOff();
            pRenderer->AddActor(actor);

            // add the color image as the texture of the displacement-mapped surface
            if(color_displacement_mapped_surface)
            {
                vtkSmartPointer<vtkTexture> texture = vtkSmartPointer<vtkTexture>::New();
                texture->SetInputConnection(image_mapper->GetOutputPort());
                if(use_image_interpolation)
                    texture->InterpolateOn();
                else
                    texture->InterpolateOn();
                actor->SetTexture(texture);
            }

            // add the bounding box
            if(show_bounding_box)
            {
                vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
                box->SetBounds(0,this->GetX()-1,0,this->GetY()-1,low*scaling,high*scaling);

                vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
                edges->SetInputConnection(box->GetOutputPort());

                vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
                mapper->SetInputConnection(edges->GetOutputPort());
                mapper->ImmediateModeRenderingOn();

                vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
                actor->SetMapper(mapper);
                actor->GetProperty()->SetColor(0,0,0);  
                actor->GetProperty()->SetAmbient(1);
                actor->SetPosition(offset[0]+0.5,offset[1]+0.5+this->GetY()+this->ygap,offset[2]);
                actor->PickableOff();

                pRenderer->AddActor(actor);
            }
        }

        // add a text label
        if(this->GetNumberOfChemicals()>1)
        {
            vtkSmartPointer<vtkTextActor3D> label = vtkSmartPointer<vtkTextActor3D>::New();
            label->SetInput(GetChemicalName(iChem).c_str());
            const float text_label_offset = 20.0f;
            label->SetPosition(offset[0]+this->GetX()/2,offset[1]-text_label_offset,offset[2]);
            label->PickableOff();
            pRenderer->AddActor(label);
        }

        offset[0] += this->GetX()+this->xgap; // the next chemical should appear further to the right
    }

    if(show_displacement_mapped_surface)
    {
        // add an axis
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(0.5,0.5,this->GetY()-0.5+this->GetY()+this->ygap,this->GetY()-0.5+this->GetY()+this->ygap,low*scaling,high*scaling);
        axis->SetRanges(0,0,0,0,low,high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel("");
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }

    // add a scalar bar to show how the colors correspond to values
    if(show_color_scale)
    {
        AddScalarBar(pRenderer,lut);
    }

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        float posY = this->ygap*2 + this->GetY();
        if(show_displacement_mapped_surface)
            posY += this->ygap + this->GetY();

        this->AddPhasePlot(pRenderer,this->GetX()/(high-low),low,high,0.0f,posY,0.0f,iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void ImageRD::InitializeVTKPipeline_3D(vtkRenderer* pRenderer,const Properties& render_settings)
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float r,g,b,low_hue,low_sat,low_val,high_hue,high_sat,high_val;
    render_settings.GetProperty("color_low").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&low_hue,&low_sat,&low_val);
    render_settings.GetProperty("color_high").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&high_hue,&high_sat,&high_val);
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();
    bool use_wireframe = render_settings.GetProperty("use_wireframe").GetBool();
    bool slice_3D = render_settings.GetProperty("slice_3D").GetBool();
    string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
    float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();
    float surface_r,surface_g,surface_b;
    render_settings.GetProperty("surface_color").GetColor(surface_r,surface_g,surface_b);
    bool show_color_scale = render_settings.GetProperty("show_color_scale").GetBool();
    bool show_cell_edges = render_settings.GetProperty("show_cell_edges").GetBool();
    bool show_bounding_box = render_settings.GetProperty("show_bounding_box").GetBool();
    bool show_phase_plot = render_settings.GetProperty("show_phase_plot").GetBool();
    int iPhasePlotX = IndexFromChemicalName(render_settings.GetProperty("phase_plot_x_axis").GetChemical());
    int iPhasePlotY = IndexFromChemicalName(render_settings.GetProperty("phase_plot_y_axis").GetChemical());
    int iPhasePlotZ = IndexFromChemicalName(render_settings.GetProperty("phase_plot_z_axis").GetChemical());

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->ImmediateModeRenderingOn();

    vtkImageData *image = this->GetImage(iActiveChemical);
    int *extent = image->GetExtent();

    // we first convert the image from point data to cell data, to match the users expectations

    vtkSmartPointer<vtkImageWrapPad> pad = vtkSmartPointer<vtkImageWrapPad>::New();
    #if VTK_MAJOR_VERSION >= 6
        pad->SetInputData(image);
    #else
        pad->SetInput(image);
    #endif
    pad->SetOutputWholeExtent(extent[0],extent[1]+1,extent[2],extent[3]+1,extent[4],extent[5]+1);

    // move the pixel values (stored in the point data) to cell data
    vtkSmartPointer<vtkRearrangeFields> prearrange_fields = vtkSmartPointer<vtkRearrangeFields>::New();
    #if VTK_MAJOR_VERSION >= 6
        prearrange_fields->SetInputData(image);
    #else
        prearrange_fields->SetInput(image);
    #endif
    prearrange_fields->AddOperation(vtkRearrangeFields::MOVE,vtkDataSetAttributes::SCALARS,
        vtkRearrangeFields::POINT_DATA,vtkRearrangeFields::CELL_DATA);

    // get the image scalars name from the first array
    prearrange_fields->Update();
    const char *scalars_array_name = prearrange_fields->GetOutput()->GetCellData()->GetArray(0)->GetName();

    // mark the new cell data array as the active attribute
    vtkSmartPointer<vtkAssignAttribute> assign_attribute = vtkSmartPointer<vtkAssignAttribute>::New();
    assign_attribute->SetInputConnection(prearrange_fields->GetOutputPort());
    assign_attribute->Assign(scalars_array_name, vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);

    // save the filters so we can perform a manual update step on the pipeline in Update() (TODO: work out how to do this properly)
    this->rearrange_fields_filter = prearrange_fields;
    this->assign_attribute_filter = assign_attribute;

    vtkSmartPointer<vtkMergeFilter> merge_datasets = vtkSmartPointer<vtkMergeFilter>::New();
    merge_datasets->SetGeometryConnection(pad->GetOutputPort());
    merge_datasets->SetScalarsConnection(assign_attribute->GetOutputPort());

    vtkSmartPointer<vtkCellDataToPointData> to_point_data = vtkSmartPointer<vtkCellDataToPointData>::New(); // (only used if needed)
    to_point_data->SetInputConnection(merge_datasets->GetOutputPort());

    if(use_image_interpolation)
    {
        // turns the 3d grid of sampled values into a polygon mesh for rendering,
        // by making a surface that contours the volume at a specified level    
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInputConnection(to_point_data->GetOutputPort());
        surface->SetValue(0,contour_level);
        mapper->SetInputConnection(surface->GetOutputPort());
        mapper->ScalarVisibilityOff();
    }
    else
    {
        // render as cubes, Minecraft-style
        vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
        threshold->SetInputConnection(merge_datasets->GetOutputPort());
        threshold->SetInputArrayToProcess(0, 0, 0,
            vtkDataObject::FIELD_ASSOCIATION_CELLS,
            vtkDataSetAttributes::SCALARS);
        threshold->ThresholdByUpper(contour_level);

        vtkSmartPointer<vtkGeometryFilter> geometry = vtkSmartPointer<vtkGeometryFilter>::New();
        geometry->SetInputConnection(threshold->GetOutputPort());

        mapper->SetInputConnection(geometry->GetOutputPort());
    }

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(surface_r,surface_g,surface_b);  
    actor->GetProperty()->SetAmbient(0.1);
    actor->GetProperty()->SetDiffuse(0.5);
    actor->GetProperty()->SetSpecular(0.4);
    actor->GetProperty()->SetSpecularPower(10);
    if(use_wireframe)
        actor->GetProperty()->SetRepresentationToWireframe();
    if(show_cell_edges && !use_image_interpolation)
    {
        actor->GetProperty()->EdgeVisibilityOn();
        actor->GetProperty()->SetEdgeColor(0,0,0);
    }
    vtkSmartPointer<vtkProperty> bfprop = vtkSmartPointer<vtkProperty>::New();
    actor->SetBackfaceProperty(bfprop);
    bfprop->SetColor(0.7,0.6,0.55);
    bfprop->SetAmbient(0.1);
    bfprop->SetDiffuse(0.5);
    bfprop->SetSpecular(0.4);
    bfprop->SetSpecularPower(10);

    // add the actor to the renderer's scene
    actor->PickableOff(); // not sure about this - sometimes it is nice to paint on the contoured surface too, for 3d sculpting
    pRenderer->AddActor(actor);

    // add the bounding box
    if(show_bounding_box)
    {
        vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
        box->SetBounds(extent[0],extent[1]+1,extent[2],extent[3]+1,extent[4],extent[5]+1);

        vtkSmartPointer<vtkExtractEdges> edges = vtkSmartPointer<vtkExtractEdges>::New();
        edges->SetInputConnection(box->GetOutputPort());

        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(edges->GetOutputPort());
        mapper->ImmediateModeRenderingOn();

        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(0,0,0);  
        actor->GetProperty()->SetAmbient(1);
        actor->PickableOff();

        pRenderer->AddActor(actor);
    }

    // add a 2D slice too
    if(slice_3D)
    {
        // create a lookup table for mapping values to colors
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->SetRampToLinear();
        lut->SetScaleToLinear();
        lut->SetTableRange(low,high);
        lut->SetSaturationRange(low_sat,high_sat);
        lut->SetHueRange(low_hue,high_hue);
        lut->SetValueRange(low_val,high_val);
        lut->Build();

        vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
        vtkFloatingPointType *bounds = image->GetBounds();
        plane->SetOrigin(slice_3D_position*(bounds[1]-bounds[0])+bounds[0],
                         slice_3D_position*(bounds[3]-bounds[2])+bounds[2],
                         slice_3D_position*(bounds[5]-bounds[4])+bounds[4]);
        if(slice_3D_axis=="x")
            plane->SetNormal(1,0,0);
        else if(slice_3D_axis=="y")
            plane->SetNormal(0,1,0);
        else
            plane->SetNormal(0,0,1);
        vtkSmartPointer<vtkCutter> cutter = vtkSmartPointer<vtkCutter>::New();
        cutter->SetCutFunction(plane);
        if(use_image_interpolation)
            cutter->SetInputConnection(to_point_data->GetOutputPort());
        else
            cutter->SetInputConnection(merge_datasets->GetOutputPort());
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(cutter->GetOutputPort());
        mapper->ImmediateModeRenderingOn();
        mapper->SetLookupTable(lut);
        mapper->UseLookupTableScalarRangeOn();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->LightingOff();
        if(show_cell_edges)
        {
            actor->GetProperty()->EdgeVisibilityOn();
            actor->GetProperty()->SetEdgeColor(0,0,0); // could be a user option
        }
        pRenderer->AddActor(actor);

        // also add a scalar bar to show how the colors correspond to values
        if(show_color_scale)
        {
            AddScalarBar(pRenderer,lut);
        }
    }

    // add a phase plot
    if(show_phase_plot && this->GetNumberOfChemicals()>=2)
    {
        this->AddPhasePlot( pRenderer,this->GetX()/(high-low),low,high,0.0f,this->GetY()+this->ygap,0.0f,
                            iPhasePlotX,iPhasePlotY,iPhasePlotZ);
    }
}

// ---------------------------------------------------------------------

void ImageRD::AddPhasePlot(vtkRenderer* pRenderer,float scaling,float low,float high,float posX,float posY,float posZ,
    int iChemX,int iChemY,int iChemZ)
{
    iChemX = max( 0, min( iChemX, this->GetNumberOfChemicals()-1 ) );
    iChemY = max( 0, min( iChemY, this->GetNumberOfChemicals()-1 ) );
    iChemZ = max( 0, min( iChemZ, this->GetNumberOfChemicals()-1 ) );

    // ensure plot points remain within a reasonable range (else get view clipping issues)
    double minVal = low-(high-low)*100.0;
    double maxVal = high+(high-low)*100.0;

    vtkSmartPointer<vtkPointSource> points = vtkSmartPointer<vtkPointSource>::New();
    points->SetNumberOfPoints(this->GetNumberOfCells());
    points->SetRadius(0);

    vtkSmartPointer<vtkImageThreshold> thresholdXmin = vtkSmartPointer<vtkImageThreshold>::New();
    #if VTK_MAJOR_VERSION >= 6
        thresholdXmin->SetInputData(this->GetImage(iChemX));
    #else
        thresholdXmin->SetInput(this->GetImage(iChemX));
    #endif
    thresholdXmin->ThresholdByLower(minVal);
    thresholdXmin->ReplaceInOn();
    thresholdXmin->SetInValue(minVal);
    thresholdXmin->ReplaceOutOff();
    vtkSmartPointer<vtkImageThreshold> thresholdXmax = vtkSmartPointer<vtkImageThreshold>::New();
    thresholdXmax->SetInputConnection(thresholdXmin->GetOutputPort());
    thresholdXmax->ThresholdByUpper(maxVal);
    thresholdXmax->ReplaceInOn();
    thresholdXmax->SetInValue(maxVal);
    thresholdXmax->ReplaceOutOff();
    
    vtkSmartPointer<vtkMergeFilter> mergeX = vtkSmartPointer<vtkMergeFilter>::New();
    mergeX->SetGeometryConnection(points->GetOutputPort());
    mergeX->SetScalarsConnection(thresholdXmax->GetOutputPort());

    vtkSmartPointer<vtkWarpScalar> warpX = vtkSmartPointer<vtkWarpScalar>::New();
    warpX->UseNormalOn();
    warpX->SetNormal(1,0,0);
    warpX->SetInputConnection(mergeX->GetOutputPort());
    warpX->SetScaleFactor(scaling);

    vtkSmartPointer<vtkImageThreshold> thresholdYmin = vtkSmartPointer<vtkImageThreshold>::New();
    #if VTK_MAJOR_VERSION >= 6
        thresholdYmin->SetInputData(this->GetImage(iChemY));
    #else
        thresholdYmin->SetInput(this->GetImage(iChemY));
    #endif
    thresholdYmin->ThresholdByLower(minVal);
    thresholdYmin->ReplaceInOn();
    thresholdYmin->SetInValue(minVal);
    thresholdYmin->ReplaceOutOff();
    vtkSmartPointer<vtkImageThreshold> thresholdYmax = vtkSmartPointer<vtkImageThreshold>::New();
    thresholdYmax->SetInputConnection(thresholdYmin->GetOutputPort());
    thresholdYmax->ThresholdByUpper(maxVal);
    thresholdYmax->ReplaceInOn();
    thresholdYmax->SetInValue(maxVal);
    thresholdYmax->ReplaceOutOff();

    vtkSmartPointer<vtkMergeFilter> mergeY = vtkSmartPointer<vtkMergeFilter>::New();
    mergeY->SetGeometryConnection(warpX->GetOutputPort());
    mergeY->SetScalarsConnection(thresholdYmax->GetOutputPort());

    vtkSmartPointer<vtkWarpScalar> warpY = vtkSmartPointer<vtkWarpScalar>::New();
    warpY->UseNormalOn();
    warpY->SetNormal(0,1,0);
    warpY->SetInputConnection(mergeY->GetOutputPort());
    warpY->SetScaleFactor(scaling);

    vtkSmartPointer<vtkVertexGlyphFilter> glyph = vtkSmartPointer<vtkVertexGlyphFilter>::New();

    float offsetZ = 0.0f;
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkImageThreshold> thresholdZmin = vtkSmartPointer<vtkImageThreshold>::New();
        #if VTK_MAJOR_VERSION >= 6
            thresholdZmin->SetInputData(this->GetImage(iChemZ));
        #else
            thresholdZmin->SetInput(this->GetImage(iChemZ));
        #endif
        thresholdZmin->ThresholdByLower(minVal);
        thresholdZmin->ReplaceInOn();
        thresholdZmin->SetInValue(minVal);
        thresholdZmin->ReplaceOutOff();
        vtkSmartPointer<vtkImageThreshold> thresholdZmax = vtkSmartPointer<vtkImageThreshold>::New();
        thresholdZmax->SetInputConnection(thresholdZmin->GetOutputPort());
        thresholdZmax->ThresholdByUpper(maxVal);
        thresholdZmax->ReplaceInOn();
        thresholdZmax->SetInValue(maxVal);
        thresholdZmax->ReplaceOutOff();

        vtkSmartPointer<vtkMergeFilter> mergeZ = vtkSmartPointer<vtkMergeFilter>::New();
        mergeZ->SetGeometryConnection(warpY->GetOutputPort());
        mergeZ->SetScalarsConnection(thresholdZmax->GetOutputPort());

        vtkSmartPointer<vtkWarpScalar> warpZ = vtkSmartPointer<vtkWarpScalar>::New();
        warpZ->UseNormalOn();
        warpZ->SetNormal(0,0,1);
        warpZ->SetInputConnection(mergeZ->GetOutputPort());
        warpZ->SetScaleFactor(scaling);

        glyph->SetInputConnection(warpZ->GetOutputPort());

        offsetZ = low*scaling;
    }
    else
    {
        glyph->SetInputConnection(warpY->GetOutputPort());
    }

    vtkSmartPointer<vtkTransform> trans = vtkSmartPointer<vtkTransform>::New();
    trans->Scale(1,1,-1);
    vtkSmartPointer<vtkTransformFilter> transFilter = vtkSmartPointer<vtkTransformFilter>::New();
    transFilter->SetTransform(trans);
    transFilter->SetInputConnection(glyph->GetOutputPort());

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(transFilter->GetOutputPort());
    mapper->ScalarVisibilityOff();
    mapper->ImmediateModeRenderingOn();
    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetAmbient(1);
    actor->GetProperty()->SetPointSize(1);
    actor->PickableOff();
    actor->SetPosition(posX-low*scaling,posY-low*scaling,posZ+offsetZ);
    pRenderer->AddActor(actor);

    // also add the axes
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX+scaling*(high-low),posY,posY,posZ,posZ);
        axis->SetRanges(low,high,0,0,0,0);
        axis->UseRangesOn();
        axis->YAxisVisibilityOff();
        axis->ZAxisVisibilityOff();
        axis->SetXLabel(GetChemicalName(iChemX).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX,posY,posY+(high-low)*scaling,posZ,posZ);
        axis->SetRanges(0,0,low,high,0,0);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->ZAxisVisibilityOff();
        axis->SetYLabel(GetChemicalName(iChemY).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
    if(this->GetNumberOfChemicals()>2)
    {
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(posX,posX,posY,posY,posZ,posZ-scaling*(high-low));
        axis->SetRanges(0,0,0,0,low,high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel(GetChemicalName(iChemZ).c_str());
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        axis->PickableOff();
        pRenderer->AddActor(axis);
    }
}

// ---------------------------------------------------------------------

void ImageRD::SaveStartingPattern()
{
    this->GetImage(this->starting_pattern);
}

// ---------------------------------------------------------------------

void ImageRD::RestoreStartingPattern()
{
    this->CopyFromImage(this->starting_pattern);
    this->timesteps_taken = 0;
}

// ---------------------------------------------------------------------

void ImageRD::SetDimensions(int x, int y, int z)
{
    this->AllocateImages(x,y,z,this->GetNumberOfChemicals());
}

// ---------------------------------------------------------------------

void ImageRD::SetNumberOfChemicals(int n)
{
    this->AllocateImages(this->GetX(),this->GetY(),this->GetZ(),n);
}

// ---------------------------------------------------------------------

void ImageRD::SetDimensionsAndNumberOfChemicals(int x,int y,int z,int nc)
{
    this->AllocateImages(x,y,z,nc);
}

// ---------------------------------------------------------------------

int ImageRD::GetNumberOfCells() const
{
    return this->images.front()->GetDimensions()[0] * 
        this->images.front()->GetDimensions()[1] * 
        this->images.front()->GetDimensions()[2];
}

// ---------------------------------------------------------------------

void ImageRD::GetAsMesh(vtkPolyData *out, const Properties &render_settings) const
{
    bool use_image_interpolation = render_settings.GetProperty("use_image_interpolation").GetBool();
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    float contour_level = render_settings.GetProperty("contour_level").GetFloat();

    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float vertical_scale_1D = render_settings.GetProperty("vertical_scale_1D").GetFloat();
    float vertical_scale_2D = render_settings.GetProperty("vertical_scale_2D").GetFloat();

    switch(this->GetArenaDimensionality())
    {
        case 1:
            {
                float scaling = vertical_scale_1D / (high-low); // vertical_scale gives the height of the graph in worldspace units

                vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
                #if VTK_MAJOR_VERSION >= 6
                    plane->SetInputData(this->GetImage(iActiveChemical));
                #else
                    plane->SetInput(this->GetImage(iActiveChemical));
                #endif
                vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
                warp->SetInputConnection(plane->GetOutputPort());
                warp->SetScaleFactor(-scaling);
                warp->Update();
                out->DeepCopy(warp->GetOutput());
            }
            break;
        case 2:
            {
                float scaling = vertical_scale_2D / (high-low); // vertical_scale gives the height of the graph in worldspace units

                vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
                #if VTK_MAJOR_VERSION >= 6
                    plane->SetInputData(this->GetImage(iActiveChemical));
                #else
                    plane->SetInput(this->GetImage(iActiveChemical));
                #endif
                vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
                warp->SetInputConnection(plane->GetOutputPort());
                warp->SetScaleFactor(scaling);
                vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
                normals->SetInputConnection(warp->GetOutputPort());
                normals->SplittingOff();
                normals->Update();
                out->DeepCopy(normals->GetOutput());
            }
            break;
        case 3:
            if(use_image_interpolation)
            {
                // turns the 3d grid of sampled values into a polygon mesh for rendering,
                // by making a surface that contours the volume at a specified level    
                vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
                #if VTK_MAJOR_VERSION >= 6
                    surface->SetInputData(this->GetImage(iActiveChemical));
                #else
                    surface->SetInput(this->GetImage(iActiveChemical));
                #endif
                surface->SetValue(0, contour_level);
                surface->Update();
                out->DeepCopy(surface->GetOutput());
            }
            else
            {
                // render as cubes, Minecraft-style
                vtkImageData *image = this->GetImage(iActiveChemical);
                int *extent = image->GetExtent();

                vtkSmartPointer<vtkImageWrapPad> pad = vtkSmartPointer<vtkImageWrapPad>::New();
                #if VTK_MAJOR_VERSION >= 6
                    pad->SetInputData(image);
                #else
                    pad->SetInput(image);
                #endif
                pad->SetOutputWholeExtent(extent[0],extent[1]+1,extent[2],extent[3]+1,extent[4],extent[5]+1);
                pad->Update();
                pad->GetOutput()->GetCellData()->SetScalars(image->GetPointData()->GetScalars()); // a non-pipelined operation

                vtkSmartPointer<vtkThreshold> threshold = vtkSmartPointer<vtkThreshold>::New();
                threshold->SetInputConnection(pad->GetOutputPort());
                threshold->SetInputArrayToProcess(0, 0, 0,
                    vtkDataObject::FIELD_ASSOCIATION_CELLS,
                    vtkDataSetAttributes::SCALARS);
                threshold->ThresholdByUpper(contour_level);

                vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
                transform->Translate (-.5, -.5, -.5);
                vtkSmartPointer<vtkTransformFilter> transformModel = vtkSmartPointer<vtkTransformFilter>::New();
                transformModel->SetTransform(transform);
                transformModel->SetInputConnection(threshold->GetOutputPort());

                vtkSmartPointer<vtkGeometryFilter> geometry = vtkSmartPointer<vtkGeometryFilter>::New();
                geometry->SetInputConnection(transformModel->GetOutputPort());
                geometry->Update();

                out->DeepCopy(geometry->GetOutput());
            }
            break;
    }
}

// ---------------------------------------------------------------------

void ImageRD::SaveFile(const char* filename,const Properties& render_settings,bool generate_initial_pattern_when_loading) const
{
    // convert the image to named arrays
    vtkSmartPointer<vtkImageData> im = vtkSmartPointer<vtkImageData>::New();
    im->DeepCopy(this->images.front());
    im->GetPointData()->SetScalars(NULL);
    for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
    {
        vtkSmartPointer<vtkFloatArray> fa = vtkSmartPointer<vtkFloatArray>::New();
        fa->DeepCopy(this->images[iChem]->GetPointData()->GetScalars());
        fa->SetName(GetChemicalName(iChem).c_str());
        im->GetPointData()->AddArray(fa);
    }

    vtkSmartPointer<RD_XMLImageWriter> iw = vtkSmartPointer<RD_XMLImageWriter>::New();
    iw->SetSystem(this);
    iw->SetRenderSettings(&render_settings);
    if(generate_initial_pattern_when_loading)
        iw->GenerateInitialPatternWhenLoading();
    iw->SetFileName(filename);
    iw->SetDataModeToBinary(); // (to match MeshRD::SaveFile())
    #if VTK_MAJOR_VERSION >= 6
        iw->SetInputData(im);
    #else
        iw->SetInput(im);
    #endif
    iw->Write();
}

// --------------------------------------------------------------------------------

void ImageRD::GetAs2DImage(vtkImageData *out,const Properties& render_settings) const
{
    float low = render_settings.GetProperty("low").GetFloat();
    float high = render_settings.GetProperty("high").GetFloat();
    float r,g,b,low_hue,low_sat,low_val,high_hue,high_sat,high_val;
    render_settings.GetProperty("color_low").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&low_hue,&low_sat,&low_val);
    render_settings.GetProperty("color_high").GetColor(r,g,b);
    vtkMath::RGBToHSV(r,g,b,&high_hue,&high_sat,&high_val);
    int iActiveChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    
    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetSaturationRange(low_sat,high_sat);
    lut->SetHueRange(low_hue,high_hue);
    lut->SetValueRange(low_val,high_val);

    // pass the image through the lookup table
    vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
    image_mapper->SetLookupTable(lut);
    image_mapper->SetOutputFormatToRGB(); // without this, vtkJPEGWriter writes JPEGs that some software struggles with
    switch(this->GetArenaDimensionality())
    {
        case 1: 
        case 2:
            #if VTK_MAJOR_VERSION >= 6
                image_mapper->SetInputData(this->GetImage(iActiveChemical));
            #else
                image_mapper->SetInput(this->GetImage(iActiveChemical));
            #endif
            break;
        case 3:
            {
                string slice_3D_axis = render_settings.GetProperty("slice_3D_axis").GetAxis();
                float slice_3D_position = render_settings.GetProperty("slice_3D_position").GetFloat();

                // Matrices for axial, coronal, sagittal, oblique view orientations
                static double sagittalElements[16] = { // x
                       0, 0,-1, 0,
                       1, 0, 0, 0,
                       0,-1, 0, 0,
                       0, 0, 0, 1 };
                static double coronalElements[16] = { // y
                         1, 0, 0, 0,
                         0, 0, 1, 0,
                         0,-1, 0, 0,
                         0, 0, 0, 1 };
                static double axialElements[16] = { // z
                         1, 0, 0, 0,
                         0, 1, 0, 0,
                         0, 0, 1, 0,
                         0, 0, 0, 1 };
                /*static double obliqueElements[16] = { // could get fancy and have slanting slices
                         1, 0, 0, 0,
                         0, 0.866025, -0.5, 0,
                         0, 0.5, 0.866025, 0,
                         0, 0, 0, 1 };*/
                // Set the slice orientation
                vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
                if(slice_3D_axis=="x")
                    resliceAxes->DeepCopy(sagittalElements);
                else if(slice_3D_axis=="y")
                    resliceAxes->DeepCopy(coronalElements);
                else if(slice_3D_axis=="z")
                    resliceAxes->DeepCopy(axialElements);
                resliceAxes->SetElement(0, 3, slice_3D_position * this->GetX());
                resliceAxes->SetElement(1, 3, slice_3D_position * this->GetY());
                resliceAxes->SetElement(2, 3, slice_3D_position * this->GetZ());

                vtkSmartPointer<vtkImageReslice> voi = vtkSmartPointer<vtkImageReslice>::New();
                #if VTK_MAJOR_VERSION >= 6
                    voi->SetInputData(this->GetImage(iActiveChemical));
                #else
                    voi->SetInput(this->GetImage(iActiveChemical));
                #endif
                voi->SetOutputDimensionality(2);
                voi->SetResliceAxes(resliceAxes);
                image_mapper->SetInputConnection(voi->GetOutputPort());
            };
    }
    image_mapper->Update();

    out->DeepCopy(image_mapper->GetOutput());
}

// --------------------------------------------------------------------------------

float ImageRD::GetValue(float x,float y,float z,const Properties& render_settings)
{
    const int X = this->images.front()->GetDimensions()[0];
    const int Y = this->images.front()->GetDimensions()[1];
    const int Z = this->images.front()->GetDimensions()[2];

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals && this->GetArenaDimensionality()==1)
    {
        // detect which chemical was drawn on from the click position
        const double image_height = X / this->image_ratio1D;
        iChemical = int(floor((- y + this->image_top1D + image_height)/(image_height*2))); 
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
    }
    else if(show_multiple_chemicals && this->GetArenaDimensionality()==2)
    {
        // detect which chemical was drawn on from the click position
        iChemical = int(floor((x+this->xgap/2)/(X+this->xgap))); 
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X+this->xgap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    int ix,iy,iz;
    ix = int(floor(x-offset_x));
    iy = int(floor(y));
    iz = int(floor(z));
    ix = min(X-1,max(0,ix));
    iy = min(Y-1,max(0,iy));
    iz = min(Z-1,max(0,iz));

    return *vtk_at(static_cast<float*>(this->GetImage(iChemical)->GetScalarPointer()),ix,iy,iz,X,Y);
}

// --------------------------------------------------------------------------------

void ImageRD::SetValue(float x,float y,float z,float val,const Properties& render_settings)
{
    const int X = this->images.front()->GetDimensions()[0];
    const int Y = this->images.front()->GetDimensions()[1];
    const int Z = this->images.front()->GetDimensions()[2];

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals && this->GetArenaDimensionality()==1)
    {
        // detect which chemical was drawn on from the click position
        const double image_height = X / this->image_ratio1D;
        iChemical = int(floor((- y + this->image_top1D + image_height)/(image_height*2))); 
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
    }
    else if(show_multiple_chemicals && this->GetArenaDimensionality()==2)
    {
        // detect which chemical was drawn on from the click position
        iChemical = int(floor((x+this->xgap/2)/(X+this->xgap))); 
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X+this->xgap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    int ix,iy,iz;
    ix = int(floor(x-offset_x));
    iy = int(floor(y));
    iz = int(floor(z));
    ix = min(X-1,max(0,ix));
    iy = min(Y-1,max(0,iy));
    iz = min(Z-1,max(0,iz));

    float *pCell = vtk_at(static_cast<float*>(this->GetImage(iChemical)->GetScalarPointer()),ix,iy,iz,X,Y);
    int iCell = pCell - static_cast<float*>(this->GetImage(iChemical)->GetScalarPointer());
    this->StorePaintAction(iChemical,iCell,*pCell);
    *pCell = val;
    this->images[iChemical]->Modified();
}

// --------------------------------------------------------------------------------

void ImageRD::SetValuesInRadius(float x,float y,float z,float r,float val,const Properties& render_settings)
{
    const int X = this->images.front()->GetDimensions()[0];
    const int Y = this->images.front()->GetDimensions()[1];
    const int Z = this->images.front()->GetDimensions()[2];

    // which chemical was clicked-on?
    float offset_x = 0.0f;
    bool show_multiple_chemicals = render_settings.GetProperty("show_multiple_chemicals").GetBool();
    int iChemical;
    if(show_multiple_chemicals && this->GetArenaDimensionality()==1)
    {
        // detect which chemical was drawn on from the click position
        const double image_height = X / this->image_ratio1D;
        iChemical = int(floor((- y + this->image_top1D + image_height)/(image_height*2))); 
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
    }
    else if(show_multiple_chemicals && this->GetArenaDimensionality()==2)
    {
        // detect which chemical was drawn on from the click position
        iChemical = int(floor((x+this->xgap/2)/(X+this->xgap))); 
        iChemical = min(this->GetNumberOfChemicals()-1,max(0,iChemical)); // clamp to allowed range (just in case)
        offset_x = iChemical * (X+this->xgap);
    }
    else
    {
        // only one chemical is shown, must be that one
        iChemical = IndexFromChemicalName(render_settings.GetProperty("active_chemical").GetChemical());
    }

    double *dataset_bbox = this->images.front()->GetBounds();
    r *= hypot3(dataset_bbox[1]-dataset_bbox[0],dataset_bbox[3]-dataset_bbox[2],dataset_bbox[5]-dataset_bbox[4]);

    int ix,iy,iz;
    ix = int(floor(x-offset_x));
    iy = int(floor(y));
    iz = int(floor(z));
    ix = min(X-1,max(0,ix));
    iy = min(Y-1,max(0,iy));
    iz = min(Z-1,max(0,iz));

    for(int tz=max(0,int(iz-r));tz<=min(Z-1,int(iz+r));tz++)
    {
        for(int ty=max(0,int(iy-r));ty<=min(Y-1,int(iy+r));ty++)
        {
            for(int tx=max(0,int(ix-r));tx<=min(X-1,int(ix+r));tx++)
            {
                if(hypot3(ix-tx,iy-ty,iz-tz)<r)
                {
                    float *pCell = vtk_at(static_cast<float*>(this->GetImage(iChemical)->GetScalarPointer()),tx,ty,tz,X,Y);
                    int iCell = pCell - static_cast<float*>(this->GetImage(iChemical)->GetScalarPointer());
                    this->StorePaintAction(iChemical,iCell,*pCell);
                    *pCell = val;
                }
            }
        }
    }
    this->images[iChemical]->Modified();
}

// --------------------------------------------------------------------------------

void ImageRD::GetFromOpenCLBuffers( float* dest, int chemical_id )
{
	//do nothing for now, TODO, provide an implementation.
}

// --------------------------------------------------------------------------------

void ImageRD::FlipPaintAction(PaintAction& cca)
{
    float *pCell = static_cast<float*>(this->GetImage(cca.iChemical)->GetScalarPointer()) + cca.iCell;
    float old_val = *pCell;
    *pCell = cca.val;
    cca.val = old_val;
    cca.done = !cca.done;
    this->images[cca.iChemical]->Modified();
}

// --------------------------------------------------------------------------------
