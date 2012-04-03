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
#include "ImageRD.hpp"
#include "utils.hpp"
#include "overlays.hpp"

// stdlib:
#include <stdlib.h>
#include <math.h>

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageExtractComponents.h>
#include <vtkPointData.h>
#include <vtkXMLUtilities.h>
#include <vtkCellData.h>
#include <vtkImageWrapPad.h>

ImageRD::ImageRD()
{
    this->timesteps_taken = 0;
    this->need_reload_formula = true;
    this->is_modified = false;
    this->image_wrap_pad_filter = NULL;
}

ImageRD::~ImageRD()
{
    this->DeallocateImages();
    this->ClearInitialPatternGenerator();
}

void ImageRD::DeallocateImages()
{
    for(int iChem=0;iChem<(int)this->images.size();iChem++)
    {
        if(this->images[iChem])
            this->images[iChem]->Delete();
    }
}

int ImageRD::GetDimensionality() const
{
    assert(this->images.front());
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->images.front()->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

int ImageRD::GetX() const
{
    return this->images.front()->GetDimensions()[0];
}

int ImageRD::GetY() const
{
    return this->images.front()->GetDimensions()[1];
}

int ImageRD::GetZ() const
{
    return this->images.front()->GetDimensions()[2];
}

vtkImageData* ImageRD::GetImage(int iChemical) const
{ 
    return this->images[iChemical];
}

vtkSmartPointer<vtkImageData> ImageRD::GetImage() const
{ 
    vtkSmartPointer<vtkImageAppendComponents> iac = vtkSmartPointer<vtkImageAppendComponents>::New();
    for(int i=0;i<this->GetNumberOfChemicals();i++)
        iac->AddInput(this->GetImage(i));
    iac->Update();
    return iac->GetOutput();
}


void ImageRD::CopyFromImage(vtkImageData* im)
{
    if(im->GetNumberOfScalarComponents()!=this->GetNumberOfChemicals()) throw runtime_error("ImageRD::CopyFromImage : chemical count mismatch");
    vtkSmartPointer<vtkImageExtractComponents> iec = vtkSmartPointer<vtkImageExtractComponents>::New();
    iec->SetInput(im);
    for(int i=0;i<this->GetNumberOfChemicals();i++)
    {
        iec->SetComponents(i);
        iec->Update();
        this->images[i]->DeepCopy(iec->GetOutput());
    }
    UpdateImageWrapPadFilter();
}

void ImageRD::AllocateImages(int x,int y,int z,int nc)
{
    this->DeallocateImages();
    this->n_chemicals = nc;
    this->images.resize(nc);
    for(int i=0;i<nc;i++)
        this->images[i] = AllocateVTKImage(x,y,z);
    this->is_modified = true;
}

/* static */ vtkImageData* ImageRD::AllocateVTKImage(int x,int y,int z)
{
    vtkImageData *im = vtkImageData::New();
    assert(im);
    im->SetNumberOfScalarComponents(1);
    im->SetScalarTypeToFloat();
    im->SetDimensions(x,y,z);
    im->AllocateScalars();
    if(im->GetDimensions()[0]!=x || im->GetDimensions()[1]!=y || im->GetDimensions()[2]!=z)
        throw runtime_error("ImageRD::AllocateVTKImage : Failed to allocate image data - dimensions too big?");
    return im;
}

void ImageRD::GenerateInitialPattern()
{
    this->BlankImage();

    const int X = this->GetX();
    const int Y = this->GetY();
    const int Z = this->GetZ();

    for(int z=0;z<Z;z++)
    {
        for(int y=0;y<Y;y++)
        {
            for(int x=0;x<X;x++)
            {
                for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
                    this->initial_pattern_generator[iOverlay]->Apply(this,x,y,z);
            }
        }
    }
    for(int i=0;i<(int)this->images.size();i++)
        this->images[i]->Modified();
    UpdateImageWrapPadFilter();
    this->timesteps_taken = 0;
}

void ImageRD::BlankImage()
{
    for(int iImage=0;iImage<(int)this->images.size();iImage++)
    {
        this->images[iImage]->GetPointData()->GetScalars()->FillComponent(0,0.0);
        this->images[iImage]->Modified();
    }
    this->timesteps_taken = 0;
}

// load things from the XML that are standard across all implementations
void ImageRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
{
    string str;
    const char *s;
    float f;
    int i;

    // check whether we should warn the user that they need to update Ready
    {
        read_required_attribute(rd,"format_version",i);
        warn_to_update = (i>1);
        // (we will still proceed and try to read the file but it might fail or give poor results)
    }

    vtkSmartPointer<vtkXMLDataElement> rule = rd->FindNestedElementWithName("rule");
    if(!rule) throw runtime_error("rule node not found in file");

    // rule_name:
    read_required_attribute(rule,"name",str);
    this->SetRuleName(str);

    // parameters:
    this->DeleteAllParameters();
    for(int i=0;i<rule->GetNumberOfNestedElements();i++)
    {
        vtkSmartPointer<vtkXMLDataElement> node = rule->GetNestedElement(i);
        if(string(node->GetName())!="param") continue;
        string name;
        s = node->GetAttribute("name");
        if(!s) throw runtime_error("Failed to read param attribute: name");
        name = trim_multiline_string(s);
        s = node->GetCharacterData();
        if(!s || !from_string(s,f)) throw runtime_error("Failed to read param value");
        this->AddParameter(name,f);
    }

    // description:
    vtkSmartPointer<vtkXMLDataElement> xml_description = rd->FindNestedElementWithName("description");
    if(!xml_description) this->SetDescription(""); // optional, default is empty string
    else this->SetDescription(trim_multiline_string(xml_description->GetCharacterData()));

    // initial_pattern_generator:
    this->ClearInitialPatternGenerator();
    vtkSmartPointer<vtkXMLDataElement> xml_initial_pattern_generator = rd->FindNestedElementWithName("initial_pattern_generator");
    if(xml_initial_pattern_generator) // optional, default is none
    {
        for(int i=0;i<xml_initial_pattern_generator->GetNumberOfNestedElements();i++)
            this->AddInitialPatternGeneratorOverlay(new Overlay(xml_initial_pattern_generator->GetNestedElement(i)));
    }
}

// TODO: ImageRD could inherit from XML_Object (but as VTKFile element, not RD element!)
vtkSmartPointer<vtkXMLDataElement> ImageRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    rd->SetAttribute("format_version","1");
    // (Use this for when the format changes so much that the user will get better results if they update their Ready. File reading will still proceed but may fail.) 

    // description
    vtkSmartPointer<vtkXMLDataElement> description = vtkSmartPointer<vtkXMLDataElement>::New();
    description->SetName("description");
    description->SetCharacterData(this->GetDescription().c_str(),(int)this->GetDescription().length());
    rd->AddNestedElement(description);

    // rule
    vtkSmartPointer<vtkXMLDataElement> rule = vtkSmartPointer<vtkXMLDataElement>::New();
    rule->SetName("rule");
    rule->SetAttribute("name",this->GetRuleName().c_str());
    rule->SetAttribute("type",this->GetRuleType().c_str());
    for(int i=0;i<this->GetNumberOfParameters();i++)    // parameters
    {
        vtkSmartPointer<vtkXMLDataElement> param = vtkSmartPointer<vtkXMLDataElement>::New();
        param->SetName("param");
        param->SetAttribute("name",this->GetParameterName(i).c_str());
        string s = to_string(this->GetParameterValue(i));
        param->SetCharacterData(s.c_str(),(int)s.length());
        rule->AddNestedElement(param);
    }
    rd->AddNestedElement(rule);

    // initial pattern generator
    vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator = vtkSmartPointer<vtkXMLDataElement>::New();
    initial_pattern_generator->SetName("initial_pattern_generator");
    for(int i=0;i<this->GetNumberOfInitialPatternGeneratorOverlays();i++)
        initial_pattern_generator->AddNestedElement(this->GetInitialPatternGeneratorOverlay(i)->GetAsXML());
    rd->AddNestedElement(initial_pattern_generator);

    return rd;
}

void ImageRD::Update(int n_steps)
{
    this->InternalUpdate(n_steps);

    this->timesteps_taken += n_steps;

    for(int ic=0;ic<this->GetNumberOfChemicals();ic++)
        this->images[ic]->Modified();

    UpdateImageWrapPadFilter();
}

void ImageRD::UpdateImageWrapPadFilter()
{
    // kludgy workaround for the GenerateCubesFromLabels approach not being fully pipelined (see vtk_pipeline.cpp)
    if(this->image_wrap_pad_filter)
    {
        this->image_wrap_pad_filter->Update();
        this->image_wrap_pad_filter->GetOutput()->GetCellData()->SetScalars(
            dynamic_cast<vtkImageData*>(this->image_wrap_pad_filter->GetInput())->GetPointData()->GetScalars());
    }
}
