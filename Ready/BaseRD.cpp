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
#include "BaseRD.hpp"
#include "overlays.hpp"
#include "utils.hpp"

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

BaseRD::BaseRD()
{
    this->timesteps_taken = 0;
    this->need_reload_formula = true;
    this->is_modified = false;
}

BaseRD::~BaseRD()
{
    this->Deallocate();
    this->ClearInitialPatternGenerator();
}

void BaseRD::Deallocate()
{
    for(int iChem=0;iChem<(int)this->images.size();iChem++)
    {
        if(this->images[iChem])
            this->images[iChem]->Delete();
    }
}

void BaseRD::ClearInitialPatternGenerator()
{
    for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
        delete this->initial_pattern_generator[iOverlay];
    this->initial_pattern_generator.clear();
}

void BaseRD::AddInitialPatternGeneratorOverlay(Overlay* overlay)
{
    this->initial_pattern_generator.push_back(overlay);
}

int BaseRD::GetDimensionality() const
{
    assert(this->images.front());
    int dimensionality=0;
    for(int iDim=0;iDim<3;iDim++)
        if(this->images.front()->GetDimensions()[iDim]>1)
            dimensionality++;
    return dimensionality;
}

int BaseRD::GetX() const
{
    return this->images.front()->GetDimensions()[0];
}

int BaseRD::GetY() const
{
    return this->images.front()->GetDimensions()[1];
}

int BaseRD::GetZ() const
{
    return this->images.front()->GetDimensions()[2];
}

vtkImageData* BaseRD::GetImage(int iChemical) const
{ 
    return this->images[iChemical];
}

vtkSmartPointer<vtkImageData> BaseRD::GetImage() const
{ 
    vtkSmartPointer<vtkImageAppendComponents> iac = vtkSmartPointer<vtkImageAppendComponents>::New();
    for(int i=0;i<this->GetNumberOfChemicals();i++)
        iac->AddInput(this->GetImage(i));
    iac->Update();
    return iac->GetOutput();
}


void BaseRD::CopyFromImage(vtkImageData* im)
{
    if(im->GetNumberOfScalarComponents()!=this->GetNumberOfChemicals()) throw runtime_error("BaseRD::CopyFromImage : chemical count mismatch");
    vtkSmartPointer<vtkImageExtractComponents> iec = vtkSmartPointer<vtkImageExtractComponents>::New();
    iec->SetInput(im);
    for(int i=0;i<this->GetNumberOfChemicals();i++)
    {
        iec->SetComponents(i);
        iec->Update();
        this->images[i]->DeepCopy(iec->GetOutput());
    }
}

int BaseRD::GetTimestepsTaken() const
{
    return this->timesteps_taken;
}

void BaseRD::Allocate(int x,int y,int z,int nc)
{
    this->Deallocate();
    this->n_chemicals = nc;
    this->images.resize(nc);
    for(int i=0;i<nc;i++)
        this->images[i] = AllocateVTKImage(x,y,z);
}

void BaseRD::SetFormula(string s)
{
    if(s != this->formula)
        this->need_reload_formula = true;
    this->formula = s;
    this->is_modified = true;
}

string BaseRD::GetFormula() const
{
    return this->formula;
}

std::string BaseRD::GetRuleName() const
{
    return this->rule_name;
}

std::string BaseRD::GetDescription() const
{
    return this->description;
}

void BaseRD::SetRuleName(std::string s)
{
    this->rule_name = s;
    this->is_modified = true;
}

void BaseRD::SetDescription(std::string s)
{
    this->description = s;
    this->is_modified = true;
}

int BaseRD::GetNumberOfParameters() const
{
    return (int)this->parameters.size();
}

std::string BaseRD::GetParameterName(int iParam) const
{
    return this->parameters[iParam].first;
}

float BaseRD::GetParameterValue(int iParam) const
{
    return this->parameters[iParam].second;
}

float BaseRD::GetParameterValueByName(std::string name) const
{
    for(int iParam=0;iParam<(int)this->parameters.size();iParam++)
        if(this->parameters[iParam].first == name)
            return this->parameters[iParam].second;
    throw runtime_error("BaseRD::GetParameterValueByName : parameter name not found: "+name);
}

void BaseRD::AddParameter(std::string name,float val)
{
    this->parameters.push_back(make_pair(name,val));
    this->is_modified = true;
}

