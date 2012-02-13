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

// local:
#include "vtk_pipeline.hpp"
#include "wxVTKRenderWindowInteractor.h"

// readybase:
#include "BaseRD.hpp"

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
#include <vtkWarpScalar.h>
#include <vtkImageDataGeometryFilter.h>
#include <vtkPolyDataNormals.h>
#include <vtkImageMirrorPad.h>
#include <vtkCubeAxesActor2D.h>
// volume rendering
#include <vtkVolumeRayCastCompositeFunction.h>
#include <vtkVolumeRayCastMapper.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolumeProperty.h>
#include <vtkVolume.h>
#include <vtkExtractVOI.h>

// STL:
#include <stdexcept>
using namespace std;

Properties::Properties(vtkXMLDataElement *node) : XML_Object(node)
{    
    for(int i=0;i<node->GetNumberOfNestedElements();i++)
    {
        vtkXMLDataElement *prop = node->GetNestedElement(i);
        string prop_name,prop_type;
        read_required_attribute(prop,"name",prop_name);
        read_required_attribute(prop,"type",prop_type);
        if(prop_type=="float")
        {
            float f;
            read_required_attribute(prop,"value",f);
            this->float_properties[prop_name] = f;
        }
        else if(prop_type=="int")
        {
            int i;
            read_required_attribute(prop,"value",i);
            this->int_properties[prop_name] = i;
        }
        else if(prop_type=="bool")
        {
            int i;
            read_required_attribute(prop,"value",i);
            this->bool_properties[prop_name] = (i==1);
        }
        else throw runtime_error("Properties : unrecognised type: "+prop_type);
    }
}

vtkSmartPointer<vtkXMLDataElement> Properties::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> root = vtkSmartPointer<vtkXMLDataElement>::New();
    root->SetName(this->name.c_str());
    for(map<string,float>::const_iterator it=this->float_properties.begin();it!=this->float_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("property");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","float");
        node->SetFloatAttribute("value",it->second);
        root->AddNestedElement(node);
    }
    for(map<string,int>::const_iterator it=this->int_properties.begin();it!=this->int_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("property");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","int");
        node->SetIntAttribute("value",it->second);
        root->AddNestedElement(node);
    }
    for(map<string,bool>::const_iterator it=this->bool_properties.begin();it!=this->bool_properties.end();it++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = vtkSmartPointer<vtkXMLDataElement>::New();
        node->SetName("property");
        node->SetAttribute("name",it->first.c_str());
        node->SetAttribute("type","bool");
        node->SetIntAttribute("value",it->second?1:0);
        root->AddNestedElement(node);
    }
    return root;
}

float Properties::GetFloat(const std::string &name) const
{
    if(this->float_properties.count(name)) 
        return this->float_properties.find(name)->second;
    throw runtime_error("Properties::GetFloat: unknown property: "+name);
}

int Properties::GetInt(const std::string &name) const
{
    if(this->int_properties.count(name)) 
        return this->int_properties.find(name)->second;
    throw runtime_error("Properties::GetInt: unknown property: "+name);
}

bool Properties::GetBool(const std::string &name) const
{
    if(this->bool_properties.count(name)) 
        return this->bool_properties.find(name)->second;
    throw runtime_error("Properties::GetBool: unknown property: "+name);
}

// TODO: allow visualizations that use more than one chemical

void InitializeVTKPipeline_1D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings);
void InitializeVTKPipeline_2D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings);
void InitializeVTKPipeline_3D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings);

void InitializeVTKPipeline(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings)
{
    assert(pVTKWindow);
    assert(system);

    switch(system->GetDimensionality())
    {
        // TODO: merge the dimensionalities (often want one/more slices from lower dimensionalities)
        case 1: InitializeVTKPipeline_1D(pVTKWindow,system,render_settings); break;
        case 2: InitializeVTKPipeline_2D(pVTKWindow,system,render_settings); break;
        case 3: InitializeVTKPipeline_3D(pVTKWindow,system,render_settings); break;
        default:
            throw runtime_error("InitializeVTKPipeline : Unsupported dimensionality");
    }
    pVTKWindow->Refresh(false);
}

