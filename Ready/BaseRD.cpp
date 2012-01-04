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

// stdlib:
#include <stdlib.h>
#include <math.h>

// STL:
#include <cassert>
#include <stdexcept>
using namespace std;

// VTK:
#include <vtkImageData.h>

BaseRD::BaseRD()
{
    this->timesteps_taken = 0;
    this->need_reload_program = true;
}

BaseRD::~BaseRD()
{
    for(int iChem=0;iChem<this->GetNumberOfChemicals();iChem++)
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

int BaseRD::GetNumberOfChemicals() const
{
    return this->images.size();
}

float BaseRD::GetTimestep() const
{ 
    return this->timestep; 
}

vtkImageData* BaseRD::GetImage(int iChemical) const
{ 
    return this->images[iChemical];
}

int BaseRD::GetTimestepsTaken() const
{
    return this->timesteps_taken;
}

void BaseRD::AllocateImages(int x,int y,int z,int nc)
{
    this->images.resize(nc);
    for(int i=0;i<nc;i++)
        this->images[i] = AllocateVTKImage(x,y,z);
}

vtkImageData* BaseRD::AllocateVTKImage(int x,int y,int z)
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

float* BaseRD::vtk_at(float* origin,int x,int y,int z,int X,int Y)
{
    // single-component vtkImageData scalars are stored as: float,float,... for consecutive x, then y, then z
    return origin + x + X*(y + Y*z);
}

void BaseRD::SetProgram(string s)
{
    if(s != this->program_string)
        this->need_reload_program = true;
    this->program_string = s;
}

string BaseRD::GetProgram() const
{
    return this->program_string;
}