void BaseRD::DeleteParameter(int iParam)
{
    this->parameters.erase(this->parameters.begin()+iParam);
    this->is_modified = true;
}

void BaseRD::DeleteAllParameters()
{
    this->parameters.clear();
    this->is_modified = true;
}

void BaseRD::SetParameterName(int iParam,string s)
{
    this->parameters[iParam].first = s;
    this->is_modified = true;
}

void BaseRD::SetParameterValue(int iParam,float val)
{
    this->parameters[iParam].second = val;
    this->is_modified = true;
}

/* static */ vtkImageData* BaseRD::AllocateVTKImage(int x,int y,int z)
{
    vtkImageData *im = vtkImageData::New();
    assert(im);
    im->SetNumberOfScalarComponents(1);
    im->SetScalarTypeToFloat();
    im->SetDimensions(x,y,z);
    im->AllocateScalars();
    if(im->GetDimensions()[0]!=x || im->GetDimensions()[1]!=y || im->GetDimensions()[2]!=z)
        throw runtime_error("BaseRD::AllocateVTKImage : Failed to allocate image data - dimensions too big?");
    return im;
}

bool BaseRD::IsModified() const
{
    return this->is_modified;
}

void BaseRD::SetModified(bool m)
{
    this->is_modified = m;
}

std::string BaseRD::GetFilename() const
{
    return this->filename;
}

void BaseRD::SetFilename(std::string s)
{
    this->filename = s;
}

void BaseRD::GenerateInitialPattern()
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
	this->timesteps_taken = 0;
}

void BaseRD::BlankImage()
{
	for(int iImage=0;iImage<(int)this->images.size();iImage++)
	{
		this->images[iImage]->GetPointData()->GetScalars()->FillComponent(0,0.0);
		this->images[iImage]->Modified();
	}
	this->timesteps_taken = 0;
}

// load things from the XML that are standard across all implementations
void BaseRD::InitializeFromXML(vtkXMLDataElement *rd, bool &warn_to_update)
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

// TODO: BaseRD could inherit from XML_Object (but as VTKFile element, not RD element!)
vtkSmartPointer<vtkXMLDataElement> BaseRD::GetAsXML() const
{
    vtkSmartPointer<vtkXMLDataElement> rd = vtkSmartPointer<vtkXMLDataElement>::New();
    rd->SetName("RD");
    rd->SetAttribute("format_version","1");
    // (Use this for when the format changes so much that the user will get better results if they update their Ready. File reading will still proceed but may fail.) 

    vtkSmartPointer<vtkXMLDataElement> rule = vtkSmartPointer<vtkXMLDataElement>::New();
    rule->SetName("rule");
    rule->SetAttribute("name",this->GetRuleName().c_str());

    // parameters
    for(int i=0;i<this->GetNumberOfParameters();i++)
    {
        vtkSmartPointer<vtkXMLDataElement> param = vtkSmartPointer<vtkXMLDataElement>::New();
        param->SetName("param");
        param->SetAttribute("name",this->GetParameterName(i).c_str());
        string s = to_string(this->GetParameterValue(i));
        param->SetCharacterData(s.c_str(),(int)s.length());
        rule->AddNestedElement(param);
    }

    rd->AddNestedElement(rule);

    // description
    vtkSmartPointer<vtkXMLDataElement> description = vtkSmartPointer<vtkXMLDataElement>::New();
    description->SetName("description");
    {
        ostringstream oss;
        vtkXMLUtilities::EncodeString(this->GetDescription().c_str(),VTK_ENCODING_UNKNOWN,oss,VTK_ENCODING_UNKNOWN,true);
        description->SetCharacterData(oss.str().c_str(),(int)oss.str().length());
    }
    rd->AddNestedElement(description);

    // initial pattern generator
    vtkSmartPointer<vtkXMLDataElement> initial_pattern_generator = vtkSmartPointer<vtkXMLDataElement>::New();
    initial_pattern_generator->SetName("initial_pattern_generator");
    for(int i=0;i<this->GetNumberOfInitialPatternGeneratorOverlays();i++)
        initial_pattern_generator->AddNestedElement(this->GetInitialPatternGeneratorOverlay(i)->GetAsXML());
    rd->AddNestedElement(initial_pattern_generator);

    return rd;
}