void InitializeVTKPipeline_1D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings)
{
    float low = render_settings.GetFloat("low");
    float high = render_settings.GetFloat("high");
    float vertical_scale_1D = render_settings.GetFloat("vertical_scale_1D");
    float vertical_scale_2D = render_settings.GetFloat("vertical_scale_2D");
    float hue_low = render_settings.GetFloat("hue_low");
    float hue_high = render_settings.GetFloat("hue_high");
    bool use_image_interpolation = render_settings.GetBool("use_image_interpolation");
    int iActiveChemical = render_settings.GetInt("iActiveChemical");
    float contour_level = render_settings.GetFloat("contour_level");
    
    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    pVTKWindow->GetRenderWindow()->GetRenderers()->RemoveAllItems();
    pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    // create a lookup table for mapping values to colors
    vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
    lut->SetRampToLinear();
    lut->SetScaleToLinear();
    lut->SetTableRange(low,high);
    lut->SetHueRange(hue_low,hue_high);

    // pad the image a little so we can actually see it as a 2D strip rather than being invisible
    vtkSmartPointer<vtkImageMirrorPad> pad = vtkSmartPointer<vtkImageMirrorPad>::New();
    pad->SetInput(system->GetImage(iActiveChemical));
    pad->SetOutputWholeExtent(0,system->GetX()-1,-1,system->GetY(),0,system->GetZ()-1);

    // pass the image through the lookup table
    vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
    image_mapper->SetLookupTable(lut);
    image_mapper->SetInputConnection(pad->GetOutputPort());
  
    // an actor determines how a scene object is displayed
    vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
    actor->SetInput(image_mapper->GetOutput());
    actor->SetPosition(0,-5.0,0);
    if(!use_image_interpolation)
        actor->InterpolateOff();

    // add the actor to the renderer's scene
    pRenderer->AddActor(actor);

    // also add a scalar bar to show how the colors correspond to values
    vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
    scalar_bar->SetLookupTable(lut);
    pRenderer->AddActor2D(scalar_bar);

    // add a line graph for all the chemicals (active one highlighted)
    for(int iChemical=0;iChemical<system->GetNumberOfChemicals();iChemical++)
    {
        vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
        plane->SetInput(system->GetImage(iChemical));
        vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
        warp->SetInputConnection(plane->GetOutputPort());
        warp->SetScaleFactor(-vertical_scale_1D);
        vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
        normals->SetInputConnection(warp->GetOutputPort());
        normals->SplittingOff();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(normals->GetOutputPort());
        mapper->ScalarVisibilityOff();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        if(iChemical==iActiveChemical)
            actor->GetProperty()->SetColor(1,1,1);
        else
            actor->GetProperty()->SetColor(0.5,0.5,0.5);
        actor->RotateX(90.0);
        pRenderer->AddActor(actor);
    }
    
    // add an axis
    vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
    axis->SetCamera(pRenderer->GetActiveCamera());
    axis->SetBounds(0,0,low*vertical_scale_1D,high*vertical_scale_1D,0,0);
    axis->SetRanges(0,0,low,high,0,0);
    axis->UseRangesOn();
    axis->XAxisVisibilityOff();
    axis->ZAxisVisibilityOff();
    axis->SetYLabel("");
    axis->SetLabelFormat("%.2f");
    axis->SetInertia(10000);
    axis->SetCornerOffset(0);
    axis->SetNumberOfLabels(5);
    pRenderer->AddActor(axis);

    // set the background color
    pRenderer->SetBackground(0,0,0);
    
    // change the interactor style to a trackball
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    pVTKWindow->SetInteractorStyle(is);

    pRenderer->ResetCamera();
}

