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
#include "BaseOverlay.hpp"

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

BaseRD::BaseRD()
{
    this->timesteps_taken = 0;
    this->need_reload_formula = true;
    this->is_modified = false;
}

BaseRD::~BaseRD()
{
    this->Deallocate();
    for(int iOverlay=0;iOverlay<(int)this->initial_pattern_generator.size();iOverlay++)
        delete this->initial_pattern_generator[iOverlay];
    this->initial_pattern_generator.clear();
}

void BaseRD::Deallocate()
{
    for(int iChem=0;iChem<(int)this->images.size();iChem++)
    {
        if(this->images[iChem])
            this->images[iChem]->Delete();
    }
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

float BaseRD::GetTimestep() const
{ 
    return this->timestep; 
}

void BaseRD::SetTimestep(float t)
{
    this->timestep = t;
    this->is_modified = true;
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
    if(nc!=this->n_chemicals) throw runtime_error("BaseRD::Allocate : chemical count mismatch");
    this->Deallocate();
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

std::string BaseRD::GetRuleDescription() const
{
    return this->rule_description;
}

std::string BaseRD::GetPatternDescription() const
{
    return this->pattern_description;
}

void BaseRD::SetRuleName(std::string s)
{
    this->rule_name = s;
    this->is_modified = true;
}

void BaseRD::SetRuleDescription(std::string s)
{
    this->rule_description = s;
    this->is_modified = true;
}

void BaseRD::SetPatternDescription(std::string s)
{
    this->pattern_description = s;
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

/* static */ float* BaseRD::vtk_at(float* origin,int x,int y,int z,int X,int Y)
{
    // single-component vtkImageData scalars are stored as: float,float,... for consecutive x, then y, then z
    return origin + x + X*(y + Y*z);
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
	// first fill each image with zero
	for(int i=0;i<(int)this->images.size();i++)
	{
		this->images[i]->GetPointData()->GetScalars()->FillComponent(0,0.0);
		this->images[i]->Modified();
	}
	// TODO: apply overlays
	this->timesteps_taken = 0;
}