void InitializeVTKPipeline_2D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings)
{
    float low = render_settings.GetFloat("low");
    float high = render_settings.GetFloat("high");
    float vertical_scale_1D = render_settings.GetFloat("vertical_scale_1D");
    float vertical_scale_2D = render_settings.GetFloat("vertical_scale_2D");
    float hue_low = render_settings.GetFloat("hue_low");
    float hue_high = render_settings.GetFloat("hue_high");
    bool use_image_interpolation = render_settings.GetBool("use_image_interpolation");
    int iActiveChemical = render_settings.GetInt("iActiveChemical");
    float contour_level = render_settings.GetFloat("contour_level");
    
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
        lut->SetTableRange(low,high);
        lut->SetHueRange(hue_low,hue_high);

        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInput(system->GetImage(iActiveChemical));
      
        // an actor determines how a scene object is displayed
        vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInput(image_mapper->GetOutput());
        //actor->InterpolateOff();
        actor->SetPosition(0,0,5);

        // add the actor to the renderer's scene
        pRenderer->AddActor(actor);

        // also add a scalar bar to show how the colors correspond to values
        {
            vtkSmartPointer<vtkScalarBarActor> scalar_bar = vtkSmartPointer<vtkScalarBarActor>::New();
            scalar_bar->SetLookupTable(lut);
            pRenderer->AddActor2D(scalar_bar);
        }
    }

    // also add a displacement-mapped surface
    {
        vtkSmartPointer<vtkImageDataGeometryFilter> plane = vtkSmartPointer<vtkImageDataGeometryFilter>::New();
        plane->SetInput(system->GetImage(iActiveChemical));
        vtkSmartPointer<vtkWarpScalar> warp = vtkSmartPointer<vtkWarpScalar>::New();
        warp->SetInputConnection(plane->GetOutputPort());
        warp->SetScaleFactor(-vertical_scale_2D);
        vtkSmartPointer<vtkPolyDataNormals> normals = vtkSmartPointer<vtkPolyDataNormals>::New();
        normals->SetInputConnection(warp->GetOutputPort());
        normals->SplittingOff();
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(normals->GetOutputPort());
        mapper->ScalarVisibilityOff();
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        pRenderer->AddActor(actor);

        // add an axis
        vtkSmartPointer<vtkCubeAxesActor2D> axis = vtkSmartPointer<vtkCubeAxesActor2D>::New();
        axis->SetCamera(pRenderer->GetActiveCamera());
        axis->SetBounds(0,0,0,0,low*vertical_scale_2D,high*vertical_scale_2D);
        axis->SetRanges(0,0,0,0,low,high);
        axis->UseRangesOn();
        axis->XAxisVisibilityOff();
        axis->YAxisVisibilityOff();
        axis->SetZLabel("");
        axis->SetLabelFormat("%.2f");
        axis->SetInertia(10000);
        axis->SetCornerOffset(0);
        axis->SetNumberOfLabels(5);
        pRenderer->AddActor(axis);
    }

    // set the background color
    pRenderer->SetBackground(0,0,0);
    
    // change the interactor style to a trackball
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    pVTKWindow->SetInteractorStyle(is);

    pRenderer->ResetCamera();
}

void InitializeVTKPipeline_3D(wxVTKRenderWindowInteractor* pVTKWindow,BaseRD* system,const Properties& render_settings)
{
    float low = render_settings.GetFloat("low");
    float high = render_settings.GetFloat("high");
    float vertical_scale_1D = render_settings.GetFloat("vertical_scale_1D");
    float vertical_scale_2D = render_settings.GetFloat("vertical_scale_2D");
    float hue_low = render_settings.GetFloat("hue_low");
    float hue_high = render_settings.GetFloat("hue_high");
    bool use_image_interpolation = render_settings.GetBool("use_image_interpolation");
    int iActiveChemical = render_settings.GetInt("iActiveChemical");
    float contour_level = render_settings.GetFloat("contour_level");
    
    // the VTK renderer is responsible for drawing the scene onto the screen
    vtkSmartPointer<vtkRenderer> pRenderer = vtkSmartPointer<vtkRenderer>::New();
    pVTKWindow->GetRenderWindow()->GetRenderers()->RemoveAllItems();
    pVTKWindow->GetRenderWindow()->AddRenderer(pRenderer); // connect it to the window

    if(0)
    {
        // use volume rendering

        // convert image to unsigned char
        vtkSmartPointer<vtkImageShiftScale> char_image = vtkSmartPointer<vtkImageShiftScale>::New();
        char_image->SetInput(system->GetImage(iActiveChemical));
        char_image->SetScale(255.0);
        char_image->SetOutputScalarTypeToUnsignedChar();

        // The volume will be displayed by ray-cast alpha compositing.
        // A ray-cast mapper is needed to do the ray-casting, and a
        // compositing function is needed to do the compositing along the ray. 
        vtkSmartPointer<vtkVolumeRayCastCompositeFunction> rayCastFunction = vtkSmartPointer<vtkVolumeRayCastCompositeFunction>::New();

        vtkSmartPointer<vtkVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkVolumeRayCastMapper>::New();
        volumeMapper->SetInput(char_image->GetOutput());
        volumeMapper->SetVolumeRayCastFunction(rayCastFunction);

        // The color transfer function maps voxel intensities to colors.
        vtkSmartPointer<vtkColorTransferFunction>volumeColor = vtkSmartPointer<vtkColorTransferFunction>::New();
        volumeColor->AddRGBPoint(0,1,1,1);
        volumeColor->AddRGBPoint(255,1,1,1);

        // The opacity transfer function is used to control the opacity
        // of different tissue types.
        vtkSmartPointer<vtkPiecewiseFunction> volumeScalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
        volumeScalarOpacity->AddPoint(0,0.0);
        volumeScalarOpacity->AddPoint(60,0.0);
        volumeScalarOpacity->AddPoint(80,0.8);
        volumeScalarOpacity->AddPoint(255,1.0);

        // The gradient opacity function is used to decrease the opacity
        // in the "flat" regions of the volume while maintaining the opacity
        // at the boundaries between tissue types.  The gradient is measured
        // as the amount by which the intensity changes over unit distance.
        // For most medical data, the unit distance is 1mm.
        vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
        volumeGradientOpacity->AddPoint(0,0.0);
        volumeGradientOpacity->AddPoint(10,0.5);
        volumeGradientOpacity->AddPoint(100,1.0);
 
        // The VolumeProperty attaches the color and opacity functions to the
        // volume, and sets other volume properties.
        vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
        volumeProperty->SetColor(volumeColor);
        volumeProperty->SetScalarOpacity(volumeScalarOpacity);
        volumeProperty->SetGradientOpacity(volumeGradientOpacity);
        volumeProperty->SetInterpolationTypeToLinear();
        volumeProperty->ShadeOff();
        volumeProperty->SetAmbient(0.4);
        volumeProperty->SetDiffuse(0.6);
        volumeProperty->SetSpecular(0.2);

        // The vtkVolume is a vtkProp3D (like a vtkActor) and controls the position
        // and orientation of the volume in world coordinates.
        vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
        volume->SetMapper(volumeMapper);
        volume->SetProperty(volumeProperty);

        // Finally, add the volume to the renderer
        pRenderer->AddViewProp(volume);
    }
    else
    {
        // contour the 3D volume and render as a polygonal surface

        // turns the 3d grid of sampled values into a polygon mesh for rendering,
        // by making a surface that contours the volume at a specified level
        vtkSmartPointer<vtkContourFilter> surface = vtkSmartPointer<vtkContourFilter>::New();
        surface->SetInput(system->GetImage(iActiveChemical));
        surface->SetValue(0, contour_level);
        // TODO: allow user to control the contour level

        // a mapper converts scene objects to graphics primitives
        vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        mapper->SetInputConnection(surface->GetOutputPort());
        mapper->ScalarVisibilityOff();

        // an actor determines how a scene object is displayed
        vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
        actor->SetMapper(mapper);
        actor->GetProperty()->SetColor(1,1,1);  
        actor->GetProperty()->SetAmbient(0.1);
        actor->GetProperty()->SetDiffuse(0.7);
        actor->GetProperty()->SetSpecular(0.2);
        actor->GetProperty()->SetSpecularPower(3);
        vtkSmartPointer<vtkProperty> bfprop = vtkSmartPointer<vtkProperty>::New();
        actor->SetBackfaceProperty(bfprop);
        bfprop->SetColor(0.3,0.3,0.3);
        bfprop->SetAmbient(0.3);
        bfprop->SetDiffuse(0.6);
        bfprop->SetSpecular(0.1);

        // add the actor to the renderer's scene
        pRenderer->AddActor(actor);
    }
    // add the bounding box
    {
        vtkSmartPointer<vtkCubeSource> box = vtkSmartPointer<vtkCubeSource>::New();
        box->SetBounds(system->GetImage(iActiveChemical)->GetBounds());

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

    // add a 2D slice too
    {
        // create a lookup table for mapping values to colors
        vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
        lut->SetRampToLinear();
        lut->SetScaleToLinear();
        lut->SetTableRange(low,high);
        lut->SetHueRange(hue_low,hue_high);

        // extract a slice
        vtkSmartPointer<vtkExtractVOI> voi = vtkSmartPointer<vtkExtractVOI>::New();
        voi->SetInput(system->GetImage(iActiveChemical));
        voi->SetVOI(0,system->GetX(),0,system->GetY(),system->GetZ()/2.0,system->GetZ()/2.0);

        // pass the image through the lookup table
        vtkSmartPointer<vtkImageMapToColors> image_mapper = vtkSmartPointer<vtkImageMapToColors>::New();
        image_mapper->SetLookupTable(lut);
        image_mapper->SetInputConnection(voi->GetOutputPort());
      
        // an actor determines how a scene object is displayed
        vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInput(image_mapper->GetOutput());
        //actor->InterpolateOff();

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
    pRenderer->GradientBackgroundOn();
    pRenderer->SetBackground(0,0.4,0.6);
    pRenderer->SetBackground2(0,0.2,0.3);
    
    // change the interactor style to a trackball
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> is = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    pVTKWindow->SetInteractorStyle(is);

    pRenderer->ResetCamera();
}
